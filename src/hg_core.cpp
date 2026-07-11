#include "hg_core.hpp"
#include "hg_assets.hpp"
#include "hg_audio.hpp"
#include "hg_concurrency.hpp"
#include "hg_gpu.hpp"
#include "hg_memory.hpp"
#include "hg_platform.hpp"
#include "hg_rendering.hpp"
#include "hg_strings.hpp"
#include "hg_templates.hpp"
#include "hg_window.hpp"

#include <cstdio>
#include <cstring>

namespace hg {

thread_local static char errorMessageData[4096];
thread_local static u64 errorMessageLength = 0;

void printStdout(String str)
{
    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    fputs(cString(sc, str), stdout);
}

void printStderr(String str)
{
    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    fputs(cString(sc, str), stderr);
}

void logInternal(String format, ...)
{
    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    va_list args;
    va_start(args, format);

    StringBuilder begin = stringCopy(sc, "HurdyGurdy Log: ");
    stringAppend(sc, &begin, format);
    StringBuilder formatted = stringFormatVar(sc, begin, args);
    printStderr(formatted);

    va_end(args);
}

void warnInternal(String format, ...)
{
    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    va_list args;
    va_start(args, format);

    StringBuilder begin = stringCopy(sc, "HurdyGurdy Warn: ");
    stringAppend(sc, &begin, format);
    printStderr(stringFormatVar(sc, begin, args));

    va_end(args);
}

void panicInternal(String format, ...)
{
    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    va_list args;
    va_start(args, format);

    StringBuilder begin = stringCopy(sc, "HurdyGurdy Panic: ");
    stringAppend(sc, &begin, format);
    begin.length += stringFormat(sc, "\tLast error: \"%.*s\"\n", (int)errorGet().length, errorGet().chars).length;
    printStderr(stringFormatVar(sc, begin, args));

    va_end(args);

    abort();
}

String errorGet()
{
    return {errorMessageData, errorMessageLength};
}

void errorSet(String error)
{
    u64 newLength = min(error.length, sizeof(errorMessageData));
    memCopy(errorMessageData, error.chars, newLength);
    errorMessageLength = newLength;
}

void errorFormat(String errorFmt, ...)
{
    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    va_list args;
    va_start(args, errorFmt);
    errorSet(stringFormatVar(sc, errorFmt, args));
    va_end(args);
}

static SubsystemFlags initialized = 0;

bool init(SubsystemFlags init)
{
    if (init & Subsystem_memory)
    {
        scratchInit(2, (u64)1 << 24);
        initialized |= Subsystem_memory;
    }

    if (init & Subsystem_concurrency)
    {
        concurrencyInit();
        initialized |= Subsystem_concurrency;
    }

    if (init & Subsystem_gpu ||
        init & Subsystem_windowing ||
        init & Subsystem_audio)
    {
        if (!platformInit())
            goto platformFailed;
    }

    if (init & Subsystem_gpu ||
        init & Subsystem_windowing)
    {
        if (!gpuInit())
            goto gpuFailed;
        initialized |= Subsystem_gpu;
    }

    if (init & Subsystem_assets)
    {
        assetInitDefaults();
        initialized |= Subsystem_assets;
    }

    if (init & Subsystem_windowing)
    {
        windowsInit();
        initialized |= Subsystem_windowing;
    }

    if (init & Subsystem_audio)
    {
        if (!audioInit())
            goto audioFailed;
        initialized |= Subsystem_audio;
    }

    return true;

audioFailed:
    if (initialized & Subsystem_windowing)
        windowsDeinit();
    if (initialized & Subsystem_assets)
        assetDeinitDefaults();
    if (initialized & Subsystem_gpu)
        gpuDeinit();
gpuFailed:
    if (initialized & Subsystem_gpu ||
        initialized & Subsystem_windowing ||
        initialized & Subsystem_audio)
        platformDeinit();
platformFailed:
    if (initialized & Subsystem_concurrency)
        concurrencyDeinit();
    if (initialized & Subsystem_memory)
        scratchDeinit();
    initialized = 0;
    return false;
}

void deinit()
{
    if (initialized & Subsystem_audio)
        audioDeinit();

    if (initialized & Subsystem_windowing)
        windowsDeinit();

    if (initialized & Subsystem_assets)
        assetDeinitDefaults();

    if (initialized & Subsystem_gpu)
        gpuDeinit();

    if (initialized & Subsystem_gpu ||
        initialized & Subsystem_windowing ||
        initialized & Subsystem_audio)
        platformDeinit();

    if (initialized & Subsystem_concurrency)
        concurrencyDeinit();

    if (initialized & Subsystem_memory)
        scratchDeinit();

    initialized = 0;
}

void swap(void* a, void* b, u64 size)
{
    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    void* tmp = arenaAlloc(sc, size, 1);
    memCopy(tmp, a, size);
    memCopy(a, b, size);
    memCopy(b, tmp, size);
}

void memClear(void* dst, u64 size, u8 val)
{
    memset(dst, val, size);
}

void memCopy(void* __restrict dst, const void* __restrict src, u64 size)
{
    memcpy(dst, src, size);
}

void memMove(void* dst, const void* src, u64 size)
{
    memmove(dst, src, size);
}

bool memEqual(const void* dst, const void* src, u64 size)
{
    return size == 0 || memcmp(dst, src, size) == 0;
}

void* gpaAlloc(u64 size, u64 alignment)
{
    (void)alignment;
    void* alloc = malloc(size);
    if (alloc == nullptr)
        HG_PANIC("malloc out of memory");
    return alloc;
}

void* gpaRealloc(void* allocation, u64 oldSize, u64 newSize, u64 alignment)
{
    (void)oldSize;
    (void)alignment;
    void* alloc = realloc(allocation, newSize);
    if (alloc == nullptr)
        HG_PANIC("malloc out of memory");
    return alloc;
}

template<>
void gpaFree(void* allocation, u64 size)
{
    (void)size;
    free(allocation);
}

void* arenaAlloc(Arena* arena, u64 size, u64 alignment)
{
    HG_ASSERT(arena != nullptr);

    u64 newHead = align((u64)arena->head, alignment) + size;
    if (newHead > arena->capacity)
    {
        errorSet("Arena out of memory");
        return nullptr;
    }

    arena->head = newHead;
    return (void*)((uptr)arena->memory + arena->head - size);
}

void* arenaRealloc(Arena* arena, void* allocation, u64 oldSize, u64 newSize, u64 alignment)
{
    HG_ASSERT(arena != nullptr);

    if (allocation >= arena->memory && (uptr)allocation + oldSize <= (uptr)arena->memory + arena->capacity)
    {
        if ((uptr)allocation + oldSize - (uptr)arena->memory == (uptr)arena->head)
        {
            u64 newHead = (uptr)allocation + newSize - (uptr)arena->memory;
            if (newHead > arena->capacity)
            {
                errorSet("Arena out of memory");
                return nullptr;
            }

            arena->head = newHead;
            return allocation;
        }

        if (newSize < oldSize)
            return allocation;
    }

    void* newAllocation = arenaAlloc(arena, newSize, alignment);
    if (allocation != nullptr)
        memCopy(newAllocation, allocation, min(oldSize, newSize));
    return newAllocation;
}

static thread_local Arena* arenas{};
static thread_local u32 arenaCount = 0;

void scratchInit(u32 count, u64 size)
{
    size = align(size, 16);
    HG_ASSERT(size < UINT64_MAX / count);

    void* block = gpaAlloc(count * size, 16);
    Arena base = {block, size, 0};
    arenas = arenaAlloc<Arena>(&base, count);
    arenaCount = count;

    arenas[0] = base;
    for (u32 i = 1; i < arenaCount; ++i)
    {
        arenas[i] = {(u8*)block + i * size, size, 0};
    }
}

void scratchDeinit()
{
    if (arenas != nullptr)
    {
        gpaFree(arenas[0].memory, arenas[0].capacity * arenaCount);
    }
}

Arena* scratch(Arena const* const* conflicts, u32 count)
{
    if (conflicts != nullptr)
        HG_ASSERT(count > 0);

    for (u32 i = 0; i < arenaCount; ++i)
    {
        HG_ASSERT(arenas[i].memory != nullptr);

        for (u32 j = 0; j < count; ++j)
        {
            if (&arenas[i] == conflicts[j])
                goto next;
        }
        return &arenas[i];
next:
        continue;
    }
    HG_PANIC("No sc arena available\n");
}

void assetInitDefaults()
{
    assetInit<Binary>();
    assetInit<TextureData>();
    assetInit<Texture>();
    assetInit<MeshData>();
    assetInit<Mesh>();
    assetInit<Sound>();
}

void assetDeinitDefaults()
{
    assetDeinit<Sound>();
    assetDeinit<Mesh>();
    assetDeinit<MeshData>();
    assetDeinit<Texture>();
    assetDeinit<TextureData>();
    assetDeinit<Binary>();
}

template<>
void assetLoadImpl(Asset<Binary>* data)
{
    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    char* cpath = cString(sc, data->path);

    FILE* fileHandle = fopen(cpath, "rb");
    if (fileHandle == nullptr)
    {
        errorFormat("Could not find file to read binary: %s", cpath);
        return;
    }
    HG_DEFER(fclose(fileHandle));

    if (fseek(fileHandle, 0, SEEK_END) != 0)
    {
        errorFormat("Failed to read binary from file: %s", cpath);
        return;
    }

    data->asset.size = (u64)ftell(fileHandle);
    data->asset.data = gpaAlloc(data->asset.size, 1);

    rewind(fileHandle);
    if (fread((void*)data->asset.data, 1, data->asset.size, fileHandle) != data->asset.size)
    {
        gpaFree((void*)data->asset.data, data->asset.size);
        errorFormat("Failed to read binary from file: %s", cpath);
        data->asset = {};
        return;
    }
}

template<>
void assetUnloadImpl(Asset<Binary>* data)
{
    gpaFree((void*)data->asset.data, data->asset.size);
}

bool binaryStore(Binary bin, String path)
{
    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    char* cpath = cString(sc, path);

    FILE* fileHandle = fopen(cpath, "wb");
    if (fileHandle == nullptr)
    {
        errorFormat("Failed to create file to write binary: %s", cpath);
        return false;
    }
    HG_DEFER(fclose(fileHandle));

    if (fwrite(bin.data, 1, bin.size, fileHandle) != bin.size)
    {
        errorFormat("Failed to write binary data to file: %s", cpath);
        return false;
    }

    return true;
}

template<>
void assetLoadImpl(Asset<Json>* data)
{
    BinaryAsset* bin = assetLoad<Binary>(data->path);
    HG_DEFER(assetUnload(bin));

    Arena* sc = scratch();
    u64 head = sc->head;
    HG_DEFER(sc->head = head);

    String jsonStr = {(char*)bin->asset.data, bin->asset.size};
    Json parse = parseJson(sc, jsonStr);

    JsonError* e = parse.errors;
    if (e != nullptr)
        errorSet("Json parse errors");
    while (e != nullptr)
    {
        HG_WARN("Json parse error: %.*s\n", (int)e->msg.length, e->msg.chars);
        e = e->next;
    }

    data->asset.file = (JsonNode*)malloc(sc->head - head);
    if (parse.errors != nullptr)
    {
        data->asset.errors = (JsonError*)(
            (u8*)data->asset.file +
                ((uptr)parse.errors - (uptr)parse.file));
    }
    else
    {
        data->asset.errors = nullptr;
    }
    memCopy((void*)data->asset.file, (void*)parse.file, sc->head - head);
}

template<>
void assetUnloadImpl(Asset<Json>* data)
{
    free(data->asset.file);
}

} // namespace hg


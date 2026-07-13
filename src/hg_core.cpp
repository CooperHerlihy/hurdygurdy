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

thread_local static char errorData[4096];
thread_local static u64 errorLength = 0;

String getError()
{
    return {errorData, errorLength};
}

void setError(String error)
{
    u64 newLength = min(error.length, sizeof(errorData));
    memCopy(errorData, error.chars, newLength);
    errorLength = newLength;
}

void formatError(String errorFmt, ...)
{
    va_list args;
    va_start(args, errorFmt);
    formatErrorVar(errorFmt, args);
    va_end(args);
}

void formatErrorVar(String errorFmt, va_list args)
{
    Arena* scratch = getScratch();
    HG_ARENA_SCOPE(scratch);

    setError(stringFormatVar(scratch, errorFmt, args));
}

void printStdout(String str)
{
    Arena* scratch = getScratch();
    HG_ARENA_SCOPE(scratch);

    fputs(cString(scratch, str), stdout);
}

void printStderr(String str)
{
    Arena* scratch = getScratch();
    HG_ARENA_SCOPE(scratch);

    fputs(cString(scratch, str), stderr);
}

void logInternal(String format, ...)
{
    Arena* scratch = getScratch();
    HG_ARENA_SCOPE(scratch);

    va_list args;
    va_start(args, format);

    StringBuilder begin = stringCopy(scratch, "HurdyGurdy Log: ");
    stringAppend(scratch, &begin, format);
    StringBuilder formatted = stringFormatVar(scratch, begin, args);
    printStderr(formatted);

    va_end(args);
}

void warnInternal(String format, ...)
{
    Arena* scratch = getScratch();
    HG_ARENA_SCOPE(scratch);

    va_list args;
    va_start(args, format);

    StringBuilder begin = stringCopy(scratch, "HurdyGurdy Warn: ");
    stringAppend(scratch, &begin, format);
    printStderr(stringFormatVar(scratch, begin, args));

    va_end(args);
}

void panicInternal(String format, ...)
{
    Arena* scratch = getScratch();
    HG_ARENA_SCOPE(scratch);

    va_list args;
    va_start(args, format);

    StringBuilder begin = stringCopy(scratch, "HurdyGurdy Panic: ");
    stringAppend(scratch, &begin, format);
    begin.length += stringFormat(scratch, "\tLast error: \"%.*s\"\n", (int)getError().length, getError().chars).length;
    printStderr(stringFormatVar(scratch, begin, args));

    va_end(args);

    abort();
}

void binaryRead(Binary bin, u64 idx, void* dst, u64 len)
{
    HG_ASSERT(idx + len <= bin.size);
    memCopy(dst, (u8*)bin.data + idx, len);
}

static SubsystemFlags initialized = 0;

bool init(SubsystemFlags init)
{
    if (init & Subsystem_memory)
    {
        initScratch(2, (u64)1 << 24);
        initialized |= Subsystem_memory;
    }

    if (init & Subsystem_concurrency)
    {
        initConcurrency();
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
        if (!initGpu())
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
        deinitGpu();
gpuFailed:
    if (initialized & Subsystem_gpu ||
        initialized & Subsystem_windowing ||
        initialized & Subsystem_audio)
        platformDeinit();
platformFailed:
    if (initialized & Subsystem_concurrency)
        deinitConcurrency();
    if (initialized & Subsystem_memory)
        deinitScratch();
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
        deinitGpu();

    if (initialized & Subsystem_gpu ||
        initialized & Subsystem_windowing ||
        initialized & Subsystem_audio)
        platformDeinit();

    if (initialized & Subsystem_concurrency)
        deinitConcurrency();

    if (initialized & Subsystem_memory)
        deinitScratch();

    initialized = 0;
}

void swap(void* a, void* b, u64 size)
{
    Arena* scratch = getScratch();
    HG_ARENA_SCOPE(scratch);

    void* tmp = arenaAlloc(scratch, size, 1);
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

void* allocGpa(u64 size, u64 alignment)
{
    (void)alignment;
    void* alloc = malloc(size);
    if (alloc == nullptr)
        HG_PANIC("malloc out of memory");
    return alloc;
}

void* reallocGpa(void* allocation, u64 oldSize, u64 newSize, u64 alignment)
{
    (void)oldSize;
    (void)alignment;
    void* alloc = realloc(allocation, newSize);
    if (alloc == nullptr)
        HG_PANIC("malloc out of memory");
    return alloc;
}

template<>
void freeGpa(void* allocation, u64 size)
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
        setError("Arena out of memory");
        return nullptr;
    }

    arena->head = newHead;
    return (void*)((uptr)arena->memory + arena->head - size);
}

void* arenaRealloc(Arena* arena, void* allocation, u64 oldSize, u64 newSize, u64 alignment)
{
    HG_ASSERT(arena != nullptr);

    if (arenaCanExtend(arena, allocation, oldSize, alignment))
    {
        u64 newHead = (uptr)allocation + newSize - (uptr)arena->memory;
        if (newHead > arena->capacity)
        {
            setError("Arena out of memory");
            return nullptr;
        }

        arena->head = newHead;
        return allocation;
    }

    if (newSize < oldSize)
        return allocation;

    void* newAllocation = arenaAlloc(arena, newSize, alignment);
    if (allocation != nullptr)
        memCopy(newAllocation, allocation, min(oldSize, newSize));
    return newAllocation;
}

bool arenaCanExtend(Arena* arena, void* allocation, u64 size, u64 align)
{
    (void)align;
    return (uptr)allocation + size - (uptr)arena->memory == (uptr)arena->head;
}

static thread_local Arena* arenas{};
static thread_local u32 arenaCount = 0;

void initScratch(u32 count, u64 size)
{
    size = align(size, 16);
    HG_ASSERT(size < UINT64_MAX / count);

    void* block = allocGpa(count * size, 16);
    Arena base = {block, size, 0};
    arenas = arenaAlloc<Arena>(&base, count);
    arenaCount = count;

    arenas[0] = base;
    for (u32 i = 1; i < arenaCount; ++i)
    {
        arenas[i] = {(u8*)block + i * size, size, 0};
    }
}

void deinitScratch()
{
    if (arenas != nullptr)
    {
        freeGpa(arenas[0].memory, arenas[0].capacity * arenaCount);
    }
}

Arena* getScratch(Arena const* const* conflicts, u32 count)
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
    Arena* scratch = getScratch();
    HG_ARENA_SCOPE(scratch);

    char* cpath = cString(scratch, data->path);

    FILE* fileHandle = fopen(cpath, "rb");
    if (fileHandle == nullptr)
    {
        formatError("Could not find file to read binary: %s", cpath);
        return;
    }
    HG_DEFER(fclose(fileHandle));

    if (fseek(fileHandle, 0, SEEK_END) != 0)
    {
        formatError("Failed to read binary from file: %s", cpath);
        return;
    }

    data->asset.size = (u64)ftell(fileHandle);
    data->asset.data = allocGpa(data->asset.size, 1);

    rewind(fileHandle);
    if (fread((void*)data->asset.data, 1, data->asset.size, fileHandle) != data->asset.size)
    {
        freeGpa((void*)data->asset.data, data->asset.size);
        formatError("Failed to read binary from file: %s", cpath);
        data->asset = {};
        return;
    }
}

template<>
void assetUnloadImpl(Asset<Binary>* data)
{
    freeGpa((void*)data->asset.data, data->asset.size);
}

bool binaryStore(Binary bin, String path)
{
    Arena* scratch = getScratch();
    HG_ARENA_SCOPE(scratch);

    char* cpath = cString(scratch, path);

    FILE* fileHandle = fopen(cpath, "wb");
    if (fileHandle == nullptr)
    {
        formatError("Failed to create file to write binary: %s", cpath);
        return false;
    }
    HG_DEFER(fclose(fileHandle));

    if (fwrite(bin.data, 1, bin.size, fileHandle) != bin.size)
    {
        formatError("Failed to write binary data to file: %s", cpath);
        return false;
    }

    return true;
}

} // namespace hg


#include "hg_assets.hpp"
#include "hg_audio.hpp"
#include "hg_concurrency.hpp"
#include "hg_containers.hpp"
#include "hg_core.hpp"
#include "hg_ecs.hpp"
#include "hg_gpu.hpp"
#include "hg_math.hpp"
#include "hg_memory.hpp"
#include "hg_platform.hpp"
#include "hg_rendering.hpp"
#include "hg_serialization.hpp"
#include "hg_strings.hpp"
#include "hg_templates.hpp"
#include "hg_time.hpp"
#include "hg_utils.hpp"
#include "hg_window.hpp"

#include <cstdio>
#include <cstring>
#include <random>

#include "stb_image.h"
#include "stb_image_write.h"

thread_local static char errorMessageData[4096];
thread_local static u64 errorMessageLength = 0;

void hgPrintStdout(HgString str)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    fputs(hgCString(scratch, str), stdout);
}

void hgPrintStderr(HgString str)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    fputs(hgCString(scratch, str), stderr);
}

void hgLogInternal(HgString format, ...)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    va_list args;
    va_start(args, format);

    HgStringBuilder begin = hgStringCopy(scratch, "HurdyGurdy Log: ");
    hgStringAppend(scratch, &begin, format);
    HgStringBuilder formatted = hgStringFormatVar(scratch, begin, args);
    hgPrintStderr(formatted);

    va_end(args);
}

void hgWarnInternal(HgString format, ...)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    va_list args;
    va_start(args, format);

    HgStringBuilder begin = hgStringCopy(scratch, "HurdyGurdy Warn: ");
    hgStringAppend(scratch, &begin, format);
    hgPrintStderr(hgStringFormatVar(scratch, begin, args));

    va_end(args);
}

void hgPanicInternal(HgString format, ...)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    va_list args;
    va_start(args, format);

    HgStringBuilder begin = hgStringCopy(scratch, "HurdyGurdy Panic: ");
    hgStringAppend(scratch, &begin, format);
    begin.length += hgStringFormat(scratch, "\tLast error: \"%.*s\"\n", (int)hgErrorGet().length, hgErrorGet().chars).length;
    hgPrintStderr(hgStringFormatVar(scratch, begin, args));

    va_end(args);

    abort();
}

HgString hgErrorGet()
{
    return {errorMessageData, errorMessageLength};
}

void hgErrorSet(HgString error)
{
    u64 newLength = hgMin(error.length, sizeof(errorMessageData));
    hgMemCopy(errorMessageData, error.chars, newLength);
    errorMessageLength = newLength;
}

void hgErrorFormat(HgString errorFmt, ...)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    va_list args;
    va_start(args, errorFmt);
    hgErrorSet(hgStringFormatVar(scratch, errorFmt, args));
    va_end(args);
}

static HgSubsystemFlags initialized = 0;

bool hgInit(HgSubsystemFlags init)
{
    if (init & HgSubsystem_memory)
    {
        hgScratchInit(2, (u64)1 << 24);
        initialized |= HgSubsystem_memory;
    }

    if (init & HgSubsystem_concurrency)
    {
        hgConcurrencyInit();
        initialized |= HgSubsystem_concurrency;
    }

    if (init & HgSubsystem_gpu ||
        init & HgSubsystem_windowing ||
        init & HgSubsystem_audio)
    {
        if (!hgPlatformInit())
            goto platformFailed;
    }

    if (init & HgSubsystem_gpu ||
        init & HgSubsystem_windowing)
    {
        if (!hgGpuInit())
            goto gpuFailed;
        initialized |= HgSubsystem_gpu;
    }

    if (init & HgSubsystem_assets)
    {
        hgAssetInitDefaults();
        initialized |= HgSubsystem_assets;
    }

    if (init & HgSubsystem_windowing)
    {
        hgWindowsInit();
        initialized |= HgSubsystem_windowing;
    }

    if (init & HgSubsystem_audio)
    {
        if (!hgAudioInit())
            goto audioFailed;
        initialized |= HgSubsystem_audio;
    }

    return true;

audioFailed:
    if (initialized & HgSubsystem_windowing)
        hgWindowsDeinit();
    if (initialized & HgSubsystem_assets)
        hgAssetDeinitDefaults();
    if (initialized & HgSubsystem_gpu)
        hgGpuDeinit();
gpuFailed:
    if (initialized & HgSubsystem_gpu ||
        initialized & HgSubsystem_windowing ||
        initialized & HgSubsystem_audio)
        hgPlatformDeinit();
platformFailed:
    if (initialized & HgSubsystem_concurrency)
        hgConcurrencyDeinit();
    if (initialized & HgSubsystem_memory)
        hgScratchDeinit();
    initialized = 0;
    return false;
}

void hgDeinit()
{
    if (initialized & HgSubsystem_audio)
        hgAudioDeinit();

    if (initialized & HgSubsystem_windowing)
        hgWindowsDeinit();

    if (initialized & HgSubsystem_assets)
        hgAssetDeinitDefaults();

    if (initialized & HgSubsystem_gpu)
        hgGpuDeinit();

    if (initialized & HgSubsystem_gpu ||
        initialized & HgSubsystem_windowing ||
        initialized & HgSubsystem_audio)
        hgPlatformDeinit();

    if (initialized & HgSubsystem_concurrency)
        hgConcurrencyDeinit();

    if (initialized & HgSubsystem_memory)
        hgScratchDeinit();

    initialized = 0;
}

void hgSwap(void* a, void* b, u64 size)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    void* tmp = hgArenaAlloc(scratch, size, 1);
    hgMemCopy(tmp, a, size);
    hgMemCopy(a, b, size);
    hgMemCopy(b, tmp, size);
}

void hgMemClear(void* dst, u64 size, u8 val)
{
    memset(dst, val, size);
}

void hgMemCopy(void* __restrict dst, const void* __restrict src, u64 size)
{
    memcpy(dst, src, size);
}

void hgMemMove(void* dst, const void* src, u64 size)
{
    memmove(dst, src, size);
}

bool hgMemEqual(const void* dst, const void* src, u64 size)
{
    return size == 0 || memcmp(dst, src, size) == 0;
}

void* hgGpaAlloc(u64 size, u64 alignment)
{
    (void)alignment;
    void* alloc = malloc(size);
    if (alloc == nullptr)
        hgPanic("malloc out of memory");
    return alloc;
}

void* hgGpaRealloc(void* allocation, u64 oldSize, u64 newSize, u64 alignment)
{
    (void)oldSize;
    (void)alignment;
    void* alloc = realloc(allocation, newSize);
    if (alloc == nullptr)
        hgPanic("malloc out of memory");
    return alloc;
}

template<>
void hgGpaFree(void* allocation, u64 size)
{
    (void)size;
    free(allocation);
}

void* hgArenaAlloc(HgArena* arena, u64 size, u64 alignment)
{
    hgAssert(arena != nullptr);

    u64 newHead = hgAlign((u64)arena->head, alignment) + size;
    if (newHead > arena->capacity)
    {
        hgErrorSet("Arena out of memory");
        return nullptr;
    }

    arena->head = newHead;
    return (void*)((uptr)arena->memory + arena->head - size);
}

void* hgArenaRealloc(HgArena* arena, void* allocation, u64 oldSize, u64 newSize, u64 alignment)
{
    hgAssert(arena != nullptr);

    if (allocation >= arena->memory && (uptr)allocation + oldSize <= (uptr)arena->memory + arena->capacity)
    {
        if ((uptr)allocation + oldSize - (uptr)arena->memory == (uptr)arena->head)
        {
            u64 newHead = (uptr)allocation + newSize - (uptr)arena->memory;
            if (newHead > arena->capacity)
            {
                hgErrorSet("Arena out of memory");
                return nullptr;
            }

            arena->head = newHead;
            return allocation;
        }

        if (newSize < oldSize)
            return allocation;
    }

    void* newAllocation = hgArenaAlloc(arena, newSize, alignment);
    if (allocation != nullptr)
        hgMemCopy(newAllocation, allocation, hgMin(oldSize, newSize));
    return newAllocation;
}

static thread_local HgArena* arenas{};
static thread_local u32 arenaCount = 0;

void hgScratchInit(u32 count, u64 size)
{
    size = hgAlign(size, 16);
    hgAssert(size < UINT64_MAX / count);

    void* block = hgGpaAlloc(count * size, 16);
    HgArena base = {block, size, 0};
    arenas = hgArenaAlloc<HgArena>(&base, count);
    arenaCount = count;

    arenas[0] = base;
    for (u32 i = 1; i < arenaCount; ++i)
    {
        arenas[i] = {(u8*)block + i * size, size, 0};
    }
}

void hgScratchDeinit()
{
    if (arenas != nullptr)
    {
        hgGpaFree(arenas[0].memory, arenas[0].capacity * arenaCount);
    }
}

HgArena* hgScratch(HgArena const* const* conflicts, u32 count)
{
    if (conflicts != nullptr)
        hgAssert(count > 0);

    for (u32 i = 0; i < arenaCount; ++i)
    {
        hgAssert(arenas[i].memory != nullptr);

        for (u32 j = 0; j < count; ++j)
        {
            if (&arenas[i] == conflicts[j])
                goto next;
        }
        return &arenas[i];
next:
        continue;
    }
    hgPanic("No scratch arena available\n");
}

void hgBinaryRead(HgBinary bin, u64 idx, void* dst, u64 len)
{
    hgAssert(idx + len <= bin.size);
    hgMemCopy(dst, (u8*)bin.data + idx, len);
}

void hgBinaryResize(HgArena* arena, HgBinaryBuilder* bin, u64 newSize)
{
    bin->data = hgArenaRealloc(arena, bin->data, bin->size, newSize, 1);
    bin->size = newSize;
}

void hgBinaryOverwrite(HgBinaryBuilder* bin, u64 idx, const void* src, u64 len)
{
    hgAssert(idx + len <= bin->size);
    hgMemCopy((u8*)bin->data + idx, src, len);
}

char* hgCString(HgArena* arena, HgString str)
{
    hgAssert(arena != nullptr);
    if (str.length > 0)
        hgAssert(str.chars != nullptr);

    char* cStr = hgArenaAlloc<char>(arena, str.length + 1);
    hgMemCopy(cStr, str.chars, str.length);
    cStr[str.length] = 0;
    return cStr;
}

HgString hgStringCreate(HgString data)
{
    HgString str{};
    if (data != "")
    {
        str.chars = hgGpaAlloc<char>(data.length);
        hgMemCopy((char*)str.chars, data.chars, data.length);
        str.length = data.length;
    }
    return str;
}

void hgStringDestroy(HgString* str)
{
    hgGpaFree((char*)str->chars, str->length);
}

HgStringBuilder hgStringCopy(HgArena* arena, HgString str)
{
    hgAssert(arena != nullptr);

    HgStringBuilder copy{};
    copy.chars = hgArenaAlloc<char>(arena, str.length);
    hgMemCopy(copy.chars, str.chars, str.length);
    copy.length = str.length;
    return copy;
}

HgStringBuilder hgStringFormat(HgArena* arena, HgString format, ...)
{
    va_list args;
    va_start(args, format);
    HgStringBuilder ret = hgStringFormatVar(arena, format, args);
    va_end(args);
    return ret;
}

HgStringBuilder hgStringFormatVar(HgArena* arena, HgString fmt, va_list args)
{
    HgArena* scratch = hgScratch(&arena, 1);
    hgArenaScope(scratch);

    int len = vsnprintf((char*)arena->memory + arena->head, arena->capacity - arena->head, hgCString(scratch, fmt), args);
    if (len < 0)
        hgPanic("snprintf returned an error");

    HgStringBuilder ret{(char*)arena->memory + arena->head, (u64)len};
    arena->head += (u64)len;

    return ret;
}

void hgStringInsert(HgArena* arena, HgStringBuilder* dst, u64 idx, HgString src)
{
    hgAssert(arena != nullptr);
    hgAssert(dst != nullptr);
    hgAssert(idx <= dst->length);
    if (src.length > 0)
        hgAssert(src.chars != nullptr);

    u64 newLength = dst->length + src.length;

    dst->chars = hgArenaRealloc(arena, dst->chars, dst->length, newLength);

    if (idx != dst->length)
        hgMemMove(&dst->chars[idx + src.length], &dst->chars[idx], dst->length - idx);
    hgMemCopy(&dst->chars[idx], src.chars, src.length);

    dst->length = newLength;
}

bool hgIsWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool hgIsNumeral(char c)
{
    return c >= '0' && c <= '9';
}

bool hgIsInteger(HgString str)
{
    if (str.length == 0)
        return false;

    u64 head = 0;
    if (!hgIsNumeral(str[head]) && str[head] != '+' && str[head] != '-')
        return false;

    ++head;
    while (head < str.length)
    {
        if (!hgIsNumeral(str[head]))
            return false;
        ++head;
    }
    return true;
}

bool hgIsFloat(HgString str)
{
    if (str.length == 0)
        return false;

    bool hasDecimal = false;
    bool hasExponent = false;

    u64 head = 0;

    if (!hgIsNumeral(str[head]) && str[head] != '.' && str[head] != '+' && str[head] != '-')
        return false;

    if (str[head] == '.')
        hasDecimal = true;

    ++head;
    while (head < str.length)
    {
        if (hgIsNumeral(str[head]))
        {
            ++head;
            continue;
        }

        if (str[head] == '.' && !hasDecimal)
        {
            hasDecimal = true;
            ++head;
            continue;
        }

        if (str[head] == 'e' && !hasExponent)
        {
            hasExponent = true;
            ++head;
            if (hgIsNumeral(str[head]) || str[head] == '+' || str[head] == '-')
            {
                ++head;
                continue;
            }
            return false;
        }

        if (str[head] == 'f' && head == str.length - 1)
            break;

        return false;
    }

    return hasDecimal || hasExponent;
}

i64 hgStringToInteger(HgString str)
{
    hgAssert(hgIsInteger(str));

    i64 power = 1;
    i64 ret = 0;

    u64 head = str.length - 1;
    while (head > 0)
    {
        ret += (i64)(str[head] - '0') * power;
        power *= 10;
        --head;
    }

    if (str[head] != '+')
    {
        if (str[head] == '-')
            ret *= -1;
        else
            ret += (i64)(str[head] - '0') * power;
    }

    return ret;
}

f64 hgStringToFloat(HgString str)
{
    hgAssert(hgIsFloat(str));

    f64 ret = 0.0;
    u64 head = 0;

    bool isNegative = str[head] == '-';
    if (isNegative || str[head] == '+')
        ++head;

    if (hgIsNumeral(str[head]))
    {
        u64 intPartBegin = head;
        while (head < str.length && str[head] != '.' && str[head] != 'e')
        {
            ++head;
        }
        ret += (f64)hgStringToInteger({&str[intPartBegin], &str[head]});
    }

    if (head < str.length && str[head] == '.')
    {
        ++head;

        f64 power = 0.1;
        while (head < str.length && hgIsNumeral(str[head]))
        {
            ret += (f64)(str[head] - '0') * power;
            power *= 0.1;
            ++head;
        }
    }

    if (head < str.length && str[head] == 'e')
    {
        ++head;

        bool expIsNegative = str[head] == '-';
        if (expIsNegative || str[head] == '+')
            ++head;

        u64 expBegin = head;
        while (head < str.length && hgIsNumeral(str[head]))
        {
            ++head;
        }

        i64 exp = hgStringToInteger({&str[expBegin], str.chars + head});
        if (exp != 0)
        {
            if (expIsNegative)
            {
                for (i64 i = 0; i < exp; ++i)
                {
                    ret *= 0.1;
                }
            } else {
                for (i64 i = 0; i < exp; ++i)
                {
                    ret *= 10.0;
                }
            }
        } else {
            ret = 1.0;
        }
    }

    if (isNegative)
        ret *= -1.0;

    return ret;
}

HgStringBuilder hgIntegerToString(HgArena* arena, i64 num)
{
    hgAssert(arena != nullptr);

    HgArena* scratch = hgScratch(&arena, 1);
    hgArenaScope(scratch);

    if (num == 0)
        return hgStringCopy(arena, "0");

    bool isNegative = num < 0;
    u64 unum = (u64)std::abs(num);

    HgStringBuilder reverse{};
    while (unum != 0)
    {
        u64 digit = unum % 10;
        unum = (u64)((f64)unum / 10.0);
        hgStringAppendC(scratch, &reverse, '0' + (char)digit);
    }

    HgStringBuilder ret{};
    if (isNegative)
        hgStringAppendC(arena, &ret, '-');
    for (u64 i = reverse.length - 1; i < reverse.length; --i)
    {
        hgStringAppendC(arena, &ret, reverse[i]);
    }
    return ret;
}

HgStringBuilder hgFloatToString(HgArena* arena, f64 num, u32 decimalCount)
{
    hgAssert(arena != nullptr);

    HgArena* scratch = hgScratch(&arena, 1);
    hgArenaScope(scratch);

    if (num == 0.0)
        return hgStringCopy(arena, "0.0");

    HgStringBuilder intStr = hgIntegerToString(scratch, (i64)std::abs(num));

    HgStringBuilder decStr{};
    hgStringAppendC(scratch, &decStr, '.');

    f64 decPart = std::abs(num);
    for (u64 i = 0; i < decimalCount; ++i)
    {
        decPart *= 10.0;
        hgStringAppendC(scratch, &decStr, '0' + (char)((u64)decPart % 10));
    }

    HgStringBuilder ret{};
    if (num < 0.0)
        hgStringAppendC(arena, &ret, '-');
    hgStringAppend(arena, &ret, intStr);
    hgStringAppend(arena, &ret, decStr);
    return ret;
}

const char* hgSerialTypeToString(HgSerialType s)
{
    switch (s)
    {
        case HgSerialType_object:
            return "HgSerialType_object";
        case HgSerialType_string:
            return "HgSerialType_string";
        case HgSerialType_integer:
            return "HgSerialType_integer";
        case HgSerialType_floating:
            return "HgSerialType_floating";
        case HgSerialType_boolean:
            return "HgSerialType_boolean";
        default:
            return "invalid HgSerialType";
    }
}

HgSerializer hgSerialWriter(HgArena* arena)
{
    HgSerializer s{};
    s.arena = arena;
    s.root = hgArenaAlloc<HgSerialNode>(arena, 1);
    s.root->parent = nullptr;
    s.root->next = nullptr;
    s.parent = nullptr;
    s.current = nullptr;
    s.writing = true;
    return s;
}

HgSerializer hgSerialReader(HgArena* arena, HgSerialNode* begin)
{
    HgSerializer s{};
    s.arena = arena;
    s.root = begin;
    s.parent = nullptr;
    s.current = nullptr;
    s.writing = false;
    return s;
}

void hgSerializeNodeStart(HgSerializer* s)
{
    if (s->writing)
    {
        if (s->current != nullptr)
        {
            s->current->next = hgArenaAlloc<HgSerialNode>(s->arena, 1);
            s->current = s->current->next;
            s->current->parent = s->parent;
            s->current->next = nullptr;
        }
        else
        {
            if (s->parent != nullptr)
            {
                s->current = hgArenaAlloc<HgSerialNode>(s->arena, 1);
                s->parent->children = s->current;
                s->current->parent = s->parent;
                s->current->next = nullptr;
            }
            else
            {
                hgAssert(s->root != nullptr);
                s->current = s->root;
            }
        }

        if (s->parent != nullptr)
            ++s->parent->count;
    }
    else
    {
        if (s->current != nullptr)
        {
            s->current = s->current->next;
        }
        else
        {
            if (s->parent != nullptr)
            {
                s->current = s->parent->children;
            }
            else
            {
                hgAssert(s->root != nullptr);
                s->current = s->root;
            }
        }
    }
}

void hgSerializeBegin(HgSerializer* s, u32* size)
{
    hgSerializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = HgSerialType_object;
        s->current->count = 0;
        s->current->children = nullptr;
    }
    else
    {
        if (size != nullptr)
            *size = s->current->count;
    }

    s->parent = s->current;
    s->current = nullptr;
}

void hgSerializeEnd(HgSerializer* s)
{
    hgAssert(s->parent != nullptr);

    s->current = s->parent;
    s->parent = s->parent->parent;
}

void hgSerializeVoid(HgSerializer* s, void* val, u32 size)
{
    hgSerializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = HgSerialType_string;
        s->current->string = hgStringCopy(s->arena, {(char*)val, size});
    }
    else
    {
        hgAssert(s->current->type == HgSerialType_string);
        hgAssert(s->current->string.length == size);
        hgMemCopy(val, s->current->string.chars, size);
    }
}

template<>
void hgSerialize(HgSerializer* s, HgBinary* val)
{
    hgSerialize(s, (HgString*)val);
}

template<>
void hgSerialize(HgSerializer* s, HgString* val)
{
    hgSerializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = HgSerialType_string;
        s->current->string = hgStringCopy(s->arena, *val);
    }
    else
    {
        hgAssert(s->current->type == HgSerialType_string);
        *val = hgStringCreate(s->current->string);
    }
}

template<>
void hgSerialize(HgSerializer* s, HgStringBuilder* val)
{
    hgSerializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = HgSerialType_string;
        s->current->string = hgStringCopy(s->arena, *val);
    }
    else
    {
        hgAssert(s->current->type == HgSerialType_string);
        *val = hgStringCopy(s->arena, s->current->string);
    }
}

template<typename T>
static void serializeInt(HgSerializer* s, T* val)
{
    hgSerializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = HgSerialType_integer;
        s->current->integer = (i64)*val;
    }
    else
    {
        hgAssert(s->current->type == HgSerialType_integer);
        *val = (T)s->current->integer;
    }
}

template<>
void hgSerialize(HgSerializer* s, u8* val)
{
    serializeInt(s, val);
}

template<>
void hgSerialize(HgSerializer* s, u16* val)
{
    serializeInt(s, val);
}

template<>
void hgSerialize(HgSerializer* s, u32* val)
{
    serializeInt(s, val);
}

template<>
void hgSerialize(HgSerializer* s, u64* val)
{
    serializeInt(s, val);
}

template<>
void hgSerialize(HgSerializer* s, i8* val)
{
    serializeInt(s, val);
}

template<>
void hgSerialize(HgSerializer* s, i16* val)
{
    serializeInt(s, val);
}

template<>
void hgSerialize(HgSerializer* s, i32* val)
{
    serializeInt(s, val);
}

template<>
void hgSerialize(HgSerializer* s, i64* val)
{
    serializeInt(s, val);
}

template<typename T>
static void serializeFloat(HgSerializer* s, T* val)
{
    hgSerializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = HgSerialType_floating;
        s->current->floating = (f64)*val;
    }
    else
    {
        hgAssert(s->current->type == HgSerialType_floating);
        *val = (T)s->current->floating;
    }
}

template<>
void hgSerialize(HgSerializer* s, f32* val)
{
    serializeFloat(s, val);
}

template<>
void hgSerialize(HgSerializer* s, f64* val)
{
    serializeFloat(s, val);
}

template<>
void hgSerialize(HgSerializer* s, bool* val)
{
    hgSerializeNodeStart(s);

    if (s->writing)
    {
        s->current->type = HgSerialType_boolean;
        s->current->boolean = *val;
    }
    else
    {
        hgAssert(s->current->type == HgSerialType_boolean);
        *val = s->current->boolean;
    }
}

template<>
void hgSerialize(HgSerializer* s, HgVec2* val)
{
    hgSerializeObject(s,
        &val->x,
        &val->y);
}

template<>
void hgSerialize(HgSerializer* s, HgVec3* val)
{
    hgSerializeObject(s,
        &val->x,
        &val->y,
        &val->z);
}

template<>
void hgSerialize(HgSerializer* s, HgVec4* val)
{
    hgSerializeObject(s,
        &val->x,
        &val->y,
        &val->z,
        &val->w);
}

template<>
void hgSerialize(HgSerializer* s, HgMat2* val)
{
    hgSerializeObject(s,
        &val->x,
        &val->y);
}

template<>
void hgSerialize(HgSerializer* s, HgMat3* val)
{
    hgSerializeObject(s,
        &val->x,
        &val->y,
        &val->z);
}

template<>
void hgSerialize(HgSerializer* s, HgMat4* val)
{
    hgSerializeObject(s,
        &val->x,
        &val->y,
        &val->z,
        &val->w);
}

template<>
void hgSerialize(HgSerializer* s, HgComplex* val)
{
    hgSerializeObject(s,
        &val->r,
        &val->i);
}

template<>
void hgSerialize(HgSerializer* s, HgQuat* val)
{
    hgSerializeObject(s,
        &val->r,
        &val->i,
        &val->j,
        &val->k);
}

static constexpr char serialBinTag[] = "HgData";
static constexpr u32 serialBinVersionMajor = 0;
static constexpr u32 serialBinVersionMinor = 0;
static constexpr u32 serialBinVersionPatch = 0;

struct SerialBinHeader {
    char tag[sizeof(serialBinTag)];
    u32 versionMajor;
    u32 versionMinor;
    u32 versionPatch;

    u32 nodeBegin;
};

struct SerialBinObject {
    u32 fieldCount;
    u32 fieldsBegin;
};

struct SerialBinString {
    u32 begin;
    u32 length;
};

struct SerialBinNode {
    HgSerialType type;
    union {
        SerialBinObject object;
        SerialBinString string;
        i64 integer;
        f64 floating;
        bool boolean;
    };
};

static void serialBinWriteNode(HgArena* arena, HgBinaryBuilder* bin, u32 idx, HgSerialNode* node);

static void serialBinWriteString(HgArena* arena, HgBinaryBuilder* bin, u32 idx, HgString string)
{
    SerialBinNode node{};
    node.type = HgSerialType_string;
    node.string.length = (u32)string.length;

    node.string.begin = (u32)bin->size;
    hgBinaryResize(arena, bin, bin->size + string.length);

    hgBinaryOverwrite(bin, idx, node);

    hgBinaryOverwrite(bin, node.string.begin, string.chars, string.length);
}

static void serialBinWriteInteger(HgBinaryBuilder* bin, u32 idx, i64 integer)
{
    SerialBinNode node{};
    node.type = HgSerialType_integer;
    node.integer = integer;
    hgBinaryOverwrite(bin, idx, node);
}

static void serialBinWriteFloating(HgBinaryBuilder* bin, u32 idx, f64 floating)
{
    SerialBinNode node{};
    node.type = HgSerialType_floating;
    node.floating = floating;
    hgBinaryOverwrite(bin, idx, node);
}

static void serialBinWriteBoolean(HgBinaryBuilder* bin, u32 idx, bool boolean)
{
    SerialBinNode node{};
    node.type = HgSerialType_boolean;
    node.boolean = boolean;
    hgBinaryOverwrite(bin, idx, node);
}

static void serialBinWriteObject(HgArena* arena, HgBinaryBuilder* bin, u32 idx, HgSerialNode* object)
{
    SerialBinNode node{};
    node.type = HgSerialType_object;
    node.object.fieldCount = object->count;

    node.object.fieldsBegin = (u32)bin->size;
    hgBinaryResize(arena, bin, bin->size + object->count * sizeof(SerialBinNode));

    hgBinaryOverwrite(bin, idx, node);

    HgSerialNode* data = object->children;
    for (u32 i = 0; i < object->count; ++i)
    {
        serialBinWriteNode(arena, bin, node.object.fieldsBegin + i * (u32)sizeof(SerialBinNode), data);
        data = data->next;
    }
}

static void serialBinWriteNode(HgArena* arena, HgBinaryBuilder* bin, u32 idx, HgSerialNode* node)
{
    switch (node->type)
    {
        case HgSerialType_object:
            serialBinWriteObject(arena, bin, idx, node);
            return;
        case HgSerialType_string:
            serialBinWriteString(arena, bin, idx, node->string);
            return;
        case HgSerialType_integer:
            serialBinWriteInteger(bin, idx, node->integer);
            return;
        case HgSerialType_floating:
            serialBinWriteFloating(bin, idx, node->floating);
            return;
        case HgSerialType_boolean:
            serialBinWriteBoolean(bin, idx, node->boolean);
            return;
        default:
            hgPanic("Invalid HgSerialType: %s\n", hgSerialTypeToString(node->type));
    }
}

HgBinary hgBinaryWriteSerial(HgArena* arena, HgSerializer* serial)
{
    HgBinaryBuilder bin{};

    SerialBinHeader header{};
    hgBinaryResize(arena, &bin, bin.size + sizeof(header));

    hgMemCopy(header.tag, serialBinTag, sizeof(serialBinTag));
    header.versionMajor = serialBinVersionMajor;
    header.versionMinor = serialBinVersionMinor;
    header.versionPatch = serialBinVersionPatch;

    header.nodeBegin = (u32)bin.size;
    hgBinaryResize(arena, &bin, bin.size + sizeof(SerialBinNode));

    hgBinaryOverwrite(&bin, 0, header);

    serialBinWriteNode(arena, &bin, header.nodeBegin, serial->current);

    return bin;
}

static void serialBinReadNode(HgBinary bin, u32 idx, HgSerializer* s);

static void serialBinReadObject(HgBinary bin, SerialBinObject object, HgSerializer* s)
{
    hgSerializeBegin(s);
    for (u32 i = 0; i < object.fieldCount; ++i)
    {
        serialBinReadNode(bin, object.fieldsBegin + i * (u32)sizeof(SerialBinNode), s);
    }
    hgSerializeEnd(s);
}

static void serialBinReadString(HgBinary bin, SerialBinString string, HgSerializer* s)
{
    HgString val = {(char*)bin.data + string.begin, string.length};
    hgSerialize(s, &val);
}

static void serialBinReadNode(HgBinary bin, u32 idx, HgSerializer* s)
{
    SerialBinNode node = hgBinaryRead<SerialBinNode>(bin, idx);
    switch (node.type)
    {
        case HgSerialType_object:
            serialBinReadObject(bin, node.object, s);
            return;
        case HgSerialType_string:
            serialBinReadString(bin, node.string, s);
            return;
        case HgSerialType_integer:
            hgSerialize(s, &node.integer);
            return;
        case HgSerialType_floating:
            hgSerialize(s, &node.floating);
            return;
        case HgSerialType_boolean:
            hgSerialize(s, &node.boolean);
            return;
        default:
            hgPanic("Invalid HgSerialType: %s\n", hgSerialTypeToString(node.type));
    }
}

HgSerializer hgBinaryReadSerial(HgArena* arena, HgBinary bin)
{
    SerialBinHeader header = hgBinaryRead<SerialBinHeader>(bin, 0);

    if (!hgMemEqual(header.tag, serialBinTag, sizeof(serialBinTag)))
    {
        hgWarn("Serial binary could not be read, does not have a header\n");
        return {};
    }
    else if (header.versionMajor != serialBinVersionMajor)
    {
        hgWarn("Serial binary has wrong major version: %d instead of %d", header.versionMajor, serialBinVersionMajor);
    }
    else if (header.versionMinor != serialBinVersionMinor)
    {
        hgWarn("Serial binary has wrong minor version: %d instead of %d", header.versionMinor, serialBinVersionMinor);
    }
    else if (header.versionPatch != serialBinVersionPatch)
    {
        hgWarn("Serial binary has wrong patch version: %d instead of %d", header.versionPatch, serialBinVersionPatch);
    }

    HgSerializer s = hgSerialWriter(arena);
    serialBinReadNode(bin, header.nodeBegin, &s);
    return hgSerialReader(arena, s.current);
}

static void serialJsonWriteNode(HgArena* arena, HgStringBuilder* str, u32 indentation, HgSerialNode* node);

static void serialJsonWriteString(HgArena* arena, HgStringBuilder* str, HgString string)
{
    hgStringAppendC(arena, str, '"');
    for (u32 i = 0; i < string.length; ++i)
    {
        switch (string[i])
        {
        case '\\':
            hgStringAppend(arena, str, "\\\\");
            break;
        case '\"':
            hgStringAppend(arena, str, "\\\"");
            break;
        case '\n':
            hgStringAppend(arena, str, "\\n");
            break;
        case '\r':
            hgStringAppend(arena, str, "\\r");
            break;
        case '\f':
            hgStringAppend(arena, str, "\\f");
            break;
        case '\b':
            hgStringAppend(arena, str, "\\b");
            break;
        default:
            hgStringAppendC(arena, str, string[i]);
            break;
        }
    }
    hgStringAppendC(arena, str, '"');
}

static void serialJsonWriteArray(HgArena* arena, HgStringBuilder* str, u32 indentation, HgSerialNode* node)
{
    if (node->count > 0)
    {
        hgStringAppend(arena, str, "[\n");

        HgSerialNode* elem = node->children;

        for (u32 i = 0; i < indentation + 1; ++i)
        {
            hgStringAppend(arena, str, "    ");
        }
        serialJsonWriteNode(arena, str, indentation + 1, elem);

        elem = elem->next;

        for (u32 i = 1; i < node->count; ++i)
        {
            hgStringAppend(arena, str, ",\n");
            for (u32 i = 0; i < indentation + 1; ++i)
            {
                hgStringAppend(arena, str, "    ");
            }
            serialJsonWriteNode(arena, str, indentation + 1, elem);

            elem = elem->next;
        }

        hgStringAppendC(arena, str, '\n');
        for (u32 i = 0; i < indentation; ++i)
        {
            hgStringAppend(arena, str, "    ");
        }
        hgStringAppendC(arena, str, ']');
    }
    else
    {
        hgStringAppend(arena, str, "[]");
    }
}

// static void serialJsonWriteObject(HgArena* arena, HgStringBuilder* str, u32 indentation, HgSerialNode* node)
// {
//     if (node->count > 0)
//     {
//         hgStringAppend(arena, str, "{\n");
//
//         HgSerialNode* field = node->children;
//
//         for (u32 i = 0; i < indentation + 1; ++i)
//         {
//             hgStringAppend(arena, str, "    ");
//         }
//         serialJsonWriteString(arena, str, field->name);
//         hgStringAppend(arena, str, " : ");
//         serialJsonWriteNode(arena, str, indentation + 1, field);
//
//         field = field->next;
//
//         for (u32 i = 1; i < node->count; ++i)
//         {
//             hgStringAppend(arena, str, ",\n");
//             for (u32 i = 0; i < indentation + 1; ++i)
//             {
//                 hgStringAppend(arena, str, "    ");
//             }
//             serialJsonWriteString(arena, str, field->name);
//             hgStringAppend(arena, str, " : ");
//             serialJsonWriteNode(arena, str, indentation + 1, field);
//
//             field = field->next;
//         }
//
//         hgStringAppendC(arena, str, '\n');
//         for (u32 i = 0; i < indentation; ++i)
//         {
//             hgStringAppend(arena, str, "    ");
//         }
//         hgStringAppendC(arena, str, '}');
//     }
//     else
//     {
//         hgStringAppend(arena, str, "{}");
//     }
// }

static void serialJsonWriteNode(HgArena* arena, HgStringBuilder* str, u32 indentation, HgSerialNode* node)
{
    switch (node->type)
    {
        case HgSerialType_object:
            serialJsonWriteArray(arena, str, indentation, node);
            return;
        case HgSerialType_string:
            serialJsonWriteString(arena, str, node->string);
            return;
        case HgSerialType_integer:
            hgStringAppend(arena, str, hgIntegerToString(hgScratch(), node->integer));
            return;
        case HgSerialType_floating:
            hgStringAppend(arena, str, hgFloatToString(hgScratch(), node->floating, 6));
            return;
        case HgSerialType_boolean:
            if (node->boolean)
                hgStringAppend(arena, str, "true");
            else
                hgStringAppend(arena, str, "false");
            return;
        default:
            hgPanic("Invalid HgSerialType: %s\n", hgSerialTypeToString(node->type));
    }
}

HgString hgJsonWriteSerial(HgArena* arena, HgSerializer* serial)
{
    HgStringBuilder str{};
    serialJsonWriteNode(arena, &str, 0, serial->current);
    hgStringAppendC(arena, &str, '\n');
    return str;
}

// HgSerializer hgJsonReadSerial(HgArena* arena, HgStringView json)
// {
// }

struct JsonParseState {
    HgString text;
    u64 head;
    u64 line;
};

static HgJson jsonParseNext(HgArena* arena, JsonParseState* state);
static HgJson jsonParseStruct(HgArena* arena, JsonParseState* state);
static HgJson jsonParseArray(HgArena* arena, JsonParseState* state);
static HgJson jsonParseString(HgArena* arena, JsonParseState* state);
static HgJson jsonParseNumber(HgArena* arena, JsonParseState* state);
static HgJson jsonParseBoolean(HgArena* arena, JsonParseState* state);

static HgJson jsonParseNext(HgArena* arena, JsonParseState* state)
{
    while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->head >= state->text.length)
        return {};

    switch (state->text[state->head])
    {
        case '{':
            ++state->head;
            return jsonParseStruct(arena, state);
        case '[':
            ++state->head;
            return jsonParseArray(arena, state);
        case '\'': [[fallthrough]];
        case '"':
            ++state->head;
            return jsonParseString(arena, state);
        case '.': [[fallthrough]];
        case '+': [[fallthrough]];
        case '-':
            return jsonParseNumber(arena, state);
        case 't': [[fallthrough]];
        case 'f':
            return jsonParseBoolean(arena, state);
        case '}': {
            HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            HgStringBuilder msg{};
            hgStringAppend(arena, &msg, "on line ");
            hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
            hgStringAppend(arena, &msg, ", found unexpected token \"}\"\n");
            error->msg = msg;
            return {nullptr, error};
        }
        case ']': {
            HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            HgStringBuilder msg{};
            hgStringAppend(arena, &msg, "on line ");
            hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
            hgStringAppend(arena, &msg, ", found unexpected token \"]\"\n");
            error->msg = msg;
            return {nullptr, error};
        }
    }
    if (hgIsNumeral(state->text[state->head]))
    {
        return jsonParseNumber(arena, state);
    }

    HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);
    error->next = nullptr;

    u64 begin = state->head;
    while (state->head < state->text.length && !hgIsWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    HgStringBuilder msg{};
    hgStringAppend(arena, &msg, "on line ");
    hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
    hgStringAppend(arena, &msg, ", found unexpected token \"");
    hgStringAppend(arena, &msg, {&state->text[begin], &state->text[state->head]});
    hgStringAppend(arena, &msg, "\"\n");
    error->msg = msg;

    return {nullptr, error};
}

static HgJson jsonParseStruct(HgArena* arena, JsonParseState* state)
{
    HgJson json{};
    json.file = hgArenaAlloc<HgJsonNode>(arena, 1);
    json.file->type = HgJsonType::HgJsonType_struct;
    json.file->jstruct.fields = nullptr;

    HgJsonField* lastField = nullptr;
    HgJsonError* lastError = nullptr;

    for (;;)
    {
        while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head >= state->text.length)
        {
            HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            HgStringBuilder msg{};
            hgStringAppend(arena, &msg, "on line ");
            hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
            hgStringAppend(arena, &msg, ", expected struct to terminate\n");
            error->msg = msg;
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            break;
        }
        if (state->text[state->head] == ']')
        {
            HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            HgStringBuilder msg{};
            hgStringAppend(arena, &msg, "on line ");
            hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
            hgStringAppend(arena, &msg, ", struct ends with \"]\" instead of \"}\"\n");
            error->msg = msg;
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            ++state->head;
            while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }
        if (state->text[state->head] == '}')
        {
            ++state->head;
            while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }

        HgJson value = jsonParseNext(arena, state);

        if (value.file != nullptr)
        {
            if (value.file->type != HgJsonType::HgJsonType_field)
            {
                HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);
                error->next = nullptr;
                HgStringBuilder msg{};
                hgStringAppend(arena, &msg, "on line ");
                hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
                hgStringAppend(arena, &msg, ", struct has a literal instead of a field\n");
                error->msg = msg;
                if (lastError == nullptr)
                    json.errors = lastError = error;
                else
                    lastError->next = error;
                lastError = error;
            } else if (value.file->field.value == nullptr)
            {
                HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);
                error->next = nullptr;
                HgStringBuilder msg{};
                hgStringAppend(arena, &msg, "on line ");
                hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
                hgStringAppend(arena, &msg, ", struct has a field named \"");
                hgStringAppend(arena, &msg, value.file->field.name);
                hgStringAppend(arena, &msg, "\" which has no value\n");
                error->msg = msg;
                if (lastError == nullptr)
                    json.errors = lastError = error;
                else
                    lastError->next = error;
                lastError = error;
            } else {
                if (lastField == nullptr)
                    json.file->jstruct.fields = &value.file->field;
                else
                    lastField->next = &value.file->field;
                lastField = &value.file->field;
            }
        }
        if (value.errors != nullptr)
        {
            if (lastError == nullptr)
                json.errors = lastError = value.errors;
            else
                lastError->next = value.errors;
            lastError = value.errors;
        }
    }

    return json;
}

static HgJson jsonParseArray(HgArena* arena, JsonParseState* state)
{
    HgJson json{};
    json.file = hgArenaAlloc<HgJsonNode>(arena, 1);
    json.file->type = HgJsonType::HgJsonType_array;

    HgJsonType type = HgJsonType::HgJsonType_none;
    HgJsonElem* lastElem = nullptr;
    HgJsonError* lastError = nullptr;

    for (;;)
    {
        while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head >= state->text.length)
        {
            HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            HgStringBuilder msg{};
            hgStringAppend(arena, &msg, "on line ");
            hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
            hgStringAppend(arena, &msg, ", expected struct to terminate\n");
            error->msg = msg;
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            break;
        }
        if (state->text[state->head] == '}')
        {
            HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);
            error->next = nullptr;
            HgStringBuilder msg{};
            hgStringAppend(arena, &msg, "on line ");
            hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
            hgStringAppend(arena, &msg, ", array ends with \"}\" instead of \"]\"\n");
            error->msg = msg;
            if (lastError == nullptr)
                json.errors = lastError = error;
            else
                lastError->next = error;
            lastError = error;
            ++state->head;
            while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }
        if (state->text[state->head] == ']')
        {
            ++state->head;
            while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
            {
                if (state->text[state->head] == '\n')
                    ++state->line;
                ++state->head;
            }
            if (state->head < state->text.length && state->text[state->head] == ',')
                ++state->head;
            break;
        }

        HgJsonElem* elem = hgArenaAlloc<HgJsonElem>(arena, 1);
        elem->next = nullptr;

        HgJson value = jsonParseNext(arena, state);
        elem->value = value.file;

        if (value.file != nullptr)
        {
            if (type == HgJsonType::HgJsonType_none)
            {
                if (value.file->type != HgJsonType::HgJsonType_field)
                {
                    type = value.file->type;
                } else {
                    HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);
                    error->next = nullptr;
                    HgStringBuilder msg{};
                    hgStringAppend(arena, &msg, "on line ");
                    hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
                    hgStringAppend(arena, &msg, ", array has a field as an element\n");
                    error->msg = msg;
                    if (lastError == nullptr)
                        json.errors = lastError = error;
                    else
                        lastError->next = error;
                    lastError = error;
                }
            }
            if (value.file->type != type)
            {
                HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);
                error->next = nullptr;
                HgStringBuilder msg{};
                hgStringAppend(arena, &msg, "on line ");
                hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
                hgStringAppend(arena, &msg, ", array has element which is not the same type as the first valid element\n");
                error->msg = msg;
                if (lastError == nullptr)
                    json.errors = lastError = error;
                else
                    lastError->next = error;
                lastError = error;
            } else {
                if (lastElem == nullptr)
                    json.file->array.elems = elem;
                else
                    lastElem->next = elem;
                lastElem = elem;
            }
        }
        if (value.errors != nullptr)
        {
            if (lastError == nullptr)
                json.errors = lastError = value.errors;
            else
                lastError->next = value.errors;
            lastError = value.errors;
        }
    }

    return json;
}

static HgJson jsonParseString(HgArena* arena, JsonParseState* state)
{
    u64 begin = state->head;
    while (state->head < state->text.length && state->text[state->head] != '"')
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    u64 end = state->head;
    if (state->head < state->text.length)
    {
        ++state->head;
        HgStringBuilder str{};
        for (u64 i = begin; i < end; ++i)
        {
            char c = state->text[i];
            if (c == '\\')
            {
                // escape sequences : TODO
            }
            hgStringAppendC(arena, &str, c);
        }

        HgJson json{};
        json.file = hgArenaAlloc<HgJsonNode>(arena, 1);

        while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ':')
        {
            ++state->head;
            json.file->type = HgJsonType::HgJsonType_field;
            json.file->field.next = nullptr;
            json.file->field.name = str;
            HgJson next = jsonParseNext(arena, state);
            json.file->field.value = next.file;
            json.errors = next.errors;
        } else {
            json.file->type = HgJsonType::HgJsonType_string;
            json.file->string = str;
        }
        while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ',')
            ++state->head;
        return json;
    }

    HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);
    HgStringBuilder msg{};
    hgStringAppend(arena, &msg, "on line ");
    hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
    hgStringAppend(arena, &msg, ", expected string to terminate\n");
    error->msg = msg;
    return {nullptr, error};
}

static HgJson jsonParseNumber(HgArena* arena, JsonParseState* state)
{
    bool isFloat = false;
    u64 begin = state->head;
    while (state->head < state->text.length && (
        hgIsNumeral(state->text[state->head]) ||
        state->text[state->head] == '-' ||
        state->text[state->head] == '+' ||
        state->text[state->head] == '.' ||
        state->text[state->head] == 'e'
    ))
    {
        if (state->text[state->head] == '.' || state->text[state->head] == 'e')
            isFloat = true;
        ++state->head;
    }
    HgString num{&state->text[begin], &state->text[state->head]};
    while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->head < state->text.length && state->text[state->head] == ',')
        ++state->head;

    if (isFloat)
    {
        if (hgIsFloat(num))
        {
            HgJsonNode* node = hgArenaAlloc<HgJsonNode>(arena, 1);
            node->type = HgJsonType::HgJsonType_float;
            node->floating = hgStringToFloat(num);
            return {node, nullptr};
        }
    } else {
        if (hgIsInteger(num))
        {
            HgJsonNode* node = hgArenaAlloc<HgJsonNode>(arena, 1);
            node->type = HgJsonType::HgJsonType_integer;
            node->integer = hgStringToInteger(num);
            return {node, nullptr};
        }
    }

    HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);

    HgStringBuilder msg{};
    hgStringAppend(arena, &msg, "on line ");
    hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
    hgStringAppend(arena, &msg, ", expected numeral value, found \"");
    hgStringAppend(arena, &msg, num);
    hgStringAppend(arena, &msg, "\"\n");
    error->msg = msg;

    while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->text[state->head] == '}' || state->text[state->head] == ']')
    {
        return {nullptr, error};
    } else {
        HgJson next = jsonParseNext(arena, state);
        error->next = next.errors;
        return {next.file, error};
    }
}

static HgJson jsonParseBoolean(HgArena* arena, JsonParseState* state)
{
    if (state->head + 4 <= state->text.length && HgString{&state->text[state->head], 4} == "true")
    {
        state->head += 4;
        while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ',')
            ++state->head;

        HgJsonNode* node = hgArenaAlloc<HgJsonNode>(arena, 1);
        node->type = HgJsonType::HgJsonType_bool;
        node->boolean = true;
        return {node, nullptr};
    }
    if (state->head + 5 <= state->text.length && HgString{&state->text[state->head], 5} == "false")
    {
        state->head += 5;
        while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
        {
            if (state->text[state->head] == '\n')
                ++state->line;
            ++state->head;
        }
        if (state->head < state->text.length && state->text[state->head] == ',')
            ++state->head;

        HgJsonNode* node = hgArenaAlloc<HgJsonNode>(arena, 1);
        node->type = HgJsonType::HgJsonType_bool;
        node->boolean = false;
        return {node, nullptr};
    }

    HgJsonError* error = hgArenaAlloc<HgJsonError>(arena, 1);

    u64 begin = state->head;
    while (state->head < state->text.length && !hgIsWhitespace(state->text[state->head])
        && state->text[state->head] != ','
        && state->text[state->head] != '}'
        && state->text[state->head] != ']'
    )
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    HgStringBuilder msg{};
    hgStringAppend(arena, &msg, "on line ");
    hgStringAppend(arena, &msg, hgIntegerToString(arena, (i64)state->line));
    hgStringAppend(arena, &msg, ", expected boolean value, found \"");
    hgStringAppend(arena, &msg, {&state->text[begin], &state->text[state->head]});
    hgStringAppend(arena, &msg, "\"\n");
    error->msg = msg;

    if (state->text[state->head] == ',')
        ++state->head;

    while (state->head < state->text.length && hgIsWhitespace(state->text[state->head]))
    {
        if (state->text[state->head] == '\n')
            ++state->line;
        ++state->head;
    }
    if (state->text[state->head] == '}' || state->text[state->head] == ']')
    {
        return {nullptr, error};
    } else {
        HgJson next = jsonParseNext(arena, state);
        error->next = next.errors;
        return {next.file, error};
    }
}

HgJson hgParseJson(HgArena* arena, HgString text)
{
    hgAssert(arena != nullptr);
    if (text.length > 0)
        hgAssert(text.chars != nullptr);

    JsonParseState parseState{};
    parseState.text = text;
    parseState.head = 0;
    parseState.line = 1;
    return jsonParseNext(arena, &parseState);
}

void hgAssetInitDefaults()
{
    hgAssetInit<HgBinary>();
    hgAssetInit<HgTextureData>();
    hgAssetInit<HgTexture>();
    hgAssetInit<HgMeshData>();
    hgAssetInit<HgMesh>();
    hgAssetInit<HgSound>();
}

void hgAssetDeinitDefaults()
{
    hgAssetDeinit<HgSound>();
    hgAssetDeinit<HgMesh>();
    hgAssetDeinit<HgMeshData>();
    hgAssetDeinit<HgTexture>();
    hgAssetDeinit<HgTextureData>();
    hgAssetDeinit<HgBinary>();
}

template<>
void hgAssetLoadImpl(HgAsset<HgBinary>* data)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    char* cpath = hgCString(scratch, data->path);

    FILE* fileHandle = fopen(cpath, "rb");
    if (fileHandle == nullptr)
    {
        hgErrorFormat("Could not find file to read binary: %s", cpath);
        return;
    }
    hgDefer(fclose(fileHandle));

    if (fseek(fileHandle, 0, SEEK_END) != 0)
    {
        hgErrorFormat("Failed to read binary from file: %s", cpath);
        return;
    }

    data->asset.size = (u64)ftell(fileHandle);
    data->asset.data = hgGpaAlloc(data->asset.size, 1);

    rewind(fileHandle);
    if (fread((void*)data->asset.data, 1, data->asset.size, fileHandle) != data->asset.size)
    {
        hgGpaFree((void*)data->asset.data, data->asset.size);
        hgErrorFormat("Failed to read binary from file: %s", cpath);
        data->asset = {};
        return;
    }
}

template<>
void hgAssetUnloadImpl(HgAsset<HgBinary>* data)
{
    hgGpaFree((void*)data->asset.data, data->asset.size);
}

bool hgBinaryStore(HgBinary bin, HgString path)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    char* cpath = hgCString(scratch, path);

    FILE* fileHandle = fopen(cpath, "wb");
    if (fileHandle == nullptr)
    {
        hgErrorFormat("Failed to create file to write binary: %s", cpath);
        return false;
    }
    hgDefer(fclose(fileHandle));

    if (fwrite(bin.data, 1, bin.size, fileHandle) != bin.size)
    {
        hgErrorFormat("Failed to write binary data to file: %s", cpath);
        return false;
    }

    return true;
}

template<>
void hgAssetLoadImpl(HgAsset<HgJson>* data)
{
    HgBinaryAsset* bin = hgAssetLoad<HgBinary>(data->path);
    hgDefer(hgAssetUnload(bin));

    HgArena* scratch = hgScratch();
    u64 head = scratch->head;
    hgDefer(scratch->head = head);

    HgString jsonStr = {(char*)bin->asset.data, bin->asset.size};
    HgJson parse = hgParseJson(scratch, jsonStr);

    HgJsonError* e = parse.errors;
    if (e != nullptr)
        hgErrorSet("Json parse errors");
    while (e != nullptr)
    {
        hgWarn("Json parse error: %.*s\n", (int)e->msg.length, e->msg.chars);
        e = e->next;
    }

    data->asset.file = (HgJsonNode*)malloc(scratch->head - head);
    if (parse.errors != nullptr)
    {
        data->asset.errors = (HgJsonError*)(
            (u8*)data->asset.file +
                ((uptr)parse.errors - (uptr)parse.file));
    }
    else
    {
        data->asset.errors = nullptr;
    }
    hgMemCopy((void*)data->asset.file, (void*)parse.file, scratch->head - head);
}

template<>
void hgAssetUnloadImpl(HgAsset<HgJson>* data)
{
    free(data->asset.file);
}

template<>
void hgSerialize(HgSerializer* s, HgArrayAny* arr)
{
    hgSerializeBegin(s);
    hgSerialize(s, &arr->width);
    hgSerialize(s, &arr->align);
    hgSerialize(s, &arr->count);
    hgSerialize(s, &arr->capacity);
    if (!s->writing)
        arr->vals = hgGpaAlloc(arr->capacity * arr->width, arr->align);
    hgSerializeVoid(s, arr->vals, arr->count * arr->width);
    hgSerializeEnd(s);
}

HgArrayAny hgArrayAnyCreate(u32 width, u32 align, u32 count, u32 capacity)
{
    if (capacity < count)
        capacity = count;

    HgArrayAny arr{};
    arr.vals = hgGpaAlloc(capacity * width, align);
    arr.count = count;
    arr.capacity = capacity;
    arr.width = width;
    arr.align = align;

    return arr;
}

void hgArrayAnyDestroy(HgArrayAny* arr)
{
    hgAssert(arr != nullptr);

    hgGpaFree(arr->vals, arr->capacity * arr->width);
}

HgArrayAny hgArrayAnyTemp(HgArena* arena, u32 width, u32 align, u32 count, u32 capacity)
{
    hgAssert(arena != nullptr);
    hgAssert(count <= capacity);

    HgArrayAny arr{};
    arr.vals = hgArenaAlloc(arena, capacity * width, align);
    arr.count = count;
    arr.capacity = capacity;
    arr.width = width;
    arr.align = align;

    return arr;
}

void hgArrayAnyResize(HgArrayAny* arr, u32 newCount)
{
    if (newCount > arr->capacity)
    {
        arr->vals = hgGpaRealloc(
            arr->vals,
            arr->capacity * arr->width,
            newCount * 2 * arr->width,
            arr->align);
        arr->capacity = newCount * 2;
    }
    arr->count = newCount;
}

void hgArrayAnyResizeTemp(HgArena* arena, HgArrayAny* arr, u32 newCount)
{
    if (newCount > arr->capacity)
    {
        arr->vals = hgArenaRealloc(
            arena,
            arr->vals,
            arr->capacity * arr->width,
            newCount * 2 * arr->width,
            arr->align);
        arr->capacity = newCount * 2;
    }
    arr->count = newCount;
}

void* hgArrayAnyPush(HgArrayAny* arr)
{
    if (arr->count == arr->capacity)
    {
        u32 newCapacity = arr->capacity == 0 ? 16 : arr->capacity * 2;
        arr->vals = hgGpaRealloc(
            arr->vals,
            arr->capacity * arr->width,
            newCapacity * arr->width,
            arr->align);
        arr->capacity = newCapacity;
    }
    return (u8*)arr->vals + arr->count++ * arr->width;
}

void* hgArrayAnyPushTemp(HgArena* arena, HgArrayAny* arr)
{
    if (arr->count == arr->capacity)
    {
        u32 newCapacity = arr->capacity == 0 ? 16 : arr->capacity * 2;
        arr->vals = hgArenaRealloc(
            arena,
            arr->vals,
            arr->capacity * arr->width,
            newCapacity * arr->width,
            arr->align);
        arr->capacity = newCapacity;
    }
    return (u8*)arr->vals + arr->count++ * arr->width;
}

void hgArrayAnyRemove(HgArrayAny* arr, u32 idx, void* dst)
{
    hgAssert(idx < arr->count);

    hgMemCopy(dst, (*arr)[idx], arr->width);
    if (idx + 1 < arr->count)
    {
        hgMemCopy(
            (*arr)[idx],
            (*arr)[idx + 1],
            (arr->count - (idx + 1)) * arr->width);
    }
    --arr->count;
}

void hgArrayAnyRemoveSwap(HgArrayAny* arr, u32 idx, void* dst)
{
    hgAssert(idx < arr->count);

    hgMemCopy(dst, (*arr)[idx], arr->width);
    if (idx + 1 < arr->count)
    {
        hgMemCopy(
            (*arr)[idx],
            (*arr)[arr->count - 1],
            arr->width);
    }
    --arr->count;
}

void hgArrayAnyPop(HgArrayAny* arr, void* dst)
{
    hgAssert(arr->count > 0);
    --arr->count;
    if (dst != nullptr)
        hgMemCopy(dst, (u8*)arr->vals + arr->count * arr->width, arr->width);
}

static constexpr u32 poolStockSize = 1024;

static void poolRestock(HgPool* pool)
{
    hgAssert(pool != nullptr);

    void* store = hgGpaAlloc(poolStockSize * pool->width, pool->align);
    for (u32 i = 0; i < poolStockSize; ++i)
    {
        hgQueuePushBack(&pool->freeList, (u8*)store + i * pool->width);
    }
    *hgArrayPush(&pool->itemStores) = store;
}

HgPool hgPoolCreate(u32 width, u32 align)
{
    HgPool pool{};
    pool.freeList = hgQueueCreate<void*>();
    pool.itemStores = hgArrayCreate<void*>(0, 4);
    pool.width = width;
    pool.align = align;
    poolRestock(&pool);
    return pool;
}

void hgPoolDestroy(HgPool* pool)
{
    hgAssert(pool != nullptr);

    for (u32 i = 0; i < pool->itemStores.count; ++i)
    {
        hgGpaFree(pool->itemStores[i], poolStockSize);
    }
    hgArrayDestroy(&pool->itemStores);
    hgQueueDestroy(&pool->freeList);
}

void* hgPoolAlloc(HgPool* pool)
{
    hgAssert(pool != nullptr);

    if (pool->freeList.count == 0)
    {
        poolRestock(pool);
    }

    return hgQueuePopFront(&pool->freeList);
}

void hgPoolFree(HgPool* pool, void* item)
{
    hgAssert(pool != nullptr);
    hgAssert(item != nullptr);
    hgQueuePushFront(&pool->freeList, item);
}

HgHandlePool hgHandlePoolCreate()
{
    HgHandlePool handles{};
    handles.handles = hgArrayCreate<HgHandle>();
    handles.freed = hgArrayCreate<HgHandle>();

    hgHandlePoolAlloc(&handles);

    return handles;
}

void hgHandlePoolDestroy(HgHandlePool* pool)
{
    hgAssert(pool != nullptr);

    hgArrayDestroy(&pool->handles);
    hgArrayDestroy(&pool->freed);
}

void hgHandlePoolReset(HgHandlePool* pool)
{
    hgAssert(pool != nullptr);

    pool->handles.count = 0;
    pool->freed.count = 0;

    hgHandlePoolAlloc(pool);
}

HgHandle hgHandlePoolAlloc(HgHandlePool* pool)
{
    hgAssert(pool != nullptr);

    if (pool->freed.count > 0)
    {
        HgHandle handle = hgArrayPop(&pool->freed);
        pool->handles[hgHandleIdx(handle)] = handle;
        return handle;
    }
    else
    {
        HgHandle handle = {pool->handles.count};
        *hgArrayPush(&pool->handles) = handle;
        return handle;
    }
}

bool hgHandlePoolAlive(HgHandlePool* pool, HgHandle handle)
{
    hgAssert(pool != nullptr);

    u32 idx = hgHandleIdx(handle);
    return handle != hgHandleNull && idx < pool->handles.count && pool->handles[idx] == handle;
}

void hgHandlePoolFree(HgHandlePool* pool, HgHandle handle)
{
    hgAssert(pool != nullptr);
    hgAssert(hgHandlePoolAlive(pool, handle));
    pool->handles[hgHandleIdx(handle)] = hgHandleNull;
    *hgArrayPush(&pool->freed) = hgHandleNextGeneration(handle);
}

HgVec2& HgVec2::operator+=(HgVec2 other)
{
    x += other.x;
    y += other.y;
    return* this;
}

HgVec2& HgVec2::operator-=(HgVec2 other)
{
    x -= other.x;
    y -= other.y;
    return* this;
}

HgVec2& HgVec2::operator*=(HgVec2 other)
{
    x *= other.x;
    y *= other.y;
    return* this;
}

HgVec2& HgVec2::operator/=(HgVec2 other)
{
    x /= other.x;
    y /= other.y;
    return* this;
}

HgVec3& HgVec3::operator+=(HgVec3 other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return* this;
}

HgVec3& HgVec3::operator-=(HgVec3 other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return* this;
}

HgVec3& HgVec3::operator*=(HgVec3 other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return* this;
}

HgVec3& HgVec3::operator/=(HgVec3 other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return* this;
}

HgVec4& HgVec4::operator+=(HgVec4 other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return* this;
}

HgVec4& HgVec4::operator-=(HgVec4 other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return* this;
}

HgVec4& HgVec4::operator*=(HgVec4 other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
    return* this;
}

HgVec4& HgVec4::operator/=(HgVec4 other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return* this;
}

HgMat2& HgMat2::operator+=(const HgMat2& other)
{
    x += other.x;
    y += other.y;
    return* this;
}

HgMat2& HgMat2::operator-=(const HgMat2& other)
{
    x -= other.x;
    y -= other.y;
    return* this;
}

HgMat3& HgMat3::operator+=(const HgMat3& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return* this;
}

HgMat3& HgMat3::operator-=(const HgMat3& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return* this;
}

HgMat4& HgMat4::operator+=(const HgMat4& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return* this;
}

HgMat4& HgMat4::operator-=(const HgMat4& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return* this;
}

void hgMatTranspose(u32 width, u32 height, f32* dst, const f32* mat)
{
    for (u32 i = 0; i < width; ++i)
    {
        for (u32 j = 0; j < height; ++j)
        {
            dst[j * width + i] = mat[i * height + j];
        }
    }
}

HgMat2 hgMatTranspose2(const HgMat2& mat)
{
    HgMat2 ret;
    hgMatTranspose(2, 2, &ret.x.x, &mat.x.x);
    return ret;
}

HgMat3 hgMatTranspose3(const HgMat3& mat)
{
    HgMat3 ret;
    hgMatTranspose(3, 3, &ret.x.x, &mat.x.x);
    return ret;
}

HgMat4 hgMatTranspose4(const HgMat4& mat)
{
    HgMat4 ret;
    hgMatTranspose(4, 4, &ret.x.x, &mat.x.x);
    return ret;
}

HgComplex& HgComplex::operator+=(HgComplex other)
{
    r += other.r;
    i += other.i;
    return* this;
}

HgComplex& HgComplex::operator-=(HgComplex other)
{
    r -= other.r;
    i -= other.i;
    return* this;
}

HgQuat& HgQuat::operator+=(HgQuat other)
{
    r += other.r;
    i += other.i;
    j += other.j;
    k += other.k;
    return* this;
}

HgQuat& HgQuat::operator-=(HgQuat other)
{
    r -= other.r;
    i -= other.i;
    j -= other.j;
    k -= other.k;
    return* this;
}

void hgVecAdd(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = lhs[i] + rhs[i];
    }
}

void hgVecSub(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = lhs[i] - rhs[i];
    }
}

void hgVecMulPairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = lhs[i] * rhs[i];
    }
}

void hgVecMulScalar(u32 size, f32* dst, f32 scalar, const f32* vec)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = scalar * vec[i];
    }
}

void hgVecDivPairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        hgAssert(rhs[i] != 0);
        dst[i] = lhs[i] / rhs[i];
    }
}

void hgVecDivScalar(u32 size, f32* dst, const f32* vec, f32 scalar)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    hgAssert(scalar != 0);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = vec[i] / scalar;
    }
}

void hgVecDot(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    *dst = 0;
    for (u32 i = 0; i < size; ++i)
    {
        *dst += lhs[i] * rhs[i];
    }
}

void hgVecLenSqr(u32 size, f32* dst, const f32* vec)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    hgVecDot(size, dst, vec, vec);
}

void hgVecLen(u32 size, f32* dst, const f32* vec)
{
    hgVecLenSqr(size, dst, vec);
    *dst = std::sqrt(*dst);
}

f32 hgVecLen2(HgVec2 vec)
{
    return std::sqrt(hgVecLenSqr2(vec));
}

f32 hgVecLen3(HgVec3 vec)
{
    return std::sqrt(hgVecLenSqr3(vec));
}

f32 hgVecLen4(HgVec4 vec)
{
    return std::sqrt(hgVecLenSqr4(vec));
}

void hgVecNorm(u32 size, f32* dst, const f32* vec)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    f32 len;
    hgVecLen(size, &len, vec);
    hgAssert(len != 0);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = vec[i] / len;
    }
}

HgVec2 hgVecNorm2(HgVec2 vec)
{
    f32 len = hgVecLen2(vec);
    hgAssert(len != 0);
    return {vec.x / len, vec.y / len};
}

HgVec3 hgVecNorm3(HgVec3 vec)
{
    f32 len = hgVecLen3(vec);
    hgAssert(len != 0);
    return {vec.x / len, vec.y / len, vec.z / len};
}

HgVec4 hgVecNorm4(HgVec4 vec)
{
    f32 len = hgVecLen4(vec);
    hgAssert(len != 0);
    return {vec.x / len, vec.y / len, vec.z / len, vec.w / len};
}

void hgVecCross3(f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
    dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
    dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
}

f32 hgVecCross2(HgVec2 lhs, HgVec2 rhs)
{
    return lhs.x * rhs.y - lhs.y * rhs.x;
}

HgVec3 hgVecCross3(HgVec3 lhs, HgVec3 rhs)
{
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x
    };
}

void hgMatAdd(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < width; ++i)
    {
        for (u32 j = 0; j < height; ++j)
        {
            dst[i * width + j] = lhs[i * width + j] + rhs[i * width + j];
        }
    }
}

HgMat2 operator+(const HgMat2& lhs, const HgMat2& rhs)
{
    HgMat2 result{};
    hgMatAdd(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat3 operator+(const HgMat3& lhs, const HgMat3& rhs)
{
    HgMat3 result{};
    hgMatAdd(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat4 operator+(const HgMat4& lhs, const HgMat4& rhs)
{
    HgMat4 result{};
    hgMatAdd(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

void hgMatSub(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    for (u32 i = 0; i < width; ++i)
    {
        for (u32 j = 0; j < height; ++j)
        {
            dst[i * width + j] = lhs[i * width + j] - rhs[i * width + j];
        }
    }
}

HgMat2 operator-(const HgMat2& lhs, const HgMat2& rhs)
{
    HgMat2 result{};
    hgMatSub(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat3 operator-(const HgMat3& lhs, const HgMat3& rhs)
{
    HgMat3 result{};
    hgMatSub(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

HgMat4 operator-(const HgMat4& lhs, const HgMat4& rhs)
{
    HgMat4 result{};
    hgMatSub(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

void hgMatMul(f32* dst, u32 wl, u32 hl, const f32* lhs, u32 wr, u32 hr, const f32* rhs)
{
    hgAssert(hr == wl);
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    (void)hr;
    for (u32 i = 0; i < wl; ++i)
    {
        for (u32 j = 0; j < wr; ++j)
        {
            dst[i * wl + j] = 0.0f;
            for (u32 k = 0; k < hl; ++k)
            {
                dst[i * wl + j] += lhs[k * wl + j] * rhs[i * wr + k];
            }
        }
    }
}

HgMat2 operator*(const HgMat2& lhs, const HgMat2& rhs)
{
    HgMat2 result{};
    hgMatMul(&result.x.x, 2, 2, &lhs.x.x, 2, 2, &rhs.x.x);
    return result;
}

HgMat3 operator*(const HgMat3& lhs, const HgMat3& rhs)
{
    HgMat3 result{};
    hgMatMul(&result.x.x, 3, 3, &lhs.x.x, 3, 3, &rhs.x.x);
    return result;
}

HgMat4 operator*(const HgMat4& lhs, const HgMat4& rhs)
{
    HgMat4 result{};
    hgMatMul(&result.x.x, 4, 4, &lhs.x.x, 4, 4, &rhs.x.x);
    return result;
}

void hgMatMulVec(u32 width, u32 height, f32* dst, const f32* mat, const f32* vec)
{
    hgAssert(dst != nullptr);
    hgAssert(mat != nullptr);
    hgAssert(vec != nullptr);
    for (u32 i = 0; i < height; ++i)
    {
        dst[i] = 0.0f;
        for (u32 j = 0; j < width; ++j)
        {
            dst[i] += mat[j * width + i] * vec[j];
        }
    }
}

HgVec2 operator*(const HgMat2& lhs, HgVec2 rhs)
{
    HgVec2 result{};
    hgMatMulVec(2, 2, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

HgVec3 operator*(const HgMat3& lhs, HgVec3 rhs)
{
    HgVec3 result{};
    hgMatMulVec(3, 3, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

HgVec4 operator*(const HgMat4& lhs, HgVec4 rhs)
{
    HgVec4 result{};
    hgMatMulVec(4, 4, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

f32 hgComplexAbsSqr(HgComplex comp)
{
    return comp.r * comp.r + comp.i * comp.i;
}

f32 hgComplexAbs(HgComplex comp)
{
    return sqrtf(hgComplexAbsSqr(comp));
}

HgComplex hgComplexNorm(HgComplex comp)
{
    f32 len = hgComplexAbs(comp);
    hgAssert(len != 0);
    return HgComplex{comp.r / len, comp.i / len};
}

HgVec2 hgVecRot2(HgComplex lhs, HgVec2 rhs)
{
    HgComplex c = lhs * HgComplex{rhs.x, rhs.y};
    return {c.r, c.i};
}

HgQuat operator*(HgQuat lhs, HgQuat rhs)
{
    return HgQuat{
        lhs.r * rhs.r - lhs.i * rhs.i - lhs.j * rhs.j - lhs.k * rhs.k,
        lhs.r * rhs.i + lhs.i * rhs.r + lhs.j * rhs.k - lhs.k * rhs.j,
        lhs.r * rhs.j - lhs.i * rhs.k + lhs.j * rhs.r + lhs.k * rhs.i,
        lhs.r * rhs.k + lhs.i * rhs.j - lhs.j * rhs.i + lhs.k * rhs.r,
    };
}

f32 hgQuatAbsSqr(HgQuat quat)
{
    return quat.r * quat.r + quat.i * quat.i + quat.j * quat.j + quat.k * quat.k;
}

f32 hgQuatAbs(HgQuat quat)
{
    return sqrtf(hgQuatAbsSqr(quat));
}

HgQuat hgQuatNorm(HgQuat quat)
{
    f32 len = hgQuatAbs(quat);
    return HgQuat{quat.r / len, quat.i / len, quat.j / len, quat.k / len};
}

HgQuat hgQuatAxisAngle(HgVec3 axis, f32 angle)
{
    f32 halfAngle = angle * (f32)0.5;
    f32 sinHalfAngle = sinf(halfAngle);
    return HgQuat{
        cosf(halfAngle),
        axis.x * sinHalfAngle,
        axis.y * sinHalfAngle,
        axis.z * sinHalfAngle,
    };
}

HgQuat hgQuatBetween(HgVec3 from, HgVec3 to)
{
    hgAssert(from != HgVec3{0});
    hgAssert(to != HgVec3{0});

    from = hgVecNorm3(from);
    to = hgVecNorm3(to);

    f32 dot = hgVecDot3(from, to);

    if (dot > 1 - FLT_EPSILON)
    {
        return HgQuat{1};
    }

    if (dot < -1 + FLT_EPSILON)
    {
        HgVec3 axis = std::abs(from.x) >= 1 - FLT_EPSILON
            ? hgVecCross3(from, {0, 1, 0})
            : hgVecCross3(from, {1, 0, 0});
        return HgQuat(0, axis.x, axis.y, axis.z);
    }

    HgVec3 axis = hgVecCross3(from, to);
    return hgQuatNorm(HgQuat{dot + 1, axis.x, axis.y, axis.z});
}

HgVec3 hgVecRot3(HgQuat lhs, HgVec3 rhs)
{
    HgQuat q = lhs * HgQuat{0, rhs.x, rhs.y, rhs.z} * hgQuatConj(lhs);
    return {q.i, q.j, q.k};
}

HgMat3 hgMatRot3(HgQuat lhs, HgMat3 rhs)
{
    return HgMat3{
        hgVecRot3(lhs, rhs.x),
        hgVecRot3(lhs, rhs.y),
        hgVecRot3(lhs, rhs.z),
    };
}

HgMat4 hgMatModel2D(HgVec3 position, HgVec2 scale, f32 rotation)
{
    HgMat2 m2{{scale.x, 0.0f}, {0.0f, scale.y}};
    f32 rotSin = sinf(rotation);
    f32 rotCos = cosf(rotation);
    HgMat2 rot{{rotCos, rotSin}, {-rotSin, rotCos}};
    HgMat4 m4 = HgMat4{rot * m2};
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4 hgMatModel3D(const HgVec3& position, const HgVec3& scale, const HgQuat& rotation)
{
    HgMat3 m3{1.0f};
    m3.x.x = scale.x;
    m3.y.y = scale.y;
    m3.z.z = scale.z;
    m3 = hgMatRot3(rotation, m3);
    HgMat4 m4 = HgMat4{m3};
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4 hgMatView(const HgVec3& position, const HgVec3& zoom, const HgQuat& rotation)
{
    HgMat4 rot{hgMatRot3(hgQuatConj(rotation), HgMat3{1.0f})};
    HgMat4 pos{1.0f};
    pos.x.x = zoom.x;
    pos.y.y = zoom.y;
    pos.z.z = zoom.z;
    pos.w.x = -position.x;
    pos.w.y = -position.y;
    pos.w.z = -position.z;
    return rot * pos;
}

HgMat4 hgMatModelToView(const HgMat4& model)
{
    if (HgVec3{model.x} == HgVec3{0} || HgVec3{model.y} == HgVec3{0} || HgVec3{model.z} == HgVec3{0})
        return HgMat4{HgMat3{0}};

    HgMat3 inv3 = hgMatTranspose3(HgMat3{
        hgVecNorm3(HgVec3{model.x}),
        hgVecNorm3(HgVec3{model.y}),
        hgVecNorm3(HgVec3{model.z}),
    });
    HgMat4 inv4{inv3};
    inv4.w = HgVec4{inv3 * HgVec3{model.w} * -1, 1};
    return inv4;
}

HgMat4 hgMatOrthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far)
{
    return HgMat4{
        {2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (bottom - top), 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f / (far - near), 0.0f},
        {-(right + left) / (right - left), -(bottom + top) / (bottom - top), -(near) / (far - near), 1.0f},
    };
}

HgMat4 hgMatPerspective(f32 fov, f32 aspect, f32 near, f32 far)
{
    hgAssert(near > 0.0f);
    hgAssert(far > near);
    f32 scale = 1.0f / (f32)tan(fov * 0.5f);
    return HgMat4{
        {scale / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, scale, 0.0f, 0.0f},
        {0.0f, 0.0f, far / (far - near), 1.0f},
        {0.0f, 0.0f, -(far * near) / (far - near), 0.0f},
    };
}

bool hgContainsPointCircle(HgVec2 point, HgCircle circle)
{
    return hgDistPointCircle(point, circle) <= FLT_EPSILON;
}

f32 hgDistPointCircle(HgVec2 point, HgCircle circle)
{
    return hgVecLen2(point - circle.pos) - circle.radius;
}

HgVec2 hgClosestPointCircle(HgVec2 pos, HgCircle circle)
{
    return circle.pos + circle.radius * hgVecNorm2(pos - circle.pos);
}

bool hgIntersectCircles(HgCircle a, HgCircle b)
{
    return hgDistCircles(a, b) <= FLT_EPSILON;
}

f32 hgDistCircles(HgCircle a, HgCircle b)
{
    return hgVecLen2(a.pos - b.pos) - a.radius - b.radius;
}

HgRect hgRectEmpty()
{
    return {
        HgVec2{INFINITY},
        HgVec2{-INFINITY},
    };
}

HgRect hgRectAddPoint(HgRect rect, HgVec2 point)
{
    HgRect newRect;
    newRect.pos.x = hgMin(rect.pos.x, point.x - FLT_EPSILON);
    newRect.pos.y = hgMin(rect.pos.y, point.y - FLT_EPSILON);
    newRect.size.x = hgMax(rect.size.x + rect.pos.x, point.x + FLT_EPSILON) - newRect.pos.x;
    newRect.size.y = hgMax(rect.size.y + rect.pos.y, point.y + FLT_EPSILON) - newRect.pos.y;
    return newRect;
}

bool hgContainsPointRect(HgVec2 point, HgRect rect)
{
    return point.x >= rect.pos.x - FLT_EPSILON && point.x <= rect.pos.x + rect.size.x + FLT_EPSILON
        && point.y >= rect.pos.y - FLT_EPSILON && point.y <= rect.pos.y + rect.size.y + FLT_EPSILON;
}

HgVec2 hgClosestPointRect(HgVec2 pos, HgRect rect)
{
    return {
        hgClamp(pos.x, rect.pos.x, rect.pos.x + rect.size.x),
        hgClamp(pos.y, rect.pos.y, rect.pos.y + rect.size.y),
    };
}

bool hgIntersectRects(HgRect a, HgRect b)
{
    return a.pos.x + a.size.x >= b.pos.x && a.pos.x <= b.pos.x + b.size.x
        && a.pos.y + a.size.y >= b.pos.y && a.pos.y <= b.pos.y + b.size.y;
}

bool hgIntersectRectCircle(HgRect rect, HgCircle circle)
{
    return hgContainsPointCircle(hgClosestPointRect(circle.pos, rect), circle);
}

bool hgIntersectRays2D(HgRay2D ray, HgRay2D other, HgHit2D* hit)
{
    hgAssert(ray.dir != HgVec2{0});
    hgAssert(other.dir != HgVec2{0});

    f32 denom = hgVecCross2(ray.dir, other.dir);
    if (std::abs(denom) < FLT_EPSILON)
        return false;

    HgVec2 diff = other.pos - ray.pos;

    f32 t = hgVecCross2(diff, other.dir) / denom;
    if (t < -FLT_EPSILON)
        return false;

    f32 tOther = hgVecCross2(diff, ray.dir) / denom;
    if (tOther < -FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = denom < 0
            ? hgVecNorm2({other.dir.y, -other.dir.x})
            : hgVecNorm2({-other.dir.y, other.dir.x});
    }
    return true;
}

bool hgIntersectRayLine2D(HgRay2D ray, HgLine2D line, HgHit2D* hit)
{
    hgAssert(ray.dir != HgVec2{0});
    if (hgVecEq2(line.begin, line.end))
        return false;

    HgVec2 lineDir = line.end - line.begin;

    f32 denom = hgVecCross2(ray.dir, lineDir);
    if (std::abs(denom) < FLT_EPSILON)
        return false;

    HgVec2 diff = line.begin - ray.pos;

    f32 t = hgVecCross2(diff, lineDir) / denom;
    if (t < -FLT_EPSILON)
        return false;

    f32 tOther = hgVecCross2(diff, ray.dir) / denom;
    if (tOther < -FLT_EPSILON || tOther > 1 + FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = denom < 0
            ? hgVecNorm2({lineDir.y, -lineDir.x})
            : hgVecNorm2({-lineDir.y, lineDir.x});
    }
    return true;
}

bool hgIntersectRayCircle(HgRay2D ray, HgCircle circle, HgHit2D* hit)
{
    hgAssert(ray.dir != HgVec2{0});

    HgVec2 rel = ray.pos - circle.pos;
    f32 a = hgVecDot2(ray.dir, ray.dir);
    f32 b = 2 * hgVecDot2(ray.dir, rel);
    f32 c = hgVecDot2(rel, rel) - hgSquare(circle.radius);

    f32 det = hgSquare(b) - 4 * a * c;
    if (det < 0)
        return false;
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = (ray.pos + t * ray.dir - circle.pos) / circle.radius;
    }
    return true;
}

bool hgIntersectRayRect(HgRay2D ray, HgRect rect, HgHit2D* hit)
{
    hgAssert(ray.dir != HgVec2{0});
    if (hgVecEq2(rect.size, HgVec2{0}))
        return false;

    if (hgContainsPointRect(ray.pos, rect))
    {
        if (hit != nullptr)
        {
            hit->dist = 0;
            hit->normal = -ray.dir;
        }
        return true;
    }

    f32 hits[4] = {
        (rect.pos.x - ray.pos.x) / ray.dir.x,
        (rect.pos.y - ray.pos.y) / ray.dir.y,
        (rect.pos.x + rect.size.x - ray.pos.x) / ray.dir.x,
        (rect.pos.y + rect.size.y - ray.pos.y) / ray.dir.y,
    };

    constexpr HgVec2 norms[4] = {
        {-1, 0},
        {0, -1},
        {1, 0},
        {0, 1},
    };

    f32 t = INFINITY;
    HgVec2 norm;
    for (u32 i = 0; i < hgArrayCount(hits); ++i)
    {
        if (hits[i] < -FLT_EPSILON)
            continue;

        if (!hgContainsPointRect(ray.pos + hits[i] * ray.dir, rect))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = norm;
    }
    return true;
}

bool hgIntersectLines2D(HgLine2D line, HgLine2D other, HgHit2D* hit)
{
    if (hgVecEq2(line.begin, line.end) || hgVecEq2(other.begin, other.end))
        return false;

    HgVec2 lineDir = line.end - line.begin;
    HgVec2 otherDir = other.end - other.begin;

    f32 denom = hgVecCross2(lineDir, otherDir);
    if (std::abs(denom) < FLT_EPSILON)
        return false;

    HgVec2 diff = other.begin - line.begin;

    f32 t = hgVecCross2(diff, otherDir) / denom;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return false;

    f32 tOther = hgVecCross2(diff, lineDir) / denom;
    if (tOther < -FLT_EPSILON || tOther > 1 + FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = denom < 0
            ? hgVecNorm2({otherDir.y, -otherDir.x})
            : hgVecNorm2({-otherDir.y, otherDir.x});
    }
    return true;
}

bool hgIntersectLineRay2D(HgLine2D line, HgRay2D ray, HgHit2D* hit)
{
    if (hgVecEq2(line.begin, line.end))
        return false;
    hgAssert(ray.dir != HgVec2{0});

    HgVec2 lineDir = line.end - line.begin;

    f32 denom = hgVecCross2(lineDir, ray.dir);
    if (std::abs(denom) < FLT_EPSILON)
        return false;

    HgVec2 diff = ray.pos - line.begin;

    f32 t = hgVecCross2(diff, ray.dir) / denom;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return false;

    f32 tRay = hgVecCross2(diff, lineDir) / denom;
    if (tRay < -FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = denom < 0
            ? hgVecNorm2({ray.dir.y, -ray.dir.x})
            : hgVecNorm2({-ray.dir.y, ray.dir.x});
    }
    return true;
}

bool hgIntersectLineCircle(HgLine2D line, HgCircle circle, HgHit2D* hit)
{
    HgVec2 dir = line.end - line.begin;

    HgVec2 rel = line.begin - circle.pos;
    f32 a = hgVecDot2(dir, dir);
    f32 b = 2 * hgVecDot2(dir, rel);
    f32 c = hgVecDot2(rel, rel) - hgSquare(circle.radius);

    f32 det = hgSquare(b) - 4 * a * c;
    if (det < 0)
        return false;
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t > 1 + FLT_EPSILON)
        return false;
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = (line.begin + t * dir - circle.pos) / circle.radius;
    }
    return true;
}

bool hgIntersectLineRect(HgLine2D line, HgRect rect, HgHit2D* hit)
{
    if (hgVecEq2(line.begin, line.end) || hgVecEq2(rect.size, HgVec2{0}))
        return false;

    f32 hits[4] = {
        (rect.pos.x - line.begin.x) / (line.end.x - line.begin.x),
        (rect.pos.y - line.begin.y) / (line.end.y - line.begin.y),
        (rect.pos.x + rect.size.x - line.begin.x) / (line.end.x - line.begin.x),
        (rect.pos.y + rect.size.y - line.begin.y) / (line.end.y - line.begin.y),
    };

    constexpr HgVec2 norms[4] = {
        {-1, 0},
        {0, -1},
        {1, 0},
        {0, 1},
    };

    f32 t = INFINITY;
    HgVec2 norm;
    for (u32 i = 0; i < hgArrayCount(hits); ++i)
    {
        if (hits[i] < -FLT_EPSILON || hits[i] > 1 + FLT_EPSILON)
            continue;

        if (!hgContainsPointRect(line.begin + hits[i] * (line.end - line.begin), rect))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = norm;
    }
    return true;
}

bool hgContainsPointSphere(HgVec3 point, HgSphere sphere)
{
    return hgDistPointSphere(point, sphere) <= 0;
}

f32 hgDistPointSphere(HgVec3 point, HgSphere sphere)
{
    return hgVecLen3(point - sphere.pos) - sphere.radius;
}

HgVec3 hgClosestPointSphere(HgVec3 pos, HgSphere sphere)
{
    HgVec3 rel = pos - sphere.pos;
    if (hgVecLenSqr3(rel) <= sphere.radius + FLT_EPSILON)
        return pos;
    return sphere.pos + sphere.radius * hgVecNorm3(rel);
}

bool hgIntersectSpheres(HgSphere a, HgSphere b)
{
    return hgDistSpheres(a, b) <= 0;
}

f32 hgDistSpheres(HgSphere a, HgSphere b)
{
    return hgVecLen3(a.pos - b.pos) - a.radius - b.radius;
}

HgBox hgBoxEmpty()
{
    return {
        HgVec3{INFINITY},
        HgVec3{-INFINITY},
    };
}

HgBox hgBoxAddPoint(HgBox box, HgVec3 point)
{
    HgBox newBox;
    newBox.pos.x = hgMin(box.pos.x, point.x - FLT_EPSILON);
    newBox.pos.y = hgMin(box.pos.y, point.y - FLT_EPSILON);
    newBox.pos.z = hgMin(box.pos.z, point.z - FLT_EPSILON);
    newBox.size.x = hgMax(box.size.x + box.pos.x, point.x + FLT_EPSILON) - newBox.pos.x;
    newBox.size.y = hgMax(box.size.y + box.pos.y, point.y + FLT_EPSILON) - newBox.pos.y;
    newBox.size.z = hgMax(box.size.z + box.pos.z, point.z + FLT_EPSILON) - newBox.pos.z;
    return newBox;
}

bool hgContainsPointBox(HgVec3 point, HgBox box)
{
    return point.x >= box.pos.x && point.x <= box.pos.x + box.size.x
        && point.y >= box.pos.y && point.y <= box.pos.y + box.size.y
        && point.z >= box.pos.z && point.z <= box.pos.z + box.size.z;
}

HgVec3 hgClosestPointBox(HgVec3 pos, HgBox box)
{
    return {
        hgClamp(pos.x, box.pos.x, box.pos.x + box.size.x),
        hgClamp(pos.y, box.pos.y, box.pos.y + box.size.y),
        hgClamp(pos.z, box.pos.z, box.pos.z + box.size.z),
    };
}

bool hgIntersectBox(HgBox a, HgBox b)
{
    return a.pos.x + a.size.x >= b.pos.x && a.pos.x <= b.pos.x + b.size.x
        && a.pos.y + a.size.y >= b.pos.y && a.pos.y <= b.pos.y + b.size.y
        && a.pos.z + a.size.z >= b.pos.z && a.pos.z <= b.pos.z + b.size.z;
}

bool hgIntersectBoxSphere(HgBox box, HgSphere sphere)
{
    return hgContainsPointSphere(hgClosestPointBox(sphere.pos, box), sphere);
}

HgPlane hgPlaneFromPoint(HgVec3 point, HgVec3 normal)
{
    HgPlane plane;
    plane.normal = hgVecNorm3(normal);
    plane.dist = hgVecDot3(plane.normal, point);
    return plane;
}

HgPlane hgPlaneFromTri(HgTri tri)
{
    HgPlane plane;
    plane.normal = hgVecNorm3(hgVecCross3(tri.b - tri.a, tri.c - tri.a));
    plane.dist = hgVecDot3(plane.normal, tri.a);
    return plane;
}

bool hgIntersectRaySphere(HgRay3D ray, HgSphere sphere, HgHit3D* hit)
{
    hgAssert(ray.dir != HgVec3{0});

    HgVec3 rel = ray.pos - sphere.pos;
    f32 a = hgVecDot3(ray.dir, ray.dir);
    f32 b = 2 * hgVecDot3(ray.dir, rel);
    f32 c = hgVecDot3(rel, rel) - hgSquare(sphere.radius);

    f32 det = hgSquare(b) - 4 * a * c;
    if (det < 0)
        return false;
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = (ray.pos + t * ray.dir - sphere.pos) / sphere.radius;
    }
    return true;
}

bool hgIntersectRayBox(HgRay3D ray, HgBox box, HgHit3D* hit)
{
    hgAssert(ray.dir != HgVec3{0});
    if (hgVecEq3(box.size, HgVec3{0}))
        return false;

    if (hgContainsPointBox(ray.pos, box))
    {
        if (hit != nullptr)
        {
            hit->dist = 0;
            hit->normal = -ray.dir;
        }
        return true;
    }

    f32 hits[6] = {
        (box.pos.x - ray.pos.x) / ray.dir.x,
        (box.pos.y - ray.pos.y) / ray.dir.y,
        (box.pos.z - ray.pos.z) / ray.dir.z,
        (box.pos.x + box.size.x - ray.pos.x) / ray.dir.x,
        (box.pos.y + box.size.y - ray.pos.y) / ray.dir.y,
        (box.pos.z + box.size.z - ray.pos.y) / ray.dir.z,
    };

    constexpr HgVec3 norms[6] = {
        {-1, 0, 0},
        {0, -1, 0},
        {0, 0, -1},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
    };

    f32 t = INFINITY;
    HgVec3 norm;
    for (u32 i = 0; i < hgArrayCount(hits); ++i)
    {
        if (hits[i] < -FLT_EPSILON)
            continue;

        if (!hgContainsPointBox(ray.pos + hits[i] * ray.dir, box))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = norm;
    }
    return true;
}

// Moller-Trumbore, Real Time Rendering 4th Edition
bool hgIntersectRayTri(HgRay3D ray, HgTri tri, HgHit3D* hit)
{
    hgAssert(ray.dir != HgVec3{0});

    if (tri.a == tri.b || tri.a == tri.c || tri.b == tri.c)
        return false;

    HgVec3 e1 = tri.b - tri.a;
    HgVec3 e2 = tri.c - tri.a;
    HgVec3 q = hgVecCross3(ray.dir, e2);

    f32 a = hgVecDot3(e1, q);
    if (std::abs(a) < FLT_EPSILON)
        return false;

    HgVec3 s = ray.pos - tri.a;

    f32 u = hgVecDot3(s, q) / a;
    if (u < -FLT_EPSILON)
        return false;

    HgVec3 r = hgVecCross3(s, e1);

    f32 v = hgVecDot3(ray.dir, r) / a;
    if (v < -FLT_EPSILON || u + v > 1 + FLT_EPSILON)
        return false;

    f32 t = hgVecDot3(e2, r) / a;
    if (t < -FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = a < 0
            ? hgVecNorm3(hgVecCross3(e2, e1))
            : hgVecNorm3(hgVecCross3(e1, e2));
    }
    return true;
}

bool hgIntersectRayPlane(HgRay3D ray, HgPlane plane, HgHit3D* hit)
{
    hgAssert(ray.dir != HgVec3{0});
    hgAssert(plane.normal != HgVec3{0});

    f32 denom = hgVecDot3(ray.dir, plane.normal);
    if (std::abs(denom) < FLT_EPSILON)
        return false;

    f32 t = (plane.dist - hgVecDot3(ray.pos, plane.normal)) / denom;
    if (t < -FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = denom < 0
            ? plane.normal
            : -plane.normal;
    }
    return true;
}

bool hgIntersectLineSphere(HgLine3D line, HgSphere sphere, HgHit3D* hit)
{
    HgVec3 dir = line.end - line.begin;

    HgVec3 rel = line.begin - sphere.pos;
    f32 a = hgVecDot3(dir, dir);
    f32 b = 2 * hgVecDot3(dir, rel);
    f32 c = hgVecDot3(rel, rel) - hgSquare(sphere.radius);

    f32 det = hgSquare(b) - 4 * a * c;
    if (det < 0)
        return false;
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t > 1 + FLT_EPSILON)
        return false;
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = (line.begin + t * dir - sphere.pos) / sphere.radius;
    }
    return true;
}

bool hgIntersectLineBox(HgLine3D line, HgBox box, HgHit3D* hit)
{
    if (hgVecEq3(line.begin, line.end) || hgVecEq3(box.size, HgVec3{0}))
        return false;

    f32 hits[6] = {
        (box.pos.x - line.begin.x) / (line.end.x - line.begin.x),
        (box.pos.y - line.begin.y) / (line.end.y - line.begin.y),
        (box.pos.z - line.begin.z) / (line.end.z - line.begin.z),
        (box.pos.x + box.size.x - line.begin.x) / (line.end.x - line.begin.x),
        (box.pos.y + box.size.y - line.begin.y) / (line.end.y - line.begin.y),
        (box.pos.z + box.size.z - line.begin.z) / (line.end.z - line.begin.z),
    };

    constexpr HgVec3 norms[6] = {
        {-1, 0, 0},
        {0, -1, 0},
        {0, 0, -1},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
    };

    f32 t = INFINITY;
    HgVec3 norm;
    for (u32 i = 0; i < hgArrayCount(hits); ++i)
    {
        if (hits[i] < -FLT_EPSILON || hits[i] > 1 + FLT_EPSILON)
            continue;

        if (!hgContainsPointBox(line.begin + hits[i] * (line.end - line.begin), box))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = norm;
    }
    return true;
}

// Moller-Trumbore, Real Time Rendering 4th Edition
bool hgIntersectLineTri(HgLine3D line, HgTri tri, HgHit3D* hit)
{
    if (hgVecEq3(line.begin, line.end))
        return false;

    if (tri.a == tri.b || tri.a == tri.c || tri.b == tri.c)
        return false;

    HgVec3 lineDir = line.end - line.begin;

    HgVec3 e1 = tri.b - tri.a;
    HgVec3 e2 = tri.c - tri.a;
    HgVec3 q = hgVecCross3(lineDir, e2);

    f32 a = hgVecDot3(e1, q);
    if (std::abs(a) < FLT_EPSILON)
        return false;

    HgVec3 s = line.begin - tri.a;

    f32 u = hgVecDot3(s, q) / a;
    if (u < -FLT_EPSILON)
        return false;

    HgVec3 r = hgVecCross3(s, e1);

    f32 v = hgVecDot3(lineDir, r) / a;
    if (v < -FLT_EPSILON || u + v > 1 + FLT_EPSILON)
        return false;

    f32 t = hgVecDot3(e2, r) / a;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = a < 0
            ? hgVecNorm3(hgVecCross3(e2, e1))
            : hgVecNorm3(hgVecCross3(e1, e2));
    }
    return true;
}

bool hgIntersectLinePlane(HgLine3D line, HgPlane plane, HgHit3D* hit)
{
    if (line.begin == line.end)
        return false;

    hgAssert(plane.normal != HgVec3{0});

    HgVec3 lineDir = line.end - line.begin;

    f32 denom = hgVecDot3(lineDir, plane.normal);
    if (std::abs(denom) < FLT_EPSILON)
        return false;

    f32 t = (plane.dist - hgVecDot3(line.begin, plane.normal)) / denom;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = denom < 0
            ? plane.normal
            : -plane.normal;
    }
    return true;
}

u32 hgNoise(u32 seed, u32 pos)
{
    u32 ret = (pos + 384521713u) * 955740521u;
    ret ^= ret >> 13;
    ret *= seed * 725937977u;
    ret ^= ret >> 7;
    ret *= 358166231u;
    ret ^= ret >> 11;
    return ret;
}

u32 hgNoise2D(u32 seed, u32 x, u32 y)
{
    return hgNoise(seed, x + (y * 425537443u));
}

u32 hgNoise3D(u32 seed, u32 x, u32 y, u32 z)
{
    return hgNoise(seed, x + y * 425537443u + z * 682607u);
}

u32 hgNoise4D(u32 seed, u32 x, u32 y, u32 z, u32 w)
{
    return hgNoise(seed, x + y * 425537443u + z * 682607u + w * 9067u);
}

f32 hgNoiseNorm(u32 seed, f32 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)hgNoise(seed, Convert{pos}.asU32) / (f32)UINT32_MAX;
}

f32 hgNoiseNorm2D(u32 seed, HgVec2 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)hgNoise2D(seed, Convert{pos.x}.asU32, Convert{pos.y}.asU32) / (f32)UINT32_MAX;
}

f32 hgNoiseNorm3D(u32 seed, HgVec3 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)hgNoise3D(seed, Convert{pos.x}.asU32, Convert{pos.y}.asU32, Convert{pos.z}.asU32) / (f32)UINT32_MAX;
}

f32 hgNoiseNorm4D(u32 seed, HgVec4 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)hgNoise4D(
        seed,
        Convert{pos.x}.asU32,
        Convert{pos.y}.asU32,
        Convert{pos.z}.asU32,
        Convert{pos.w}.asU32) / (f32)UINT32_MAX;
}

f32 hgNoiseVec1D(u32 seed, f32 pos)
{
    return hgNoiseNorm(seed, pos) * 2.0f - 1.0f;
}

HgVec2 hgNoiseVec2D(u32 seed, HgVec2 pos)
{
    f32 rot = 2.0f * (f32)hgPi * hgNoiseNorm2D(seed, pos);
    return HgVec2(cosf(rot), sinf(rot));
}

u32 hgTrueRandom()
{
    static std::random_device trueRandom{};
    return trueRandom();
}

void hgRngSeed(HgRng* rng, u32 seed)
{
    rng->seed = seed;
}

u32 hgRngNext(HgRng* rng)
{
    return rng->pos = hgNoise(rng->seed, rng->pos);
}

u64 hgRngNext64(HgRng* rng)
{
    return ((u64)hgRngNext(rng) << 32) | (u64)hgRngNext(rng);
}

u32 hgGetMaxMipmaps(u32 width, u32 height, u32 depth)
{
    u32 max = width > height ? width : height;
    max = max > depth ? max : depth;
    return max == 0 ? 0 : (u32)log2((f32)max) + 1;
}

HgPerf hgPerfCreate(HgArena* arena, u32 count)
{
    hgAssert(arena != nullptr);

    HgPerf perf;
    perf.times = hgArenaAlloc<f64>(arena, count);
    perf.count = count;
    perf.current = 0;
    return perf;
}

void hgPerfBegin(HgPerf* perf)
{
    hgAssert(perf != nullptr);
    hgClockTick(&perf->clock);
}

f64 hgPerfEnd(HgPerf* perf)
{
    hgAssert(perf != nullptr);
    hgAssert(perf->current < perf->count);

    f64 time = hgClockTick(&perf->clock);
    perf->times[perf->current++] = time;

    return time;
}

HgPerfStats hgPerfAnalyze(const HgPerf* perf)
{
    hgAssert(perf != nullptr);

    HgPerfStats stats;
    stats.avg = 0.0;
    stats.best = INFINITY;
    stats.worst = 0.0;

    for (u32 i = 0; i < perf->current; ++i)
    {
        if (perf->times[i] < stats.best)
            stats.best = perf->times[i];
        if (perf->times[i] > stats.worst)
            stats.worst = perf->times[i];
        stats.avg += perf->times[i];
    }
    stats.avg /= (f64)perf->current;

    return stats;
}

void hgPerfLog(HgString title, const HgPerfStats* stats, HgPerfScale scale)
{
    hgAssert(stats != nullptr);
    if (title.length == 0 || title.chars == nullptr)
        title = "Title Missing";

    switch (scale)
    {
        case HgPerfScale_seconds:
            printf("HG Performance - %.*s: avg: %.4fs, best: %.4fs, worst: %.4fs\n",
                (int)title.length, title.chars, stats->avg, stats->best, stats->worst);
            break;
        case HgPerfScale_milli:
            printf("HG Performance - %.*s: avg: %.4fms, best: %.4fms, worst: %.4fms\n",
                (int)title.length, title.chars, stats->avg * 1.e3, stats->best * 1.e3, stats->worst * 1.e3);
            break;
        case HgPerfScale_micro:
            printf("HG Performance - %.*s: avg: %.4fmcs, best: %.4fmcs, worst: %.4fmcs\n",
                (int)title.length, title.chars, stats->avg * 1.e6, stats->best * 1.e6, stats->worst * 1.e6);
            break;
        case HgPerfScale_nano:
            printf("HG Performance - %.*s: avg: %.4fns, best: %.4fns, worst: %.4fns\n",
                (int)title.length, title.chars, stats->avg * 1.e9, stats->best * 1.e9, stats->worst * 1.e9);
            break;
    }
}

template<>
void hgAssetLoadImpl(HgAsset<HgTextureData>* data)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);
    char* cpath = hgCString(scratch, data->path);

    int x, y, channels;
    data->asset.pixels = stbi_load(cpath, &x, &y, &channels, 4);
    if (data->asset.pixels == nullptr)
    {
        hgErrorFormat("Could not load image: %s", cpath);
        return;
    }
    data->asset.width = (u32)x;
    data->asset.height = (u32)y;
    data->asset.depth = 1;
    data->asset.format = HgFormat_r8g8b8a8_srgb;
}

template<>
void hgAssetUnloadImpl(HgAsset<HgTextureData>* data)
{
    free(data->asset.pixels);
}

bool hgTextureStorePng(HgTextureData* texture, HgString path)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    const char* cpath = hgCString(scratch, path);

    if (!stbi_write_png(
         cpath,
         (int)texture->width,
         (int)texture->height,
         4,
         texture->pixels,
         (int)(texture->width * sizeof(u32))))
    {
        hgErrorFormat("Could not store image: %s", cpath);
        return false;
    }
    return true;
}

template<>
void hgAssetLoadImpl(HgAsset<HgTexture>* data)
{
    HgTextureDataAsset* tex = hgAssetLoad<HgTextureData>(data->path);
    hgDefer(hgAssetUnload(tex));
    if (tex->asset.pixels == nullptr)
        return;

    HgGpuImageCreateEx imageInfo{};
    imageInfo.format = tex->asset.format;
    imageInfo.width = tex->asset.width;
    imageInfo.height = tex->asset.height;
    imageInfo.depth = tex->asset.depth;
    imageInfo.usage = HgGpuImageUsage_transferDst | HgGpuImageUsage_sampled;

    data->asset.image = hgGpuImageCreateEx(&imageInfo);
    data->asset.view = hgGpuViewCreate(data->asset.image, HgGpuAspect_color, HgGpuFilter_nearest);

    hgGpuImageWrite(data->asset.view, tex->asset.pixels);
}

template<>
void hgAssetUnloadImpl(HgAsset<HgTexture>* data)
{
    hgGpuViewDestroy(data->asset.view);
    hgGpuImageDestroy(data->asset.image);
}

template<>
void hgAssetLoadImpl(HgAsset<HgMeshData>* data)
{
    (void)data;
    hgPanic("load gltf file : TODO\n");
}

template<>
void hgAssetUnloadImpl(HgAsset<HgMeshData>* data)
{
    hgGpaFree(data->asset.indices, data->asset.indexCount);
    hgGpaFree(data->asset.vertices, data->asset.vertexCount);
}

void hgMeshStoreGltf(HgMeshData* data, HgString path, HgFence* fence)
{
    (void)data;
    (void)path;
    (void)fence;
    hgPanic("store gltf file : TODO\n");
}

template<>
void hgAssetLoadImpl(HgAsset<HgMesh>* data)
{
    HgMeshDataAsset* mesh = hgAssetLoad<HgMeshData>(data->path);
    hgDefer(hgAssetUnload(mesh));

    if (mesh->asset.vertices == nullptr || mesh->asset.indices == nullptr)
        return;

    data->asset.vertexCount = mesh->asset.vertexCount;
    data->asset.vertexWidth = mesh->asset.vertexWidth;
    data->asset.indexCount = mesh->asset.indexCount;

    data->asset.vertexBuffer = hgGpuBufferCreate(
        data->asset.vertexCount * data->asset.vertexWidth,
        HgGpuBufferUsage_storageBuffer | HgGpuBufferUsage_transferDst);

    data->asset.indexBuffer = hgGpuBufferCreate(
        data->asset.indexCount * sizeof(u32),
        HgGpuBufferUsage_storageBuffer | HgGpuBufferUsage_transferDst);

    hgGpuBufferWrite(
        data->asset.vertexBuffer,
        0,
        mesh->asset.vertices,
        data->asset.vertexCount * data->asset.vertexWidth);

    hgGpuBufferWrite(
        data->asset.indexBuffer,
        0,
        mesh->asset.indices,
        data->asset.indexCount * sizeof(u32));
}

template<>
void hgAssetUnloadImpl(HgAsset<HgMesh>* data)
{
    hgGpuBufferDestroy(data->asset.vertexBuffer);
    hgGpuBufferDestroy(data->asset.indexBuffer);
}

template<>
void hgSerialize(HgSerializer* s, HgCamera* camera)
{
    hgSerializeBegin(s);
    hgSerialize(s, &camera->rotation);
    hgSerialize(s, &camera->position);
    hgSerialize(s, &camera->type);
    if (camera->type == HgCameraType_perspective)
    {
        hgSerializeObject(s,
            &camera->perspective.aspect,
            &camera->perspective.fov,
            &camera->perspective.near,
            &camera->perspective.far);
    }
    else
    {
        hgSerializeObject(s,
            &camera->orthographic.left,
            &camera->orthographic.right,
            &camera->orthographic.top,
            &camera->orthographic.bottom,
            &camera->orthographic.near,
            &camera->orthographic.far);
    }
    hgSerializeEnd(s);
}

struct VPUniform {
    HgMat4 proj;
    HgMat4 view;
};

HgCamera hgCameraCreate()
{
    HgCamera camera{};

    camera.vpBuffer = hgGpuBufferCreate(
        sizeof(VPUniform),
        HgGpuBufferUsage_uniformBuffer,
        HgGpuMemoryUsage_frequentUpdate);

    camera.rotation = HgQuat{1.0f};
    camera.position = HgVec3{0.0f};

    return camera;
}

void hgCameraDestroy(HgCamera* camera)
{
    hgGpuBufferDestroy(camera->vpBuffer);
}

void hgCameraSetPerspective(HgCamera* camera, f32 aspect, f32 fov, f32 near, f32 far)
{
    camera->type = HgCameraType_perspective;
    camera->perspective.aspect = aspect;
    camera->perspective.fov = fov;
    camera->perspective.near = near;
    camera->perspective.far = far;
}

void hgCameraSetOrthographic(HgCamera* camera, f32 width, f32 height, f32 actualAspect)
{
    camera->type = HgCameraType_orthographic;
    camera->orthographic.left = 0;
    camera->orthographic.right = width;
    camera->orthographic.top = 0;
    camera->orthographic.bottom = height;
    camera->orthographic.near = 0;
    camera->orthographic.far = 1;

    if (actualAspect != 0.0)
    {
        if (actualAspect > (f32)width / (f32)height)
        {
            f32 margin = actualAspect - (f32)width / (f32)height;
            camera->orthographic.left -= margin * width / 2.0f;
            camera->orthographic.right += margin * width / 2.0f;
        }
        else
        {
            f32 margin = 1.0f / actualAspect - (f32)height / (f32)width;
            camera->orthographic.top -= margin * height / 2.0f;
            camera->orthographic.bottom += margin * height / 2.0f;
        }
    }
}

void hgCameraUpdate(HgCamera* camera)
{
    VPUniform vp{};
    vp.view = hgMatView(camera->position, HgVec3{1.0f}, camera->rotation);
    if (camera->type == HgCameraType_perspective)
    {
        vp.proj = hgMatPerspective(
            camera->perspective.fov,
            camera->perspective.aspect,
            camera->perspective.near,
            camera->perspective.far);
    }
    else
    {
        vp.proj = hgMatOrthographic(
            camera->orthographic.left,
            camera->orthographic.right,
            camera->orthographic.top,
            camera->orthographic.bottom,
            camera->orthographic.near,
            camera->orthographic.far);
    }

    hgGpuBufferWrite(camera->vpBuffer, 0, &vp, sizeof(vp));
}

HgCamera* hgCameraAdd(HgEcs* ecs, HgEntity e)
{
    HgCamera* camera = hgEcsAdd<HgCamera>(ecs, e);
    *camera = hgCameraCreate();
    return camera;
}

template<>
void hgEcsDtor(HgCamera* camera)
{
    hgCameraDestroy(camera);
}

void hgCameraUpdateEcs(HgEcs* ecs, HgEntity e)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsHas<HgCamera>(ecs, e));
    hgAssert(hgEcsHas<HgTransform>(ecs, e));

    HgCamera* camera = hgEcsGet<HgCamera>(ecs, e);
    HgTransform* tf = hgEcsGet<HgTransform>(ecs, e);
    hgAssert(camera->type == HgCameraType_perspective || camera->type == HgCameraType_orthographic);

    VPUniform vp{};
    vp.view = hgMatModelToView(tf->mat);
    if (camera->type == HgCameraType_perspective)
    {
        vp.proj = hgMatPerspective(
            camera->perspective.fov,
            camera->perspective.aspect,
            camera->perspective.near,
            camera->perspective.far);
    }
    else
    {
        vp.proj = hgMatOrthographic(
            camera->orthographic.left,
            camera->orthographic.right,
            camera->orthographic.top,
            camera->orthographic.bottom,
            camera->orthographic.near,
            camera->orthographic.far);
    }

    hgGpuBufferWrite(camera->vpBuffer, 0, &vp, sizeof(vp));
}

struct RenderState2D {
    HgGpuPipeline* pipeline;
    HgGpuPipeline* debugPipeline;
    HgTexture defaultTex;
};

static RenderState2D render2D;

struct RenderPush2D {
    HgMat4 model;
    u32 vpIdx;
    u32 instIdx;
};

#include "render2d.vert.spv.h"
#include "render2d.frag.spv.h"
#include "debug2d.frag.spv.h"

void hgRendererInit2D(HgFormat colorFormat)
{
    HgCreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = render2d_vert_spv;
    pipelineConfig.vertexShaderSize = sizeof(render2d_vert_spv);
    pipelineConfig.fragmentShader = render2d_frag_spv;
    pipelineConfig.fragmentShaderSize = sizeof(render2d_frag_spv);
    pipelineConfig.pushConstantSize = sizeof(RenderPush2D);
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    render2D.pipeline = hgGpuPipelineCreateGraphics(&pipelineConfig);

    pipelineConfig.fragmentShader = debug2d_frag_spv;
    pipelineConfig.fragmentShaderSize = sizeof(debug2d_frag_spv);
    pipelineConfig.topology = HgGpuTopology_lineStrip;

    render2D.debugPipeline = hgGpuPipelineCreateGraphics(&pipelineConfig);

    struct Color {
        u8 r, g, b, a;
    };
    Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    render2D.defaultTex.image = hgGpuImageCreate(2, 2, HgFormat_r8g8b8a8_srgb,
        HgGpuImageUsage_sampled | HgGpuImageUsage_transferDst);

    render2D.defaultTex.view = hgGpuViewCreate(
        render2D.defaultTex.image, HgGpuAspect_color, HgGpuFilter_nearest);

    hgGpuImageWrite(render2D.defaultTex.view, defaultColors);
}

void hgRendererDeinit2D()
{
    hgGpuViewDestroy(render2D.defaultTex.view);
    hgGpuImageDestroy(render2D.defaultTex.image);
    hgGpuPipelineDestroy(render2D.debugPipeline);
    hgGpuPipelineDestroy(render2D.pipeline);
}

HgLayer2D hgLayerCreate2D()
{
    HgLayer2D layer{};

    layer.instances = hgArrayCreate<HgRender2DInstance>();
    layer.instanceBuffer = hgGpuBufferCreate(layer.instances.capacity * sizeof(HgRender2DInstance),
        HgGpuBufferUsage_transferDst | HgGpuBufferUsage_storageBuffer, HgGpuMemoryUsage_frequentUpdate);
    layer.instanceCapacity = layer.instances.capacity;
    layer.transform = HgMat4{1.0f};
    layer.changed = true;

    return layer;
}

void hgLayerDestroy2D(HgLayer2D* layer)
{
    hgAssert(layer != nullptr);

    hgGpuBufferDestroy(layer->instanceBuffer);
    hgArrayDestroy(&layer->instances);
}

void hgLayerClear2D(HgLayer2D* layer)
{
    hgAssert(layer != nullptr);

    layer->instances.count = 0;
    layer->changed = true;
}

static void renderLayer2D(HgGpuCmd* cmd, HgCamera* camera, HgLayer2D* layer, HgGpuPipeline* pipeline)
{
    hgAssert(cmd != nullptr);
    hgAssert(camera != nullptr);
    hgAssert(layer != nullptr);

    if (layer->changed)
    {
        if (layer->instances.capacity > layer->instanceCapacity)
        {
            hgGpuWaitIdle();
            hgGpuBufferDestroy(layer->instanceBuffer);

            layer->instanceBuffer = hgGpuBufferCreate(layer->instances.capacity * sizeof(HgRender2DInstance),
                HgGpuBufferUsage_transferDst | HgGpuBufferUsage_storageBuffer, HgGpuMemoryUsage_frequentUpdate);
            layer->instanceCapacity = layer->instances.capacity;
        }

        hgGpuBufferWrite(layer->instanceBuffer, 0, layer->instances.vals, layer->instances.count * sizeof(HgRender2DInstance));

        layer->changed = false;
    }

    hgGpuBindPipeline(cmd, pipeline);

    RenderPush2D push{};
    push.model = layer->transform;
    push.vpIdx = hgGpuBufferUniformDescriptor(camera->vpBuffer);
    push.instIdx = hgGpuBufferStorageDescriptor(layer->instanceBuffer);

    hgGpuPushConstants(cmd, pipeline, &push, sizeof(push));

    hgGpuDraw(cmd, 0, 6, 0, layer->instances.count);
}

void hgRenderLayer2D(HgGpuCmd* cmd, HgCamera* camera, HgLayer2D* layer)
{
    renderLayer2D(cmd, camera, layer, render2D.pipeline);
}

void hgRenderDebug2D(HgGpuCmd* cmd, HgCamera* camera, HgLayer2D* layer)
{
    renderLayer2D(cmd, camera, layer, render2D.debugPipeline);
}

void hgDrawRect2D(HgLayer2D* layer, HgVec4 color, HgRect dst)
{
    hgAssert(layer != nullptr);

    HgRender2DInstance instance{};
    instance.rect.pos = dst.pos;
    instance.rect.size = dst.size;
    instance.rect.type = HgRender2DInstanceType_color;
    instance.rect.color = color;

    *hgArrayPush(&layer->instances) = instance;

    layer->changed = true;
}

void hgDrawSprite2D(HgLayer2D* layer, HgSprite2D* sprite, HgRect dst)
{
    hgAssert(layer != nullptr);
    hgAssert(sprite != nullptr);

    HgTexture* texture = sprite->texture == nullptr
        ? &render2D.defaultTex
        : &sprite->texture->asset;

    HgRender2DInstance instance{};
    instance.sprite.pos = dst.pos;
    instance.sprite.size = dst.size;
    instance.sprite.type = HgRender2DInstanceType_sprite;
    instance.sprite.tex = hgGpuImageSamplerDescriptor(texture->view);
    instance.sprite.uvPos = sprite->uv.pos;
    instance.sprite.uvSize = sprite->uv.size;

    *hgArrayPush(&layer->instances) = instance;

    layer->changed = true;
}

HgAtlas2D hgAtlasCreate2D(HgTextureAsset* texture)
{
    hgAssert(texture != nullptr);

    HgAtlas2D atlas{};
    atlas.texture = texture;
    atlas.sprites = hgArrayCreate<HgRect>();
    return atlas;
}

void hgAtlasDestroy2D(HgAtlas2D* atlas)
{
    hgAssert(atlas != nullptr);
    hgArrayDestroy(&atlas->sprites);
}

u32 hgAtlasAdd2D(HgAtlas2D* atlas, HgRect sprite)
{
    hgAssert(atlas != nullptr);

    u32 idx = atlas->sprites.count;
    *hgArrayPush(&atlas->sprites) = sprite;
    return idx;
}

u32 hgAtlasAddGrid2D(HgAtlas2D* atlas, HgRect grid, u32 width, u32 height)
{
    hgAssert(atlas != nullptr);

    u32 idx = atlas->sprites.count;

    HgVec2 spriteSize = grid.size / HgVec2{(f32)width, (f32)height};
    HgVec2 pos = grid.pos;
    for (u32 y = 0; y < height; ++y)
    {
        pos.x = grid.pos.x;
        for (u32 x = 0; x < width; ++x)
        {
            *hgArrayPush(&atlas->sprites) = {pos, spriteSize};
            pos.x += spriteSize.x;
        }
        pos.y += spriteSize.y;
    }

    return idx;
}

HgSprite2D hgAtlasGet2D(HgAtlas2D* atlas, u32 idx)
{
    hgAssert(atlas != nullptr);

    return {atlas->texture, atlas->sprites[idx]};
}

HgTilemap2D hgTilemapCreate2D(u32 width, u32 height)
{
    HgTilemap2D tilemap{};
    tilemap.tiles = hgGpaAlloc<u32>(width * height);
    tilemap.width = width;
    tilemap.height = height;
    for (u32 i = 0; i < width * height; ++i)
    {
        tilemap.tiles[i] = (u32)-1;
    }

    return tilemap;
}

void hgTilemapDestroy2D(HgTilemap2D* tilemap)
{
    hgAssert(tilemap != nullptr);
    hgGpaFree(tilemap->tiles, tilemap->width * tilemap->height);
}

u32 hgTilemapGet2D(HgTilemap2D* tilemap, u32 x, u32 y)
{
    hgAssert(tilemap != nullptr);
    return tilemap->tiles[y * tilemap->width + x];
}

void hgTilemapSet2D(HgTilemap2D* tilemap, u32 x, u32 y, u32 tile)
{
    hgAssert(tilemap != nullptr);
    tilemap->tiles[y * tilemap->width + x] = tile;
}

void hgDrawTilemap2D(HgLayer2D* layer, HgAtlas2D* atlas, HgTilemap2D* tilemap, HgRect dst)
{
    hgAssert(layer != nullptr);
    hgAssert(tilemap != nullptr);

    HgVec2 pos = dst.pos;
    HgVec2 size = dst.size / HgVec2{(f32)tilemap->width, (f32)tilemap->height};
    for (u32 y = 0; y < tilemap->width; ++y)
    {
        pos.x = dst.pos.x;
        for (u32 x = 0; x < tilemap->height; ++x)
        {
            u32 tile = hgTilemapGet2D(tilemap, x, y);
            HgSprite2D sprite = hgAtlasGet2D(atlas, tile);
            hgDrawSprite2D(layer, &sprite, {pos, size});
            pos.x += size.x;
        }
        pos.y += size.y;
    }
}

struct SpritePipelinePush {
    HgMat4 model;
    HgVec2 uvPos;
    HgVec2 uvSize;
    u32 viewProj;
    u32 texture;
};

struct SpritePipelineState {
    HgGpuPipeline* pipeline;
    HgTexture defaultTex;
};

static SpritePipelineState spritePipeline{};

#include "sprite.vert.spv.h"
#include "sprite.frag.spv.h"

void hgSpritesInit(
    HgFormat colorFormat,
    HgFormat depthFormat)
{
    hgAssert(colorFormat != HgFormat_undefined);
    hgAssert(depthFormat != HgFormat_undefined);

    HgCreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = sprite_vert_spv;
    pipelineConfig.vertexShaderSize = sizeof(sprite_vert_spv);
    pipelineConfig.fragmentShader = sprite_frag_spv;
    pipelineConfig.fragmentShaderSize = sizeof(sprite_frag_spv);
    pipelineConfig.pushConstantSize = sizeof(SpritePipelinePush);
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    pipelineConfig.enableDepthRead = true;
    pipelineConfig.enableDepthWrite = true;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    spritePipeline.pipeline = hgGpuPipelineCreateGraphics(&pipelineConfig);

    struct Color {
        u8 r, g, b, a;
    };
    Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    spritePipeline.defaultTex.image = hgGpuImageCreate(2, 2, HgFormat_r8g8b8a8_srgb,
        HgGpuImageUsage_sampled | HgGpuImageUsage_transferDst);

    spritePipeline.defaultTex.view = hgGpuViewCreate(
        spritePipeline.defaultTex.image, HgGpuAspect_color, HgGpuFilter_nearest);

    hgGpuImageWrite(spritePipeline.defaultTex.view, defaultColors);
}

void hgSpritesDeinit()
{
    hgGpuPipelineDestroy(spritePipeline.pipeline);

    hgGpuViewDestroy(spritePipeline.defaultTex.view);
    hgGpuImageDestroy(spritePipeline.defaultTex.image);
}

template<>
void hgSerialize(HgSerializer* s, HgSprite* sprite)
{
    hgSerializeObject(s,
        &sprite->texture,
        &sprite->uvPos,
        &sprite->uvSize);
}

HgSprite* hgSpriteAdd(HgEcs* ecs, HgEntity e, HgTextureAsset* texture, HgVec2 uvPos, HgVec2 uvSize)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    HgSprite* sprite = hgEcsAdd<HgSprite>(ecs, e);
    *sprite = {};
    sprite->texture = texture;
    sprite->uvPos = uvPos;
    sprite->uvSize = uvSize;

    return sprite;
}

template<>
void hgEcsDtor(HgSprite* sprite)
{
    hgAssetUnload(sprite->texture);
}

void hgSpritesDraw(HgEcs* ecs, HgEntity camera, HgGpuCmd* cmd)
{
    hgAssert(ecs != nullptr);
    hgAssert(cmd != nullptr);

    hgGpuBindPipeline(cmd, spritePipeline.pipeline);

    hgEcsForEach<HgSprite, HgTransform>(ecs, [&](HgEntity, HgSprite* sprite, HgTransform* tf)
    {
        HgTexture* texture = sprite->texture == nullptr
            ? &spritePipeline.defaultTex
            : &sprite->texture->asset;

        SpritePipelinePush push{};
        push.model = tf->mat;
        push.uvPos = sprite->uvPos;
        push.uvSize = sprite->uvSize;
        push.viewProj = hgGpuBufferUniformDescriptor(hgEcsGet<HgCamera>(ecs, camera)->vpBuffer);
        push.texture = hgGpuImageSamplerDescriptor(texture->view);

        hgGpuPushConstants(cmd, spritePipeline.pipeline, &push, sizeof(push));

        hgGpuDraw(cmd, 0, 6, 0, 1);
    });
}

struct SkyboxPipelinePush {
    u32 viewProj;
    u32 texture;
};

struct SkyboxPipelineState {
    HgGpuPipeline* pipeline;
    HgTexture defaultTex;
};

static SkyboxPipelineState skyboxPipeline{};

#include "skybox.vert.spv.h"
#include "skybox.frag.spv.h"

void hgSkyboxInit(HgFormat colorFormat, HgFormat depthFormat)
{
    hgAssert(colorFormat != HgFormat_undefined);

    HgCreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = skybox_vert_spv;
    pipelineConfig.vertexShaderSize = sizeof(skybox_vert_spv);
    pipelineConfig.fragmentShader = skybox_frag_spv;
    pipelineConfig.fragmentShaderSize = sizeof(skybox_frag_spv);
    pipelineConfig.pushConstantSize = sizeof(SkyboxPipelinePush);
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    skyboxPipeline.pipeline = hgGpuPipelineCreateGraphics(&pipelineConfig);

    struct Color {
        u8 r, g, b, a;
    };
    Color top = {0x00, 0x22, 0x44, 0xff};
    Color mid = {0x00, 0x11, 0x33, 0xff};
    Color bot = {0x00, 0x00, 0x00, 0xff};
    Color nul = {};
    Color defaultColors[]{
        nul, top, nul, nul,
        mid, mid, mid, mid,
        nul, bot, nul, nul,
    };

    HgGpuImageCreateEx imageConfig{};
    imageConfig.width = 1;
    imageConfig.height = 1;
    imageConfig.format = HgFormat_r8g8b8a8_srgb;
    imageConfig.arrayLayers = 6;
    imageConfig.usage = HgGpuImageUsage_sampled | HgGpuImageUsage_transferDst;
    imageConfig.flags = HgGpuImageConfig_cubeCompatible;

    skyboxPipeline.defaultTex.image = hgGpuImageCreateEx(&imageConfig);

    HgGpuViewCreateEx viewConfig{};
    viewConfig.image = skyboxPipeline.defaultTex.image;
    viewConfig.baseMipLevel = 0;
    viewConfig.levelCount = 1;
    viewConfig.baseArrayLayer = 0;
    viewConfig.layerCount = 6;
    viewConfig.aspectFlags = HgGpuAspect_color;
    viewConfig.type = HgGpuViewType_cube;
    viewConfig.filter = HgGpuFilter_nearest;
    viewConfig.edgeMode = HgGpuSamplerEdgeMode_repeat;
    viewConfig.border = HgGpuSamplerBorder_floatTransparentBlack;

    skyboxPipeline.defaultTex.view = hgGpuViewCreateEx(&viewConfig);

    hgGpuImageWriteCubemap(skyboxPipeline.defaultTex.view, defaultColors);
}

void hgSkyboxDeinit()
{
    hgGpuPipelineDestroy(skyboxPipeline.pipeline);

    hgGpuViewDestroy(skyboxPipeline.defaultTex.view);
    hgGpuImageDestroy(skyboxPipeline.defaultTex.image);
}

template<>
void hgSerialize(HgSerializer* s, HgSkybox* skybox)
{
    hgSerializeObject(s,
        &skybox->texture);
}

HgSkybox* hgSkyboxAdd(HgEcs* ecs, HgEntity e, HgTextureAsset* texture)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    HgSkybox* skybox = hgEcsAdd<HgSkybox>(ecs, e);
    *skybox = {texture};

    return skybox;
}

template<>
void hgEcsDtor(HgSkybox* skybox)
{
    hgAssetUnload(skybox->texture);
}

void hgSkyboxDraw(HgEcs* ecs, HgEntity camera, HgGpuCmd* cmd)
{
    hgGpuBindPipeline(cmd, skyboxPipeline.pipeline);

    hgEcsForEach<HgSkybox>(ecs, [&](HgEntity, HgSkybox* skybox)
    {
        HgTexture* texture = skybox->texture == nullptr
            ? &skyboxPipeline.defaultTex
            : &skybox->texture->asset;

        SkyboxPipelinePush push{};
        push.viewProj = hgGpuBufferUniformDescriptor(hgEcsGet<HgCamera>(ecs, camera)->vpBuffer);
        push.texture = hgGpuImageSamplerDescriptor(texture->view);

        hgGpuPushConstants(cmd, skyboxPipeline.pipeline, &push, sizeof(push));

        hgGpuDraw(cmd, 0, 36, 0, 1);
    });
}

template<>
void hgSerialize(HgSerializer* s, HgDirLight* light)
{
    hgSerializeObject(s,
        &light->dir,
        &light->color);
}

HgDirLight* hgDirLightAdd(HgEcs* ecs, HgEntity e, HgVec3 dir, HgVec4 color)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    HgDirLight* light = hgEcsAdd<HgDirLight>(ecs, e);
    *light = {dir, color};

    return light;
}

template<>
void hgSerialize(HgSerializer* s, HgPointLight* light)
{
    hgSerializeObject(s,
        &light->color);
}

HgPointLight* hgPointLightAdd(HgEcs* ecs, HgEntity e, HgVec4 color)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    HgPointLight* light = hgEcsAdd<HgPointLight>(ecs, e);
    *light = {color};

    return light;
}

struct ModelPipelineDirLightData {
    HgVec4 dir;
    HgVec4 color;
};

struct ModelPipelinePointLightData {
    HgVec4 pos;
    HgVec4 color;
};

struct ModelPipelinePush {
    HgMat4 model;
    u32 indices;
    u32 vertices;
    u32 viewProj;
    u32 normalMap;
    u32 colorMap;
    u32 dirLights;
    u32 dirLightCount;
    u32 pointLights;
    u32 pointLightCount;
};

struct ModelPipelineState {
    HgGpuPipeline* pipeline;

    HgGpuBuffer* dirLightBuffer;
    u32 dirLightCapacity;

    HgGpuBuffer* pointLightBuffer;
    u32 pointLightCapacity;

    HgMesh defaultModel;
    HgTexture defaultColorMap;
    HgTexture defaultNormalMap;
};

static ModelPipelineState modelPipeline{};

#include "model.vert.spv.h"
#include "model.frag.spv.h"

void hgModelsInit(
    HgFormat colorFormat,
    HgFormat depthFormat)
{
    hgAssert(colorFormat != HgFormat_undefined);
    hgAssert(depthFormat != HgFormat_undefined);

    HgCreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = model_vert_spv;
    pipelineConfig.vertexShaderSize = sizeof(model_vert_spv);
    pipelineConfig.fragmentShader = model_frag_spv;
    pipelineConfig.fragmentShaderSize = sizeof(model_frag_spv);
    pipelineConfig.pushConstantSize = sizeof(ModelPipelinePush);
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    pipelineConfig.cullMode = HgGpuCull_back;
    pipelineConfig.enableDepthRead = true;
    pipelineConfig.enableDepthWrite = true;

    modelPipeline.pipeline = hgGpuPipelineCreateGraphics(&pipelineConfig);

    modelPipeline.dirLightCapacity = 16;
    modelPipeline.dirLightBuffer = hgGpuBufferCreate(
        sizeof(ModelPipelineDirLightData) * modelPipeline.dirLightCapacity,
        HgGpuBufferUsage_storageBuffer,
        HgGpuMemoryUsage_frequentUpdate);

    modelPipeline.pointLightCapacity = 64;
    modelPipeline.pointLightBuffer = hgGpuBufferCreate(
        sizeof(ModelPipelinePointLightData) * modelPipeline.dirLightCapacity,
        HgGpuBufferUsage_storageBuffer,
        HgGpuMemoryUsage_frequentUpdate);

    HgMeshVertex cubeVertices[]{
        {{ 0.5f,-0.5f,-0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {0,0}},
        {{ 0.5f, 0.5f,-0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {1,0}},
        {{ 0.5f, 0.5f, 0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {1,1}},
        {{ 0.5f,-0.5f, 0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {0,1}},

        {{-0.5f,-0.5f, 0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {0,0}},
        {{-0.5f, 0.5f, 0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {1,0}},
        {{-0.5f, 0.5f,-0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {1,1}},
        {{-0.5f,-0.5f,-0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {0,1}},

        {{-0.5f, 0.5f,-0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {0,0}},
        {{-0.5f, 0.5f, 0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {1,0}},
        {{ 0.5f, 0.5f, 0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {1,1}},
        {{ 0.5f, 0.5f,-0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {0,1}},

        {{-0.5f,-0.5f, 0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {0,0}},
        {{-0.5f,-0.5f,-0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {1,0}},
        {{ 0.5f,-0.5f,-0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {1,1}},
        {{ 0.5f,-0.5f, 0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {0,1}},

        {{-0.5f,-0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {0,0}},
        {{ 0.5f,-0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {1,0}},
        {{ 0.5f, 0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {1,1}},
        {{-0.5f, 0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {0,1}},

        {{ 0.5f,-0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {0,0}},
        {{-0.5f,-0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {1,0}},
        {{-0.5f, 0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {1,1}},
        {{ 0.5f, 0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {0,1}},
    };

    u32 cubeIndices[]{
         0,  1,  2,  0,  2,  3,
         4,  5,  6,  4,  6,  7,
         8,  9, 10,  8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };

    modelPipeline.defaultModel.vertexBuffer = hgGpuBufferCreate(
        sizeof(cubeVertices),
        HgGpuBufferUsage_storageBuffer | HgGpuBufferUsage_transferDst);

    modelPipeline.defaultModel.indexBuffer = hgGpuBufferCreate(
        sizeof(cubeIndices),
        HgGpuBufferUsage_storageBuffer | HgGpuBufferUsage_transferDst);

    hgGpuBufferWrite(modelPipeline.defaultModel.vertexBuffer, 0, cubeVertices, sizeof(cubeVertices));
    hgGpuBufferWrite(modelPipeline.defaultModel.indexBuffer, 0, cubeIndices, sizeof(cubeIndices));

    modelPipeline.defaultModel.vertexCount = (u32)hgArrayCount(cubeVertices);
    modelPipeline.defaultModel.vertexWidth = (u32)sizeof(HgMeshVertex);
    modelPipeline.defaultModel.indexCount = (u32)hgArrayCount(cubeIndices);

    struct Color {
        u8 r, g, b, a;
    };
    Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    HgVec4 defaultNormal{0, 0, -1, 0};

    modelPipeline.defaultColorMap.image = hgGpuImageCreate(3, 3, HgFormat_r8g8b8a8_srgb,
        HgGpuImageUsage_sampled | HgGpuImageUsage_transferDst);
    modelPipeline.defaultNormalMap.image = hgGpuImageCreate(1, 1, HgFormat_r32g32b32a32_sfloat,
        HgGpuImageUsage_sampled | HgGpuImageUsage_transferDst);

    modelPipeline.defaultColorMap.view = hgGpuViewCreate(
        modelPipeline.defaultColorMap.image, HgGpuAspect_color, HgGpuFilter_nearest);
    modelPipeline.defaultNormalMap.view = hgGpuViewCreate(
        modelPipeline.defaultNormalMap.image, HgGpuAspect_color, HgGpuFilter_nearest);

    hgGpuImageWrite(modelPipeline.defaultColorMap.view, defaultColors);
    hgGpuImageWrite(modelPipeline.defaultNormalMap.view, &defaultNormal);
}

void hgModelsDeinit()
{
    hgGpuPipelineDestroy(modelPipeline.pipeline);

    hgGpuViewDestroy(modelPipeline.defaultNormalMap.view);
    hgGpuImageDestroy(modelPipeline.defaultNormalMap.image);

    hgGpuViewDestroy(modelPipeline.defaultColorMap.view);
    hgGpuImageDestroy(modelPipeline.defaultColorMap.image);

    hgGpuBufferDestroy(modelPipeline.defaultModel.indexBuffer);
    hgGpuBufferDestroy(modelPipeline.defaultModel.vertexBuffer);

    hgGpuBufferDestroy(modelPipeline.pointLightBuffer);
    hgGpuBufferDestroy(modelPipeline.dirLightBuffer);
}

template<>
void hgSerialize(HgSerializer* s, HgModel* model)
{
    hgSerializeObject(s,
        &model->mesh,
        &model->colorMap,
        &model->normalMap);
}

HgModel* hgModelAdd(
    HgEcs* ecs,
    HgEntity e,
    HgMeshAsset* mesh,
    HgTextureAsset* colorMap,
    HgTextureAsset* normalMap)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    HgModel* model = hgEcsAdd<HgModel>(ecs, e);
    *model = {};
    model->mesh = mesh;
    model->colorMap = colorMap;
    model->normalMap = normalMap;

    return model;
}

template<>
void hgEcsDtor(HgModel* model)
{
    hgAssetUnload(model->normalMap);
    hgAssetUnload(model->colorMap);
    hgAssetUnload(model->mesh);
}

void hgModelsDraw(HgEcs* ecs, HgEntity camera, HgGpuCmd* cmd)
{
    hgAssert(ecs != nullptr);
    hgAssert(cmd != nullptr);

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    HgCamera* cameraC = hgEcsGet<HgCamera>(ecs, camera);
    HgTransform* cameraTf = hgEcsGet<HgTransform>(ecs, camera);

    HgMat4 view = hgMatModelToView(cameraTf->mat);

    u32 dirLightCount = hgEcsCount<HgDirLight>(ecs);
    u32 pointLightCount = hgEcsCount<HgPointLight>(ecs);

    if (dirLightCount > modelPipeline.dirLightCapacity)
    {
        if (modelPipeline.dirLightCapacity == 0)
            modelPipeline.dirLightCapacity = 1;
        while (modelPipeline.dirLightCapacity < dirLightCount)
        {
            modelPipeline.dirLightCapacity *= 2;
        }

        hgGpuWaitIdle();

        hgGpuBufferDestroy(modelPipeline.dirLightBuffer);
        modelPipeline.dirLightBuffer = hgGpuBufferCreate(
            sizeof(ModelPipelineDirLightData) * modelPipeline.dirLightCapacity,
            HgGpuBufferUsage_storageBuffer,
            HgGpuMemoryUsage_frequentUpdate);
    }

    if (pointLightCount > modelPipeline.pointLightCapacity)
    {
        if (modelPipeline.pointLightCapacity == 0)
            modelPipeline.pointLightCapacity = 1;
        while (modelPipeline.pointLightCapacity < pointLightCount)
        {
            modelPipeline.pointLightCapacity *= 2;
        }

        hgGpuWaitIdle();

        hgGpuBufferDestroy(modelPipeline.pointLightBuffer);
        modelPipeline.pointLightBuffer = hgGpuBufferCreate(
            sizeof(ModelPipelinePointLightData) * modelPipeline.pointLightCapacity,
            HgGpuBufferUsage_storageBuffer,
            HgGpuMemoryUsage_frequentUpdate);
    }

    ModelPipelineDirLightData* dirLights = hgArenaAlloc<ModelPipelineDirLightData>(scratch, dirLightCount);
    ModelPipelinePointLightData* pointLights = hgArenaAlloc<ModelPipelinePointLightData>(scratch, pointLightCount);

    u32 i = 0;
    hgEcsForEach<HgDirLight>(ecs, [&](HgEntity, HgDirLight* light)
    {
        dirLights[i].dir = HgVec4{HgMat3{view} * light->dir, 0.0};
        dirLights[i].color = light->color;
        ++i;
    });

    i = 0;
    hgEcsForEach<HgPointLight, HgTransform>(ecs, [&](HgEntity, HgPointLight* light, HgTransform* tf)
    {
        pointLights[i].pos = view * HgVec4{hgTransformWorldPos(*tf), 1.0};
        pointLights[i].color = light->color;
        ++i;
    });

    if (dirLightCount > 0)
        hgGpuBufferWrite(modelPipeline.dirLightBuffer, 0, dirLights, sizeof(*dirLights) * dirLightCount);

    if (pointLightCount > 0)
        hgGpuBufferWrite(modelPipeline.pointLightBuffer, 0, pointLights, sizeof(*pointLights) * pointLightCount);

    hgGpuBindPipeline(cmd, modelPipeline.pipeline);

    hgEcsForEach<HgModel, HgTransform>(ecs, [&](HgEntity, HgModel* model, HgTransform* tf)
    {
        HgTexture* colorMap = model->colorMap == nullptr
            ? &modelPipeline.defaultColorMap
            : &model->colorMap->asset;

        HgTexture* normalMap = model->normalMap == nullptr
            ? &modelPipeline.defaultNormalMap
            : &model->normalMap->asset;

        HgMesh* gpuModel = model->mesh == nullptr
            ? &modelPipeline.defaultModel
            : &model->mesh->asset;

        ModelPipelinePush push{};
        push.model = tf->mat;
        push.vertices = hgGpuBufferStorageDescriptor(gpuModel->vertexBuffer);
        push.indices = hgGpuBufferStorageDescriptor(gpuModel->indexBuffer);
        push.viewProj = hgGpuBufferUniformDescriptor(cameraC->vpBuffer);
        push.normalMap = hgGpuImageSamplerDescriptor(normalMap->view);
        push.colorMap = hgGpuImageSamplerDescriptor(colorMap->view);
        push.dirLights = hgGpuBufferStorageDescriptor(modelPipeline.dirLightBuffer);
        push.dirLightCount = dirLightCount;
        push.pointLights = hgGpuBufferStorageDescriptor(modelPipeline.pointLightBuffer);
        push.pointLightCount = pointLightCount;

        hgGpuPushConstants(cmd, modelPipeline.pipeline, &push, sizeof(push));

        hgGpuDraw(cmd, 0, gpuModel->indexCount, 0, 1);
    });
}

template<>
void hgAssetLoadImpl(HgAsset<HgSound>* data)
{
    (void)data;
    hgPanic("Load audio file impl : TODO\n");
}

template<>
void hgAssetUnloadImpl(HgAsset<HgSound>* data)
{
    (void)data;
    hgPanic("Unload audio file impl : TODO\n");
}

template<>
void hgSerialize(HgSerializer* s, HgAudioSource* src)
{
    hgSerializeObject(s,
        &src->audio,
        &src->position,
        &src->repeat);
}

HgAudioPlayer hgAudioPlayerCreate()
{
    HgAudioPlayer player{};
    player.music = hgArrayCreate<HgAudioPlayerMusic>();
    player.sounds = hgArrayCreate<HgAudioStream*>();
    return player;
}

void hgAudioPlayerDestroy(HgAudioPlayer* player)
{
    hgAssert(player != nullptr);

    for (u32 i = 0; i < player->sounds.count; ++i)
    {
        hgAudioStreamDestroy(player->sounds[i]);
    }

    for (u32 i = 0; i < player->music.count; ++i)
    {
        hgAudioStreamDestroy(player->music[i].stream);
    }

    hgArrayDestroy(&player->sounds);
    hgArrayDestroy(&player->music);
}

void hgAudioPlayerUpdate(HgAudioPlayer* player)
{
    for (u32 i = player->sounds.count - 1; i < player->sounds.count; --i)
    {
        if (hgAudioStreamQueuedSize(player->sounds[i]) == 0)
        {
            HgAudioStream* stream = hgArrayRemove(&player->sounds, i);
            hgAudioStreamDestroy(stream);
        }
    }

    for (u32 i = 0; i < player->music.count; ++i)
    {
        HgAudioPlayerMusic* music = &player->music[i];
        if (!music->playing)
            continue;

        HgSound* sound = &music->sound->asset;

        HgArena* scratch = hgScratch();
        hgArenaScope(scratch);

        u32 width = sizeof(f32);

        u32 total = sound->frequency * width / 16;
        u32 queued = hgAudioStreamQueuedSize(music->stream);
        if (queued >= total)
            continue;
        u32 sizeToPush = total - queued;

        f32* queue = (f32*)hgArenaAlloc(scratch, sizeToPush, width);
        u32 queueSize = 0;

        while (queueSize < sizeToPush)
        {
            if (music->pos == sound->size)
                music->pos = 0;

            u32 sizeToQueue = hgMin(sizeToPush - queueSize, (u32)(sound->size - music->pos));
            hgMemCopy((u8*)queue + queueSize, (u8*)sound->data + music->pos, sizeToQueue);
            queueSize += sizeToQueue;
            music->pos += sizeToQueue;
        }

        hgAssert(queueSize <= sizeToPush);
        hgAudioStreamPush(music->stream, queue, queueSize);
    }
}

void hgAudioPlayerMusic(HgAudioPlayer* player, HgSoundAsset* music)
{
    for (u32 i = 0; i < player->music.count; ++i)
    {
        if (player->music[i].sound == music)
        {
            player->music[i].playing = true;
            return;
        }
    }

    HgAudioPlayerMusic* track = hgArrayPush(&player->music);
    track->stream = hgAudioStreamCreate(music->asset.frequency, music->asset.channels);
    track->sound = music;
    track->pos = 0;
    track->playing = true;
}

void hgAudioPlayerMusicKill(HgAudioPlayer* player, HgSoundAsset* music)
{
    for (u32 i = 0; i < player->music.count; ++i)
    {
        if (player->music[i].sound == music)
        {
            HgAudioPlayerMusic track = hgArrayRemove(&player->music, i);
            hgAudioStreamDestroy(track.stream);
            return;
        }
    }
}

void hgAudioPlayerMusicPause(HgAudioPlayer* player, HgSoundAsset* music)
{
    for (u32 i = 0; i < player->music.count; ++i)
    {
        if (player->music[i].sound == music)
        {
            player->music[i].playing = false;
            return;
        }
    }
}

void hgAudioPlayerSetMusicGain(HgAudioPlayer* player, HgSoundAsset* music, f32 gain)
{
    for (u32 i = 0; i < player->music.count; ++i)
    {
        if (player->music[i].sound == music)
        {
            hgAudioStreamSetGain(player->music[i].stream, gain);
            return;
        }
    }
}

void hgAudioPlayerSound(HgAudioPlayer* player, HgSoundAsset* sound, f32 gain)
{
    HgAudioStream** stream = hgArrayPush(&player->sounds);
    *stream = hgAudioStreamCreate(sound->asset.frequency, sound->asset.channels);
    hgAudioStreamSetGain(*stream, gain);
    hgAudioStreamPush(*stream, sound->asset.data, sound->asset.size);
}

HgAudioSource* hgAudioSourceAdd(HgEcs* ecs, HgEntity e, HgSoundAsset* audio, bool repeat)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    HgAudioSource* src = hgEcsAdd<HgAudioSource>(ecs, e);
    *src = {};
    src->player = hgAudioStreamCreate(8000, 1);
    src->audio = audio;
    src->position = 0;
    src->repeat = repeat;

    return src;
}

template<>
void hgEcsDtor(HgAudioSource* src)
{
    hgAssetUnload(src->audio);
    hgAudioStreamDestroy(src->player);
}

void hgAudioUpdate(HgEcs* ecs, HgEntity listener)
{
    hgEcsForEach<HgAudioSource>(ecs, [&](HgEntity e, HgAudioSource* src)
    {
        HgSound* audio = &src->audio->asset;
        if (src->position == audio->size && !src->repeat)
            return;

        HgArena* scratch = hgScratch();
        hgArenaScope(scratch);

        u32 width = sizeof(f32);

        u32 total = audio->frequency * width / 8;
        u32 queued = hgAudioStreamQueuedSize(src->player);
        if (queued >= total)
            return;
        u32 sizeToPush = total - queued;

        f32* queue = (f32*)hgArenaAlloc(scratch, sizeToPush, width);
        u32 queueSize = 0;

        if (src->repeat)
        {
            while (queueSize < sizeToPush)
            {
                if (src->position == audio->size)
                    src->position = 0;

                u32 sizeToQueue = hgMin(sizeToPush - queueSize, (u32)(audio->size - src->position));
                hgMemCopy((u8*)queue + queueSize, (u8*)audio->data + src->position, sizeToQueue);
                queueSize += sizeToQueue;
                src->position += sizeToQueue;
            }
        }
        else
        {
            queueSize = hgMin(sizeToPush, (u32)(audio->size - src->position));
            hgMemCopy(queue, (u8*)audio->data + src->position, queueSize);
            src->position += queueSize;
        }

        if (hgEcsHas<HgTransform>(ecs, e))
        {
            hgAssert(hgEcsHas<HgTransform>(ecs, listener));

            HgVec3 relPos = hgTransformWorldPos(*hgEcsGet<HgTransform>(ecs, listener))
                          - hgTransformWorldPos(*hgEcsGet<HgTransform>(ecs, e));
            f32 factor = 1.0f / hgVecDot3(relPos, relPos);

            for (u64 i = 0; i < sizeToPush / sizeof(f32); ++i)
            {
                ((f32*)queue)[i] *= factor;
            }
        }

        hgAssert(queueSize <= sizeToPush);
        hgAudioStreamPush(src->player, queue, queueSize);
    });
}

HgEcs hgEcsCreate()
{
    HgEcs ecs{};
    ecs.entities = hgHandlePoolCreate();
    ecs.components = hgMapCreate<u64, HgComponent>(128);
    hgEcsReset(&ecs);
    return ecs;
}

void hgEcsDestroy(HgEcs* ecs)
{
    hgEcsReset(ecs);

    hgMapForEach(&ecs->components, [&](u64*, HgComponent* system)
    {
        hgArrayAnyDestroy(&system->components);
        hgArrayDestroy(&system->entities);
        hgArrayDestroy(&system->indices);
        hgStringDestroy(&system->name);
    });

    hgMapDestroy(&ecs->components);
    hgHandlePoolDestroy(&ecs->entities);
}

void hgEcsReset(HgEcs* ecs)
{
    hgAssert(ecs != nullptr);

    hgMapForEach(&ecs->components, [&](u64*, HgComponent* system)
    {
        for (u32 c = 1; c < system->components.count; ++c)
        {
            system->dtor(system->components[c]);
        }
        system->entities.count = 1;
        system->components.count = 1;
        hgMemClear(system->indices.vals, system->indices.count * sizeof(*system->indices.vals));
    });
    hgHandlePoolReset(&ecs->entities);
}

void hgEcsRegisterComponent(HgEcs* ecs, HgEcsRegisterComponent* config)
{
    hgAssert(ecs != nullptr);

    u64 id = hgHash(config->name);
    hgAssert(hgMapGet(&ecs->components, id) == nullptr);

    if (ecs->components.count * 2 > ecs->components.capacity)
        hgMapResize(&ecs->components, ecs->components.capacity * 2);
    HgComponent* system = hgMapAdd(&ecs->components, id, {});

    system->name = hgStringCreate(config->name);
    system->indices = hgArrayCreate<u32>();
    system->entities = hgArrayCreate<HgEntity>();
    system->components = hgArrayAnyCreate(config->width, config->align);
    system->dtor = config->dtor;
    system->serialize = config->serialize;

    hgArrayPush(&system->entities);
    hgArrayAnyPush(&system->components);
}

void hgEcsUnregisterComponent(HgEcs* ecs, u64 componentId)
{
    HgComponent* system = hgMapGet(&ecs->components, componentId);

    for (u32 c = 1; c < system->components.count; ++c)
    {
        system->dtor(system->components[c]);
    }
    hgArrayAnyDestroy(&system->components);
    hgArrayDestroy(&system->entities);
    hgArrayDestroy(&system->indices);
    hgStringDestroy(&system->name);

    hgMapRemove(&ecs->components, componentId);
}

HgString hgEcsComponentName(HgEcs* ecs, u64 componentId)
{
    hgAssert(ecs != nullptr);
    HgComponent* system = hgMapGet(&ecs->components, componentId);
    hgAssert(system != nullptr);
    return system->name;
}

HgEntity hgEcsSpawn(HgEcs* ecs)
{
    hgAssert(ecs != nullptr);
    return {hgHandlePoolAlloc(&ecs->entities)};
}

void hgEcsDespawn(HgEcs* ecs, HgEntity e)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    hgMapForEach(&ecs->components, [&](u64* id, HgComponent*)
    {
        if (hgEcsHas(ecs, e, *id))
            hgEcsRemove(ecs, e, *id);
    });
    hgHandlePoolFree(&ecs->entities, e.handle);
}

bool hgEcsAlive(HgEcs* ecs, HgEntity e)
{
    hgAssert(ecs != nullptr);
    return hgHandlePoolAlive(&ecs->entities, e.handle);
}

void* hgEcsAdd(HgEcs* ecs, HgEntity e, u64 componentId)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));
    hgAssert(!hgEcsHas(ecs, e, componentId));

    HgComponent* system = hgMapGet(&ecs->components, componentId);
    hgAssert(system != nullptr);

    u32 idx = hgHandleIdx(e.handle);
    if (idx >= system->indices.count)
    {
        u32 oldCount = system->indices.count;
        u32 newCount = idx * 2;
        hgArrayResize(&system->indices, newCount);
        for (u32 i = oldCount; i < newCount; ++i)
            system->indices[i] = 0;
    }
    system->indices[idx] = system->entities.count;
    *hgArrayPush(&system->entities) = e;
    return hgArrayAnyPush(&system->components);
}

void hgEcsRemove(HgEcs* ecs, HgEntity e, u64 componentId)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));
    hgAssert(hgEcsHas(ecs, e, componentId));

    HgComponent* system = hgMapGet(&ecs->components, componentId);
    hgAssert(system != nullptr);

    u32 idx = system->indices[hgHandleIdx(e.handle)];
    system->dtor(system->components[idx]);

    HgEntity last = hgArrayPop(&system->entities);
    if (e != last)
    {
        system->entities[idx] = last;
        system->indices[hgHandleIdx(last.handle)] = idx;
        hgMemCopy(
            system->components[idx],
            system->components[system->components.count - 1],
            system->components.width);
    }
    system->indices[hgHandleIdx(e.handle)] = 0;
    --system->components.count;
}

bool hgEcsHas(HgEcs* ecs, HgEntity e, u64 componentId)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    HgComponent* system = hgMapGet(&ecs->components, componentId);
    if (system == nullptr)
        return false;

    u32 idx = hgHandleIdx(e.handle);
    return idx < system->indices.count && system->indices[idx] != 0;
}

void* hgEcsGet(HgEcs* ecs, HgEntity e, u64 componentId)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    HgComponent* system = hgMapGet(&ecs->components, componentId);
    hgAssert(system != nullptr);
    hgAssert(system->indices[hgHandleIdx(e.handle)] != 0);
    hgAssert(system->indices[hgHandleIdx(e.handle)] < system->entities.count);

    return system->components[system->indices[hgHandleIdx(e.handle)]];
}

HgEntity hgEcsGetEntity(HgEcs* ecs, const void* component, u64 componentId)
{
    hgAssert(ecs != nullptr);
    hgAssert(component != nullptr);

    HgComponent* system = hgMapGet(&ecs->components, componentId);
    hgAssert(system != nullptr);

    return system->entities[(u32)((uptr)component - (uptr)system->components.vals) / system->components.width];
}

HgEntity* hgEcsEntities(HgEcs* ecs, u64 componentId)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgMapGet(&ecs->components, componentId) != nullptr);
    hgAssert(hgMapGet(&ecs->components, componentId)->entities.count != 0);
    return hgMapGet(&ecs->components, componentId)->entities.vals + 1;
}

void* hgEcsComponents(HgEcs* ecs, u64 componentId)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgMapGet(&ecs->components, componentId) != nullptr);
    hgAssert(hgMapGet(&ecs->components, componentId)->components.count != 0);
    HgComponent* system = hgMapGet(&ecs->components, componentId);
    return (u8*)system->components.vals + system->components.width;
}

u32 hgEcsCount(HgEcs* ecs, u64 componentId)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgMapGet(&ecs->components, componentId) != nullptr);
    hgAssert(hgMapGet(&ecs->components, componentId)->entities.count != 0);
    return hgMapGet(&ecs->components, componentId)->entities.count - 1;
}

u64 hgEcsFindSmallest(HgEcs* ecs, u64* ids, u32 idCount)
{
    hgAssert(ecs != nullptr);

    u32 smallestCount = (u32)-1;
    u64 smallest = ids[0];

    for (u32 i = 1; i < idCount; ++i)
    {
        HgComponent* system = hgMapGet(&ecs->components, ids[i]);
        hgAssert(system != nullptr);

        if (system->entities.count < smallestCount)
        {
            smallestCount = system->entities.count;
            smallest = ids[i];
        }
    }
    return smallest;
}

static void swapIdxLocation(HgEcs* ecs, u32 lhs, u32 rhs, u64 componentId)
{
    hgAssert(ecs != nullptr);

    HgComponent* system = hgMapGet(&ecs->components, componentId);
    hgAssert(system != nullptr);

    hgAssert(lhs != 0 && lhs < system->entities.count);
    hgAssert(rhs != 0 && rhs < system->entities.count);

    HgEntity lhsEntity = system->entities[lhs];
    HgEntity rhsEntity = system->entities[rhs];

    hgAssert(hgEcsAlive(ecs, lhsEntity));
    hgAssert(hgEcsAlive(ecs, rhsEntity));
    hgAssert(hgEcsHas(ecs, lhsEntity, componentId));
    hgAssert(hgEcsHas(ecs, rhsEntity, componentId));

    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);

    system->entities[lhs] = rhsEntity;
    system->entities[rhs] = lhsEntity;
    system->indices[hgHandleIdx(lhsEntity.handle)] = rhs;
    system->indices[hgHandleIdx(rhsEntity.handle)] = lhs;

    void* temp = hgArenaAlloc(scratch, system->components.width, 1);
    hgMemCopy(temp, system->components[lhs], system->components.width);
    hgMemCopy(system->components[lhs], system->components[rhs], system->components.width);
    hgMemCopy(system->components[rhs], temp, system->components.width);
}

namespace {
    struct QuicksortData {
        HgEcs* ecs;
        HgComponent* system;
        u64 comp;
        void* data;
        bool (*compare)(void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs);

        u32 quicksortInter(u32 pivot, u32 inc, u32 dec)
        {
            while (inc != dec)
            {
                while (!compare(data, ecs, system->entities[dec], system->entities[pivot]))
                {
                    --dec;
                    if (dec == inc)
                        goto finish;
                }
                while (!compare(data, ecs, system->entities[pivot], system->entities[inc]))
                {
                    ++inc;
                    if (inc == dec)
                        goto finish;
                }
                swapIdxLocation(ecs, inc, dec, comp);
            }

        finish:
            if (compare(data, ecs, system->entities[inc], system->entities[pivot]))
                swapIdxLocation(ecs, pivot, inc, comp);

            return inc;
        }

        void quicksort(u32 begin, u32 end)
        {
            if (begin + 1 >= end)
                return;

            u32 middle = quicksortInter(begin, begin + 1, end - 1);
            quicksort(begin, middle);
            quicksort(middle, end);
        }
    };
}

void hgEcsSort(
    HgEcs* ecs,
    u64 componentId,
    void* data,
    bool (*compare)(void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs))
{
    hgAssert(ecs != nullptr);
    hgAssert(compare != nullptr);

    HgComponent* system = hgMapGet(&ecs->components, componentId);
    hgAssert(system != nullptr);

    QuicksortData q{ecs, system, componentId, data, compare};
    q.quicksort(1, system->entities.count);
}

template<>
void hgSerialize(HgSerializer* s, HgEcs* ecs)
{
    hgAssert(s != nullptr);
    hgAssert(ecs != nullptr);

    HgArena* scratch = hgScratch(&s->arena, 1);
    hgArenaScope(scratch);

    hgSerializeBegin(s);
    hgDefer(hgSerializeEnd(s));

    HgEntitySerializer ecsSerial{};
    u32 entityCount = 0;
    if (s->writing)
    {
        ecsSerial.entityToIdx = hgArenaAlloc<u32>(scratch, ecs->entities.handles.count);
        for (u32 i = 1; i < ecs->entities.handles.count; ++i)
        {
            if (ecs->entities.handles[i] != hgHandleNull)
                ecsSerial.entityToIdx[hgHandleIdx(ecs->entities.handles[i])] = entityCount++;
        }

        hgSerialize(s, &entityCount);
    }
    else
    {
        hgSerialize(s, &entityCount);

        ecsSerial.idxToEntity = hgArenaAlloc<HgEntity>(scratch, entityCount);
        for (u32 i = 0; i < entityCount; ++i)
        {
            ecsSerial.idxToEntity[i] = hgEcsSpawn(ecs);
        }
    }

    u32 systemCount;
    if (s->writing)
        systemCount = ecs->components.count;
    hgSerializeBegin(s, &systemCount);
    hgDefer(hgSerializeEnd(s));

    u32 systemIdx = (u32)-1;
    for (u32 i = 0; i < systemCount; ++i)
    {
        hgSerializeBegin(s);
        hgDefer(hgSerializeEnd(s));

        u64 systemId = (u64)-1;
        HgComponent* system;
        if (s->writing)
        {
            ++systemIdx;
            while (!ecs->components.hasVal[systemIdx])
            {
                ++systemIdx;
            }
            systemId = ecs->components.keys[systemIdx];
            system = &ecs->components.vals[systemIdx];
            hgSerialize(s, &system->name);
        }
        else
        {
            HgString compName;
            hgSerialize(s, &compName);
            systemId = hgHash(compName);
            system = hgMapGet(&ecs->components, systemId);
        }

        u32 compCount;
        if (s->writing)
            compCount = system->entities.count - 1;
        hgSerializeBegin(s, &compCount);
        hgDefer(hgSerializeEnd(s));

        for (u32 c = 0; c < compCount; ++c)
        {
            hgSerializeBegin(s);
            hgDefer(hgSerializeEnd(s));

            u32 entityIdx;
            if (s->writing)
                entityIdx = ecsSerial.entityToIdx[hgHandleIdx(system->entities[c + 1].handle)];
            hgSerialize(s, &entityIdx);

            void* compData;
            if (s->writing)
                compData = system->components[c + 1];
            else
                compData = hgEcsAdd(ecs, ecsSerial.idxToEntity[entityIdx], systemId);
            system->serialize(s, compData, &ecsSerial);
        }
    }
}

void hgEntitySerialize(HgSerializer* s, HgEntity* val, HgEntitySerializer* ecs)
{
    if (s->writing)
    {
        u32 idx = *val != hgEntityNull ? ecs->entityToIdx[hgHandleIdx(val->handle)] : (u32)-1;
        hgSerialize(s, (i32*)&idx);
    }
    else
    {
        u32 idx = (u32)-1;
        hgSerialize(s, (i32*)&idx);
        *val = idx != (u32)-1 ? ecs->idxToEntity[idx] : hgEntityNull;
    }
}

template<>
void hgEcsSerialize(HgSerializer* s, HgNode* node, HgEntitySerializer* ecs)
{
    hgSerializeBegin(s);
    hgEntitySerialize(s, &node->parent, ecs);
    hgEntitySerialize(s, &node->nextSibling, ecs);
    hgEntitySerialize(s, &node->prevSibling, ecs);
    hgEntitySerialize(s, &node->firstChild, ecs);
    hgSerializeEnd(s);
}

HgNode* hgNodeAdd(HgEcs* ecs, HgEntity e)
{
    HgNode* node = hgEcsAdd<HgNode>(ecs, e);
    *node = {};
    return node;
}

void hgNodeDestroy(HgEcs* ecs, HgEntity e)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    HgNode* node = hgEcsGet<HgNode>(ecs, e);
    if (node->parent.handle != hgHandleNull)
    {
        if (node->prevSibling.handle != hgHandleNull)
            hgEcsGet<HgNode>(ecs, node->prevSibling)->nextSibling = node->nextSibling;
        else
            hgEcsGet<HgNode>(ecs, node->parent)->firstChild = node->nextSibling;

        if (node->nextSibling.handle != hgHandleNull)
            hgEcsGet<HgNode>(ecs, node->nextSibling)->prevSibling = node->prevSibling;
    }

    HgEntity child = node->firstChild;
    while (child.handle != hgHandleNull)
    {
        HgNode* childNode = hgEcsGet<HgNode>(ecs, child);
        HgEntity next = childNode->nextSibling;
        hgNodeDestroy(ecs, child);
        child = next;
    }

    hgEcsDespawn(ecs, e);
}

void hgNodeDetach(HgEcs* ecs, HgEntity e)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    HgNode* node = hgEcsGet<HgNode>(ecs, e);
    if (node->parent.handle == hgHandleNull)
    {
        hgAssert(node->prevSibling.handle == hgHandleNull);
        hgAssert(node->nextSibling.handle == hgHandleNull);

        HgEntity child = node->firstChild;
        while (child.handle != hgHandleNull)
        {
            HgNode* childNode = hgEcsGet<HgNode>(ecs, child);
            HgEntity next = childNode->nextSibling;
            childNode->parent = HgEntity{};
            childNode->nextSibling = HgEntity{};
            childNode->prevSibling = HgEntity{};
            child = next;
        }
    } else {
        if (node->prevSibling.handle != hgHandleNull)
            hgEcsGet<HgNode>(ecs, node->prevSibling)->nextSibling = node->nextSibling;
        else
            hgEcsGet<HgNode>(ecs, node->parent)->firstChild = node->nextSibling;

        if (node->nextSibling.handle != hgHandleNull)
            hgEcsGet<HgNode>(ecs, node->nextSibling)->prevSibling = node->prevSibling;

        HgEntity child = node->firstChild;
        while (child.handle != hgHandleNull)
        {
            HgNode* childNode = hgEcsGet<HgNode>(ecs, child);
            HgEntity next = childNode->nextSibling;
            childNode->parent = HgEntity{};
            childNode->nextSibling = HgEntity{};
            childNode->prevSibling = HgEntity{};
            hgNodeAddChild(ecs, node->parent, child);
            child = next;
        }
    }
    *node = {};
}

void hgNodeAddChild(HgEcs* ecs, HgEntity parent, HgEntity child)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, parent));
    hgAssert(hgEcsAlive(ecs, child));

    HgNode* node = hgEcsGet<HgNode>(ecs, parent);
    HgNode* childNode = hgEcsGet<HgNode>(ecs, child);

    hgAssert(childNode->parent.handle == hgHandleNull);
    hgAssert(childNode->prevSibling.handle == hgHandleNull);
    hgAssert(childNode->nextSibling.handle == hgHandleNull);

    if (node->firstChild.handle != hgHandleNull)
    {
        hgEcsGet<HgNode>(ecs, node->firstChild)->prevSibling = child;
        childNode->nextSibling = node->firstChild;
    }
    node->firstChild = child;
    childNode->parent = parent;
}

template<>
void hgSerialize(HgSerializer* s, HgTransform* node)
{
    hgSerializeObject(s,
        &node->position,
        &node->scale,
        &node->rotation);
}

HgTransform* hgTransformAdd(HgEcs* ecs, HgEntity e, HgVec3 position, HgVec3 scale, HgQuat rotation)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    HgTransform* tf = hgEcsAdd<HgTransform>(ecs, e);
    *tf = {};
    tf->position = position;
    tf->scale = scale;
    tf->rotation = rotation;
    hgTransformUpdate(ecs, e);

    return tf;
}

static void transformUpdateChild(HgEcs* ecs, HgEntity e)
{
    hgAssert(hgEcsHas<HgTransform>(ecs, e));

    HgNode* node = hgEcsGet<HgNode>(ecs, e);

    HgTransform* tf = hgEcsGet<HgTransform>(ecs, e);

    tf->mat = hgEcsGet<HgTransform>(ecs, node->parent)->mat
            * hgMatModel3D(tf->position, tf->scale, tf->rotation);

    HgEntity child = node->firstChild;
    while (child.handle != hgHandleNull)
    {
        hgTransformUpdate(ecs, child);
        child = hgEcsGet<HgNode>(ecs, child)->nextSibling;
    }
}

void hgTransformUpdate(HgEcs* ecs, HgEntity e)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));
    hgAssert(hgEcsHas<HgTransform>(ecs, e));

    if (hgEcsHas<HgNode>(ecs, e))
    {
        HgNode* node = hgEcsGet<HgNode>(ecs, e);
        if (node->parent.handle != hgHandleNull && hgEcsHas<HgTransform>(ecs, node->parent))
        {
            transformUpdateChild(ecs, e);
        }
        else
        {
            HgTransform* tf = hgEcsGet<HgTransform>(ecs, e);
            tf->mat = hgMatModel3D(tf->position, tf->scale, tf->rotation);

            HgEntity child = node->firstChild;
            while (child.handle != hgHandleNull)
            {
                transformUpdateChild(ecs, child);
                child = hgEcsGet<HgNode>(ecs, child)->nextSibling;
            }
        }
    }
    else
    {
        HgTransform* tf = hgEcsGet<HgTransform>(ecs, e);
        tf->mat = hgMatModel3D(tf->position, tf->scale, tf->rotation);
    }
}


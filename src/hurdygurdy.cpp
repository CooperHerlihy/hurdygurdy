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

#include "stb_image.h"
#include "stb_image_write.h"

static HgSubsystemFlags initialized = 0;

void hgInit(HgSubsystemFlags init)
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
        hgPlatformInit();
    }

    if (init & HgSubsystem_gpu ||
        init & HgSubsystem_windowing)
    {
        hgGpuInit();
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
        initialized |= HgSubsystem_assets;
    }

    if (init & HgSubsystem_audio)
    {
        hgAudioInit();
        initialized |= HgSubsystem_assets;
    }
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

void hgMemClear(void* dst, u64 size)
{
    memset(dst, 0, size);
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
    return malloc(size);
}

void* hgGpaRealloc(void* allocation, u64 oldSize, u64 newSize, u64 alignment)
{
    (void)oldSize;
    (void)alignment;
    return realloc(allocation, newSize);
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
    arena->head = hgAlign((u64)arena->head, alignment) + size;
    hgAssert(arena->head <= arena->capacity);
    return (void*)((uptr)arena->memory + arena->head - size);
}

void* hgArenaRealloc(HgArena* arena, void* allocation, u64 oldSize, u64 newSize, u64 alignment)
{
    hgAssert(arena != nullptr);

    if (allocation >= arena->memory && (uptr)allocation + oldSize <= (uptr)arena->memory + arena->capacity)
    {
        if ((uptr)allocation + oldSize - (uptr)arena->memory == (uptr)arena->head)
        {
            arena->head = (uptr)allocation + newSize - (uptr)arena->memory;
            hgAssert(arena->head <= arena->capacity);
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
    if (arenas != nullptr)
        return;

    size = hgAlign(size, 16);
    hgAssert(size < UINT64_MAX / count);

    void* block = malloc(count * size);
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
        free(arenas);
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
    hgError("No scratch arena available\n");
}

void hgBinaryResize(HgArena* arena, HgBinary* bin, u64 newSize)
{
    bin->data = hgArenaRealloc(arena, bin->data, bin->size, newSize, 1);
    bin->size = newSize;
}

void hgBinaryRead(const HgBinary* bin, u64 idx, void* dst, u64 len)
{
    hgAssert(idx + len <= bin->size);
    hgMemCopy(dst, (u8*)bin->data + idx, len);
}

void hgBinaryOverwrite(HgBinary* bin, u64 idx, const void* src, u64 len)
{
    hgAssert(idx + len <= bin->size);
    hgMemCopy((u8*)bin->data + idx, src, len);
}

char* hgCString(HgArena* arena, HgStringView str)
{
    hgAssert(arena != nullptr);
    if (str.length > 0)
        hgAssert(str.chars != nullptr);

    char* cStr = hgArenaAlloc<char>(arena, str.length + 1);
    hgMemCopy(cStr, str.chars, str.length);
    cStr[str.length] = 0;
    return cStr;
}

HgStringOwner hgStringCreate(HgStringView data)
{
    HgStringOwner str{};
    if (data != "")
    {
        str.chars = hgGpaAlloc<char>(data.length);
        hgMemCopy((char*)str.chars, data.chars, data.length);
        str.length = data.length;
    }
    return str;
}

void hgStringDestroy(HgStringOwner* str)
{
    hgGpaFree((char*)str->chars, str->length);
}

HgStringBuilder hgStringCopy(HgArena* arena, HgStringView str)
{
    hgAssert(arena != nullptr);

    HgStringBuilder copy{};
    copy.chars = hgArenaAlloc<char>(arena, str.length);
    hgMemCopy(copy.chars, str.chars, str.length);
    copy.length = str.length;
    return copy;
}

void hgStringInsert(HgArena* arena, HgStringBuilder* dst, u64 idx, HgStringView src)
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
    return c == ' ' || c == '\t' || c == '\n';
}

bool hgIsNumeral(char c)
{
    return c >= '0' && c <= '9';
}

bool hgIsInteger(HgStringView str)
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

bool hgIsFloat(HgStringView str)
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

i64 hgStringToInteger(HgStringView str)
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

f64 hgStringToFloat(HgStringView str)
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

    HgStringBuilder intStr = hgIntegerToString(scratch, (i64)fabs(num));

    HgStringBuilder decStr{};
    hgStringAppendC(scratch, &decStr, '.');

    f64 decPart = fabs(num);
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

HgStringBuilder hgStringFormat(HgArena* arena, HgStringView fmt, ...)
{
    (void)arena;
    (void)fmt;
    hgError("hgFormatString not implemented yet : TODO\n");
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

void hgPerfLog(HgStringView title, const HgPerfStats* stats, HgPerfScale scale)
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

const char* hgSerialTypeToString(HgSerialType s)
{
    switch (s)
    {
        case HgSerialType_null:
            return "HgSerialType_null";
        case HgSerialType_object:
            return "HgSerialType_object";
        case HgSerialType_array:
            return "HgSerialType_array";
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
    s.writing = true;
    s.idx = 0;
    s.current = hgArenaAlloc<HgSerialNode>(arena, 1);
    s.current->type = HgSerialType_object;
    s.current->object = {0, nullptr};
    return s;
}

HgSerializer hgSerialReader(HgSerialNode* begin)
{
    HgSerializer s{};
    s.writing = false;
    s.idx = 0;
    s.current = begin;
    return s;
}

static HgSerialNode serialNodeNull = {HgSerialType_null, {}};
static HgSerializer serializerNull = {false, 0, &serialNodeNull};

bool hgSerializerIsNull(HgSerializer s)
{
    return
        s.current->type == HgSerialType_null ||
        (s.current->type == HgSerialType_object &&
         (s.current->object.fieldCount == s.idx ||
          s.current->object.fields[s.idx].data.type == HgSerialType_null)) ||
        (s.current->type == HgSerialType_array &&
         (s.current->array.elemCount == s.idx ||
          s.current->array.elems[s.idx].type == HgSerialType_null));
}

HgSerializer hgSerializeArray(HgArena* arena, HgSerializer* s, HgStringView name, u32* length)
{
    if (s->writing)
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (name == "")
                    hgWarn("Serializing object field with no name\n");

                s->current->object.fields = hgArenaRealloc(
                    arena,
                    s->current->object.fields,
                    s->current->object.fieldCount,
                    s->current->object.fieldCount + 1);

                HgSerialField* field = &s->current->object.fields[s->current->object.fieldCount++];
                field->name = hgStringCopy(arena, name);
                field->data.type = HgSerialType_array;
                field->data.array = {*length, hgArenaAlloc<HgSerialNode>(arena, *length)};

                HgSerializer next{};
                next.writing = true;
                next.idx = 0;
                next.current = &field->data;
                return next;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: %.*s\n", (int)name.length, name.chars);

                HgSerialNode* elem = &s->current->array.elems[s->idx++];
                elem->type = HgSerialType_array;
                elem->array = {*length, hgArenaAlloc<HgSerialNode>(arena, *length)};

                HgSerializer next{};
                next.writing = true;
                next.idx = 0;
                next.current = elem;
                return next;
            }
            default:
            {
                hgWarn("Serializer cannot write an array to a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return serializerNull;
            }
        }
    }
    else
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (s->idx >= s->current->object.fieldCount)
                {
                    hgWarn("Serializer current object cannot be further deserialized\n");
                    return serializerNull;
                }

                HgSerialField* field = &s->current->object.fields[s->idx++];

                if (field->name != name)
                    hgWarn("Serializer array name \"%.*s\" does not match expected name \"%.*s\"\n",
                            (int)field->name.length, field->name.chars, (int)name.length, name.chars);

                if (field->data.type != HgSerialType_array)
                {
                    hgWarn("Serializer expected array, found: %s\n", hgSerialTypeToString(field->data.type));
                    return serializerNull;
                };

                *length = field->data.array.elemCount;

                HgSerializer next{};
                next.writing = false;
                next.idx = 0;
                next.current = &field->data;
                return next;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: \"%.*s\"\n", (int)name.length, name.chars);

                if (s->idx >= s->current->array.elemCount)
                {
                    hgWarn("Serializer current array cannot be further deserialized\n");
                    return serializerNull;
                }

                HgSerialNode* elem = &s->current->array.elems[s->idx++];

                if (elem->type != HgSerialType_array)
                {
                    hgWarn("Serializer expected array, found: %s\n", hgSerialTypeToString(elem->type));
                    return serializerNull;
                };

                *length = elem->array.elemCount;

                HgSerializer next{};
                next.writing = false;
                next.idx = 0;
                next.current = elem;
                return next;
            }
            default:
            {
                hgWarn("Serializer cannot read an array from a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return serializerNull;
            }
        }
    }
}

HgSerializer hgSerializeObject(HgArena* arena, HgSerializer* s, HgStringView name)
{
    if (s->writing)
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (name == "")
                    hgWarn("Serializing object field with no name\n");

                s->current->object.fields = hgArenaRealloc(
                    arena,
                    s->current->object.fields,
                    s->current->object.fieldCount,
                    s->current->object.fieldCount + 1);

                HgSerialField* field = &s->current->object.fields[s->current->object.fieldCount++];
                field->name = hgStringCopy(arena, name);
                field->data.type = HgSerialType_object;
                field->data.object = {0, nullptr};

                HgSerializer next{};
                next.writing = true;
                next.idx = 0;
                next.current = &field->data;
                return next;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: %.*s\n", (int)name.length, name.chars);

                HgSerialNode* elem = &s->current->array.elems[s->idx++];
                elem->type = HgSerialType_object;
                elem->object = {0, nullptr};

                HgSerializer next{};
                next.writing = true;
                next.idx = 0;
                next.current = elem;
                return next;
            }
            default:
            {
                hgWarn("Serializer cannot write an object to a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return serializerNull;
            }
        }
    }
    else
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (s->idx >= s->current->object.fieldCount)
                {
                    hgWarn("Serializer current object cannot be further deserialized\n");
                    return serializerNull;
                }

                HgSerialField* field = &s->current->object.fields[s->idx++];

                if (field->name != name)
                    hgWarn("Serializer object name \"%.*s\" does not match expected name \"%.*s\"\n",
                            (int)field->name.length, field->name.chars, (int)name.length, name.chars);

                if (field->data.type != HgSerialType_object)
                {
                    hgWarn("Serializer expected object, found: %s\n", hgSerialTypeToString(field->data.type));
                    return serializerNull;
                };

                HgSerializer next{};
                next.writing = false;
                next.idx = 0;
                next.current = &field->data;
                return next;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: \"%.*s\"\n", (int)name.length, name.chars);

                if (s->idx >= s->current->array.elemCount)
                {
                    hgWarn("Serializer current array cannot be further deserialized\n");
                    return serializerNull;
                }

                HgSerialNode* elem = &s->current->array.elems[s->idx++];

                if (elem->type != HgSerialType_object)
                {
                    hgWarn("Serializer expected object, found: %s\n", hgSerialTypeToString(elem->type));
                    return serializerNull;
                };

                HgSerializer next{};
                next.writing = false;
                next.idx = 0;
                next.current = elem;
                return next;
            }
            default:
            {
                hgWarn("Serializer cannot read an object from a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return serializerNull;
            }
        }
    }
}

void hgSerializeNull(HgArena* arena, HgSerializer* s, HgStringView name)
{
    if (s->writing)
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (name == "")
                    hgWarn("Serializing object field with no name\n");

                s->current->object.fields = hgArenaRealloc(
                    arena,
                    s->current->object.fields,
                    s->current->object.fieldCount,
                    s->current->object.fieldCount + 1);

                HgSerialField* field = &s->current->object.fields[s->current->object.fieldCount++];
                field->name = hgStringCopy(arena, name);
                field->data.type = HgSerialType_null;
                return;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: %.*s\n", (int)name.length, name.chars);

                HgSerialNode* elem = &s->current->array.elems[s->idx++];
                elem->type = HgSerialType_object;
                elem->object = {0, nullptr};
                return;
            }
            default:
            {
                hgWarn("Serializer cannot write a value to a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return;
            }
        }
    }
    else
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (s->idx >= s->current->object.fieldCount)
                {
                    hgWarn("Serializer current object cannot be further deserialized\n");
                    return;
                }

                HgSerialField* field = &s->current->object.fields[s->idx++];

                if (field->name != name)
                    hgWarn("Serializer object name \"%.*s\" does not match expected name \"%.*s\"\n",
                            (int)field->name.length, field->name.chars, (int)name.length, name.chars);

                if (field->data.type != HgSerialType_null)
                {
                    hgWarn("Serializer expected null, found: %s\n", hgSerialTypeToString(field->data.type));
                    return;
                };

                return;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: \"%.*s\"\n", (int)name.length, name.chars);

                if (s->idx >= s->current->array.elemCount)
                {
                    hgWarn("Serializer current array cannot be further deserialized\n");
                    return;
                }

                HgSerialNode* elem = &s->current->array.elems[s->idx++];

                if (elem->type != HgSerialType_null)
                {
                    hgWarn("Serializer expected null, found: %s\n", hgSerialTypeToString(elem->type));
                    return;
                };

                return;
            }
            default:
            {
                hgWarn("Serializer cannot read a value from a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return;
            }
        }
    }
}

void serializeString(HgArena* arena, HgSerializer* s, HgStringView name, HgStringBuilder* string)
{
    if (s->writing)
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (name == "")
                    hgWarn("Serializing object field with no name\n");

                s->current->object.fields = hgArenaRealloc(
                    arena,
                    s->current->object.fields,
                    s->current->object.fieldCount,
                    s->current->object.fieldCount + 1);

                HgSerialField* field = &s->current->object.fields[s->current->object.fieldCount++];
                field->name = hgStringCopy(arena, name);
                field->data.type = HgSerialType_string;
                field->data.string = hgStringCopy(arena, *string);
                return;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: %.*s\n", (int)name.length, name.chars);

                HgSerialNode* elem = &s->current->array.elems[s->idx++];
                elem->type = HgSerialType_string;
                elem->string = hgStringCopy(arena, *string);
                return;
            }
            default:
            {
                hgWarn("Serializer cannot write a value to a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return;
            }
        }
    }
    else
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (s->idx >= s->current->object.fieldCount)
                {
                    hgWarn("Serializer current object cannot be further deserialized\n");
                    return;
                }

                HgSerialField* field = &s->current->object.fields[s->idx++];

                if (field->name != name)
                    hgWarn("Serializer object name \"%.*s\" does not match expected name \"%.*s\"\n",
                            (int)field->name.length, field->name.chars, (int)name.length, name.chars);

                if (field->data.type != HgSerialType_string)
                {
                    hgWarn("Serializer expected string, found: %s\n", hgSerialTypeToString(field->data.type));
                    return;
                };

                *string = hgStringCopy(arena, field->data.string);
                return;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: \"%.*s\"\n", (int)name.length, name.chars);

                if (s->idx >= s->current->array.elemCount)
                {
                    hgWarn("Serializer current array cannot be further deserialized\n");
                    return;
                }

                HgSerialNode* elem = &s->current->array.elems[s->idx++];

                if (elem->type != HgSerialType_string)
                {
                    hgWarn("Serializer expected string, found: %s\n", hgSerialTypeToString(elem->type));
                    return;
                };

                *string = hgStringCopy(arena, elem->string);
                return;
            }
            default:
            {
                hgWarn("Serializer cannot read a value from a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return;
            }
        }
    }
}

void hgSerializeBinary(HgArena* arena, HgSerializer* s, HgStringView name, HgBinary* binary)
{
    HgStringBuilder str = {(char*)binary->data, binary->size};
    serializeString(arena, s, name, &str);
    if (!s->writing)
        *binary = {str.chars, str.length};
}

void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgBinary* val)
{
    hgSerializeBinary(arena, s, name, val);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgStringView* val)
{
    HgStringBuilder str = {(char*)val->chars, val->length};
    serializeString(arena, s, name, &str);
    if (!s->writing)
        *val = str;
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgStringBuilder* val)
{
    serializeString(arena, s, name, val);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgStringOwner* val)
{
    HgStringBuilder str = {(char*)val->chars, val->length};
    if (s->writing)
    {
        serializeString(arena, s, name, &str);
    }
    else
    {
        serializeString(hgScratch(), s, name, &str);
        *val = hgStringCreate(str);
    }
}

template<typename T>
void serializeInteger(HgArena* arena, HgSerializer* s, HgStringView name, T* val)
{
    if (s->writing)
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (name == "")
                    hgWarn("Serializing object field with no name\n");

                s->current->object.fields = hgArenaRealloc(
                    arena,
                    s->current->object.fields,
                    s->current->object.fieldCount,
                    s->current->object.fieldCount + 1);

                HgSerialField* field = &s->current->object.fields[s->current->object.fieldCount++];
                field->name = hgStringCopy(arena, name);
                field->data.type = HgSerialType_integer;
                field->data.integer = (i64)*val;
                return;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: %.*s\n", (int)name.length, name.chars);

                HgSerialNode* elem = &s->current->array.elems[s->idx++];
                elem->type = HgSerialType_integer;
                elem->integer = (i64)*val;
                return;
            }
            default:
            {
                hgWarn("Serializer cannot write a value to a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return;
            }
        }
    }
    else
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (s->idx >= s->current->object.fieldCount)
                {
                    hgWarn("Serializer current object cannot be further deserialized\n");
                    return;
                }

                HgSerialField* field = &s->current->object.fields[s->idx++];

                if (field->name != name)
                    hgWarn("Serializer object name \"%.*s\" does not match expected name \"%.*s\"\n",
                            (int)field->name.length, field->name.chars, (int)name.length, name.chars);

                if (field->data.type != HgSerialType_integer)
                {
                    hgWarn("Serializer expected integer, found: %s\n", hgSerialTypeToString(field->data.type));
                    return;
                };

                *val = (T)field->data.integer;
                return;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: \"%.*s\"\n", (int)name.length, name.chars);

                if (s->idx >= s->current->array.elemCount)
                {
                    hgWarn("Serializer current array cannot be further deserialized\n");
                    return;
                }

                HgSerialNode* elem = &s->current->array.elems[s->idx++];

                if (elem->type != HgSerialType_integer)
                {
                    hgWarn("Serializer expected integer, found: %s\n", hgSerialTypeToString(elem->type));
                    return;
                };

                *val = (T)elem->integer;
                return;
            }
            default:
            {
                hgWarn("Serializer cannot read a value from a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return;
            }
        }
    }
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, u8* val)
{
    serializeInteger(arena, s, name, val);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, u16* val)
{
    serializeInteger(arena, s, name, val);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, u32* val)
{
    serializeInteger(arena, s, name, val);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, u64* val)
{
    serializeInteger(arena, s, name, val);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, i8* val)
{
    serializeInteger(arena, s, name, val);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, i16* val)
{
    serializeInteger(arena, s, name, val);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, i32* val)
{
    serializeInteger(arena, s, name, val);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, i64* val)
{
    serializeInteger(arena, s, name, val);
}

template<typename T>
void serializeFloating(HgArena* arena, HgSerializer* s, HgStringView name, T* val)
{
    if (s->writing)
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (name == "")
                    hgWarn("Serializing object field with no name\n");

                s->current->object.fields = hgArenaRealloc(
                    arena,
                    s->current->object.fields,
                    s->current->object.fieldCount,
                    s->current->object.fieldCount + 1);

                HgSerialField* field = &s->current->object.fields[s->current->object.fieldCount++];
                field->name = hgStringCopy(arena, name);
                field->data.type = HgSerialType_floating;
                field->data.floating = (f64)*val;
                return;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: %.*s\n", (int)name.length, name.chars);

                HgSerialNode* elem = &s->current->array.elems[s->idx++];
                elem->type = HgSerialType_floating;
                elem->floating = (f64)*val;
                return;
            }
            default:
            {
                hgWarn("Serializer cannot write a value to a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return;
            }
        }
    }
    else
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (s->idx >= s->current->object.fieldCount)
                {
                    hgWarn("Serializer current object cannot be further deserialized\n");
                    return;
                }

                HgSerialField* field = &s->current->object.fields[s->idx++];

                if (field->name != name)
                    hgWarn("Serializer object name \"%.*s\" does not match expected name \"%.*s\"\n",
                            (int)field->name.length, field->name.chars, (int)name.length, name.chars);

                if (field->data.type != HgSerialType_floating)
                {
                    hgWarn("Serializer expected floating, found: %s\n", hgSerialTypeToString(field->data.type));
                    return;
                };

                *val = (T)field->data.floating;
                return;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: \"%.*s\"\n", (int)name.length, name.chars);

                if (s->idx >= s->current->array.elemCount)
                {
                    hgWarn("Serializer current array cannot be further deserialized\n");
                    return;
                }

                HgSerialNode* elem = &s->current->array.elems[s->idx++];

                if (elem->type != HgSerialType_floating)
                {
                    hgWarn("Serializer expected floating, found: %s\n", hgSerialTypeToString(elem->type));
                    return;
                };

                *val = (T)elem->floating;
                return;
            }
            default:
            {
                hgWarn("Serializer cannot read a value from a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return;
            }
        }
    }
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, f32* val)
{
    serializeFloating(arena, s, name, val);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, f64* val)
{
    serializeFloating(arena, s, name, val);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, bool* val)
{
    if (s->writing)
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (name == "")
                    hgWarn("Serializing object field with no name\n");

                s->current->object.fields = hgArenaRealloc(
                    arena,
                    s->current->object.fields,
                    s->current->object.fieldCount,
                    s->current->object.fieldCount + 1);

                HgSerialField* field = &s->current->object.fields[s->current->object.fieldCount++];
                field->name = hgStringCopy(arena, name);
                field->data.type = HgSerialType_boolean;
                field->data.boolean = *val;
                return;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: %.*s\n", (int)name.length, name.chars);

                HgSerialNode* elem = &s->current->array.elems[s->idx++];
                elem->type = HgSerialType_boolean;
                elem->boolean = *val;
                return;
            }
            default:
            {
                hgWarn("Serializer cannot write a value to a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return;
            }
        }
    }
    else
    {
        switch (s->current->type)
        {
            case HgSerialType_object:
            {
                if (s->idx >= s->current->object.fieldCount)
                {
                    hgWarn("Serializer current object cannot be further deserialized\n");
                    return;
                }

                HgSerialField* field = &s->current->object.fields[s->idx++];

                if (field->name != name)
                    hgWarn("Serializer object name \"%.*s\" does not match expected name \"%.*s\"\n",
                            (int)field->name.length, field->name.chars, (int)name.length, name.chars);

                if (field->data.type != HgSerialType_boolean)
                {
                    hgWarn("Serializer expected boolean, found: %s\n", hgSerialTypeToString(field->data.type));
                    return;
                };

                *val = field->data.boolean;
                return;
            }
            case HgSerialType_array:
            {
                if (name != "")
                    hgWarn("Serializing array element with name: \"%.*s\"\n", (int)name.length, name.chars);

                if (s->idx >= s->current->array.elemCount)
                {
                    hgWarn("Serializer current array cannot be further deserialized\n");
                    return;
                }

                HgSerialNode* elem = &s->current->array.elems[s->idx++];

                if (elem->type != HgSerialType_boolean)
                {
                    hgWarn("Serializer expected boolean, found: %s\n", hgSerialTypeToString(elem->type));
                    return;
                };

                *val = elem->floating;
                return;
            }
            default:
            {
                hgWarn("Serializer cannot read a value from a non-object/non-array: %s\n", hgSerialTypeToString(s->current->type));
                return;
            }
        }
    }
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgVec2* val)
{
    u32 len = 2;
    HgSerializer obj = hgSerializeArray(arena, s, name, &len);
    hgSerialize(arena, &obj, "", &val->x);
    hgSerialize(arena, &obj, "", &val->y);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgVec3* val)
{
    u32 len = 3;
    HgSerializer obj = hgSerializeArray(arena, s, name, &len);
    hgSerialize(arena, &obj, "", &val->x);
    hgSerialize(arena, &obj, "", &val->y);
    hgSerialize(arena, &obj, "", &val->z);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgVec4* val)
{
    u32 len = 4;
    HgSerializer obj = hgSerializeArray(arena, s, name, &len);
    hgSerialize(arena, &obj, "", &val->x);
    hgSerialize(arena, &obj, "", &val->y);
    hgSerialize(arena, &obj, "", &val->z);
    hgSerialize(arena, &obj, "", &val->w);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgMat2* val)
{
    u32 len = 2;
    HgSerializer obj = hgSerializeArray(arena, s, name, &len);
    hgSerialize(arena, &obj, "", &val->x);
    hgSerialize(arena, &obj, "", &val->y);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgMat3* val)
{
    u32 len = 3;
    HgSerializer obj = hgSerializeArray(arena, s, name, &len);
    hgSerialize(arena, &obj, "", &val->x);
    hgSerialize(arena, &obj, "", &val->y);
    hgSerialize(arena, &obj, "", &val->z);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgMat4* val)
{
    u32 len = 4;
    HgSerializer obj = hgSerializeArray(arena, s, name, &len);
    hgSerialize(arena, &obj, "", &val->x);
    hgSerialize(arena, &obj, "", &val->y);
    hgSerialize(arena, &obj, "", &val->z);
    hgSerialize(arena, &obj, "", &val->w);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgComplex* val)
{
    u32 len = 2;
    HgSerializer obj = hgSerializeArray(arena, s, name, &len);
    hgSerialize(arena, &obj, "", &val->r);
    hgSerialize(arena, &obj, "", &val->i);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgQuat* val)
{
    u32 len = 4;
    HgSerializer obj = hgSerializeArray(arena, s, name, &len);
    hgSerialize(arena, &obj, "", &val->r);
    hgSerialize(arena, &obj, "", &val->i);
    hgSerialize(arena, &obj, "", &val->j);
    hgSerialize(arena, &obj, "", &val->k);
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

struct SerialBinArray {
    u32 elemsBegin;
    u32 elemCount;
};

struct SerialBinElem {
    u32 elemBegin;
};

struct SerialBinObject {
    u32 fieldsBegin;
    u32 fieldCount;
};

struct SerialBinField {
    u32 nameBegin;
    u32 nameLength;
    u32 dataBegin;
};

struct SerialBinString {
    u32 begin;
    u32 length;
};

struct SerialBinNode {
    HgSerialType type;
    union {
        SerialBinArray array;
        SerialBinObject object;
        SerialBinString string;
        i64 integer;
        f64 floating;
        bool boolean;
    };
};

static void serialBinWriteNode(HgArena* arena, HgBinary* bin, HgSerialNode* node);

static void serialBinWriteNull(HgArena* arena, HgBinary* bin)
{
    SerialBinNode node{};

    u32 idx = bin->size;
    hgBinaryResize(arena, bin, bin->size + sizeof(SerialBinNode));

    node.type = HgSerialType_null;
    hgBinaryOverwrite(bin, idx, node);
}

static void serialBinWriteArray(HgArena* arena, HgBinary* bin, HgSerialArray array)
{
    SerialBinNode node{};

    u32 idx = bin->size;
    hgBinaryResize(arena, bin, bin->size + sizeof(SerialBinNode));

    node.type = HgSerialType_array;
    node.array.elemsBegin = bin->size;
    node.array.elemCount = array.elemCount;
    hgBinaryOverwrite(bin, idx, node);

    hgBinaryResize(arena, bin, bin->size + array.elemCount * sizeof(SerialBinElem));
    for (u32 i = 0; i < array.elemCount; ++i)
    {
        SerialBinElem elem{};
        elem.elemBegin = bin->size;
        serialBinWriteNode(arena, bin, &array.elems[i]);
        hgBinaryOverwrite(bin, node.array.elemsBegin + i * sizeof(elem), elem);
    }
}

static void serialBinWriteObject(HgArena* arena, HgBinary* bin, HgSerialObject object)
{
    SerialBinNode node{};

    u32 idx = bin->size;
    hgBinaryResize(arena, bin, bin->size + sizeof(SerialBinNode));

    node.type = HgSerialType_object;
    node.object.fieldsBegin = bin->size;
    node.object.fieldCount = object.fieldCount;
    hgBinaryOverwrite(bin, idx, node);

    hgBinaryResize(arena, bin, bin->size + object.fieldCount * sizeof(SerialBinField));
    for (u32 i = 0; i < object.fieldCount; ++i)
    {
        SerialBinField field{};

        field.nameBegin = bin->size;
        field.nameLength = object.fields[i].name.length;
        hgBinaryResize(arena, bin, bin->size + field.nameLength);
        hgBinaryOverwrite(bin, field.nameBegin, object.fields[i].name.chars, field.nameLength);

        field.dataBegin = bin->size;
        serialBinWriteNode(arena, bin, &object.fields[i].data);

        hgBinaryOverwrite(bin, node.object.fieldsBegin + i * sizeof(field), field);
    }
}

static void serialBinWriteString(HgArena* arena, HgBinary* bin, HgStringView string)
{
    SerialBinNode node{};

    u32 idx = bin->size;
    hgBinaryResize(arena, bin, bin->size + sizeof(SerialBinNode));

    node.type = HgSerialType_string;
    node.string.begin = bin->size;
    node.string.length = string.length;
    hgBinaryOverwrite(bin, idx, node);

    hgBinaryResize(arena, bin, bin->size + string.length);
    hgBinaryOverwrite(bin, node.string.begin, string.chars, string.length);
}

static void serialBinWriteInteger(HgArena* arena, HgBinary* bin, i64 integer)
{
    SerialBinNode node{};

    u32 idx = bin->size;
    hgBinaryResize(arena, bin, bin->size + sizeof(SerialBinNode));

    node.type = HgSerialType_integer;
    node.integer = integer;
    hgBinaryOverwrite(bin, idx, node);
}

static void serialBinWriteFloating(HgArena* arena, HgBinary* bin, f64 floating)
{
    SerialBinNode node{};

    u32 idx = bin->size;
    hgBinaryResize(arena, bin, bin->size + sizeof(SerialBinNode));

    node.type = HgSerialType_floating;
    node.floating = floating;
    hgBinaryOverwrite(bin, idx, node);
}

static void serialBinWriteBoolean(HgArena* arena, HgBinary* bin, bool boolean)
{
    SerialBinNode node{};

    u32 idx = bin->size;
    hgBinaryResize(arena, bin, bin->size + sizeof(SerialBinNode));

    node.type = HgSerialType_boolean;
    node.boolean = boolean;
    hgBinaryOverwrite(bin, idx, node);
}

static void serialBinWriteNode(HgArena* arena, HgBinary* bin, HgSerialNode* node)
{
    switch (node->type)
    {
        case HgSerialType_null:
            serialBinWriteNull(arena, bin);
            return;
        case HgSerialType_array:
            serialBinWriteArray(arena, bin, node->array);
            return;
        case HgSerialType_object:
            serialBinWriteObject(arena, bin, node->object);
            return;
        case HgSerialType_string:
            serialBinWriteString(arena, bin, node->string);
            return;
        case HgSerialType_integer:
            serialBinWriteInteger(arena, bin, node->integer);
            return;
        case HgSerialType_floating:
            serialBinWriteFloating(arena, bin, node->floating);
            return;
        case HgSerialType_boolean:
            serialBinWriteBoolean(arena, bin, node->boolean);
            return;
        default:
            hgError("Invalid HgSerialType: %s\n", hgSerialTypeToString(node->type));
    }
}

HgBinary hgBinaryWriteSerial(HgArena* arena, HgSerializer serial)
{
    HgBinary bin{};

    SerialBinHeader header{};
    hgBinaryResize(arena, &bin, bin.size + sizeof(header));

    hgMemCopy(header.tag, serialBinTag, sizeof(serialBinTag));
    header.versionMajor = serialBinVersionMajor;
    header.versionMinor = serialBinVersionMinor;
    header.versionPatch = serialBinVersionPatch;
    header.nodeBegin = bin.size;
    hgBinaryOverwrite(&bin, 0, header);

    serialBinWriteNode(arena, &bin, serial.current);

    return bin;
}

static void serialBinReadNode(HgArena* arena, HgSerializer* s, HgStringView name, HgBinary bin, SerialBinNode node);

static void serialBinReadArray(HgArena* arena, HgSerializer* s, HgStringView name, HgBinary bin, SerialBinArray array)
{
    HgSerializer arr = hgSerializeArray(arena, s, name, &array.elemCount);
    for (u32 i = 0; i < array.elemCount; ++i)
    {
        SerialBinElem elem = hgBinaryRead<SerialBinElem>(&bin, array.elemsBegin + i * sizeof(elem));
        serialBinReadNode(arena, &arr, "", bin, hgBinaryRead<SerialBinNode>(&bin, elem.elemBegin));
    }
}

static void serialBinReadObject(
    HgArena* arena,
    HgSerializer* s,
    HgStringView name,
    HgBinary bin,
    SerialBinObject object)
{
    HgSerializer obj = hgSerializeObject(arena, s, name);
    for (u32 i = 0; i < object.fieldCount; ++i)
    {
        SerialBinField field = hgBinaryRead<SerialBinField>(&bin, object.fieldsBegin + i * sizeof(field));
        HgStringView fieldName = {(char*)bin.data + field.nameBegin, field.nameLength};
        serialBinReadNode(arena, &obj, fieldName, bin, hgBinaryRead<SerialBinNode>(&bin, field.dataBegin));
    }
}

static void serialBinReadString(
    HgArena* arena,
    HgSerializer* s,
    HgStringView name,
    HgBinary bin,
    SerialBinString string)
{
    HgStringView val = {(char*)bin.data + string.begin, string.length};
    hgSerialize(arena, s, name, &val);
}

static void serialBinReadNode(HgArena* arena, HgSerializer* s, HgStringView name, HgBinary bin, SerialBinNode node)
{
    switch (node.type)
    {
        case HgSerialType_null:
            hgSerializeNull(arena, s, name);
            return;
        case HgSerialType_array:
            serialBinReadArray(arena, s, name, bin, node.array);
            return;
        case HgSerialType_object:
            serialBinReadObject(arena, s, name, bin, node.object);
            return;
        case HgSerialType_string:
            serialBinReadString(arena, s, name, bin, node.string);
            return;
        case HgSerialType_integer:
            hgSerialize(arena, s, name, &node.integer);
            return;
        case HgSerialType_floating:
            hgSerialize(arena, s, name, &node.floating);
            return;
        case HgSerialType_boolean:
            hgSerialize(arena, s, name, &node.boolean);
            return;
        default:
            hgError("Invalid HgSerialType: %s\n", hgSerialTypeToString(node.type));
    }
}

HgSerializer hgBinaryReadSerial(HgArena* arena, HgBinary bin)
{
    SerialBinHeader header = hgBinaryRead<SerialBinHeader>(&bin, 0);

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

    SerialBinObject object = hgBinaryRead<SerialBinNode>(&bin, header.nodeBegin).object;
    for (u32 i = 0; i < object.fieldCount; ++i)
    {
        SerialBinField field = hgBinaryRead<SerialBinField>(&bin, object.fieldsBegin + i * sizeof(field));
        HgStringView fieldName = {(char*)bin.data + field.nameBegin, field.nameLength};
        serialBinReadNode(arena, &s, fieldName, bin, hgBinaryRead<SerialBinNode>(&bin, field.dataBegin));
    }

    return hgSerialReader(s.current);
}

void serialJsonWriteNode(HgArena* arena, HgStringBuilder* str, u32 indentation, HgStringView name, HgSerialNode* node);

void serialJsonWriteNull(HgArena* arena, HgStringBuilder* str, u32 indentation, HgStringView name)
{
    for (u32 i = 0; i < indentation; ++i)
    {
        hgStringAppend(arena, str, "    ");
    }
    if (name != "")
    {
        hgStringAppendC(arena, str, '"');
        hgStringAppend(arena, str, name);
        hgStringAppend(arena, str, "\" : ");
    }
    hgStringAppend(arena, str, "null");
}

void serialJsonWriteArray(
    HgArena* arena,
    HgStringBuilder* str,
    u32 indentation,
    HgStringView name,
    HgSerialArray array)
{
    for (u32 i = 0; i < indentation; ++i)
    {
        hgStringAppend(arena, str, "    ");
    }
    if (name != "")
    {
        hgStringAppendC(arena, str, '"');
        hgStringAppend(arena, str, name);
        hgStringAppend(arena, str, "\" : ");
    }
    if (array.elemCount > 0)
    {
        hgStringAppend(arena, str, "[\n");
        serialJsonWriteNode(arena, str, indentation + 1, "", &array.elems[0]);
        for (u32 i = 1; i < array.elemCount; ++i)
        {
            hgStringAppend(arena, str, ",\n");
            serialJsonWriteNode(arena, str, indentation + 1, "", &array.elems[i]);
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

void serialJsonWriteObject(
    HgArena* arena,
    HgStringBuilder* str,
    u32 indentation,
    HgStringView name,
    HgSerialObject object)
{
    for (u32 i = 0; i < indentation; ++i)
    {
        hgStringAppend(arena, str, "    ");
    }
    if (name != "")
    {
        hgStringAppendC(arena, str, '"');
        hgStringAppend(arena, str, name);
        hgStringAppend(arena, str, "\" : ");
    }
    if (object.fieldCount > 0)
    {
        hgStringAppend(arena, str, "{\n");
        serialJsonWriteNode(arena, str, indentation + 1, object.fields[0].name, &object.fields[0].data);
        for (u32 i = 1; i < object.fieldCount; ++i)
        {
            hgStringAppend(arena, str, ",\n");
            serialJsonWriteNode(arena, str, indentation + 1, object.fields[i].name, &object.fields[i].data);
        }
        hgStringAppendC(arena, str, '\n');
        for (u32 i = 0; i < indentation; ++i)
        {
            hgStringAppend(arena, str, "    ");
        }
        hgStringAppendC(arena, str, '}');
    }
    else
    {
        hgStringAppend(arena, str, "{}");
    }
}

void serialJsonWriteString(
    HgArena* arena,
    HgStringBuilder* str,
    u32 indentation,
    HgStringView name,
    HgStringView string)
{
    for (u32 i = 0; i < indentation; ++i)
    {
        hgStringAppend(arena, str, "    ");
    }
    if (name != "")
    {
        hgStringAppendC(arena, str, '"');
        hgStringAppend(arena, str, name);
        hgStringAppend(arena, str, "\" : ");
    }
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

void serialJsonWriteInteger(
    HgArena* arena,
    HgStringBuilder* str,
    u32 indentation,
    HgStringView name,
    i64 integer)
{
    for (u32 i = 0; i < indentation; ++i)
    {
        hgStringAppend(arena, str, "    ");
    }
    if (name != "")
    {
        hgStringAppendC(arena, str, '"');
        hgStringAppend(arena, str, name);
        hgStringAppend(arena, str, "\" : ");
    }
    hgStringAppend(arena, str, hgIntegerToString(hgScratch(), integer));
}

void serialJsonWriteFloating(
    HgArena* arena,
    HgStringBuilder* str,
    u32 indentation,
    HgStringView name,
    f64 floating)
{
    for (u32 i = 0; i < indentation; ++i)
    {
        hgStringAppend(arena, str, "    ");
    }
    if (name != "")
    {
        hgStringAppendC(arena, str, '"');
        hgStringAppend(arena, str, name);
        hgStringAppend(arena, str, "\" : ");
    }
    hgStringAppend(arena, str, hgFloatToString(hgScratch(), floating, 6));
}

void serialJsonWriteBoolean(
    HgArena* arena,
    HgStringBuilder* str,
    u32 indentation,
    HgStringView name,
    bool boolean)
{
    for (u32 i = 0; i < indentation; ++i)
    {
        hgStringAppend(arena, str, "    ");
    }
    if (name != "")
    {
        hgStringAppendC(arena, str, '"');
        hgStringAppend(arena, str, name);
        hgStringAppend(arena, str, "\" : ");
    }
    if (boolean)
        hgStringAppend(arena, str, "true");
    else
        hgStringAppend(arena, str, "false");
}

void serialJsonWriteNode(HgArena* arena, HgStringBuilder* str, u32 indentation, HgStringView name, HgSerialNode* node)
{
    switch (node->type)
    {
        case HgSerialType_null:
            serialJsonWriteNull(arena, str, indentation, name);
            return;
        case HgSerialType_array:
            serialJsonWriteArray(arena, str, indentation, name, node->array);
            return;
        case HgSerialType_object:
            serialJsonWriteObject(arena, str, indentation, name, node->object);
            return;
        case HgSerialType_string:
            serialJsonWriteString(arena, str, indentation, name, node->string);
            return;
        case HgSerialType_integer:
            serialJsonWriteInteger(arena, str, indentation, name, node->integer);
            return;
        case HgSerialType_floating:
            serialJsonWriteFloating(arena, str, indentation, name, node->floating);
            return;
        case HgSerialType_boolean:
            serialJsonWriteBoolean(arena, str, indentation, name, node->boolean);
            return;
        default:
            hgError("Invalid HgSerialType: %s\n", hgSerialTypeToString(node->type));
    }
}

HgStringView hgJsonWriteSerial(HgArena* arena, HgSerializer serial)
{
    HgStringBuilder str{};
    serialJsonWriteNode(arena, &str, 0, "", serial.current);
    hgStringAppendC(arena, &str, '\n');
    return str;
}

HgSerializer hgJsonReadSerial(HgArena* arena, HgStringView json)
{
    (void)arena;
    (void)json;
    return {};
}

struct JsonParseState {
    HgStringView text;
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
    HgStringView num{&state->text[begin], &state->text[state->head]};
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
    if (state->head + 4 < state->text.length && HgStringView{&state->text[state->head], 4} == "true")
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
    if (state->head + 5 < state->text.length && HgStringView{&state->text[state->head], 5} == "false")
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

HgJson hgParseJson(HgArena* arena, HgStringView text)
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
    hgAssetInit<HgTexture>();
    hgAssetInit<HgGpuTexture>();
    hgAssetInit<HgMesh>();
    hgAssetInit<HgGpuMesh>();
    hgAssetInit<HgAudio>();
}

void hgAssetDeinitDefaults()
{
    hgAssetDeinit<HgAudio>();
    hgAssetDeinit<HgGpuMesh>();
    hgAssetDeinit<HgMesh>();
    hgAssetDeinit<HgGpuTexture>();
    hgAssetDeinit<HgTexture>();
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
    hgWarn("Could not find file to read binary: %s\n", cpath);
    return;
    }
    hgDefer(fclose(fileHandle));

    if (fseek(fileHandle, 0, SEEK_END) != 0)
    {
        hgWarn("Failed to read binary from file: %s\n", cpath);
        return;
    }

    data->data.size = (u32)ftell(fileHandle);
    data->data.data = malloc(data->data.size);

    rewind(fileHandle);
    if (fread(data->data.data, 1, data->data.size, fileHandle) != data->data.size)
    {
        free(data->data.data);
        data->data = {};
        hgWarn("Failed to read binary from file: %s\n", cpath);
        return;
    }
}

template<>
void hgAssetUnloadImpl(HgAsset<HgBinary>* data)
{
    free(data->data.data);
}

void hgBinaryStore(HgBinary* bin, HgStringView path, HgFence* fence)
{
    struct Capture {
        HgBinary bin;
        HgStringOwner path;
    };
    Capture* c = (Capture*)malloc(sizeof(*c));
    c->bin = *bin;
    c->path = hgStringCreate(path);

    hgThreadsCall(fence, c, [](void* pc)
    {
        Capture* c = (Capture*)pc;
        hgDefer(free(c));
        hgDefer(hgStringDestroy(&c->path));

        HgArena* scratch = hgScratch();
        hgArenaScope(scratch);

        char* cpath = hgCString(scratch, c->path);

        FILE* fileHandle = fopen(cpath, "wb");
        if (fileHandle == nullptr)
        {
            hgWarn("Failed to create file to write binary: %s\n", cpath);
            return;
        }
        hgDefer(fclose(fileHandle));

        if (fwrite(c->bin.data, 1, c->bin.size, fileHandle) != c->bin.size)
        {
            hgWarn("Failed to write binary data to file: %s\n", cpath);
        }
    });
}

template<>
void hgAssetLoadImpl(HgAsset<HgJson>* data)
{
    HgBinaryAsset* bin = hgAssetLoad<HgBinary>(data->path);
    hgDefer(hgAssetUnload(bin));

    HgArena* scratch = hgScratch();
    u64 head = scratch->head;
    hgDefer(scratch->head = head);

    HgStringView jsonStr = {(char*)bin->data.data, bin->data.size};
    HgJson parse = hgParseJson(scratch, jsonStr);

    HgJsonError* e = parse.errors;
    while (e != nullptr)
    {
        hgWarn("Json parse error: %.*s\n", (int)e->msg.length, e->msg.chars);
        e = e->next;
    }

    data->data.file = (HgJsonNode*)malloc(scratch->head - head);
    if (parse.errors != nullptr)
    {
        data->data.errors = (HgJsonError*)(
            (u8*)data->data.file +
                ((uptr)parse.errors - (uptr)parse.file));
    }
    else
    {
        data->data.errors = nullptr;
    }
    hgMemCopy((void*)data->data.file, (void*)parse.file, scratch->head - head);
}

template<>
void hgAssetUnloadImpl(HgAsset<HgJson>* data)
{
    free(data->data.file);
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

void* hgArrayAnyPush(HgArrayAny* arr)
{
    if (arr->count == arr->capacity)
    {
        u32 newCapacity = arr->capacity == 0 ? 16 : arr->capacity * 2;
        hgGpaRealloc(
            arr->vals,
            arr->capacity * arr->width,
            (arr->capacity == 0 ? 16 : arr->capacity * 2) * arr->width,
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
        hgArenaRealloc(
            arena,
            arr->vals,
            arr->capacity * arr->width,
            (arr->capacity == 0 ? 16 : arr->capacity * 2) * arr->width,
            arr->align);
        arr->capacity = newCapacity;
    }
    return (u8*)arr->vals + arr->count++ * arr->width;
}

void hgArrayAnyPop(HgArrayAny* arr, void* dst)
{
    hgAssert(arr->count > 0);
    --arr->count;
    if (dst != nullptr)
        hgMemCopy(dst, (u8*)arr->vals + arr->count * arr->width, arr->width);
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

const HgVec2& HgVec2::operator+=(HgVec2 other)
{
    x += other.x;
    y += other.y;
    return* this;
}

const HgVec2& HgVec2::operator-=(HgVec2 other)
{
    x -= other.x;
    y -= other.y;
    return* this;
}

const HgVec2& HgVec2::operator*=(HgVec2 other)
{
    x *= other.x;
    y *= other.y;
    return* this;
}

const HgVec2& HgVec2::operator/=(HgVec2 other)
{
    x /= other.x;
    y /= other.y;
    return* this;
}

const HgVec3& HgVec3::operator+=(HgVec3 other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return* this;
}

const HgVec3& HgVec3::operator-=(HgVec3 other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return* this;
}

const HgVec3& HgVec3::operator*=(HgVec3 other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return* this;
}

const HgVec3& HgVec3::operator/=(HgVec3 other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return* this;
}

const HgVec4& HgVec4::operator+=(HgVec4 other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return* this;
}

const HgVec4& HgVec4::operator-=(HgVec4 other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return* this;
}

const HgVec4& HgVec4::operator*=(HgVec4 other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
    return* this;
}

const HgVec4& HgVec4::operator/=(HgVec4 other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return* this;
}

const HgMat2& HgMat2::operator+=(const HgMat2& other)
{
    x += other.x;
    y += other.y;
    return* this;
}

const HgMat2& HgMat2::operator-=(const HgMat2& other)
{
    x -= other.x;
    y -= other.y;
    return* this;
}

const HgMat3& HgMat3::operator+=(const HgMat3& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return* this;
}

const HgMat3& HgMat3::operator-=(const HgMat3& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return* this;
}

const HgMat4& HgMat4::operator+=(const HgMat4& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return* this;
}

const HgMat4& HgMat4::operator-=(const HgMat4& other)
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

const HgComplex& HgComplex::operator+=(HgComplex other)
{
    r += other.r;
    i += other.i;
    return* this;
}

const HgComplex& HgComplex::operator-=(HgComplex other)
{
    r -= other.r;
    i -= other.i;
    return* this;
}

const HgQuat& HgQuat::operator+=(HgQuat other)
{
    r += other.r;
    i += other.i;
    j += other.j;
    k += other.k;
    return* this;
}

const HgQuat& HgQuat::operator-=(HgQuat other)
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

void hgVecLen(u32 size, f32* dst, const f32* vec)
{
    hgAssert(dst != nullptr);
    hgAssert(vec != nullptr);
    hgVecDot(size, dst, vec, vec);
    *dst = (f32)sqrt(*dst);
}

f32 hgVecLen2(HgVec2 vec)
{
    return (f32)sqrt(hgVecDot2(vec, vec));
}

f32 hgVecLen3(HgVec3 vec)
{
    return (f32)sqrt(hgVecDot3(vec, vec));
}

f32 hgVecLen4(HgVec4 vec)
{
    return (f32)sqrt(hgVecDot3(vec, vec));
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
    return HgVec2{vec.x / len, vec.y / len};
}

HgVec3 hgVecNorm3(HgVec3 vec)
{
    f32 len = hgVecLen3(vec);
    hgAssert(len != 0);
    return HgVec3{vec.x / len, vec.y / len, vec.z / len};
}

HgVec4 hgVecNorm4(HgVec4 vec)
{
    f32 len = hgVecLen4(vec);
    hgAssert(len != 0);
    return HgVec4{vec.x / len, vec.y / len, vec.z / len, vec.w / len};
}

void hgVecCross(f32* dst, const f32* lhs, const f32* rhs)
{
    hgAssert(dst != nullptr);
    hgAssert(lhs != nullptr);
    hgAssert(rhs != nullptr);
    dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
    dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
    dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
}

HgVec3 hgCross(const HgVec3& lhs, const HgVec3& rhs)
{
    return HgVec3{
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

HgQuat operator*(HgQuat lhs, HgQuat rhs)
{
    return HgQuat{
        lhs.r * rhs.r - lhs.i * rhs.i - lhs.j * rhs.j - lhs.k * rhs.k,
        lhs.r * rhs.i + lhs.i * rhs.r + lhs.j * rhs.k - lhs.k * rhs.j,
        lhs.r * rhs.j - lhs.i * rhs.k + lhs.j * rhs.r + lhs.k * rhs.i,
        lhs.r * rhs.k + lhs.i * rhs.j - lhs.j * rhs.i + lhs.k * rhs.r,
    };
}

HgQuat hgQuatAxisAngle(HgVec3 axis, f32 angle)
{
    f32 halfAngle = angle * (f32)0.5;
    f32 sinHalfAngle = (f32)std::sin(halfAngle);
    return HgQuat{
        (f32)std::cos(halfAngle),
        axis.x * sinHalfAngle,
        axis.y * sinHalfAngle,
        axis.z * sinHalfAngle,
    };
}

HgVec3 hgVecRotate(HgQuat lhs, HgVec3 rhs)
{
    HgQuat q = lhs * HgQuat{0, rhs.x, rhs.y, rhs.z} * hgQuatConj(lhs);
    return HgVec3{q.i, q.j, q.k};
}

HgMat3 hgMatRotate(HgQuat lhs, HgMat3 rhs)
{
    return HgMat3{
        hgVecRotate(lhs, rhs.x),
        hgVecRotate(lhs, rhs.y),
        hgVecRotate(lhs, rhs.z),
    };
}

HgMat4 hgMatModel2D(HgVec3 position, HgVec2 scale, f32 rotation)
{
    HgMat2 m2{HgVec2{scale.x, 0.0f}, HgVec2{0.0f, scale.y}};
    f32 rotSin = (f32)std::sin(rotation);
    f32 rotCos = (f32)std::cos(rotation);
    HgMat2 rot{HgVec2{rotCos, rotSin}, HgVec2{-rotSin, rotCos}};
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
    m3 = hgMatRotate(rotation, m3);
    HgMat4 m4 = HgMat4{m3};
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

HgMat4 hgMatView(const HgVec3& position, const HgVec3& zoom, const HgQuat& rotation)
{
    HgMat4 rot{hgMatRotate(hgQuatConj(rotation), HgMat3{1.0f})};
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
    inv4.w = HgVec4{HgVec3{inv3 * HgVec3{model.w} * -1}, 1};
    return inv4;
}

HgMat4 hgMatOrthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far)
{
    return HgMat4{
        HgVec4{2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        HgVec4{0.0f, 2.0f / (bottom - top), 0.0f, 0.0f},
        HgVec4{0.0f, 0.0f, 1.0f / (far - near), 0.0f},
        HgVec4{-(right + left) / (right - left), -(bottom + top) / (bottom - top), -(near) / (far - near), 1.0f},
    };
}

HgMat4 hgMatPerspective(f32 fov, f32 aspect, f32 near, f32 far)
{
    hgAssert(near > 0.0f);
    hgAssert(far > near);
    f32 scale = 1.0f / (f32)tan(fov * 0.5f);
    return HgMat4{
        HgVec4{scale / aspect, 0.0f, 0.0f, 0.0f},
        HgVec4{0.0f, scale, 0.0f, 0.0f},
        HgVec4{0.0f, 0.0f, far / (far - near), 1.0f},
        HgVec4{0.0f, 0.0f, -(far * near) / (far - near), 0.0f},
    };
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
    return hgNoise(seed, x + y * 425537443u + z * 682607u + w * 9067);
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
    return HgVec2(std::cos(rot), std::sin(rot));
}

u32 hgGetMaxMipmaps(u32 width, u32 height, u32 depth)
{
    u32 max = width > height ? width : height;
    max = max > depth ? max : depth;
    return max == 0 ? 0 : (u32)log2((f32)max) + 1;
}

template<>
void hgAssetLoadImpl(HgAsset<HgTexture>* data)
{
    HgArena* scratch = hgScratch();
    hgArenaScope(scratch);
    char* cpath = hgCString(scratch, data->path);

    int x, y, channels;
    data->data.pixels = stbi_load(cpath, &x, &y, &channels, 4);
    if (data->data.pixels == nullptr)
    {
        hgWarn("Could not load image: %s\n", cpath);
        return;
    }
    data->data.width = (u32)x;
    data->data.height = (u32)y;
    data->data.depth = 1;
    data->data.format = HgFormat_r8g8b8a8_srgb;
}

template<>
void hgAssetUnloadImpl(HgAsset<HgTexture>* data)
{
    free(data->data.pixels);
}

void hgTextureStorePng(HgTexture* texture, HgStringView path, HgFence* fence)
{
    struct Capture {
        HgTexture tex;
        HgStringOwner path;
    };
    Capture* c = (Capture*)malloc(sizeof(*c));
    c->tex = *texture;
    c->path = hgStringCreate(path);

    hgThreadsCall(fence, c, [](void* pc)
    {
        Capture* c = (Capture*)pc;
        hgDefer(free(c));
        hgDefer(hgStringDestroy(&c->path));

        HgArena* scratch = hgScratch();
        hgArenaScope(scratch);

        char* cpath = hgCString(scratch, c->path);

        stbi_write_png(
            cpath,
            (int)c->tex.width,
            (int)c->tex.height,
            4,
            c->tex.pixels,
            (int)(c->tex.width * sizeof(u32)));
    });
}

template<>
void hgAssetLoadImpl(HgAsset<HgGpuTexture>* data)
{
    HgTextureAsset* tex = hgAssetLoad<HgTexture>(data->path);
    hgDefer(hgAssetUnload(tex));

    if (tex->data.pixels == nullptr)
    {
        hgWarn("Could not load image: %.*s\n", (int)data->path.length, data->path.chars);
    }

    HgGpuImageCreateEx imageInfo{};
    imageInfo.format = tex->data.format;
    imageInfo.width = tex->data.width;
    imageInfo.height = tex->data.height;
    imageInfo.depth = tex->data.depth;
    imageInfo.usage = HgGpuImageUsage_transferDst | HgGpuImageUsage_sampled;

    data->data.image = hgGpuImageCreateEx(&imageInfo);
    data->data.view = hgGpuViewCreate(data->data.image, HgGpuAspect_color, HgGpuFilter_nearest);

    hgGpuImageWrite(data->data.view, tex->data.pixels);
}

template<>
void hgAssetUnloadImpl(HgAsset<HgGpuTexture>* data)
{
    hgGpuViewDestroy(data->data.view);
    hgGpuImageDestroy(data->data.image);
}

template<>
void hgAssetLoadImpl(HgAsset<HgMesh>* data)
{
    (void)data;
    hgError("load gltf file : TODO\n");
}

template<>
void hgAssetUnloadImpl(HgAsset<HgMesh>* data)
{
    free(data->data.indices);
    free(data->data.vertices);
}

void hgMeshStoreGltf(HgMesh* data, HgStringView path, HgFence* fence)
{
    (void)data;
    (void)path;
    (void)fence;
    hgError("store gltf file : TODO\n");
}

template<>
void hgAssetLoadImpl(HgAsset<HgGpuMesh>* data)
{
    HgMeshAsset* mesh = hgAssetLoad<HgMesh>(data->path);
    hgDefer(hgAssetUnload(mesh));

    if (mesh->data.vertices == nullptr || mesh->data.indices == nullptr)
    {
        hgWarn("Could not load model: %.*s\n", (int)data->path.length, data->path.chars);
        return;
    }

    data->data.vertexCount = mesh->data.vertexCount;
    data->data.vertexWidth = mesh->data.vertexWidth;
    data->data.indexCount = mesh->data.indexCount;

    data->data.vertexBuffer = hgGpuBufferCreate(
        data->data.vertexCount * data->data.vertexWidth,
        HgGpuBufferUsage_storageBuffer | HgGpuBufferUsage_transferDst);

    data->data.indexBuffer = hgGpuBufferCreate(
        data->data.indexCount * sizeof(u32),
        HgGpuBufferUsage_storageBuffer | HgGpuBufferUsage_transferDst);

    hgGpuBufferWrite(
        data->data.vertexBuffer,
        0,
        mesh->data.vertices,
        data->data.vertexCount * data->data.vertexWidth);

    hgGpuBufferWrite(
        data->data.indexBuffer,
        0,
        mesh->data.indices,
        data->data.indexCount * sizeof(u32));
}

template<>
void hgAssetUnloadImpl(HgAsset<HgGpuMesh>* data)
{
    hgGpuBufferDestroy(data->data.vertexBuffer);
    hgGpuBufferDestroy(data->data.indexBuffer);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgCameraPerspective* camera)
{
    HgSerializer obj = hgSerializeObject(arena, s, name);
    hgSerialize(arena, &obj, "Field of View", &camera->fov);
    hgSerialize(arena, &obj, "Aspect Ratio", &camera->aspect);
    hgSerialize(arena, &obj, "Near Plane", &camera->near);
    hgSerialize(arena, &obj, "Far Plane", &camera->far);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgCameraOrthographic* camera)
{
    HgSerializer obj = hgSerializeObject(arena, s, name);
    hgSerialize(arena, &obj, "Left", &camera->left);
    hgSerialize(arena, &obj, "Right", &camera->right);
    hgSerialize(arena, &obj, "Top", &camera->top);
    hgSerialize(arena, &obj, "Bottom", &camera->bottom);
    hgSerialize(arena, &obj, "Near", &camera->near);
    hgSerialize(arena, &obj, "Far", &camera->far);
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgCamera* camera)
{
    HgSerializer obj = hgSerializeObject(arena, s, name);
    hgSerialize(arena, &obj, "Type", &camera->type);
    hgSerialize(arena, &obj, "Orthographic", &camera->orthographic);
    hgSerialize(arena, &obj, "Perspective", &camera->perspective);
}

struct VPUniform {
    HgMat4 proj;
    HgMat4 view;
};

HgCamera* hgCameraAdd(HgEcs* ecs, HgEntity e)
{
    HgCamera* camera = hgEcsAdd<HgCamera>(ecs, e);
    *camera = {};

    camera->vpBuffer = hgGpuBufferCreate(
        sizeof(VPUniform),
        HgGpuBufferUsage_uniformBuffer,
        HgGpuMemoryUsage_frequentUpdate);

    return camera;
}

template<>
void hgEcsDtor(HgCamera* camera)
{
    hgGpuBufferDestroy(camera->vpBuffer);
}

void hgCameraUpdate(HgEcs* ecs, HgEntity e)
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

struct SpritePipelinePush {
    HgMat4 model;
    HgVec2 uvPos;
    HgVec2 uvSize;
    u32 viewProj;
    u32 texture;
};

struct SpritePipelineState {
    HgGpuPipeline* pipeline;
    HgGpuTexture defaultTex;
};

static SpritePipelineState spritePipeline{};

void hgSpritesInit(
    HgFormat colorFormat,
    HgFormat depthFormat)
{
    hgAssert(colorFormat != HgFormat_undefined);
    hgAssert(depthFormat != HgFormat_undefined);

    HgBinaryAsset* spriteVertSpv = hgAssetLoad<HgBinary>("build/sprite.vert.spv");
    HgBinaryAsset* spriteFragSpv = hgAssetLoad<HgBinary>("build/sprite.frag.spv");
    hgDefer(hgAssetUnload(spriteVertSpv));
    hgDefer(hgAssetUnload(spriteFragSpv));

    HgCreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = spriteVertSpv->data.data;
    pipelineConfig.vertexShaderSize = spriteVertSpv->data.size;
    pipelineConfig.fragmentShader = spriteFragSpv->data.data;
    pipelineConfig.fragmentShaderSize = spriteFragSpv->data.size;
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    HgGpuPushRange push{0, sizeof(SpritePipelinePush)};
    pipelineConfig.pushRanges = &push;
    pipelineConfig.pushRangeCount = 1;
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
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgSprite* sprite)
{
    HgSerializer obj = hgSerializeObject(arena, s, name);
    hgSerialize(arena, &obj, "Texture", &sprite->texture);
    hgSerialize(arena, &obj, "UV Position", &sprite->uvPos);
    hgSerialize(arena, &obj, "UV Size", &sprite->uvSize);
}

HgSprite* hgSpriteAdd(HgEcs* ecs, HgEntity e, HgGpuTextureAsset* texture, HgVec2 uvPos, HgVec2 uvSize)
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
        HgGpuTexture* texture = sprite->texture == nullptr
            ? &spritePipeline.defaultTex
            : &sprite->texture->data;

        SpritePipelinePush push{};
        push.model = tf->mat;
        push.uvPos = sprite->uvPos;
        push.uvSize = sprite->uvSize;
        push.viewProj = hgGpuBufferUniformDescriptor(hgEcsGet<HgCamera>(ecs, camera)->vpBuffer);
        push.texture = hgGpuImageSamplerDescriptor(texture->view);

        hgGpuPushConstants(cmd, spritePipeline.pipeline, 0, &push, sizeof(push));

        hgGpuDraw(cmd, 0, 6, 0, 1);
    });
}

struct SkyboxPipelinePush {
    u32 viewProj;
    u32 texture;
};

struct SkyboxPipelineState {
    HgGpuPipeline* pipeline;
    HgGpuTexture defaultTex;
};

static SkyboxPipelineState skyboxPipeline{};

void hgSkyboxInit(HgFormat colorFormat, HgFormat depthFormat)
{
    hgAssert(colorFormat != HgFormat_undefined);

    HgBinaryAsset* vertSpv = hgAssetLoad<HgBinary>("build/skybox.vert.spv");
    HgBinaryAsset* fragSpv = hgAssetLoad<HgBinary>("build/skybox.frag.spv");
    hgDefer(hgAssetUnload(vertSpv));
    hgDefer(hgAssetUnload(fragSpv));

    HgCreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = vertSpv->data.data;
    pipelineConfig.vertexShaderSize = vertSpv->data.size;
    pipelineConfig.fragmentShader = fragSpv->data.data;
    pipelineConfig.fragmentShaderSize = fragSpv->data.size;
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    HgGpuPushRange push{0, sizeof(SkyboxPipelinePush)};
    pipelineConfig.pushRanges = &push;
    pipelineConfig.pushRangeCount = 1;
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
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgSkybox* skybox)
{
    HgSerializer obj = hgSerializeObject(arena, s, name);
    hgSerialize(arena, &obj, "Texture", &skybox->texture);
}

HgSkybox* hgSkyboxAdd(HgEcs* ecs, HgEntity e, HgGpuTextureAsset* texture)
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
        HgGpuTexture* texture = skybox->texture == nullptr
            ? &skyboxPipeline.defaultTex
            : &skybox->texture->data;

        SkyboxPipelinePush push{};
        push.viewProj = hgGpuBufferUniformDescriptor(hgEcsGet<HgCamera>(ecs, camera)->vpBuffer);
        push.texture = hgGpuImageSamplerDescriptor(texture->view);

        hgGpuPushConstants(cmd, skyboxPipeline.pipeline, 0, &push, sizeof(push));

        hgGpuDraw(cmd, 0, 36, 0, 1);
    });
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgDirLight* light)
{
    HgSerializer obj = hgSerializeObject(arena, s, name);
    hgSerialize(arena, &obj, "Direction", &light->dir);
    hgSerialize(arena, &obj, "Color", &light->color);
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
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgPointLight* light)
{
    HgSerializer obj = hgSerializeObject(arena, s, name);
    hgSerialize(arena, &obj, "Color", &light->color);
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

    HgGpuMesh defaultModel;
    HgGpuTexture defaultColorMap;
    HgGpuTexture defaultNormalMap;
};

static ModelPipelineState modelPipeline{};

void hgModelsInit(
    HgFormat colorFormat,
    HgFormat depthFormat)
{
    hgAssert(colorFormat != HgFormat_undefined);
    hgAssert(depthFormat != HgFormat_undefined);

    HgBinaryAsset* modelVertSpv = hgAssetLoad<HgBinary>("build/model.vert.spv");
    HgBinaryAsset* modelFragSpv = hgAssetLoad<HgBinary>("build/model.frag.spv");
    hgDefer(hgAssetUnload(modelVertSpv));
    hgDefer(hgAssetUnload(modelFragSpv));

    HgCreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = modelVertSpv->data.data;
    pipelineConfig.vertexShaderSize = modelVertSpv->data.size;
    pipelineConfig.fragmentShader = modelFragSpv->data.data;
    pipelineConfig.fragmentShaderSize = modelFragSpv->data.size;
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    HgGpuPushRange push{0, sizeof(ModelPipelinePush)};
    pipelineConfig.pushRanges = &push;
    pipelineConfig.pushRangeCount = 1;
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
        {HgVec3{ 0.5f,-0.5f,-0.5f}, HgVec3{ 1, 0, 0}, HgVec4{ 0, 0, 1, 1}, HgVec2{0,0}},
        {HgVec3{ 0.5f, 0.5f,-0.5f}, HgVec3{ 1, 0, 0}, HgVec4{ 0, 0, 1, 1}, HgVec2{1,0}},
        {HgVec3{ 0.5f, 0.5f, 0.5f}, HgVec3{ 1, 0, 0}, HgVec4{ 0, 0, 1, 1}, HgVec2{1,1}},
        {HgVec3{ 0.5f,-0.5f, 0.5f}, HgVec3{ 1, 0, 0}, HgVec4{ 0, 0, 1, 1}, HgVec2{0,1}},

        {HgVec3{-0.5f,-0.5f, 0.5f}, HgVec3{-1, 0, 0}, HgVec4{ 0, 0,-1, 1}, HgVec2{0,0}},
        {HgVec3{-0.5f, 0.5f, 0.5f}, HgVec3{-1, 0, 0}, HgVec4{ 0, 0,-1, 1}, HgVec2{1,0}},
        {HgVec3{-0.5f, 0.5f,-0.5f}, HgVec3{-1, 0, 0}, HgVec4{ 0, 0,-1, 1}, HgVec2{1,1}},
        {HgVec3{-0.5f,-0.5f,-0.5f}, HgVec3{-1, 0, 0}, HgVec4{ 0, 0,-1, 1}, HgVec2{0,1}},

        {HgVec3{-0.5f, 0.5f,-0.5f}, HgVec3{ 0, 1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,0}},
        {HgVec3{-0.5f, 0.5f, 0.5f}, HgVec3{ 0, 1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,0}},
        {HgVec3{ 0.5f, 0.5f, 0.5f}, HgVec3{ 0, 1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,1}},
        {HgVec3{ 0.5f, 0.5f,-0.5f}, HgVec3{ 0, 1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,1}},

        {HgVec3{-0.5f,-0.5f, 0.5f}, HgVec3{ 0,-1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,0}},
        {HgVec3{-0.5f,-0.5f,-0.5f}, HgVec3{ 0,-1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,0}},
        {HgVec3{ 0.5f,-0.5f,-0.5f}, HgVec3{ 0,-1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,1}},
        {HgVec3{ 0.5f,-0.5f, 0.5f}, HgVec3{ 0,-1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,1}},

        {HgVec3{-0.5f,-0.5f, 0.5f}, HgVec3{ 0, 0, 1}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,0}},
        {HgVec3{ 0.5f,-0.5f, 0.5f}, HgVec3{ 0, 0, 1}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,0}},
        {HgVec3{ 0.5f, 0.5f, 0.5f}, HgVec3{ 0, 0, 1}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,1}},
        {HgVec3{-0.5f, 0.5f, 0.5f}, HgVec3{ 0, 0, 1}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,1}},

        {HgVec3{ 0.5f,-0.5f,-0.5f}, HgVec3{ 0, 0,-1}, HgVec4{-1, 0, 0, 1}, HgVec2{0,0}},
        {HgVec3{-0.5f,-0.5f,-0.5f}, HgVec3{ 0, 0,-1}, HgVec4{-1, 0, 0, 1}, HgVec2{1,0}},
        {HgVec3{-0.5f, 0.5f,-0.5f}, HgVec3{ 0, 0,-1}, HgVec4{-1, 0, 0, 1}, HgVec2{1,1}},
        {HgVec3{ 0.5f, 0.5f,-0.5f}, HgVec3{ 0, 0,-1}, HgVec4{-1, 0, 0, 1}, HgVec2{0,1}},
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

    modelPipeline.defaultModel.vertexCount = hgArrayCount(cubeVertices);
    modelPipeline.defaultModel.vertexWidth = sizeof(HgMeshVertex);
    modelPipeline.defaultModel.indexCount = hgArrayCount(cubeIndices);

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

void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgModel* model)
{
    HgSerializer obj = hgSerializeObject(arena, s, name);
    hgSerialize(arena, &obj, "Mesh", &model->mesh);
    hgSerialize(arena, &obj, "Color Map", &model->colorMap);
    hgSerialize(arena, &obj, "Normal Map", &model->normalMap);
}

HgModel* hgModelAdd(
    HgEcs* ecs,
    HgEntity e,
    HgGpuMeshAsset* mesh,
    HgGpuTextureAsset* colorMap,
    HgGpuTextureAsset* normalMap)
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
        HgGpuTexture* colorMap = model->colorMap == nullptr
            ? &modelPipeline.defaultColorMap
            : &model->colorMap->data;

        HgGpuTexture* normalMap = model->normalMap == nullptr
            ? &modelPipeline.defaultNormalMap
            : &model->normalMap->data;

        HgGpuMesh* gpuModel = model->mesh == nullptr
            ? &modelPipeline.defaultModel
            : &model->mesh->data;

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

        hgGpuPushConstants(cmd, modelPipeline.pipeline, 0, &push, sizeof(push));

        hgGpuDraw(cmd, 0, gpuModel->indexCount, 0, 1);
    });
}

template<>
void hgAssetLoadImpl(HgAsset<HgAudio>* data)
{
    (void)data;
    hgError("Load audio file impl : TODO\n");
}

template<>
void hgAssetUnloadImpl(HgAsset<HgAudio>* data)
{
    (void)data;
    hgError("Unload audio file impl : TODO\n");
}

template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgAudioSource* src)
{
    HgSerializer obj = hgSerializeObject(arena, s, name);
    hgSerialize(arena, &obj, "Audio", &src->audio);
    hgSerialize(arena, &obj, "Position", &src->position);
    hgSerialize(arena, &obj, "Repeat", &src->repeat);
}

HgAudioSource* hgAudioSourceAdd(HgEcs* ecs, HgEntity e, HgAudioAsset* audio, bool repeat)
{
    hgAssert(ecs != nullptr);
    hgAssert(hgEcsAlive(ecs, e));

    HgAudioSource* src = hgEcsAdd<HgAudioSource>(ecs, e);
    *src = {};
    src->player = hgAudioPlayerCreate(HgAudioFormat_f32, 8000, 1);
    src->audio = audio;
    src->position = 0;
    src->repeat = repeat;

    return src;
}

template<>
void hgEcsDtor(HgAudioSource* src)
{
    hgAssetUnload(src->audio);
    hgAudioPlayerDestroy(src->player);
}

void hgAudioUpdate(HgEcs* ecs, HgEntity listener)
{
    hgEcsForEach<HgAudioSource>(ecs, [&](HgEntity e, HgAudioSource* src)
    {
        HgAudio* audio = &src->audio->data;
        if (src->position == audio->size && !src->repeat)
            return;

        HgArena* scratch = hgScratch();
        hgArenaScope(scratch);

        u32 width = hgAudioFormatSize(audio->format);

        u32 total = audio->frequency * width / 8;
        u32 queued = hgAudioPlayerQueuedSize(src->player);
        if (queued >= total)
            return;
        u32 sizeToPush = total - queued;

        void* queue = hgArenaAlloc(scratch, sizeToPush, width);
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
            f32 dist = hgVecDot3(relPos, relPos);
            f32 factor = 1.0f / dist;

            switch (audio->format)
            {
                case HgAudioFormat_u8:
                    for (u64 i = 0; i < sizeToPush / sizeof(u8); ++i)
                    {
                        u8 val = ((u8*)queue)[i];
                        val = (u8)((f32)val * factor);
                        ((u8*)queue)[i] = val;
                    }
                    break;
                case HgAudioFormat_s8:
                    for (u64 i = 0; i < sizeToPush / sizeof(i8); ++i)
                    {
                        i8 val = ((i8*)queue)[i];
                        val = (i8)((f32)val * factor);
                        ((i8*)queue)[i] = val;
                    }
                    break;
                case HgAudioFormat_s16:
                    for (u64 i = 0; i < sizeToPush / sizeof(i16); ++i)
                    {
                        i16 val = ((i16*)queue)[i];
                        val = (i16)((f32)val * factor);
                        ((i16*)queue)[i] = val;
                    }
                    break;
                case HgAudioFormat_s32:
                    for (u64 i = 0; i < sizeToPush / sizeof(i32); ++i)
                    {
                        i32 val = ((i32*)queue)[i];
                        val = (i32)((f32)val * factor);
                        ((i32*)queue)[i] = val;
                    }
                    break;
                case HgAudioFormat_f32:
                    for (u64 i = 0; i < sizeToPush / sizeof(f32); ++i)
                    {
                        ((f32*)queue)[i] *= factor;
                    }
                    break;
                default:
                    hgError("Invalid audio format enum: %u\n", audio->format);
            }
        }

        hgAssert(queueSize <= sizeToPush);
        hgAudioPlayerPush(src->player, queue, queueSize);
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

HgStringView hgEcsComponentName(HgEcs* ecs, u64 componentId)
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
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgEcs* ecs)
{
    hgAssert(arena != nullptr);
    hgAssert(ecs != nullptr);

    HgArena* scratch = hgScratch(&arena, 1);
    hgArenaScope(scratch);

    HgSerializer obj = hgSerializeObject(arena, s, name);

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

        hgSerialize(arena, &obj, "Entity Count", &entityCount);
    }
    else
    {
        hgSerialize(arena, &obj, "Entity Count", &entityCount);

        ecsSerial.idxToEntity = hgArenaAlloc<HgEntity>(scratch, entityCount);
        for (u32 i = 0; i < entityCount; ++i)
        {
            ecsSerial.idxToEntity[i] = hgEcsSpawn(ecs);
        }
    }

    HgSerializer systemArr{};
    u32 systemCount;
    if (s->writing)
        systemCount = ecs->components.count;
    systemArr = hgSerializeArray(arena, &obj, "Components", &systemCount);

    u32 systemIdx = (u32)-1;
    for (u32 i = 0; i < systemCount; ++i)
    {
        HgSerializer systemObj = hgSerializeObject(arena, &systemArr, "");

        u64 systemId = (u64)-1;
        HgComponent* systemData;
        if (s->writing)
        {
            ++systemIdx;
            while (!ecs->components.hasVal[systemIdx])
            {
                ++systemIdx;
            }
            systemId = ecs->components.keys[systemIdx];
            systemData = &ecs->components.vals[systemIdx];
            hgSerialize(arena, &systemObj, "Name", &systemData->name);
        }
        else
        {
            HgStringView compName;
            hgSerialize(arena, &systemObj, "Name", &compName);
            systemId = hgHash(compName);
            systemData = hgMapGet(&ecs->components, systemId);
        }

        u32 compCount;
        if (s->writing)
            compCount = systemData->entities.count - 1;
        HgSerializer compArr = hgSerializeArray(arena, &systemObj, "Data", &compCount);

        for (u32 c = 0; c < compCount; ++c)
        {
            HgSerializer compObj = hgSerializeObject(arena, &compArr, "");

            u32 entityIdx;
            if (s->writing)
                entityIdx = ecsSerial.entityToIdx[
                    hgHandleIdx(systemData->entities[c + 1].handle)];
            hgSerialize(arena, &compObj, "Entity Index", &entityIdx);

            void* compData;
            if (s->writing)
                compData = systemData->components[c + 1];
            else
                compData = hgEcsAdd(ecs, ecsSerial.idxToEntity[entityIdx], systemId);
            systemData->serialize(arena, &compObj, "Component", compData, &ecsSerial);
        }
    }
}

void hgEntitySerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgEntity* val, HgEntitySerializer* ecs)
{
    if (s->writing)
    {
        u32 idx = *val != hgEntityNull ? ecs->entityToIdx[hgHandleIdx(val->handle)] : (u32)-1;
        hgSerialize(arena, s, name, (i32*)&idx);
    }
    else
    {
        u32 idx = (u32)-1;
        hgSerialize(arena, s, name, (i32*)&idx);
        *val = idx != (u32)-1 ? ecs->idxToEntity[idx] : hgEntityNull;
    }
}

template<>
void hgEcsSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgNode* node, HgEntitySerializer* ecs)
{
    HgSerializer obj = hgSerializeObject(arena, s, name);
    hgEntitySerialize(arena, &obj, "Parent", &node->parent, ecs);
    hgEntitySerialize(arena, &obj, "Next Sibling", &node->nextSibling, ecs);
    hgEntitySerialize(arena, &obj, "Previous Sibling", &node->prevSibling, ecs);
    hgEntitySerialize(arena, &obj, "First Child", &node->firstChild, ecs);
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
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgTransform* node)
{
    HgSerializer obj = hgSerializeObject(arena, s, name);
    hgSerialize(arena, &obj, "Position", &node->position);
    hgSerialize(arena, &obj, "Scale", &node->scale);
    hgSerialize(arena, &obj, "Rotation", &node->rotation);
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


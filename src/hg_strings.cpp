#include "hg_core.hpp"
#include "hg_strings.hpp"

#include <cstdio>

namespace hg {

void binaryResize(Arena* arena, BinaryBuilder* bin, u64 newSize)
{
    bin->data = arenaRealloc(arena, bin->data, bin->size, newSize, 1);
    bin->size = newSize;
}

void binaryOverwrite(BinaryBuilder* bin, u64 idx, const void* src, u64 len)
{
    HG_ASSERT(idx + len <= bin->size);
    memCopy((u8*)bin->data + idx, src, len);
}

char* cString(Arena* arena, String str)
{
    HG_ASSERT(arena != nullptr);
    if (str.length > 0)
        HG_ASSERT(str.chars != nullptr);

    char* cStr = arenaAlloc<char>(arena, str.length + 1);
    memCopy(cStr, str.chars, str.length);
    cStr[str.length] = 0;
    return cStr;
}

String stringCreate(String data)
{
    String str{};
    if (data != "")
    {
        str.chars = allocGpa<char>(data.length);
        memCopy((char*)str.chars, data.chars, data.length);
        str.length = data.length;
    }
    return str;
}

void stringDestroy(String* str)
{
    freeGpa((char*)str->chars, str->length);
}

StringBuilder stringCopy(Arena* arena, String str)
{
    HG_ASSERT(arena != nullptr);

    StringBuilder copy{};
    copy.chars = arenaAlloc<char>(arena, str.length);
    memCopy(copy.chars, str.chars, str.length);
    copy.length = str.length;
    return copy;
}

StringBuilder stringFormat(Arena* arena, String format, ...)
{
    va_list args;
    va_start(args, format);
    StringBuilder ret = stringFormatVar(arena, format, args);
    va_end(args);
    return ret;
}

StringBuilder stringFormatVar(Arena* arena, String fmt, va_list args)
{
    Arena* scratch = getScratch(&arena, 1);
    HG_ARENA_SCOPE(scratch);

    int len = vsnprintf((char*)arena->memory + arena->head, arena->capacity - arena->head, cString(scratch, fmt), args);
    if (len < 0)
        HG_PANIC("snprintf returned an error");

    StringBuilder ret{(char*)arena->memory + arena->head, (u64)len};
    arena->head += (u64)len;

    return ret;
}

void stringInsert(Arena* arena, StringBuilder* dst, u64 idx, String src)
{
    HG_ASSERT(arena != nullptr);
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(idx <= dst->length);
    if (src.length > 0)
        HG_ASSERT(src.chars != nullptr);

    u64 newLength = dst->length + src.length;

    dst->chars = arenaRealloc(arena, dst->chars, dst->length, newLength);

    if (idx != dst->length)
        memMove(&dst->chars[idx + src.length], &dst->chars[idx], dst->length - idx);
    memCopy(&dst->chars[idx], src.chars, src.length);

    dst->length = newLength;
}

bool isWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool isNumeral(char c)
{
    return c >= '0' && c <= '9';
}

bool isInteger(String str)
{
    if (str.length == 0)
        return false;

    u64 head = 0;
    if (!isNumeral(str[head]) && str[head] != '+' && str[head] != '-')
        return false;

    ++head;
    while (head < str.length)
    {
        if (!isNumeral(str[head]))
            return false;
        ++head;
    }
    return true;
}

bool isFloat(String str)
{
    if (str.length == 0)
        return false;

    bool hasDecimal = false;
    bool hasExponent = false;

    u64 head = 0;

    if (!isNumeral(str[head]) && str[head] != '.' && str[head] != '+' && str[head] != '-')
        return false;

    if (str[head] == '.')
        hasDecimal = true;

    ++head;
    while (head < str.length)
    {
        if (isNumeral(str[head]))
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
            if (isNumeral(str[head]) || str[head] == '+' || str[head] == '-')
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

i64 stringToInteger(String str)
{
    HG_ASSERT(isInteger(str));

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

f64 stringToFloat(String str)
{
    HG_ASSERT(isFloat(str));

    f64 ret = 0.0;
    u64 head = 0;

    bool isNegative = str[head] == '-';
    if (isNegative || str[head] == '+')
        ++head;

    if (isNumeral(str[head]))
    {
        u64 intPartBegin = head;
        while (head < str.length && str[head] != '.' && str[head] != 'e')
        {
            ++head;
        }
        ret += (f64)stringToInteger({&str[intPartBegin], &str[head]});
    }

    if (head < str.length && str[head] == '.')
    {
        ++head;

        f64 power = 0.1;
        while (head < str.length && isNumeral(str[head]))
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
        while (head < str.length && isNumeral(str[head]))
        {
            ++head;
        }

        i64 exp = stringToInteger({&str[expBegin], str.chars + head});
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

StringBuilder integerToString(Arena* arena, i64 num)
{
    HG_ASSERT(arena != nullptr);

    Arena* scratch = getScratch(&arena, 1);
    HG_ARENA_SCOPE(scratch);

    if (num == 0)
        return stringCopy(arena, "0");

    bool isNegative = num < 0;
    u64 unum = (u64)std::abs(num);

    StringBuilder reverse{};
    while (unum != 0)
    {
        u64 digit = unum % 10;
        unum = (u64)((f64)unum / 10.0);
        stringAppendC(scratch, &reverse, '0' + (char)digit);
    }

    StringBuilder ret{};
    if (isNegative)
        stringAppendC(arena, &ret, '-');
    for (u64 i = reverse.length - 1; i < reverse.length; --i)
    {
        stringAppendC(arena, &ret, reverse[i]);
    }
    return ret;
}

StringBuilder floatToString(Arena* arena, f64 num, u32 decimalCount)
{
    HG_ASSERT(arena != nullptr);

    Arena* scratch = getScratch(&arena, 1);
    HG_ARENA_SCOPE(scratch);

    if (num == 0.0)
        return stringCopy(arena, "0.0");

    StringBuilder intStr = integerToString(scratch, (i64)std::abs(num));

    StringBuilder decStr{};
    stringAppendC(scratch, &decStr, '.');

    f64 decPart = std::abs(num);
    for (u64 i = 0; i < decimalCount; ++i)
    {
        decPart *= 10.0;
        stringAppendC(scratch, &decStr, '0' + (char)((u64)decPart % 10));
    }

    StringBuilder ret{};
    if (num < 0.0)
        stringAppendC(arena, &ret, '-');
    stringAppend(arena, &ret, intStr);
    stringAppend(arena, &ret, decStr);
    return ret;
}

} // namespace hg


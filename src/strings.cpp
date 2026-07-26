#include "hurdygurdy.hpp"
#include "internal.hpp"

namespace hg {

char* cString(Arena* arena, StringView str)
{
    HG_ASSERT(arena != nullptr);
    if (str.length > 0)
        HG_ASSERT(str.chars != nullptr);

    char* cStr = arena->alloc<char>(str.length + 1);
    if (str.length > 0)
        memcpy(cStr, str.chars, str.length);
    cStr[str.length] = 0;
    return cStr;
}

void StringBuilder::insert(u64 idx, StringView src)
{
    HG_ASSERT(arena != nullptr);
    HG_ASSERT(idx <= length);
    if (src.length > 0)
        HG_ASSERT(src.chars != nullptr);

    u64 newLength = length + src.length;

    if (!arena->extend(chars, length, newLength))
    {
        char* newChars = arena->alloc<char>(newLength);
        if (length > 0)
            memcpy(newChars, chars, length);
        chars = newChars;
    }

    if (idx != length)
        memmove(&chars[idx + src.length], &chars[idx], length - idx);
    memcpy(&chars[idx], src.chars, src.length);

    length = newLength;
}

String String::create(StringView data)
{
    String str;
    str.chars = heapAlloc<char>(data.length);
    str.length = data.length;
    memcpy(str.chars, data.chars, data.length);
    return str;
}

String::~String() noexcept
{
    if (chars != nullptr)
        heapFree(chars, length);
}

bool isWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool isNumeral(char c)
{
    return c >= '0' && c <= '9';
}

bool isInteger(StringView str)
{
    if (str.length == 0)
        return false;

    u64 head = 0;
    if (!isNumeral(str[head]) && str[head] != '+' && str[head] != '-')
        return false;

    if (str[head] == '+' || str[head] == '-')
    {
        ++head;
        if (head >= str.length)
            return false;
    }

    while (head < str.length)
    {
        if (!isNumeral(str[head]))
            return false;
        ++head;
    }
    return true;
}

bool isFloat(StringView str)
{
    if (str.length == 0)
        return false;

    bool hasDecimal = false;
    bool hasExponent = false;
    bool hasDigit = false;

    u64 head = 0;

    if (!isNumeral(str[head]) && str[head] != '.' && str[head] != '+' && str[head] != '-')
        return false;

    if (isNumeral(str[head]))
        hasDigit = true;

    if (str[head] == '.')
        hasDecimal = true;

    ++head;
    while (head < str.length)
    {
        if (isNumeral(str[head]))
        {
            hasDigit = true;
            ++head;
            continue;
        }

        if (str[head] == '.' && !hasDecimal && !hasExponent)
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

    return hasDigit && (hasDecimal || hasExponent);
}

i64 stringToInteger(StringView str)
{
    HG_ASSERT(isInteger(str));

    i64 power = 1;
    i64 ret = 0;

    u64 head = str.length - 1;
    while (head > 0)
    {
            ret += static_cast<i64>(str[head] - '0') * power;
        power *= 10;
        --head;
    }

    if (str[head] != '+')
    {
        if (str[head] == '-')
            ret *= -1;
        else
        ret += static_cast<i64>(str[head] - '0') * power;
    }

    return ret;
}

f64 stringToFloat(StringView str)
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
        ret += static_cast<f64>(stringToInteger({&str[intPartBegin], &str[head]}));
    }

    if (head < str.length && str[head] == '.')
    {
        ++head;

        f64 power = 0.1;
        while (head < str.length && isNumeral(str[head]))
        {
            ret += static_cast<f64>(str[head] - '0') * power;
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

    ArenaScope scratch = getScratch(&arena, 1);

    if (num == 0)
        return {arena, "0"};

    bool isNegative = num < 0;
    u64 unum = static_cast<u64>(std::abs(num));

    StringBuilder reverse{scratch};
    while (unum != 0)
    {
        u64 digit = unum % 10;
        unum /= 10;
        reverse.append('0' + static_cast<char>(digit));
    }

    StringBuilder ret{arena};
    if (isNegative)
        ret.append('-');
    for (u64 i = reverse.length - 1; i < reverse.length; --i)
    {
        ret.append(reverse[i]);
    }
    return ret;
}

StringBuilder floatToString(Arena* arena, f64 num, u32 decimalCount)
{
    HG_ASSERT(arena != nullptr);

    ArenaScope scratch = getScratch(&arena, 1);

    if (num == 0.0)
        return {arena, "0.0"};

    StringBuilder intStr = integerToString(scratch, static_cast<i64>(std::abs(num)));

    StringBuilder decStr{scratch};
    decStr.append('.');

    f64 decPart = std::abs(num);
    for (u64 i = 0; i < decimalCount; ++i)
    {
        decPart *= 10.0;
        decStr.append('0' + static_cast<char>(static_cast<u64>(decPart) % 10));
    }

    StringBuilder ret{arena};
    if (num < 0.0)
        ret.append('-');
    ret.append(intStr);
    ret.append(decStr);
    return ret;
}

} // namespace hg

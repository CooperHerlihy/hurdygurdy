#include "hg/strings.hpp"

#include <charconv>

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
    if (data.length > 0)
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

    if (str[head] == '+' || str[head] == '-')
        ++head;

    if (head == str.length)
        return false;

    while (head < str.length && isNumeral(str[head]))
    {
        ++head;
    }
    return head == str.length;
}

bool isFloat(StringView str)
{
    bool hasNumber = false;

    u64 head = 0;
    if (head == str.length)
        return false;

    if (str[head] == '+' || str[head] == '-')
        ++head;

    if (head == str.length)
        return false;

    while (head < str.length && isNumeral(str[head]))
    {
        hasNumber = true;
        ++head;
    }

    if (head == str.length)
        return false;

    if (str[head] != '.' && str[head] != 'e')
        return false;

    if (str[head] == '.')
    {
        ++head;
        while (head < str.length && isNumeral(str[head]))
        {
            hasNumber = true;
            ++head;
        }
    }

    if (!hasNumber)
        return false;

    if (head == str.length)
        return true;

    if (str[head] == 'e')
    {
        ++head;
        if (head == str.length)
            return false;

        if (str[head] == '+' || str[head] == '-')
            ++head;

        bool hasNumber = false;
        while (head < str.length && isNumeral(str[head]))
        {
            hasNumber = true;
            ++head;
        }
        if (!hasNumber)
            return false;
    }

    if (head == str.length)
        return true;

    if (str[head] == 'f')
        ++head;

    if (head == str.length)
        return true;

    return false;
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

    if (isNumeral(str[head]))
        ret += static_cast<i64>(str[head] - '0') * power;
    else if (str[head] == '-')
        ret *= -1;

    return ret;
}

f64 stringToFloat(StringView str)
{
    HG_ASSERT(str.chars != nullptr);

    f64 ret = 0.0;
    std::from_chars_result res = std::from_chars(str.chars, str.chars + str.length, ret);
    HG_ASSERT(res.ec == std::errc{});
    HG_ASSERT(res.ptr == str.chars + str.length);
    return ret;
}

StringBuilder integerToString(Arena* arena, i64 num)
{
    HG_ASSERT(arena != nullptr);

    ArenaScope scratch = getScratch(arena);

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

StringBuilder floatToString(Arena* arena, f64 num, u32 precision)
{
    HG_ASSERT(arena != nullptr);

    char buffer[64];
    std::to_chars_result res = std::to_chars(
        buffer,
        buffer + sizeof(buffer),
        num,
        std::chars_format::general,
        static_cast<i32>(precision));
    HG_ASSERT(res.ec == std::errc{});
    return {arena, StringView{buffer, static_cast<u64>(res.ptr - buffer)}};
}

} // namespace hg

#include "hg/utility.hpp"

#include <charconv>
#include <string>

namespace hg {

bool isWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool isNumeral(char c)
{
    return c >= '0' && c <= '9';
}

bool isInteger(std::string_view str)
{
    if (str.length() == 0)
        return false;

    u64 head = 0;

    if (str[head] == '+' || str[head] == '-')
        ++head;

    if (head == str.length())
        return false;

    while (head < str.length() && isNumeral(str[head]))
    {
        ++head;
    }
    return head == str.length();
}

bool isFloat(std::string_view str)
{
    bool hasNumber = false;

    u64 head = 0;
    if (head == str.length())
        return false;

    if (str[head] == '+' || str[head] == '-')
        ++head;

    if (head == str.length())
        return false;

    while (head < str.length() && isNumeral(str[head]))
    {
        hasNumber = true;
        ++head;
    }

    if (head == str.length())
        return false;

    if (str[head] != '.' && str[head] != 'e')
        return false;

    if (str[head] == '.')
    {
        ++head;
        while (head < str.length() && isNumeral(str[head]))
        {
            hasNumber = true;
            ++head;
        }
    }

    if (!hasNumber)
        return false;

    if (head == str.length())
        return true;

    if (str[head] == 'e')
    {
        ++head;
        if (head == str.length())
            return false;

        if (str[head] == '+' || str[head] == '-')
            ++head;

        bool hasNumber = false;
        while (head < str.length() && isNumeral(str[head]))
        {
            hasNumber = true;
            ++head;
        }
        if (!hasNumber)
            return false;
    }

    if (head == str.length())
        return true;

    if (str[head] == 'f')
        ++head;

    if (head == str.length())
        return true;

    return false;
}

i64 stringToInteger(std::string_view str)
{
    HG_ASSERT(isInteger(str));

    if (str.starts_with('+'))
        str.remove_prefix(1);

    i64 ret = 0;
    [[maybe_unused]]
    std::from_chars_result res = std::from_chars(str.data(), str.data() + str.length(), ret);
    HG_ASSERT(res.ec == std::errc{});
    HG_ASSERT(res.ptr == str.data() + str.length());
    return ret;
}

f64 stringToFloat(std::string_view str)
{
    HG_ASSERT(isFloat(str));

    f64 ret = 0.0;
    [[maybe_unused]]
    std::from_chars_result res = std::from_chars(str.data(), str.data() + str.length(), ret);
    HG_ASSERT(res.ec == std::errc{});
    HG_ASSERT(res.ptr == str.data() + str.length());
    return ret;
}

std::string integerToString(i64 num)
{
    char buffer[24];
    std::to_chars_result res = std::to_chars(buffer, buffer + sizeof(buffer), num);
    HG_ASSERT(res.ec == std::errc{});
    return std::string{buffer, res.ptr};
}

std::string floatToString(f64 num, u32 precision)
{
    char buffer[64];
    std::to_chars_result res = std::to_chars(
        buffer,
        buffer + sizeof(buffer),
        num,
        std::chars_format::general,
        static_cast<i32>(precision));
    *res.ptr = '\0';

    HG_ASSERT(res.ec == std::errc{});
    return buffer;
}

} // namespace hg

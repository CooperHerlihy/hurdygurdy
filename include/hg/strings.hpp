#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"
#include "hg/memory.hpp"

#include <cstring>

namespace hg {

/**
 * A non-owning view into a string
 */
struct StringView {
    /**
     * The character data
     */
    const char* chars = nullptr;
    /**
     * The view length in bytes
     */
    u64 length = 0;

    /**
     * Construct empty
     */
    StringView() = default;

    /**
     * Construct from pointer and length
     */
    constexpr StringView(const char* charsVal, u64 lengthVal)
        : chars{charsVal}, length{lengthVal}
    {}

    /**
     * Construct from begin and end
     */
    constexpr StringView(const char* begin, const char* end)
        : chars{begin}, length{static_cast<u64>(end - begin)}
    {
        HG_ASSERT(begin <= end);
    }

    /**
     * Construct from null terminated c string
     */
    constexpr StringView(const char* cStr)
        : chars{cStr}, length{0}
    {
        if (cStr != nullptr)
        {
            while (cStr[length] != '\0')
                ++length;
        }
    }

    /**
     * Access by index
     */
    constexpr const char& operator[](u64 idx) const
    {
        HG_ASSERT(chars != nullptr);
        HG_ASSERT(idx < length);
        return chars[idx];
    }

    /**
     * C++ style for loop
     */
    constexpr const char* begin() const
    {
        return chars;
    }

    /**
     * C++ style for loop
     */
    constexpr const char* end() const
    {
        return chars + length;
    }
};

/**
 * Compare strings
 */
inline bool operator==(StringView lhs, StringView rhs)
{
    return lhs.length == rhs.length && (lhs.length == 0 || memcmp(lhs.chars, rhs.chars, lhs.length) == 0);
}

/**
 * Compare strings
 */
inline bool operator!=(StringView lhs, StringView rhs)
{
    return !(lhs == rhs);
}

/**
 * Create a null terminated string for C interop
 *
 * Parameters
 * - arena The arena to allocate from
 * - str The string to create from
 */
char* cString(Arena* arena, StringView str);

/**
 * A string builder
 */
struct StringBuilder {
    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;
    /**
     * The string data
     */
    char* chars = nullptr;
    /**
     * The number of characters currently in the string
     */
    u64 length = 0;

    /**
     * Construct empty
     */
    StringBuilder() noexcept = default;

    /**
     * Construct a new builder
     */
    StringBuilder(Arena* arenaVal, StringView str = "")
        : arena{arenaVal} , chars{arenaVal->alloc<char>(str.length)} , length{str.length}
    {
        if (str != "")
            memcpy(chars, str.chars, str.length);
    }

    /**
     * Implicit converts to a string view
     */
    constexpr operator StringView() const
    {
        return {chars, length};
    }

    /**
     * Access using the index operator
     */
    constexpr char& operator[](u64 index) const
    {
        HG_ASSERT(index < length);
        return chars[index];
    }

    /**
     * Insert a string at idx
     */
    void insert(u64 idx, StringView src);

    /**
     * Add a string to the end
     */
    void append(StringView src)
    {
        insert(length, src);
    }

    /**
     * Insert a string at the beginning
     */
    void prepend(StringView src)
    {
        insert(0, src);
    }

    /**
     * Insert a char at idx
     */
    void insert(u64 idx, char c)
    {
        insert(idx, {&c, 1});
    }

    /**
     * Add a char to the end
     */
    void append(char c)
    {
        insert(length, c);
    }

    /**
     * Insert a char at the beginning
     */
    void prepend(char c)
    {
        insert(0, c);
    }

};

/**
 * Compare string builders
 */
inline bool operator==(const StringBuilder& lhs, const StringBuilder& rhs)
{
    return StringView{lhs} == StringView{rhs};
}

/**
 * Compare string builders
 */
inline bool operator!=(const StringBuilder& lhs, const StringBuilder& rhs)
{
    return !(lhs == rhs);
}

/**
 * An owning string
 */
struct String {
    /**
     * The string data
     */
    char* chars = nullptr;
    /**
     * The number of characters currently in the string
     */
    u64 length = 0;

    /**
     * Construct empty
     */
    String() noexcept = default;

    /**
     * Create a new string from data
     */
    static String create(StringView data);

    /**
     * Destroy the string
     */
    ~String() noexcept;

    /**
     * Implicit converts to a string view
     */
    constexpr operator StringView() const
    {
        return {chars, length};
    }

    /**
     * Access using the index operator
     */
    constexpr char& operator[](u64 index) const
    {
        HG_ASSERT(index < length);
        return chars[index];
    }

    /**
     * Move construct
     */
    String(String&& other) noexcept
        : chars{std::exchange(other.chars, nullptr)}
        , length{std::exchange(other.length, 0)}
    {}

    /**
     * Move assign
     */
    String& operator=(String&& other) noexcept
    {
        if (this != &other)
        {
            this->~String();
            new (this) String{std::move(other)};
        }
        return *this;
    }

    String(const String&) = delete;
    String& operator=(const String&) = delete;
};

/**
 * Compare strings
 */
inline bool operator==(const String& lhs, const String& rhs)
{
    return StringView{lhs} == StringView{rhs};
}

/**
 * Compare strings
 */
inline bool operator!=(const String& lhs, const String& rhs)
{
    return !(lhs == rhs);
}

/**
 * Create a formatted string : TODO
 */
// template<typename... Ts>
// StringBuilder stringFormat(Arena* arena, String fmt, Ts... args);

/**
 * Check whether a character is whitespace (space, tab, or newline)
 */
bool isWhitespace(char c);

/**
 * Check whether a character is a base 10 numeral (0-9)
 */
bool isNumeral(char c);

/**
 * Check whether a string is a base 10 integer
 */
bool isInteger(StringView str);

/**
 * Check whether a string is a base 10 floating point number
 */
bool isFloat(StringView str);

/**
 * Create an integer from a base 10 string
 */
i64 stringToInteger(StringView str);

/**
 * Create a float from a base 10 string
 */
f64 stringToFloat(StringView str);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 */
StringBuilder integerToString(Arena* arena, i64 num);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 * - decimalCount The number of trailing decimal digits
 */
StringBuilder floatToString(Arena* arena, f64 num, u32 decimalCount);

// base 2 and 16 string-int conversions : TODO
// arbitrary base string-int conversions : TODO?

} // namespace hg

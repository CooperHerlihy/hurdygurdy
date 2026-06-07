/*
 * =============================================================================
 *
 * Copyright (c) 2025 Cooper Herlihy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 */

#ifndef HG_STRINGS_HPP
#define HG_STRINGS_HPP

#include "hg_core.hpp"
#include "hg_memory.hpp"

/**
 * A span view into a string
 */
struct HgStringView {
    /**
     * The characters
     */
    const char* chars;
    /**
     * The number of characters;
     */
    u64 length;

    /**
     * Construct uninitialized
     */
    HgStringView() = default;

    /**
     * Create a string view from a pointer and length
     */
    constexpr HgStringView(const char* charsVal, u64 lengthVal)
        : chars{charsVal}, length{lengthVal} {}

    /**
     * Create a string view from begin and end pointers
     */
    constexpr HgStringView(const char* charsBegin, const char* charsEnd)
        : chars{charsBegin}, length{(uptr)(charsEnd - charsBegin)}
    {
        hgAssert(charsBegin <= charsEnd);
    }

    /**
     * Implicit constexpr conversion from c string
     *
     * Potentially dangerous, c string should be at most 4096 chars
     */
    constexpr HgStringView(const char* cStr) : chars{cStr}, length{0}
    {
        if (cStr != nullptr)
        {
            while (cStr[length] != '\0')
            {
                ++length;
                hgAssert(length <= 4096);
            }
        }
    }

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr const char& operator[](u64 idx) const
    {
        hgAssert(chars != nullptr);
        hgAssert(idx < length);
        return chars[idx];
    }
};

/**
 * Compare string views
 */
constexpr bool operator==(HgStringView lhs, HgStringView rhs)
{
    return lhs.length == rhs.length && hgMemEqual(lhs.chars, rhs.chars, lhs.length);
}

/**
 * Compare string views
 */
constexpr bool operator!=(HgStringView lhs, HgStringView rhs)
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
char* hgCString(HgArena* arena, HgStringView str);

/**
 * A dynamically allocated owning string
 */
struct HgStringOwner {
    /**
     * The string characters
     */
    const char* chars;
    /**
     * The length of the string
     */
    u64 length;

    /**
     * Access using the index operator
     */
    constexpr const char& operator[](u64 index) const
    {
        hgAssert(index < length);
        return chars[index];
    }

    /**
     * Implicit converts to a string view
     */
    constexpr operator HgStringView() const
    {
        return {chars, length};
    }
};

/**
 * Compare string owners
 */
inline bool operator==(const HgStringOwner& lhs, const HgStringOwner& rhs)
{
    return HgStringView{lhs} == HgStringView{rhs};
}

/**
 * Compare string owners
 */
inline bool operator!=(const HgStringOwner& lhs, const HgStringOwner& rhs)
{
    return !(lhs == rhs);
}

/**
 * Create a new HgStringOwner
 */
HgStringOwner hgStringCreate(HgStringView data);

/**
 * Destroy an HgStringOwner
 */
void hgStringDestroy(HgStringOwner* str);

/**
 * A string builder using arenas
 */
struct HgStringBuilder {
    /**
     * The string data
     */
    char* chars;
    /**
     * The number of characters currently in the string
     */
    u64 length;

    /**
     * Access using the index operator
     */
    constexpr char& operator[](u64 index) const
    {
        hgAssert(index < length);
        return chars[index];
    }

    /**
     * Implicit converts to a string view
     */
    constexpr operator HgStringView() const
    {
        return {chars, length};
    }
};

/**
 * Compare string builders
 */
inline bool operator==(const HgStringBuilder& lhs, const HgStringBuilder& rhs)
{
    return HgStringView{lhs} == HgStringView{rhs};
}

/**
 * Compare string builders
 */
inline bool operator!=(const HgStringBuilder& lhs, const HgStringBuilder& rhs)
{
    return !(lhs == rhs);
}

/**
 * Creates a new string copied from an existing string
 *
 * Parameters
 * - arena The arena to allocate from
 * - init The initial string to copy from
 */
HgStringBuilder hgStringCopy(HgArena* arena, HgStringView str);

// /**
//  * Create a formatted string : TODO
//  *
//  * Format specifiers
//  * - int (i64): "{i}"
//  * - unsigned int (u64): "{u}"
//  * - hexadecimal (i64): "{x}"
//  * - float with 6 decimals (f64): "{f}"
//  * - float with N decimals (f64): "{fN}"
//  * - char (char): "{c}"
//  * - string (HgStringView): "{s}"
//  * - c string (char*): "{cstr}"
//  *
//  * Use {{ and }} to escape the format specifier
//  *
//  * Parameters
//  * - arena The arena to allocate from
//  * - fmt The format string
//  * - ... The format parameters
//  */
// HgStringBuilder hgStringFormat(HgArena* arena, HgStringView fmt, ...);

/**
 * Copies another string into the string at index
 *
 * Parameters
 * - arena The arena to allocate from
 * - dst The string to insert into
 * - idx The index into dst
 * - src The string to copy from
 */
void hgStringInsert(HgArena* arena, HgStringBuilder* dst, u64 idx, HgStringView src);

/**
 * Copies another string to the end of the string
 */
inline void hgStringAppend(HgArena* arena, HgStringBuilder* dst, HgStringView src)
{
    hgStringInsert(arena, dst, dst->length, src);
}

/**
 * Copies another string to the beginning of the string
 */
inline void hgStringPrepend(HgArena* arena, HgStringBuilder* dst, HgStringView src)
{
    hgStringInsert(arena, dst, 0, src);
}

/**
 * Copies a character into the string at index
 *
 * Parameters
 * - arena The arena to allocate from
 * - dst The string to insert into
 * - idx The index into dst
 * - c The character to insert
 */
inline void hgStringInsertC(HgArena* arena, HgStringBuilder* dst, u64 idx, char c)
{
    hgStringInsert(arena, dst, idx, {&c, 1});
}

/**
 * Copies another string to the end of the string
 */
inline void hgStringAppendC(HgArena* arena, HgStringBuilder* dst, char c)
{
    hgStringInsertC(arena, dst, dst->length, c);
}

/**
 * Copies another string to the beginning of the string
 */
inline void hgStringPrependC(HgArena* arena, HgStringBuilder* dst, char c)
{
    hgStringInsertC(arena, dst, 0, c);
}

/**
 * Check whether a character is whitespace (space, tab, or newline)
 */
bool hgIsWhitespace(char c);

/**
 * Check whether a character is a base 10 numeral (0-9)
 */
bool hgIsNumeral(char c);

/**
 * Check whether a string is a base 10 integer
 */
bool hgIsInteger(HgStringView str);

/**
 * Check whether a string is a base 10 floating point number
 */
bool hgIsFloat(HgStringView str);

/**
 * Create an integer from a base 10 string
 */
i64 hgStringToInteger(HgStringView str);

/**
 * Create a float from a base 10 string
 */
f64 hgStringToFloat(HgStringView str);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 */
HgStringBuilder hgIntegerToString(HgArena* arena, i64 num);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 * - decimalCount The number of trailing decimal digits
 */
HgStringBuilder hgFloatToString(HgArena* arena, f64 num, u32 decimalCount);

// base 2 and 16 string-int conversions : TODO
// arbitrary base string-int conversions : TODO?

#endif // HG_STRINGS_HPP

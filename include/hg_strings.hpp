/*
 * =============================================================================
 *
 * Copyright (c) 2025-2026 Cooper Herlihy
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
#include "hg_utils.hpp"

namespace hg {

/**
 * Read data at index into a buffer
 *
 * Parameters
 * - idx The index into the file in bytes to read from
 * - dst A pointer to store the read data
 * - size The size in bytes to read
 */
void hgBinaryRead(HgBinary bin, u64 idx, void* dst, u64 len);

/**
 * Read data of arbitrary type from the file
 *
 * Parameters
 * - idx The index into the file in bytes to read from
 */
template<typename T>
T hgBinaryRead(HgBinary bin, u64 idx)
{
    T ret;
    hgBinaryRead(bin, idx, &ret, sizeof(T));
    return ret;
}

/**
 * A binary builder
 */
struct HgBinaryBuilder {
    /**
     * The data
     */
    void* data;
    /**
     * The size of data in bytes
     */
    u64 size;

    /**
     * Implicitly convert to HgBinary
     */
    constexpr operator HgBinary()
    {
        return {data, size};
    }
};

/**
 * Resize the binary
 *
 * Parameters
 * - arena The arena to allocate from
 * - newSize The new size of the file in bytes
 */
void hgBinaryResize(HgArena* arena, HgBinaryBuilder* bin, u64 newSize);

/**
 * Overwrite data at the index
 *
 * Parameters
 * - idx The index into the file to overwrite
 * - src The data to write
 * - size The size of the data in bytes
 */
void hgBinaryOverwrite(HgBinaryBuilder* bin, u64 idx, const void* src, u64 len);

/**
 * Overwrite data of arbitrary type at the index
 *
 * Parameters
 * - idx The index into the file to overwrite
 * - src The data to write
 */
template<typename T>
void hgBinaryOverwrite(HgBinaryBuilder* bin, u64 idx, const T& src)
{
    hgBinaryOverwrite(bin, idx, &src, sizeof(T));
}

/**
 * Compare strings
 */
constexpr bool operator==(HgString lhs, HgString rhs)
{
    return lhs.length == rhs.length && hgMemEqual(lhs.chars, rhs.chars, lhs.length);
}

/**
 * Compare strings
 */
constexpr bool operator!=(HgString lhs, HgString rhs)
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
char* hgCString(HgArena* arena, HgString str);

/**
 * Create a new owning string
 */
HgString hgStringCreate(HgString data);

/**
 * Destroy an owning string
 */
void hgStringDestroy(HgString* str);

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
    constexpr operator HgString() const
    {
        return {chars, length};
    }
};

/**
 * Compare string builders
 */
inline bool operator==(const HgStringBuilder& lhs, const HgStringBuilder& rhs)
{
    return HgString{lhs} == HgString{rhs};
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
HgStringBuilder hgStringCopy(HgArena* arena, HgString str);

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
// HgStringBuilder hgStringFormat(HgArena* arena, HgString fmt, ...);

/**
 * Create a formatted string, interally using snprintf
 */
HgStringBuilder hgStringFormat(HgArena* arena, HgString fmt, ...);

/**
 * Create a formatted string with varargs, interally using snprintf
 */
HgStringBuilder hgStringFormatVar(HgArena* arena, HgString fmt, va_list args);

/**
 * Copies another string into the string at index
 *
 * Parameters
 * - arena The arena to allocate from
 * - dst The string to insert into
 * - idx The index into dst
 * - src The string to copy from
 */
void hgStringInsert(HgArena* arena, HgStringBuilder* dst, u64 idx, HgString src);

/**
 * Copies another string to the end of the string
 */
inline void hgStringAppend(HgArena* arena, HgStringBuilder* dst, HgString src)
{
    hgStringInsert(arena, dst, dst->length, src);
}

/**
 * Copies another string to the beginning of the string
 */
inline void hgStringPrepend(HgArena* arena, HgStringBuilder* dst, HgString src)
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
bool hgIsInteger(HgString str);

/**
 * Check whether a string is a base 10 floating point number
 */
bool hgIsFloat(HgString str);

/**
 * Create an integer from a base 10 string
 */
i64 hgStringToInteger(HgString str);

/**
 * Create a float from a base 10 string
 */
f64 hgStringToFloat(HgString str);

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

} // namespace hg

#endif // HG_STRINGS_HPP

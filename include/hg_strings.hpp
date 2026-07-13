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
 * A binary builder
 */
struct BinaryBuilder {
    /**
     * The data
     */
    void* data;
    /**
     * The size of data in bytes
     */
    u64 size;

    /**
     * Implicitly convert to Binary
     */
    constexpr operator Binary()
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
void binaryResize(Arena* arena, BinaryBuilder* bin, u64 newSize);

/**
 * Overwrite data at the index
 *
 * Parameters
 * - idx The index into the file to overwrite
 * - src The data to write
 * - size The size of the data in bytes
 */
void binaryOverwrite(BinaryBuilder* bin, u64 idx, const void* src, u64 len);

/**
 * Overwrite data of arbitrary type at the index
 *
 * Parameters
 * - idx The index into the file to overwrite
 * - src The data to write
 */
template<typename T>
void binaryOverwrite(BinaryBuilder* bin, u64 idx, const T& src)
{
    binaryOverwrite(bin, idx, &src, sizeof(T));
}

/**
 * Compare strings
 */
inline bool operator==(String lhs, String rhs)
{
    return lhs.length == rhs.length && memEqual(lhs.chars, rhs.chars, lhs.length);
}

/**
 * Compare strings
 */
inline bool operator!=(String lhs, String rhs)
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
char* cString(Arena* arena, String str);

/**
 * Create a new owning string
 */
String stringCreate(String data);

/**
 * Destroy an owning string
 */
void stringDestroy(String* str);

/**
 * A string builder using arenas
 */
struct StringBuilder {
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
        HG_ASSERT(index < length);
        return chars[index];
    }

    /**
     * Implicit converts to a string view
     */
    constexpr operator String() const
    {
        return {chars, length};
    }
};

/**
 * Compare string builders
 */
inline bool operator==(const StringBuilder& lhs, const StringBuilder& rhs)
{
    return String{lhs} == String{rhs};
}

/**
 * Compare string builders
 */
inline bool operator!=(const StringBuilder& lhs, const StringBuilder& rhs)
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
StringBuilder stringCopy(Arena* arena, String str);

/**
 * Create a formatted string : TODO
 *
 * Format specifiers
 * - int (i64): "{i}"
 * - unsigned int (u64): "{u}"
 * - hexadecimal (i64): "{x}"
 * - float with 6 decimals (f64): "{f}"
 * - float with N decimals (f64): "{fN}"
 * - char (char): "{c}"
 * - string (StringView): "{s}"
 * - c string (char*): "{cstr}"
 *
 * Use {{ and }} to escape the format specifier
 *
 * Parameters
 * - arena The arena to allocate from
 * - fmt The format string
 * - ... The format parameters
 */

/**
 * Create a formatted string, interally using snprintf
 */
StringBuilder stringFormat(Arena* arena, String fmt, ...);

/**
 * Create a formatted string with varargs, interally using snprintf
 */
StringBuilder stringFormatVar(Arena* arena, String fmt, va_list args);

/**
 * Copies another string into the string at index
 *
 * Parameters
 * - arena The arena to allocate from
 * - dst The string to insert into
 * - idx The index into dst
 * - src The string to copy from
 */
void stringInsert(Arena* arena, StringBuilder* dst, u64 idx, String src);

/**
 * Copies another string to the end of the string
 */
inline void stringAppend(Arena* arena, StringBuilder* dst, String src)
{
    stringInsert(arena, dst, dst->length, src);
}

/**
 * Copies another string to the beginning of the string
 */
inline void stringPrepend(Arena* arena, StringBuilder* dst, String src)
{
    stringInsert(arena, dst, 0, src);
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
inline void stringInsertC(Arena* arena, StringBuilder* dst, u64 idx, char c)
{
    stringInsert(arena, dst, idx, {&c, 1});
}

/**
 * Copies another string to the end of the string
 */
inline void stringAppendC(Arena* arena, StringBuilder* dst, char c)
{
    stringInsertC(arena, dst, dst->length, c);
}

/**
 * Copies another string to the beginning of the string
 */
inline void stringPrependC(Arena* arena, StringBuilder* dst, char c)
{
    stringInsertC(arena, dst, 0, c);
}

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
bool isInteger(String str);

/**
 * Check whether a string is a base 10 floating point number
 */
bool isFloat(String str);

/**
 * Create an integer from a base 10 string
 */
i64 stringToInteger(String str);

/**
 * Create a float from a base 10 string
 */
f64 stringToFloat(String str);

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

#endif // HG_STRINGS_HPP

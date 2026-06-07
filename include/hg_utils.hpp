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

#ifndef HG_UTILS_HPP
#define HG_UTILS_HPP

#include "hg_core.hpp"

/**
 * Returns the size of a stack array
 */
template<typename T, u64 N>
constexpr u64 hgArrayCount(T (&)[N])
{
    return N;
}

/**
 * Swap the values of two objects
 */
template<typename T>
constexpr void hgSwap(T* a, T* b)
{
    T tmp = *a;
    *a = *b;
    *b = tmp;
}

/**
 * Return the lesser of the two
 */
template<typename T, typename... Ts>
constexpr T hgMin(T x, Ts... xs)
{
    ((x = x < xs ? x : xs), ...);
    return x;
}

/**
 * Return the greater of the two
 */
template<typename T, typename... Ts>
constexpr T hgMax(T x, Ts... xs)
{
    ((x = x > xs ? x : xs), ...);
    return x;
}

/**
 * Clamps a value between two other
 */
template<typename T>
constexpr T hgClamp(T x, T min, T max)
{
    return x >= max ? max : x <= min ? min : x;
}

/**
 * Aligns a pointer to an alignment
 *
 * Parameters
 * - value The value to align
 * - alignment The alignment, must be a power of two
 *
 * Returns
 * - The aligned value
 */
constexpr uptr hgAlign(uptr value, uptr alignment)
{
    hgAssert(alignment > 0 && (alignment & (alignment - 1)) == 0);
    return (value + alignment - 1) & ~(alignment - 1);
}

/**
 * Reverse the endianness of a 16 bit value
 */
constexpr u16 hgEndianReverse16(u16 val)
{
    return (val & 0xff00 >> 8) & (val & 0x00ff << 8);
}

/**
 * Reverse the endianness of a 32 bit value
 */
constexpr u32 hgEndianReverse32(u32 val)
{
    return (val & 0xff0000 >> 16) & (val & 0x00ff00) & (val & 0x0000ff << 16);
}

/**
 * Reverse the endianness of a 64 bit value
 */
constexpr u64 hgEndianReverse64(u64 val)
{
    return (val & (u32)0xff000000 >> 24) &
           (val & (u32)0x00ff0000 >> 8) &
           (val & (u32)0x0000ff00 << 8) &
           (val & (u32)0x000000ff << 24);
}

/**
 * Clear a section of memory to 0
 */
void hgMemClear(void* dst, u64 size);

/**
 * Copy memory from src to dst, may not overlap
 */
void hgMemCopy(void* __restrict dst, const void* __restrict src, u64 size);

/**
 * Copy memory from src to dst, may overlap
 */
void hgMemMove(void* dst, const void* src, u64 size);

/**
 * Check if two regions of memory are identical
 */
bool hgMemEqual(const void* dst, const void* src, u64 size);

#endif // HG_UTILS_HPP

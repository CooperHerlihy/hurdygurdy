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
 * Swap to regions of memory
 */
void hgSwap(void* a, void* b, u64 size);

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
 * Returns whether a value is a power of 2
 */
constexpr bool hgIsPowerOf2(u64 val)
{
    return val > 0 && (val & (val - 1)) == 0;
}

/**
 * Aligns a pointer to an alignment
 *
 * Parameters
 * - val The value to align
 * - align The alignment, must be a power of two
 *
 * Returns
 * - The aligned value
 */
constexpr uptr hgAlign(uptr val, uptr align)
{
    hgAssert(hgIsPowerOf2(align));
    return (val + align - 1) & ~(align - 1);
}

/**
 * Reverse the endianness of a 16 bit value
 */
constexpr u16 hgEndianReverse16(u16 val)
{
    return (val >> 8) | (val << 8);
}

/**
 * Reverse the endianness of a 32 bit value
 */
constexpr u32 hgEndianReverse32(u32 val)
{
    return (val >> 24) | ((val >> 8) & 0xff00) | ((val & 0xff00) << 8) | (val << 24);
}

/**
 * Reverse the endianness of a 64 bit value
 */
constexpr u64 hgEndianReverse64(u64 val)
{
    u64 swapped = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    swapped = ((swapped << 16) & 0xFFFF0000FFFF0000ULL) | ((swapped >> 16) & 0x0000FFFF0000FFFFULL);
    return (swapped << 32) | (swapped >> 32);
}

/**
 * Clear a section of memory
 */
void hgMemClear(void* dst, u64 size, u8 val = 0);

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

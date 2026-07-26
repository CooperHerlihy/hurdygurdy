#pragma once

#include "hg_types.hpp"

namespace hg {

/**
 * Returns whether the value is a power of 2
 */
constexpr bool isPowerOf2(u64 val)
{
    return val > 0 && (val & (val - 1)) == 0;
}

/**
 * Aligns a value up to the nearest aligned value
 */
constexpr uptr alignUp(uptr val, uptr align)
{
    HG_ASSERT(isPowerOf2(align));
    return (val + align - 1) & ~(align - 1);
}

/**
 * Reverse the endianness of a 16 bit value
 */
constexpr u16 endianReverse16(u16 val)
{
    return static_cast<u16>(val >> 8) | static_cast<u16>(val << 8);
}

/**
 * Reverse the endianness of a 32 bit value
 */
constexpr u32 endianReverse32(u32 val)
{
    return (val >> 24) | ((val >> 8) & 0xff00) | ((val & 0xff00) << 8) | (val << 24);
}

/**
 * Reverse the endianness of a 64 bit value
 */
constexpr u64 endianReverse64(u64 val)
{
    u64 swapped = ((val << 8) & 0xff00ff00ff00ff00ull) | ((val >> 8) & 0x00ff00ff00ff00ffull);
    swapped = ((swapped << 16) & 0xffff0000ffff0000ull) | ((swapped >> 16) & 0x0000ffff0000ffffull);
    return (swapped << 32) | (swapped >> 32);
}

} // namespace hg


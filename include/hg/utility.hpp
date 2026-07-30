#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"

#include <concepts>

namespace hg {

/**
 * Returns the size of an object with a size method
 */
template<typename T>
constexpr u64 size(const T& val)
{
    return static_cast<u64>(val.size());
}

/**
 * Returns the size of a c style array in elements
 */
template<typename T, u64 N>
constexpr u64 size(const T (&)[N])
{
    return N;
}

/**
 * Returns the index of T in a set of types First through Rest
 */
template<typename T, typename First, typename... Rest> requires (std::same_as<T, First> || (std::same_as<T, Rest> || ...))
constexpr u64 idxOf()
{
    if constexpr (std::same_as<T, First>)
        return 0;
    else
        return 1 + idxOf<T, Rest...>();
}

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

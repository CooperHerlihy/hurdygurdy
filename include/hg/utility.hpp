#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"

#include <bit>
#include <concepts>
#include <variant>

namespace hg {

/**
 * Get the first type in a pack
 */
template<typename... Ts>
struct FirstInPack {};

/**
 * Get the first type in a pack
 */
template<typename T, typename... Ts>
struct FirstInPack<T, Ts...> {
    using Type = T;
};

/**
 * Get the first type in a pack
 */
template<typename... Ts>
using FirstInPackT = FirstInPack<Ts...>::Type;

/**
 * Returns the index of T in a set of types First through Rest
 */
template<typename T, typename First, typename... Rest> requires (std::same_as<T, First> || (std::same_as<T, Rest> || ...))
consteval u64 idxOfInPack()
{
    if constexpr (std::same_as<T, First>)
        return 0;
    else
        return 1 + idxOfInPack<T, Rest...>();
}

/**
 * Aligns a value up to the nearest aligned value
 */
constexpr uptr alignUp(uptr val, uptr align)
{
    HG_ASSERT(std::has_single_bit(align));
    return (val + align - 1) & ~(align - 1);
}

#ifdef __cpp_lib_byteswap

/**
 * Reverse the endianness of a 16 bit value
 */
constexpr u16 endianReverse16(u16 val)
{
    return std::byteswap(val);
}

/**
 * Reverse the endianness of a 32 bit value
 */
constexpr u32 endianReverse32(u32 val)
{
    return std::byteswap(val);
}

/**
 * Reverse the endianness of a 64 bit value
 */
constexpr u64 endianReverse64(u64 val)
{
    return std::byteswap(val);
}

#else

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

#endif

/**
 * An overloaded function set for variant pattern matching
 */
template<typename... Fs>
struct Overload : Fs... {
    using Fs::operator()...;
};

/**
 * Like std::visit, but takes a list of functions and pattern matches
 */
template<typename V, typename... Fs>
auto match(V& variant, Fs&&... fs)
{
    return std::visit(Overload{std::forward<Fs>(fs)...}, variant);
}

} // namespace hg

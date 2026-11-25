#pragma once

#include "hg/inttypes.hpp"
#include "hg/assert.hpp"

#include <bit>
#include <utility>
#include <optional>
#include <variant>
#include <string_view>
#include <vector>
#include <span>

namespace hg {

/**
 * Aligns a value up to the nearest aligned value
 */
constexpr uintptr_t alignUp(uintptr_t val, uintptr_t align)
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

namespace internal {

    /**
     * An overloaded function set for variant pattern matching
     */
    template<typename... Fs>
    struct Overload : Fs... {
        using Fs::operator()...;
    };

} // namespace internal

/**
 * Like std::visit, but takes a list of functions and pattern matches
 */
template<typename V, typename... Fs>
auto match(V& variant, Fs&&... fs)
{
    return std::visit(internal::Overload{std::forward<Fs>(fs)...}, variant);
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
bool isInteger(std::string_view str);

/**
 * Check whether a string is a base 10 floating point number
 */
bool isFloat(std::string_view str);

/**
 * Create an integer from a base 10 string
 */
i64 stringToInteger(std::string_view str);

/**
 * Create a float from a base 10 string
 */
f64 stringToFloat(std::string_view str);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - num The integer number to create from
 */
std::string integerToString(i64 num);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - num The integer number to create from
 * - precision The number of significant digits
 */
std::string floatToString(f64 num, u32 precision);

/**
 * Load a file into memory
 */
std::optional<std::vector<byte>> binaryLoad(std::string_view path);

/**
 * Store a file to disc
 */
bool binaryStore(std::span<const byte> bin, std::string_view path);

} // namespace hg

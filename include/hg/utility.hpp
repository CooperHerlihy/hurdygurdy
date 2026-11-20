#pragma once

#include "hg/inttypes.hpp"

#include <cstdio>
#include <cstdlib>
#include <bit>
#include <utility>
#include <optional>
#include <variant>
#include <string_view>
#include <vector>
#include <format>

#ifdef __GNUC__
#define HG_COMPILER_GCC 1
#endif

#ifdef __clang__
#define HG_COMPILER_CLANG 1
#endif

#ifdef _MSC_VER
#define HG_COMPILER_MSVC 1
#endif

#ifdef __linux__
#define HG_PLATFORM_LINUX 1
#endif

#ifdef _WIN32
#define HG_PLATFORM_WINDOWS 1
#endif

#ifdef __APPLE__
#define HG_PLATFORM_APPLE 1
#endif

#if !defined(HG_PLATFORM_LINUX) && !defined(HG_PLATFORM_WINDOWS) && !defined(HG_PLATFORM_APPLE)
#error "Unsupported platfom"
#endif

#ifdef NDEBUG
#define HG_RELEASE_MODE 1
#else
#define HG_DEBUG_MODE 1
#endif

#ifdef HG_DEBUG_MODE

#ifndef HG_NO_LOGGING
#define HG_LOGGING 1
#endif

#ifndef HG_NO_ASSERTIONS
#define HG_ASSERTIONS 1
#endif

#ifndef HG_NO_VK_DEBUG_MESSENGER
#define HG_VK_DEBUG_MESSENGER 1
#endif

#endif

#ifdef HG_RELEASE_MODE

#ifndef HG_LOGGING
#define HG_NO_LOGGING 1
#endif

#ifndef HG_ASSERTIONS
#define HG_NO_ASSERTIONS 1
#endif

#ifndef HG_VK_DEBUG_MESSENGER
#define HG_NO_VK_DEBUG_MESSENGER 1
#endif

#endif

#define HG_MACRO_CONCAT_INTERNAL(x, y) x##y

#define HG_MACRO_CONCAT(x, y) HG_MACRO_CONCAT_INTERNAL(x, y)

/**
 * Defer a block of code to the end of the scope
 */
#define HG_DEFER(...) [[maybe_unused]] hg::internal::Defer HG_MACRO_CONCAT(defer_, __LINE__){[&]{ __VA_ARGS__ ;}}

#ifdef HG_ASSERTIONS

/**
 * Assert a condition, or panic
 */
#define HG_ASSERT(cond) do { \
    if (!(cond)) \
        ::hg::panic("Assertion failed in {}:{} {}() \"{}\"\n", __FILE__, __LINE__, __func__, #cond); \
} while(0)

#else

#define HG_ASSERT(cond) do {} while(0)

#endif

namespace hg {

namespace internal {

template<typename F>
struct Defer {
    F fn;

    Defer(F&& fnVal)
        : fn(std::forward<F>(fnVal))
    {}

    ~Defer()
    {
        fn();
    }
};

} // namespace internal

/**
 * Format a message into a stack buffer
 *
 * The buffer is null terminated on write
 *
 * Returns
 * - The number of characters written
 */
template<usize N, typename... Ts>
u64 formatBuffer(char (&buf)[N], std::format_string<Ts...> fmt, Ts&&... args)
{
    auto res = std::format_to_n(buf, sizeof(buf) - 1, fmt, std::forward<Ts>(args)...);
    *res.out = '\0';
    return static_cast<u64>(res.size) < sizeof(buf)
        ? static_cast<u64>(res.size)
        : sizeof(buf) - 1;
}

/**
 * Set this thread's current error message
 */
void setError(std::string_view error);

/**
 * Format and set this thread's current error message
 */
template<typename... Ts> requires (sizeof...(Ts) > 0)
void setError(std::format_string<Ts...> fmt, Ts&&... args)
{
    char buf[4096];
    u64 len = formatBuffer(buf, fmt, std::forward<Ts>(args)...);
    setError(std::string_view{buf, len});
}

/**
 * Get this thread's current error message
 */
std::string_view getError();

/**
 * Log this thread's current error message to stderr
 */
void printError();

/**
 * Format a log message to stderr
 */
template<typename... Ts>
void log(std::format_string<Ts...> fmt, Ts&&... args)
{
#ifdef HG_LOGGING
    char buf[4096];
    formatBuffer(buf, fmt, std::forward<Ts>(args)...);
    std::fprintf(stderr, "HurdyGurdy Log: %s", buf);
#else
    (void)fmt;
    ((void)args, ...);
#endif
}

/**
 * Format a warning to stderr
 */
template<typename... Ts>
void warn(std::format_string<Ts...> fmt, Ts&&... args)
{
#ifdef HG_LOGGING
    char buf[4096];
    formatBuffer(buf, fmt, std::forward<Ts>(args)...);
    std::fprintf(stderr, "HurdyGurdy Warn: %s", buf);
#else
    (void)fmt;
    ((void)args, ...);
#endif
}

/**
 * Format a panic message to stderr and abort the program
 */
template<typename... Ts>
[[noreturn]] void panic(std::format_string<Ts...> fmt, Ts&&... args)
{
#ifdef HG_LOGGING
    char buf[4096];
    formatBuffer(buf, fmt, std::forward<Ts>(args)...);
    std::fprintf(stderr, "HurdyGurdy Panic: %s", buf);
    printError();
#else
    (void)fmt;
    ((void)args, ...);
#endif
    std::abort();
}

/**
 * Cast between number types, asserting unsafe behavior in debug mode
 *
 * Note, float to integer still truncates down
 */
template<typename To, typename From> requires (std::is_arithmetic_v<To> && std::is_arithmetic_v<From>)
constexpr To assertCast(const From& from)
{
    if constexpr (std::is_floating_point_v<From>)
    {
        if constexpr (std::is_floating_point_v<To>)
        {
            if constexpr (std::numeric_limits<To>::max() < std::numeric_limits<From>::max())
                HG_ASSERT(from <= std::numeric_limits<To>::max());
            if constexpr (std::numeric_limits<To>::lowest() > std::numeric_limits<From>::lowest())
                HG_ASSERT(from >= std::numeric_limits<To>::lowest());
        }
        else
        {
            HG_ASSERT(from <= static_cast<From>(std::numeric_limits<To>::max()));
            HG_ASSERT(from >= static_cast<From>(std::numeric_limits<To>::lowest()));
        }
    }
    else if constexpr (std::is_integral_v<To>)
    {
        if constexpr (std::numeric_limits<To>::max() < std::numeric_limits<From>::max())
            HG_ASSERT(from <= static_cast<From>(std::numeric_limits<To>::max()));
        if constexpr (std::is_signed_v<From> && std::is_signed_v<To>)
        {
            if constexpr (std::numeric_limits<To>::lowest() > std::numeric_limits<From>::lowest())
                HG_ASSERT(from >= static_cast<From>(std::numeric_limits<To>::lowest()));
        }
        if constexpr (std::is_signed_v<From> && std::is_unsigned_v<To>)
            HG_ASSERT(from >= From(0));
    }
    return static_cast<To>(from);
}

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

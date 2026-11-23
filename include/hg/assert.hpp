#pragma once

#include "hg/inttypes.hpp"

#include <limits>
#include <type_traits>
#include <utility>

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

namespace hg {

/**
 * Report a failed assertion and abort the program
 */
[[noreturn]] void assertionFailed(const char* file, i32 line, const char* func, const char* cond);

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

} // namespace hg

#ifdef HG_ASSERTIONS

/**
 * Assert a condition, or abort the program
 */
#define HG_ASSERT(cond) do { \
    if (!(cond)) \
        ::hg::assertionFailed(__FILE__, __LINE__, __func__, #cond); \
} while(0)

#else

#define HG_ASSERT(cond) do {} while(0)

#endif

namespace hg {

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

} // namespace hg

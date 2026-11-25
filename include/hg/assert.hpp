#pragma once

#include "hg/inttypes.hpp"
#include "hg/definitions.hpp"

#include <cstdio>
#include <cstdlib>
#include <limits>
#include <type_traits>

#ifdef HG_ASSERTIONS

/**
 * Assert a condition, or abort the program
 */
#define HG_ASSERT(cond) do { \
    if (!(cond)) \
        ::hg::internal::assertionFailed(__FILE__, __LINE__, __func__, #cond); \
} while(0)

#else

#define HG_ASSERT(cond) do {} while(0)

#endif

namespace hg {

namespace internal {

/**
 * Report a failed assertion and abort the program
 */
[[noreturn]] inline void assertionFailed(const char* file, i32 line, const char* func, const char* cond)
{
#ifdef HG_LOGGING
    std::fprintf(stderr, "HurdyGurdy Assertion failed in %s:%d %s() \"%s\"\n", file, line, func, cond);
#else
    (void)file;
    (void)line;
    (void)func;
    (void)cond;
#endif
    std::abort();
}

} // namespace internal

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

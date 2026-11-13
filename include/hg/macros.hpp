#pragma once

#include "hg/inttypes.hpp"

#include <cstdio>
#include <cstdlib>
#include <format>
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

namespace hg {

/**
 * Forward declaration
 */
void logError();

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

/**
 * Format a message into a stack buffer
 *
 * The buffer is null terminated on write
 *
 * Returns
 * - The number of characters written
 */
template<typename... Ts>
inline u64 formatStack(char (&buf)[4096], std::format_string<Ts...> fmt, Ts&&... args)
{
    auto res = std::format_to_n(buf, sizeof(buf) - 1, fmt, std::forward<Ts>(args)...);
    *res.out = '\0';
    return static_cast<u64>(res.size) < sizeof(buf)
        ? static_cast<u64>(res.size)
        : sizeof(buf) - 1;
}

} // namespace internal

/**
 * Format a log message to stderr
 */
template<typename... Ts>
inline void logFormat(std::format_string<Ts...> fmt, Ts&&... args)
{
    char buf[4096];
    internal::formatStack(buf, fmt, std::forward<Ts>(args)...);
    std::fprintf(stderr, "HurdyGurdy Log: %s", buf);
}

/**
 * Format a warning to stderr
 */
template<typename... Ts>
inline void warnFormat(std::format_string<Ts...> fmt, Ts&&... args)
{
    char buf[4096];
    internal::formatStack(buf, fmt, std::forward<Ts>(args)...);
    std::fprintf(stderr, "HurdyGurdy Warn: %s", buf);
}

/**
 * Format a panic message to stderr and abort the program
 */
template<typename... Ts>
[[noreturn]] inline void panicFormat(std::format_string<Ts...> fmt, Ts&&... args)
{
    char buf[4096];
    internal::formatStack(buf, fmt, std::forward<Ts>(args)...);
    std::fprintf(stderr, "HurdyGurdy Panic: %s", buf);
    std::abort();
}

} // namespace hg

#define HG_MACRO_CONCAT_INTERNAL(x, y) x##y

#define HG_MACRO_CONCAT(x, y) HG_MACRO_CONCAT_INTERNAL(x, y)

/**
 * Defer a block of code to the end of the scope
 */
#define HG_DEFER(...) [[maybe_unused]] hg::internal::Defer HG_MACRO_CONCAT(defer_, __LINE__){[&]{ __VA_ARGS__ ;}};

#ifdef HG_LOGGING

/**
 * Log to stderr
 */
#define HG_LOG(...) do { ::hg::logFormat(__VA_ARGS__); } while(0)

/**
 * Log a warning to stderr
 */
#define HG_WARN(...) do { ::hg::warnFormat(__VA_ARGS__); } while(0)

/**
 * Log the last error, a message, and abort the program
 */
#define HG_PANIC(...) do { ::hg::logError(); ::hg::panicFormat(__VA_ARGS__); } while(0)

#else

#define HG_LOG(...) do {} while(0)
#define HG_WARN(...) do {} while(0)
#define HG_PANIC(...) do { std::abort(); } while(0)

#endif

#ifdef HG_ASSERTIONS

/**
 * Assert a condition, or panic
 */
#define HG_ASSERT(cond) do { \
    if (!(cond)) \
        HG_PANIC("Assertion failed in {}:{} {}() \"{}\"\n", __FILE__, __LINE__, __func__, #cond); \
} while(0)

#else

#define HG_ASSERT(cond) do {} while(0)

#endif

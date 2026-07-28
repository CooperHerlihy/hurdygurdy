#pragma once

#include <cstdio>
#include <cstdlib>

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

#if !defined(HG_PLATFORM_LINUX) && !defined(HG_PLATFORM_WINDOWS)
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

    Defer(F fnVal)
        : fn(fnVal)
    {}

    ~Defer()
    {
        fn();
    }
};

} // namespace internal

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
#define HG_LOG(...) do { std::fprintf(stderr, "HurdyGurdy Log: " __VA_ARGS__); } while(0)

/**
 * Log a warning to stderr
 */
#define HG_WARN(...) do { std::fprintf(stderr, "HurdyGurdy Warn: " __VA_ARGS__); } while(0)

/**
 * Log a message and the last error, and abort the program
 */
#define HG_PANIC(...) do { logError(); std::fprintf(stderr, "HurdyGurdy Panic: " __VA_ARGS__); std::abort(); \
} while(0)

#else

#define HG_LOG(...) do {} while(0)
#define HG_WARN(...) do {} while(0)
#define HG_PANIC(...) do { abort(); } while(0)

#endif

#ifdef HG_ASSERTIONS

/**
 * Assert a condition, or panic
 */
#define HG_ASSERT(cond) do { \
    if (!(cond)) \
        HG_PANIC("Assertion failed in " __FILE__ ":%d %s() \"" #cond "\"\n", __LINE__, __func__); \
} while(0)

#else

#define HG_ASSERT(cond) do {} while(0)

#endif

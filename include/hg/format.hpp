#pragma once

#include "hg/inttypes.hpp"
#include "hg/utility.hpp"

#include <cstdio>
#include <cstdlib>
#include <format>

namespace hg {

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

} // namespace hg

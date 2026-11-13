#pragma once

#include "hg/inttypes.hpp"
#include "hg/macros.hpp"

#include <format>
#include <string_view>

namespace hg {

/**
 * Get this thread's current error message
 */
std::string_view getError();

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
    u64 len = internal::formatStack(buf, fmt, std::forward<Ts>(args)...);
    setError(std::string_view{buf, len});
}

/**
 * Log this thread's current error message to stderr
 */
void logError();

} // namespace hg

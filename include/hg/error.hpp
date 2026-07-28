#pragma once

#include "hg/inttypes.hpp"
#include "hg/strings.hpp"

#include <cstring>

namespace hg {

/**
 * Get this thread's current error message
 */
StringView getError();

/**
 * Set this thread's current error message
 */
void setError(StringView error);

/**
 * Format and set this thread's current error message
 */
template<typename... Ts> requires (sizeof...(Ts) > 0)
void setError(StringView errorFmt, Ts... args)
{
    char fmt[4096];
    u64 fmtLen = errorFmt.length < sizeof(fmt) - 1
        ? errorFmt.length
        : sizeof(fmt) - 1;
    memcpy(fmt, errorFmt.chars, fmtLen);
    fmt[fmtLen] = 0;

    char buf[4096];
    snprintf(buf, sizeof(buf), fmt, args...);

    setError(buf);
}

/**
 * Log this thread's current error message to stderr
 */
void logError();

} // namespace hg

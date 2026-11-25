#pragma once

#include <string_view>

namespace hg {

/**
 * Set this thread's current error message
 *
 * Note, to format an error message, include logging.hpp
 */
void setError(std::string_view error);

/**
 * Get this thread's current error message
 */
std::string_view getError();

/**
 * Log this thread's current error message to stderr
 */
void printError();

} // namespace hg

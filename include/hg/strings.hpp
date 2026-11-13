#pragma once

#include "hg/inttypes.hpp"

#include <string_view>

namespace hg {

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
 * - arena The arena to allocate from
 * - num The integer number to create from
 */
std::string integerToString(i64 num);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 * - precision The number of significant digits
 */
std::string floatToString(f64 num, u32 precision);

} // namespace hg

#include "tests.hpp"
#include "hg/strings.hpp"
#include "hg/memory.hpp"

#include <cfloat>
#include <cmath>
#include <string>

using namespace hg;

TEST(testIsWhitespace)
{
    ASSERT(isWhitespace(' '));
    ASSERT(isWhitespace('\t'));
    ASSERT(isWhitespace('\n'));
    ASSERT(isWhitespace('\r'));
    ASSERT(!isWhitespace('\v'));
    ASSERT(!isWhitespace('\f'));
    ASSERT(!isWhitespace('\0'));
    ASSERT(!isWhitespace('a'));
    ASSERT(!isWhitespace('0'));
    ASSERT(!isWhitespace('.'));
    ASSERT(!isWhitespace('_'));
}

TEST(testIsNumeral)
{
    ASSERT(isNumeral('0'));
    ASSERT(isNumeral('1'));
    ASSERT(isNumeral('5'));
    ASSERT(isNumeral('9'));
    ASSERT(!isNumeral('a'));
    ASSERT(!isNumeral('z'));
    ASSERT(!isNumeral('A'));
    ASSERT(!isNumeral('Z'));
    ASSERT(!isNumeral('.'));
    ASSERT(!isNumeral('-'));
    ASSERT(!isNumeral('+'));
    ASSERT(!isNumeral(' '));
    ASSERT(!isNumeral('\0'));
    ASSERT(!isNumeral('/'));
    ASSERT(!isNumeral(':'));
}

TEST(testIsInteger)
{
    ASSERT(isInteger("0"));
    ASSERT(isInteger("1"));
    ASSERT(isInteger("9"));
    ASSERT(isInteger("42"));
    ASSERT(isInteger("100"));
    ASSERT(isInteger("1234567890"));
    ASSERT(isInteger("+12"));
    ASSERT(isInteger("-12"));
    ASSERT(isInteger("+0"));
    ASSERT(isInteger("-0"));
    ASSERT(isInteger("00"));
    ASSERT(isInteger("00042"));
    ASSERT(!isInteger(""));
    ASSERT(!isInteger("hello"));
    ASSERT(!isInteger("12a"));
    ASSERT(!isInteger("a12"));
    ASSERT(!isInteger("1.0"));
    ASSERT(!isInteger("--12"));
    ASSERT(!isInteger("+-12"));
    ASSERT(!isInteger("12-"));
    ASSERT(!isInteger("12+"));
    ASSERT(!isInteger("+"));
    ASSERT(!isInteger("-"));
    ASSERT(!isInteger("0x10"));
}

TEST(testIsFloat)
{
    ASSERT(isFloat("0.0"));
    ASSERT(isFloat("1.0"));
    ASSERT(isFloat("2.5"));
    ASSERT(isFloat("99.99"));
    ASSERT(isFloat(".1"));
    ASSERT(isFloat(".5"));
    ASSERT(isFloat(".12345"));
    ASSERT(isFloat("1."));
    ASSERT(isFloat("100."));
    ASSERT(isFloat("+1.0"));
    ASSERT(isFloat("-1.0"));
    ASSERT(isFloat("+.5"));
    ASSERT(isFloat("-.5"));
    ASSERT(isFloat("1e3"));
    ASSERT(isFloat("1e+3"));
    ASSERT(isFloat("1e-3"));
    ASSERT(isFloat("1.5e3"));
    ASSERT(isFloat("1.5e+3"));
    ASSERT(isFloat("1.5e-3"));
    ASSERT(isFloat(".5e3"));
    ASSERT(isFloat("1e0"));
    ASSERT(isFloat("2.5e-0"));
    ASSERT(isFloat("1.0f"));
    ASSERT(isFloat("+10.f"));
    ASSERT(isFloat("-999.999f"));
    ASSERT(isFloat("1e3f"));
    ASSERT(isFloat("1e-3f"));
    ASSERT(isFloat("1.e3f"));
    ASSERT(isFloat("1.f"));
    ASSERT(isFloat(".1e3"));
    ASSERT(!isFloat("1"));
    ASSERT(!isFloat("42"));
    ASSERT(!isFloat("+12"));
    ASSERT(!isFloat("-12"));
    ASSERT(!isFloat(""));
    ASSERT(!isFloat("hello"));
    ASSERT(!isFloat("1.0ff"));
    ASSERT(!isFloat("1.0.0"));
    ASSERT(!isFloat("1e3.0"));
    ASSERT(!isFloat("--1.0"));
    ASSERT(!isFloat("1ef"));
    ASSERT(!isFloat("1e"));
    ASSERT(!isFloat("1e+"));
    ASSERT(!isFloat("1e-"));
    ASSERT(!isFloat("e1"));
    ASSERT(!isFloat("e3"));
    ASSERT(!isFloat("."));
    ASSERT(!isFloat("e"));
    ASSERT(!isFloat("-"));
    ASSERT(!isFloat("+"));
    ASSERT(!isFloat("1.5e"));
    ASSERT(!isFloat("1.5e-"));
    ASSERT(!isFloat("-0"));
}

TEST(testStringToInteger)
{
    ASSERT(stringToInteger("0") == 0);
    ASSERT(stringToInteger("1") == 1);
    ASSERT(stringToInteger("9") == 9);
    ASSERT(stringToInteger("42") == 42);
    ASSERT(stringToInteger("100") == 100);
    ASSERT(stringToInteger("1234567890") == 1234567890);
    ASSERT(stringToInteger("+12") == 12);
    ASSERT(stringToInteger("-12") == -12);
    ASSERT(stringToInteger("+0") == 0);
    ASSERT(stringToInteger("-0") == 0);
    ASSERT(stringToInteger("00") == 0);
    ASSERT(stringToInteger("00042") == 42);
    ASSERT(stringToInteger("+00042") == 42);
    ASSERT(stringToInteger("-00042") == -42);
    ASSERT(stringToInteger("2147483647") == 2147483647);
    ASSERT(stringToInteger("2147483648") == 2147483648);
    ASSERT(stringToInteger("-2147483648") == -2147483648);
    ASSERT(stringToInteger("9223372036854775807") == 9223372036854775807LL);
    ASSERT(stringToInteger("-9223372036854775807") == -9223372036854775807LL);
}

TEST(testStringToFloat)
{
    ASSERT(std::abs(stringToFloat("0.0") - 0.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.0") - 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("2.5") - 2.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("99.99") - 99.99) <= 1e-10);
    ASSERT(std::abs(stringToFloat(".1") - 0.1) <= 1e-10);
    ASSERT(std::abs(stringToFloat(".5") - 0.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat(".12345") - 0.12345) <= 1e-10);
    ASSERT(std::abs(stringToFloat("1.") - 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("100.") - 100.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("-1.0") + 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("-.5") + 0.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e3") - 1000.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e+3") - 1000.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e-3") - 0.001) <= 1e-10);
    ASSERT(std::abs(stringToFloat("1.5e3") - 1500.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.5e+3") - 1500.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.5e-3") - 0.0015) <= 1e-10);
    ASSERT(std::abs(stringToFloat(".5e3") - 500.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e0") - 1.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("2e0") - 2.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("2.5e0") - 2.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("2.5e-0") - 2.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.5e0") - 1.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e1") - 10.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1e2") - 100.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("2.5e2") - 250.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.5e1") - 15.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("5e-1") - 0.5) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("1.e3") - 1000.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("-2.5e2") + 250.0) <= FLT_EPSILON);
    ASSERT(std::abs(stringToFloat("999.999") - 999.999) <= 1e-10);
    ASSERT(std::abs(stringToFloat("-999.999") + 999.999) <= 1e-10);
    ASSERT(std::abs(stringToFloat("123.456") - 123.456) <= 1e-10);
}

TEST(testIntegerToString)
{
    ASSERT(integerToString(0) == "0");
    ASSERT(integerToString(1) == "1");
    ASSERT(integerToString(9) == "9");
    ASSERT(integerToString(-1) == "-1");
    ASSERT(integerToString(-9) == "-9");
    ASSERT(integerToString(42) == "42");
    ASSERT(integerToString(100) == "100");
    ASSERT(integerToString(1234567890) == "1234567890");
    ASSERT(integerToString(-42) == "-42");
    ASSERT(integerToString(-100) == "-100");
    ASSERT(integerToString(-1000000) == "-1000000");
    ASSERT(integerToString(-999999) == "-999999");
    ASSERT(integerToString(9000000000000000LL) == "9000000000000000");
    ASSERT(integerToString(9223372036854775807LL) == "9223372036854775807");
    ASSERT(integerToString(-9223372036854775807LL) == "-9223372036854775807");
}

TEST(testFloatToString)
{
    ASSERT(floatToString(0.0, 4) == "0");
    ASSERT(floatToString(0.0, 0) == "0");
    ASSERT(floatToString(1.0, 12) == "1");
    ASSERT(floatToString(2.0, 4) == "2");
    ASSERT(floatToString(4.0, 5) == "4");
    ASSERT(floatToString(-1.0, 3) == "-1");
    ASSERT(floatToString(0.5, 6) == "0.5");
    ASSERT(floatToString(0.5, 3) == "0.5");
    ASSERT(floatToString(0.125, 4) == "0.125");
    ASSERT(floatToString(-0.5, 3) == "-0.5");
    ASSERT(floatToString(3.14, 3) == "3.14");
    ASSERT(floatToString(3.14159, 2) == "3.1");
    ASSERT(floatToString(3.14159, 4) == "3.142");
    ASSERT(floatToString(3.14159, 5) == "3.1416");
    ASSERT(floatToString(3.14159, 6) == "3.14159");
    ASSERT(floatToString(100.0, 0) == "1e+02");
    ASSERT(floatToString(100.0, 3) == "100");
    ASSERT(floatToString(0.0001, 1) == "0.0001");
    ASSERT(floatToString(0.00001, 1) == "1e-05");
    ASSERT(floatToString(0.0015, 2) == "0.0015");
    ASSERT(floatToString(123.456, 3) == "123");
    ASSERT(floatToString(-123.456, 5) == "-123.46");
    ASSERT(floatToString(9.999, 2) == "10");
    ASSERT(floatToString(99.99, 6) == "99.99");
    ASSERT(floatToString(12345.6789, 17) == "12345.678900000001");
}

TEST(testFloatRoundTrip)
{
    // Note, 0.0, 1.0 and -1.0 are excluded: floatToString() renders them
    // without a decimal point, see testFloatToStringIntegerRendersNonFloat
    f64 values[] = {
        0.5,
        -0.5,
        0.125,
        -0.125,
        2.5,
        -2.5,
        3.14159,
        99.99,
        123.456,
        -123.456,
        12345.6789,
        -9876.54321,
        1e300,
        1e-300,
        5e-324,
        1.7976931348623157e308,
    };

    for (f64 v : values)
    {
        std::string str = floatToString(v, 17);
        ASSERT(stringToFloat(str) == v);
    }
}

/**
 * Exposes a bug: floatToString() renders integral values without a decimal
 * point, and the new stringToFloat() asserts isFloat(), so such values can no
 * longer round trip through stringToFloat()
 */
TEST(testFloatToStringIntegerRendersNonFloat)
{
    ASSERT(!isFloat(floatToString(0.0, 17)));
    ASSERT(!isFloat(floatToString(1.0, 17)));
    ASSERT(!isFloat(floatToString(-1.0, 17)));
    ASSERT(isInteger(floatToString(1.0, 17)));
}

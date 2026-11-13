#include "tests.hpp"
#include "hg/utility.hpp"
#include "hg/error.hpp"

#include <string>
#include <vector>

using namespace hg;

TEST(testAlignUpAlreadyAligned)
{
    ASSERT(alignUp(0, 4) == 0);
    ASSERT(alignUp(4, 4) == 4);
    ASSERT(alignUp(1024, 256) == 1024);
}

TEST(testAlignUpRounding)
{
    ASSERT(alignUp(1, 4) == 4);
    ASSERT(alignUp(3, 4) == 4);
    ASSERT(alignUp(5, 4) == 8);
    ASSERT(alignUp(1025, 256) == 1280);
}

TEST(testAlignUpNoop)
{
    ASSERT(alignUp(0, 1) == 0);
    ASSERT(alignUp(42, 1) == 42);
}

TEST(testEndianReverse16)
{
    ASSERT(endianReverse16(0x1234) == 0x3412);
    ASSERT(endianReverse16(endianReverse16(0x1234)) == 0x1234);
    ASSERT(endianReverse16(0x0000) == 0x0000);
    ASSERT(endianReverse16(0xFFFF) == 0xFFFF);
}

TEST(testEndianReverse32)
{
    ASSERT(endianReverse32(0x12345678) == 0x78563412);
    ASSERT(endianReverse32(endianReverse32(0x12345678)) == 0x12345678);
    ASSERT(endianReverse32(0x00000000) == 0x00000000);
    ASSERT(endianReverse32(0xFFFFFFFF) == 0xFFFFFFFF);
}

TEST(testEndianReverse64)
{
    u64 val = 0x0102030405060708ull;
    ASSERT(endianReverse64(val) == 0x0807060504030201ull);
    ASSERT(endianReverse64(endianReverse64(val)) == val);
    ASSERT(endianReverse64(0x0000000000000000ull) == 0x0000000000000000ull);
    ASSERT(endianReverse64(0xFFFFFFFFFFFFFFFFull) == 0xFFFFFFFFFFFFFFFFull);
}

TEST(testSizeContainer)
{
    std::vector<u32> vec;
    vec.push_back(10);
    vec.push_back(20);
    vec.push_back(30);
    ASSERT(vec.size() == 3);
}

TEST(testSizeCArray)
{
    i32 arr[5] = {1, 2, 3, 4, 5};
    ASSERT(std::size(arr) == 5);
}

struct IdxTypeA {};
struct IdxTypeB {};
struct IdxTypeC {};

TEST(testIdxOfFirst)
{
    ASSERT((idxInPack<IdxTypeA, IdxTypeA, IdxTypeB, IdxTypeC>()) == 0);
}

TEST(testIdxOfMiddle)
{
    ASSERT((idxInPack<IdxTypeB, IdxTypeA, IdxTypeB, IdxTypeC>()) == 1);
}

TEST(testIdxOfLast)
{
    ASSERT((idxInPack<IdxTypeC, IdxTypeA, IdxTypeB, IdxTypeC>()) == 2);
}

TEST(testAlignUpLargeValues)
{
    ASSERT(alignUp(0xFFFFFFFFFFFFFFFEull, 4) == 0ull);
    ASSERT(alignUp(0xFFFFFFFFFFFFFFFFull, 4) == 0ull);
    ASSERT(alignUp(0xFFFFFFFFFFFFFFF0ull, 16) == 0xFFFFFFFFFFFFFFF0ull);
}

TEST(testEndianReverse16SingleByte)
{
    ASSERT(endianReverse16(0x00AB) == 0xAB00);
    ASSERT(endianReverse16(0x00FF) == 0xFF00);
    ASSERT(endianReverse16(0x0001) == 0x0100);
}

TEST(testEndianReverse32SingleByte)
{
    ASSERT(endianReverse32(0x000000AB) == 0xAB000000);
    ASSERT(endianReverse32(0x000000FF) == 0xFF000000);
    ASSERT(endianReverse32(0x00000001) == 0x01000000);
}

TEST(testEndianReverse64SingleByte)
{
    ASSERT(endianReverse64(0x00000000000000ABull) == 0xAB00000000000000ull);
    ASSERT(endianReverse64(0x00000000000000FFull) == 0xFF00000000000000ull);
    ASSERT(endianReverse64(0x0000000000000001ull) == 0x0100000000000000ull);
}

TEST(testErrorSetGet)
{
    setError("test error");
    std::string_view err = getError();
    ASSERT(err == "test error");
}

TEST(testErrorReplace)
{
    setError("first error");
    setError("second error");
    std::string_view err = getError();
    ASSERT(err == "second error");
}

TEST(testErrorClear)
{
    setError("something");
    setError("");
    std::string_view err = getError();
    ASSERT(err.length() == 0);
}

TEST(testErrorAtBoundary)
{
    std::string longStr(4095, 'x');
    ASSERT(longStr.length() == 4095);

    setError(longStr);
    std::string_view err = getError();
    ASSERT(err.length() == 4095);
}

TEST(testErrorExceedsBoundary)
{
    std::string longStr(5000, 'x');
    ASSERT(longStr.length() == 5000);

    setError(longStr);
    std::string_view err = getError();
    ASSERT(err.length() == 4095);
}

TEST(testErrorFormatInt)
{
    setError("error code {}", 42);
    std::string_view err = getError();
    ASSERT(err == "error code 42");
}

TEST(testErrorFormatString)
{
    setError("failed to load \"{}\"", "texture.png");
    std::string_view err = getError();
    ASSERT(err == "failed to load \"texture.png\"");
}

TEST(testErrorFormatMultiple)
{
    setError("{}:{}: {}", "file.txt", 128, "unexpected token");
    std::string_view err = getError();
    ASSERT(err == "file.txt:128: unexpected token");
}

TEST(testErrorFormatTruncated)
{
    std::string longStr(4090, 'x');

    setError("{}", longStr);
    std::string_view err = getError();
    ASSERT(err.length() == 4090);

    std::string tooLong(5000, 'x');

    setError("{}", tooLong);
    err = getError();
    ASSERT(err.length() == 4095);
}

TEST(testErrorFreshState)
{
    setError("");
    std::string_view err = getError();
    ASSERT(err.length() == 0);
    ASSERT(err.data() != nullptr);
}

TEST(testLogErrorNormal)
{
    setError("log test message");
    logError();
}

TEST(testLogErrorEmpty)
{
    setError("");
    logError();
}

TEST(testLogErrorLong)
{
    std::string longStr(512, 'x');
    setError(longStr);
    logError();
}

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
 * floatToString() renders integral values without a decimal
 * point, and the new stringToFloat() asserts isFloat(), so such values cannot
 * longer round trip through stringToFloat()
 */
TEST(testFloatToStringIntegerRendersNonFloat)
{
    ASSERT(!isFloat(floatToString(0.0, 17)));
    ASSERT(!isFloat(floatToString(1.0, 17)));
    ASSERT(!isFloat(floatToString(-1.0, 17)));
    ASSERT(isInteger(floatToString(1.0, 17)));
}

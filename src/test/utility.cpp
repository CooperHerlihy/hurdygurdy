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
    ASSERT((idxOfInPack<IdxTypeA, IdxTypeA, IdxTypeB, IdxTypeC>()) == 0);
}

TEST(testIdxOfMiddle)
{
    ASSERT((idxOfInPack<IdxTypeB, IdxTypeA, IdxTypeB, IdxTypeC>()) == 1);
}

TEST(testIdxOfLast)
{
    ASSERT((idxOfInPack<IdxTypeC, IdxTypeA, IdxTypeB, IdxTypeC>()) == 2);
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

#include "tests.hpp"
#include "hg/utility.hpp"
#include "hg/error.hpp"

#include <vector>

using namespace hg;

TEST(testIsPowerOf2True)
{
    ASSERT(isPowerOf2(1));
    ASSERT(isPowerOf2(2));
    ASSERT(isPowerOf2(4));
    ASSERT(isPowerOf2(1024));
    ASSERT(isPowerOf2(0x80000000ull));
}

TEST(testIsPowerOf2False)
{
    ASSERT(!isPowerOf2(0));
    ASSERT(!isPowerOf2(3));
    ASSERT(!isPowerOf2(5));
    ASSERT(!isPowerOf2(1023));
    ASSERT(!isPowerOf2(0x80000001ull));
}

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
    ASSERT(hg::size(vec) == 3);
}

TEST(testSizeCArray)
{
    i32 arr[5] = {1, 2, 3, 4, 5};
    ASSERT(size(arr) == 5);
}

struct IdxTypeA {};
struct IdxTypeB {};
struct IdxTypeC {};

TEST(testIdxOfFirst)
{
    ASSERT((idxOf<IdxTypeA, IdxTypeA, IdxTypeB, IdxTypeC>()) == 0);
}

TEST(testIdxOfMiddle)
{
    ASSERT((idxOf<IdxTypeB, IdxTypeA, IdxTypeB, IdxTypeC>()) == 1);
}

TEST(testIdxOfLast)
{
    ASSERT((idxOf<IdxTypeC, IdxTypeA, IdxTypeB, IdxTypeC>()) == 2);
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
    StringView err = getError();
    ASSERT(err == "test error");
}

TEST(testErrorReplace)
{
    setError("first error");
    setError("second error");
    StringView err = getError();
    ASSERT(err == "second error");
}

TEST(testErrorClear)
{
    setError("something");
    setError("");
    StringView err = getError();
    ASSERT(err.length == 0);
}

TEST(testErrorAtBoundary)
{
    ArenaScope arena = getScratch();
    StringBuilder longStr{arena};
    for (u32 i = 0; i < 4096; ++i)
        longStr.append('x');
    ASSERT(longStr.length == 4096);

    setError(longStr);
    StringView err = getError();
    ASSERT(err.length == 4096);
}

TEST(testErrorExceedsBoundary)
{
    ArenaScope arena = getScratch();
    StringBuilder longStr{arena};
    for (u32 i = 0; i < 5000; ++i)
        longStr.append('x');
    ASSERT(longStr.length == 5000);

    setError(longStr);
    StringView err = getError();
    ASSERT(err.length == 4096);
}

TEST(testErrorFormatInt)
{
    setError("error code %d", 42);
    StringView err = getError();
    ASSERT(err == "error code 42");
}

TEST(testErrorFormatString)
{
    setError("failed to load \"%s\"", "texture.png");
    StringView err = getError();
    ASSERT(err == "failed to load \"texture.png\"");
}

TEST(testErrorFormatMultiple)
{
    setError("%s:%d: %s", "file.txt", 128, "unexpected token");
    StringView err = getError();
    ASSERT(err == "file.txt:128: unexpected token");
}

TEST(testErrorFormatTruncated)
{
    ArenaScope arena = getScratch();
    StringBuilder longStr{arena};
    for (u32 i = 0; i < 4090; ++i)
        longStr.append('x');

    setError("%.*s", (int)longStr.length, longStr.chars);
    StringView err = getError();
    ASSERT(err.length == 4090);

    StringBuilder tooLong{arena};
    for (u32 i = 0; i < 5000; ++i)
        tooLong.append('x');

    setError("%.*s", (int)tooLong.length, tooLong.chars);
    err = getError();
    ASSERT(err.length == 4095);
}

TEST(testErrorFreshState)
{
    setError("");
    StringView err = getError();
    ASSERT(err.length == 0);
    ASSERT(err.chars != nullptr);
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
    ArenaScope arena = getScratch();
    StringBuilder longStr{arena};
    for (u32 i = 0; i < 512; ++i)
        longStr.append('x');
    setError(longStr);
    logError();
}

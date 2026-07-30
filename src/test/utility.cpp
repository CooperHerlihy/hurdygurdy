#include "tests.hpp"
#include "hg/utility.hpp"

void testUtils()
{
    // ============================================================================
    // Utility
    // ============================================================================
    //
    // Utility functions: isPowerOf2, alignUp, endianReverse16/32/64.

    // ------------------------------------------------------------------
    // isPowerOf2
    // ------------------------------------------------------------------

    // Powers of two
    {
        TEST(isPowerOf2(1));
        TEST(isPowerOf2(2));
        TEST(isPowerOf2(4));
        TEST(isPowerOf2(1024));
        TEST(isPowerOf2(0x80000000ull));
    }

    // Not powers of two
    {
        TEST(!isPowerOf2(0));
        TEST(!isPowerOf2(3));
        TEST(!isPowerOf2(5));
        TEST(!isPowerOf2(1023));
        TEST(!isPowerOf2(0x80000001ull));
    }

    // ------------------------------------------------------------------
    // alignUp
    // ------------------------------------------------------------------

    // Already aligned
    {
        TEST(alignUp(0, 4) == 0);
        TEST(alignUp(4, 4) == 4);
        TEST(alignUp(1024, 256) == 1024);
    }

    // Needs rounding up
    {
        TEST(alignUp(1, 4) == 4);
        TEST(alignUp(3, 4) == 4);
        TEST(alignUp(5, 4) == 8);
        TEST(alignUp(1025, 256) == 1280);
    }

    // Alignment of 1 is a no-op
    {
        TEST(alignUp(0, 1) == 0);
        TEST(alignUp(42, 1) == 42);
    }

    // ------------------------------------------------------------------
    // endianReverse
    // ------------------------------------------------------------------

    // endianReverse16
    {
        TEST(endianReverse16(0x1234) == 0x3412);
        TEST(endianReverse16(endianReverse16(0x1234)) == 0x1234);
        TEST(endianReverse16(0x0000) == 0x0000);
        TEST(endianReverse16(0xFFFF) == 0xFFFF);
    }

    // endianReverse32
    {
        TEST(endianReverse32(0x12345678) == 0x78563412);
        TEST(endianReverse32(endianReverse32(0x12345678)) == 0x12345678);
        TEST(endianReverse32(0x00000000) == 0x00000000);
        TEST(endianReverse32(0xFFFFFFFF) == 0xFFFFFFFF);
    }

    // endianReverse64
    {
        u64 val = 0x0102030405060708ull;
        TEST(endianReverse64(val) == 0x0807060504030201ull);
        TEST(endianReverse64(endianReverse64(val)) == val);
        TEST(endianReverse64(0x0000000000000000ull) == 0x0000000000000000ull);
        TEST(endianReverse64(0xFFFFFFFFFFFFFFFFull) == 0xFFFFFFFFFFFFFFFFull);
    }
}


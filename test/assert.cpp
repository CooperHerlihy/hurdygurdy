#include "tests.hpp"
#include "hg/assert.hpp"

#include <limits>

using namespace hg;

TEST(testAssertCastIdentity)
{
    static_assert(assertCast<i8>(i8(-5)) == -5);
    static_assert(assertCast<u32>(7u) == 7u);
    static_assert(assertCast<f64>(2.5) == 2.5);
    ASSERT(assertCast<i64>(-5) == -5);
    ASSERT(assertCast<u32>(7u) == 7u);
    ASSERT(assertCast<f64>(2.5) == 2.5);
    ASSERT(assertCast<f32>(-0.5f) == -0.5f);
}

TEST(testAssertCastWiden)
{
    static_assert(assertCast<i64>(i8(-100)) == -100);
    static_assert(assertCast<u32>(u8(200)) == 200u);
    static_assert(assertCast<f64>(1.5f) == 1.5);
    ASSERT(assertCast<i64>(i8(-100)) == -100);
    ASSERT(assertCast<u64>(u8(200)) == 200u);
    ASSERT(assertCast<f64>(1.5f) == 1.5);
    ASSERT(assertCast<i32>(u8(255)) == 255);
}

TEST(testAssertCastNarrow)
{
    static_assert(assertCast<i32>(100LL) == 100);
    static_assert(assertCast<u8>(42u) == 42u);
    static_assert(assertCast<f32>(2.5) == 2.5f);
    ASSERT(assertCast<i32>(100LL) == 100);
    ASSERT(assertCast<i16>(-30000) == -30000);
    ASSERT(assertCast<u8>(42u) == 42u);
    ASSERT(assertCast<f32>(2.5) == 2.5f);
}

TEST(testAssertCastUpperBoundary)
{
    static_assert(assertCast<i64>(std::numeric_limits<u32>::max()) == 4294967295LL);
    static_assert(assertCast<i32>(2147483647LL) == std::numeric_limits<i32>::max());
    static_assert(assertCast<u8>(255u) == 255u);
    static_assert(assertCast<f32>(std::numeric_limits<f32>::max()) == std::numeric_limits<f32>::max());
    ASSERT(assertCast<i64>(std::numeric_limits<u32>::max()) == 4294967295LL);
    ASSERT(assertCast<i32>(2147483647LL) == std::numeric_limits<i32>::max());
    ASSERT(assertCast<u8>(255u) == 255u);
    ASSERT(assertCast<i8>(127) == std::numeric_limits<i8>::max());
    ASSERT(assertCast<f32>(std::numeric_limits<f32>::max()) == std::numeric_limits<f32>::max());
}

TEST(testAssertCastLowerBoundary)
{
    static_assert(assertCast<i32>(-2147483648LL) == std::numeric_limits<i32>::min());
    static_assert(assertCast<i8>(-128) == std::numeric_limits<i8>::min());
    static_assert(assertCast<u32>(0u) == 0u);
    static_assert(assertCast<u64>(0LL) == 0uLL);
    ASSERT(assertCast<i32>(-2147483648LL) == std::numeric_limits<i32>::min());
    ASSERT(assertCast<i8>(-128) == std::numeric_limits<i8>::min());
    ASSERT(assertCast<u32>(0u) == 0u);
    ASSERT(assertCast<u64>(0LL) == 0uLL);
    ASSERT(assertCast<f64>(std::numeric_limits<f32>::lowest()) == std::numeric_limits<f32>::lowest());
}

TEST(testAssertCastSignedUnsigned)
{
    static_assert(assertCast<u32>(123) == 123u);
    static_assert(assertCast<i32>(123u) == 123);
    static_assert(assertCast<u64>(-0) == 0u);
    ASSERT(assertCast<u32>(123) == 123u);
    ASSERT(assertCast<i32>(123u) == 123);
    ASSERT(assertCast<u64>(i64(1)) == 1u);
    ASSERT(assertCast<i64>(1u) == 1);
    ASSERT(assertCast<u32>(i32(0)) == 0u);
    ASSERT(assertCast<u8>(i32(0)) == 0u);
}

TEST(testAssertCastFloatToInt)
{
    static_assert(assertCast<i32>(3.0) == 3);
    static_assert(assertCast<i64>(-4.0) == -4);
    ASSERT(assertCast<i32>(3.0) == 3);
    ASSERT(assertCast<i64>(-4.0) == -4);
    ASSERT(assertCast<u32>(7.0) == 7u);
    ASSERT(assertCast<i32>(3.9) == 3);
    ASSERT(assertCast<i32>(-3.9) == -3);
}

TEST(testAssertCastIntToFloat)
{
    static_assert(assertCast<f64>(-3) == -3.0);
    static_assert(assertCast<f32>(100) == 100.0f);
    ASSERT(assertCast<f64>(-3) == -3.0);
    ASSERT(assertCast<f32>(100) == 100.0f);
    ASSERT(assertCast<f64>(1LL << 53) == 9007199254740992.0);
    ASSERT(assertCast<f64>(1LL << 62) == 4611686018427387904.0);
}

TEST(testAssertCastReferenceBinding)
{
    const f64 val = 4.5;
    ASSERT(assertCast<i32>(val) == 4);
    const i64 small = 1000;
    ASSERT(assertCast<u16>(small) == 1000u);
}



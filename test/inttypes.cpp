#include "tests.hpp"
#include "hg/inttypes.hpp"

#include <limits>

using namespace hg;

// Compile time checks: the aliases must be exact width and signedness
static_assert(sizeof(u8) == 1 && std::is_unsigned_v<u8>);
static_assert(sizeof(u16) == 2 && std::is_unsigned_v<u16>);
static_assert(sizeof(u32) == 4 && std::is_unsigned_v<u32>);
static_assert(sizeof(u64) == 8 && std::is_unsigned_v<u64>);
static_assert(sizeof(i8) == 1 && std::is_signed_v<i8>);
static_assert(sizeof(i16) == 2 && std::is_signed_v<i16>);
static_assert(sizeof(i32) == 4 && std::is_signed_v<i32>);
static_assert(sizeof(i64) == 8 && std::is_signed_v<i64>);
static_assert(sizeof(f32) == 4 && std::is_floating_point_v<f32>);
static_assert(sizeof(f64) == 8 && std::is_floating_point_v<f64>);
static_assert(std::is_same_v<usize, std::size_t>);
static_assert(std::is_same_v<byte, std::byte>);

TEST(testInttypesLimits)
{
    ASSERT(std::numeric_limits<u8>::max() == 0xFFu);
    ASSERT(std::numeric_limits<u16>::max() == 0xFFFFu);
    ASSERT(std::numeric_limits<u32>::max() == 0xFFFFFFFFu);
    ASSERT(std::numeric_limits<u64>::max() == 0xFFFFFFFFFFFFFFFFull);
    ASSERT(std::numeric_limits<i8>::min() == -128);
    ASSERT(std::numeric_limits<i8>::max() == 127);
    ASSERT(std::numeric_limits<i16>::min() == -32768);
    ASSERT(std::numeric_limits<i16>::max() == 32767);
    ASSERT(std::numeric_limits<i32>::min() == -2147483648);
    ASSERT(std::numeric_limits<i32>::max() == 2147483647);
    ASSERT(std::numeric_limits<f32>::is_iec559);
    ASSERT(std::numeric_limits<f64>::is_iec559);
}

TEST(testInttypesArithmetic)
{
    u32 u = 0;
    u -= 1;
    ASSERT(u == std::numeric_limits<u32>::max());
    i8 i = 127;
    i += 1;
    ASSERT(i == -128);
    f64 f = 0.1 + 0.2;
    ASSERT(f > 0.3 && f < 0.30000001);
    byte b{0x0F};
    b <<= 4;
    ASSERT(b == byte{0xF0});
}

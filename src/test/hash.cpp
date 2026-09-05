#include "tests.hpp"
#include "hg/hash.hpp"
#include "hg/strings.hpp"

using namespace hg;

TEST(testHashStringConsistency)
{
    ArenaScope arena = getScratch();
    const char* cstr = "hello";
    StringView sv{cstr};
    String s = String::create(sv);
    StringBuilder sb{arena, sv};

    u64 hSv = hash(sv);
    u64 hStr = hash(s);
    u64 hCstr = hash(cstr);
    u64 hSb = hash(sb);
    u64 hLit = hash("hello");

    ASSERT(hSv == hStr);
    ASSERT(hSv == hCstr);
    ASSERT(hSv == hSb);
    ASSERT(hSv == hLit);
}

TEST(testHashEmptyString)
{
    StringView sv{};
    String s = String::create(sv);
    u64 hSv = hash(sv);
    u64 hStr = hash(s);
    u64 hLit = hash("");
    ASSERT(hSv == hStr);
    ASSERT(hSv == hLit);
}

TEST(testHashCArray)
{
    char buf[] = "abc";
    u64 hArr = hash(buf);
    u64 hPtr = hash((const char*)buf);
    ASSERT(hArr == hPtr);
}

TEST(testHashStringViewLength)
{
    const char* data = "hello world";
    StringView sv{data, 5};
    u64 hLen = hash(sv);
    u64 hTerm = hash(StringView{"hello"});
    ASSERT(hLen == hTerm);
}

TEST(testHashDistinct)
{
    ASSERT(hash(StringView{"abc"}) != hash(StringView{"abd"}));
    ASSERT(hash(StringView{""}) != hash(StringView{"a"}));
}

TEST(testHashInteger)
{
    ASSERT(hash(static_cast<u32>(5)) == hash(static_cast<u32>(5)));
    ASSERT(hash(static_cast<u32>(5)) != hash(static_cast<u32>(6)));
    ASSERT(hash(static_cast<i32>(-1)) == hash(static_cast<i32>(-1)));
    ASSERT(hash(static_cast<i32>(-1)) != hash(static_cast<i32>(1)));
}

TEST(testHashU8)
{
    ASSERT(hash(static_cast<u8>(0)) == static_cast<u64>(0));
    ASSERT(hash(static_cast<u8>(255)) == static_cast<u64>(255));
    ASSERT(hash(static_cast<u8>(42)) == hash(static_cast<u8>(42)));
    ASSERT(hash(static_cast<u8>(42)) != hash(static_cast<u8>(43)));
}

TEST(testHashU16)
{
    ASSERT(hash(static_cast<u16>(0)) == static_cast<u64>(0));
    ASSERT(hash(static_cast<u16>(65535)) == static_cast<u64>(65535));
    ASSERT(hash(static_cast<u16>(123)) == hash(static_cast<u16>(123)));
    ASSERT(hash(static_cast<u16>(123)) != hash(static_cast<u16>(124)));
}

TEST(testHashU64)
{
    ASSERT(hash(static_cast<u64>(0)) == static_cast<u64>(0));
    ASSERT(hash(static_cast<u64>(0xFFFFFFFFFFFFFFFF)) == static_cast<u64>(0xFFFFFFFFFFFFFFFF));
    ASSERT(hash(static_cast<u64>(999)) == hash(static_cast<u64>(999)));
    ASSERT(hash(static_cast<u64>(999)) != hash(static_cast<u64>(1000)));
}

TEST(testHashI8)
{
    ASSERT(hash(static_cast<i8>(0)) == static_cast<u64>(0));
    ASSERT(hash(static_cast<i8>(-1)) == static_cast<u64>(static_cast<u64>(-1)));
    ASSERT(hash(static_cast<i8>(-128)) == hash(static_cast<i8>(-128)));
    ASSERT(hash(static_cast<i8>(127)) == hash(static_cast<i8>(127)));
    ASSERT(hash(static_cast<i8>(-1)) != hash(static_cast<i8>(1)));
}

TEST(testHashI16)
{
    ASSERT(hash(static_cast<i16>(0)) == static_cast<u64>(0));
    ASSERT(hash(static_cast<i16>(-1)) == static_cast<u64>(static_cast<u64>(-1)));
    ASSERT(hash(static_cast<i16>(-32768)) == hash(static_cast<i16>(-32768)));
    ASSERT(hash(static_cast<i16>(32767)) == hash(static_cast<i16>(32767)));
    ASSERT(hash(static_cast<i16>(-32768)) != hash(static_cast<i16>(32767)));
}

TEST(testHashI64)
{
    ASSERT(hash(static_cast<i64>(0)) == static_cast<u64>(0));
    ASSERT(hash(static_cast<i64>(-1)) == static_cast<u64>(static_cast<u64>(-1)));
    ASSERT(hash(static_cast<i64>(-9223372036854775807LL - 1)) == hash(static_cast<i64>(-9223372036854775807LL - 1)));
    ASSERT(hash(static_cast<i64>(-1)) != hash(static_cast<i64>(1)));
}

TEST(testHashFloat)
{
    ASSERT(hash(static_cast<f32>(0.0f)) == hash(static_cast<f32>(0.0f)));
    ASSERT(hash(static_cast<f32>(1.0f)) == hash(static_cast<f32>(1.0f)));
    ASSERT(hash(static_cast<f32>(1.0f)) != hash(static_cast<f32>(2.0f)));
    ASSERT(hash(static_cast<f32>(-1.0f)) == hash(static_cast<f32>(-1.0f)));
    ASSERT(hash(static_cast<f32>(0.0f)) != hash(static_cast<f32>(-0.0f)));
}

TEST(testHashDouble)
{
    ASSERT(hash(static_cast<f64>(0.0)) == hash(static_cast<f64>(0.0)));
    ASSERT(hash(static_cast<f64>(1.0)) == hash(static_cast<f64>(1.0)));
    ASSERT(hash(static_cast<f64>(1.0)) != hash(static_cast<f64>(2.0)));
    ASSERT(hash(static_cast<f64>(-1.0)) == hash(static_cast<f64>(-1.0)));
    ASSERT(hash(static_cast<f64>(0.0)) != hash(static_cast<f64>(-0.0)));
}

TEST(testHashPointer)
{
    int a = 1;
    int b = 2;
    void* pa = &a;
    void* pb = &b;
    ASSERT(hash(pa) == hash(pa));
    ASSERT(hash(pa) != hash(pb));
    ASSERT(hash((void*)nullptr) == hash((void*)nullptr));
    ASSERT(hash(pa) != hash((void*)nullptr));
}

TEST(testHashStringViewEmpty)
{
    StringView sv{};
    u64 h = hash(sv);
    ASSERT(h == hash(StringView{}));
    ASSERT(h == 0);
}

TEST(testHashStringViewNonEmpty)
{
    const char* data = "test";
    StringView sv{data};
    u64 hSv = hash(sv);
    u64 hContent = hash(StringView{"test"});
    ASSERT(hSv == hContent);
    ASSERT(hSv != 0);
}

TEST(testHashDistinctStrings)
{
    ASSERT(hash(StringView{"aaa"}) != hash(StringView{"aab"}));
    ASSERT(hash(StringView{"a"}) != hash(StringView{"b"}));
    ASSERT(hash(StringView{"ab"}) != hash(StringView{"ba"}));
    ASSERT(hash(StringView{"hello"}) != hash(StringView{"world"}));
    ASSERT(hash(StringView{""}) != hash(StringView{"x"}));
}

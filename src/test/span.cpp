#include "tests.hpp"
#include "hg/span.hpp"

using namespace hg;

TEST(testSpanDefault)
{
    Span<i32> s;
    ASSERT(s.data == nullptr);
    ASSERT(s.count == 0);
}

TEST(testSpanFromPtrCount)
{
    i32 vals[3] = {10, 20, 30};
    Span<i32> s{vals, 3};
    ASSERT(s.data == vals);
    ASSERT(s.count == 3);
    ASSERT(s[0] == 10);
    ASSERT(s[2] == 30);
}

TEST(testSpanFromBeginEnd)
{
    i32 vals[3] = {10, 20, 30};
    Span<i32> s{vals, vals + 3};
    ASSERT(s.count == 3);
    ASSERT(s[0] == 10);
    ASSERT(s[1] == 20);
}

TEST(testSpanFromArray)
{
    i32 vals[3] = {10, 20, 30};
    Span<i32> s = vals;
    ASSERT(s.count == 3);
    ASSERT(s[0] == 10);
}

TEST(testSpanRangeFor)
{
    i32 vals[4] = {1, 2, 3, 4};
    Span<i32> s{vals, 4};
    i32 sum = 0;
    for (i32 v : s)
        sum += v;
    ASSERT(sum == 10);
}

TEST(testSpanBeginEnd)
{
    i32 vals[2] = {100, 200};
    Span<i32> s{vals, 2};
    ASSERT(s.begin() == vals);
    ASSERT(s.end() == vals + 2);
}

TEST(testSpanVoidDefault)
{
    Span<void> s;
    ASSERT(s.data == nullptr);
    ASSERT(s.size == 0);
}

TEST(testSpanVoidFromPtr)
{
    f32 vals[3] = {1.0f, 2.0f, 3.0f};
    Span<void> s{static_cast<void*>(vals), 3};
    ASSERT(s.data == vals);
    ASSERT(s.size == 3);
}

TEST(testSpanVoidFromBeginEnd)
{
    u8 data[4] = {10, 20, 30, 40};
    Span<void> s{data, data + 4};
    ASSERT(s.size == 4);
    void* ptr = s[2];
    ASSERT(ptr == static_cast<void*>(data + 2));
}

TEST(testSpanConstAccess)
{
    i32 vals[3] = {10, 20, 30};
    const Span<i32> s{vals, 3};
    ASSERT(s[0] == 10);
    ASSERT(s[1] == 20);
    ASSERT(s[2] == 30);
}

TEST(testSpanMutateViaRef)
{
    i32 vals[3] = {1, 2, 3};
    Span<i32> s{vals, 3};
    s[0] = 100;
    s[2] = 300;
    ASSERT(vals[0] == 100);
    ASSERT(vals[1] == 2);
    ASSERT(vals[2] == 300);
}

TEST(testSpanSingleElement)
{
    i32 val = 42;
    Span<i32> s{&val, 1};
    ASSERT(s.count == 1);
    ASSERT(s[0] == 42);
    s[0] = 99;
    ASSERT(val == 99);
    ASSERT(s.begin() + 1 == s.end());
}

TEST(testSpanRangeForEmpty)
{
    Span<i32> s;
    i32 sum = 0;
    for (i32 v : s)
        sum += v;
    ASSERT(sum == 0);
}

TEST(testSpanVoidOperator)
{
    u8 data[8] = {0, 1, 2, 3, 4, 5, 6, 7};
    Span<void> s{static_cast<void*>(data), 8};
    ASSERT(s[0] == static_cast<void*>(data + 0));
    ASSERT(s[3] == static_cast<void*>(data + 3));
    ASSERT(s[7] == static_cast<void*>(data + 7));
}

TEST(testSpanBeginEndSame)
{
    Span<i32> s;
    ASSERT(s.begin() == s.end());
}

TEST(testSpanFromNullptr)
{
    Span<i32> s{static_cast<i32*>(nullptr), u64{0}};
    ASSERT(s.data == nullptr);
    ASSERT(s.count == 0);
    ASSERT(s.begin() == s.end());
}

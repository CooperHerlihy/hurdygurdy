#include "tests.hpp"
#include "hg/span.hpp"

void testSpan()
{
    // ============================================================================
    // Span<T>
    // ============================================================================
    //
    // Span is a non-owning typed view (pointer + count). Supports array,
    // ptr+count, begin+end constructors, indexing, and range-for.

    // Default-constructed Span is empty
    {
        Span<i32> s;
        TEST(s.data == nullptr);
        TEST(s.count == 0);
    }

    // Construct from pointer and count
    {
        i32 vals[3] = {10, 20, 30};
        Span<i32> s{vals, 3};
        TEST(s.data == vals);
        TEST(s.count == 3);
        TEST(s[0] == 10);
        TEST(s[2] == 30);
    }

    // Construct from begin and end
    {
        i32 vals[3] = {10, 20, 30};
        Span<i32> s{vals, vals + 3};
        TEST(s.count == 3);
        TEST(s[0] == 10);
        TEST(s[1] == 20);
    }

    // Construct from array
    {
        i32 vals[3] = {10, 20, 30};
        Span<i32> s = vals;
        TEST(s.count == 3);
        TEST(s[0] == 10);
    }

    // Range-for over Span
    {
        i32 vals[4] = {1, 2, 3, 4};
        Span<i32> s{vals, 4};
        i32 sum = 0;
        for (i32 v : s)
            sum += v;
        TEST(sum == 10);
    }

    // begin() / end() give correct boundaries
    {
        i32 vals[2] = {100, 200};
        Span<i32> s{vals, 2};
        TEST(s.begin() == vals);
        TEST(s.end() == vals + 2);
    }

    // ============================================================================
    // Span<void>
    // ============================================================================
    //
    // Span<void> is a type-erased non-owning view. Same constructors as
    // Span<T> but indexing returns void*.
    //
    // Functions covered:
    // - Span<void>() — default
    // - Span<void>(void*, u64) — ptr + count
    // - Span<void>(void*, void*) — begin + end
    // - operator[]

    // Default-constructed Span<void> is empty
    {
        Span<void> s;
        TEST(s.data == nullptr);
        TEST(s.size == 0);
    }

    // Construct from pointer and count
    {
        f32 vals[3] = {1.0f, 2.0f, 3.0f};
        Span<void> s{static_cast<void*>(vals), 3};
        TEST(s.data == vals);
        TEST(s.size == 3);
    }

    // Construct from begin and end
    {
        u8 data[4] = {10, 20, 30, 40};
        Span<void> s{data, data + 4};
        TEST(s.size == 4);
        void* ptr = s[2];
        TEST(ptr == static_cast<void*>(data + 2));
    }
}


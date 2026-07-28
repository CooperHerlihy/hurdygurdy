#include "tests.hpp"

void testMaybe()
{
    // ============================================================================
    // Maybe
    // ============================================================================
    //
    // Maybe<T> is a lightweight optional type used for recoverable error
    // handling. It holds a boolean `has` and a union containing the value.
    // orElse(default) returns the value or a fallback; expect(msg) returns
    // the value or panics.

    // ------------------------------------------------------------------
    // with trivial types
    // ------------------------------------------------------------------

    // create a filled Maybe
    {
        Maybe<u32> m = 42;
        TEST(m.has);
        TEST(m.val == 42);
    }

    // empty Maybe
    {
        Maybe<i64> m = {};
        TEST(!m.has);
    }

    // with floating point
    {
        Maybe<f32> m = 3.14f;
        TEST(m.has);
        TEST(std::abs(m.val - 3.14f) <= FLT_EPSILON);
    }

    // default constructed with some
    {
        Maybe<bool> m = some<bool>();
        TEST(m.has);
        TEST(m.val == bool{});
    }

    // ------------------------------------------------------------------
    // orElse()
    // ------------------------------------------------------------------

    // orElse returns the value when present
    {
        Maybe<i32> m = 42;
        i32 result = m.orElse(-1);
        TEST(result == 42);
        TEST(!m.has); // value was moved out
    }

    // orElse returns the default when empty
    {
        Maybe<i32> m = {};
        i32 result = m.orElse(-1);
        TEST(result == -1);
        TEST(!m.has);
    }

    // orElse with floating point
    {
        Maybe<f32> m = {};
        f32 result = m.orElse(1.0f);
        TEST(std::abs(result - 1.0f) <= FLT_EPSILON);
    }

    // orElse can be called on an already-consumed Maybe (no-op)
    {
        Maybe<i32> m = 42;
        m.orElse(-1);
        TEST(!m.has); // consumed

        i32 result = m.orElse(-2);
        TEST(result == -2);
    }

    // ------------------------------------------------------------------
    // expect() (positive cases — negative case would panic/abort)
    // ------------------------------------------------------------------

    // expect returns the value when present
    {
        Maybe<i32> m = 42;
        i32 result = m.expect("should have value");
        TEST(result == 42);
        TEST(!m.has); // value was moved out
    }

    // expect with string type
    {
        ArenaScope arena = getScratch();
        Maybe<StringBuilder> m = some<StringBuilder>(arena, "hello");
        StringBuilder result = m.expect("string should exist");
        TEST(result == "hello");
        TEST(!m.has);
    }

    // ------------------------------------------------------------------
    // Copy semantics
    // ------------------------------------------------------------------

    // Copy construct a filled Maybe
    {
        Maybe<i32> a = 42;
        Maybe<i32> b{a};

        TEST(a.has);
        TEST(a.val == 42);
        TEST(b.has);
        TEST(b.val == 42);
    }

    // Copy construct an empty Maybe
    {
        Maybe<i32> a = {};
        Maybe<i32> b{a};

        TEST(!a.has);
        TEST(!b.has);
    }

    // Copy assign a filled Maybe
    {
        Maybe<i32> a = 42;
        Maybe<i32> b = {};
        b = a;

        TEST(a.has);
        TEST(a.val == 42);
        TEST(b.has);
        TEST(b.val == 42);
    }

    // Copy assign an empty Maybe
    {
        Maybe<i32> a = {};
        Maybe<i32> b = 10;
        b = a;

        TEST(!a.has);
        TEST(!b.has);
    }

    // Copy assign a filled Maybe onto another filled Maybe destroys old value
    {
        Maybe<i32> a = 42;
        Maybe<i32> b = 10;

        // Both alive before
        TEST(a.has && a.val == 42);
        TEST(b.has && b.val == 10);

        b = a;

        TEST(a.has && a.val == 42);
        TEST(b.has && b.val == 42);
    }

    // ------------------------------------------------------------------
    // Move semantics
    // ------------------------------------------------------------------

    // Move construct a filled Maybe
    {
        Maybe<i32> a = 42;
        Maybe<i32> b{std::move(a)};

        TEST(!a.has); // moved-from is empty
        TEST(b.has);
        TEST(b.val == 42);
    }

    // Move construct an empty Maybe
    {
        Maybe<i32> a = {};
        Maybe<i32> b{std::move(a)};

        TEST(!a.has);
        TEST(!b.has);
    }

    // Move assign a filled Maybe
    {
        Maybe<i32> a = 42;
        Maybe<i32> b = {};
        b = std::move(a);

        TEST(!a.has); // moved-from is empty
        TEST(b.has);
        TEST(b.val == 42);
    }

    // Move assign an empty Maybe
    {
        Maybe<i32> a = {};
        Maybe<i32> b = 10;
        b = std::move(a);

        TEST(!a.has);
        TEST(!b.has);
    }

    // ------------------------------------------------------------------
    // Maybe with non-trivial types
    // ------------------------------------------------------------------

    // some() with String (non-trivial: has ~String(), copy deleted)
    {
        Maybe<String> m = String::create("hello world");
        TEST(m.has);
        TEST(m.val == "hello world");
    }

    // Move a Maybe<String>
    {
        Maybe<String> a = String::create("move me");
        Maybe<String> b = std::move(a);

        TEST(!a.has);
        TEST(b.has);
        TEST(b.val == "move me");
    }

    // Move-assign a Maybe<String>
    {
        Maybe<String> a = String::create("first");
        Maybe<String> b = String::create("second");
        b = std::move(a);

        TEST(!a.has);
        TEST(b.has);
        TEST(b.val == "first");
    }

    // ------------------------------------------------------------------
    // Maybe with custom struct
    // ------------------------------------------------------------------

    // construct with a plain-old-data struct
    {
        struct Pod {
            i64 a;
            f32 b;
        };

        Maybe<Pod> m = Pod{-12, 3.14f};
        TEST(m.has);
        TEST(m.val.a == -12);
        TEST(std::abs(m.val.b - 3.14f) <= FLT_EPSILON);
    }

    // some() to construct in place
    {
        struct Pod {
            i64 a;
            f32 b;
        };

        Maybe<Pod> m = some<Pod>(-12, 3.14f);
        TEST(m.has);
        TEST(m.val.a == -12);
        TEST(std::abs(m.val.b - 3.14f) <= FLT_EPSILON);
    }

    // Empty with a struct type
    {
        struct Pod {
            i64 a;
            f32 b;
        };

        Maybe<Pod> m = {};
        TEST(!m.has);
    }

    // ------------------------------------------------------------------
    // Maybe lifecycle: destruction tracking
    // ------------------------------------------------------------------
    //
    // Use a tracked type to verify constructors and destructors are
    // called exactly once per object across all Maybe operations.

    // (Lifecycle struct defined at file scope above)

    // Empty should not construct or destroy anything
    {
        Lifecycle::stats.reset();
        {
            Maybe<Lifecycle> m = {};
            TEST(!m.has);
            TEST(Lifecycle::stats.alive == 0);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // some<T>() constructs, Maybe destructor destroys
    {
        Lifecycle::stats.reset();
        {
            Maybe<Lifecycle> m = some<Lifecycle>();
            TEST(m.has);
            TEST(Lifecycle::stats.alive == 1);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // Move construct from filled: value transferred, no extra construction
    {
        Lifecycle::stats.reset();
        {
            Maybe<Lifecycle> a = some<Lifecycle>();
            TEST(Lifecycle::stats.alive == 1);
            {
                Maybe<Lifecycle> b = std::move(a);
                TEST(!a.has);
                TEST(b.has);
                TEST(Lifecycle::stats.alive == 1);
            }
            TEST(Lifecycle::stats.alive == 0);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // Move construct from empty: no construction
    {
        Lifecycle::stats.reset();
        Maybe<Lifecycle> a = {};
        Maybe<Lifecycle> b = std::move(a);
        TEST(!a.has);
        TEST(!b.has);
        TEST(Lifecycle::stats.alive == 0);
    }

    // Move assign (filled → empty): value transferred
    {
        Lifecycle::stats.reset();
        {
            Maybe<Lifecycle> a = some<Lifecycle>();
            Maybe<Lifecycle> b = {};
            TEST(Lifecycle::stats.alive == 1);
            b = std::move(a);
            TEST(!a.has);
            TEST(b.has);
            TEST(Lifecycle::stats.alive == 1);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // Move assign (filled → filled): old dest destroyed
    {
        Lifecycle::stats.reset();
        {
            Maybe<Lifecycle> a = some<Lifecycle>();
            Maybe<Lifecycle> b = some<Lifecycle>();
            TEST(Lifecycle::stats.alive == 2);
            b = std::move(a);
            TEST(!a.has);
            TEST(b.has);
            TEST(Lifecycle::stats.alive == 1);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // Copy construct from filled: new copy constructed
    {
        Lifecycle::stats.reset();
        {
            Maybe<Lifecycle> a = some<Lifecycle>();
            TEST(Lifecycle::stats.alive == 1);
            {
                Maybe<Lifecycle> b{a};
                TEST(a.has);
                TEST(b.has);
                TEST(Lifecycle::stats.alive == 2);
            }
            TEST(Lifecycle::stats.alive == 1);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // Copy assign (filled → filled): old dest destroyed, new copy
    {
        Lifecycle::stats.reset();
        {
            Maybe<Lifecycle> a = some<Lifecycle>();
            Maybe<Lifecycle> b = some<Lifecycle>();
            TEST(Lifecycle::stats.alive == 2);
            b = a;
            TEST(a.has);
            TEST(b.has);
            TEST(Lifecycle::stats.alive == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // Copy assign (empty → filled): dest destroyed, no construct
    {
        Lifecycle::stats.reset();
        {
            Maybe<Lifecycle> a = {};
            Maybe<Lifecycle> b = some<Lifecycle>();
            TEST(Lifecycle::stats.alive == 1);
            b = a;
            TEST(!a.has);
            TEST(!b.has);
            TEST(Lifecycle::stats.alive == 0);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // Copy assign (filled → empty): copy constructed into dest
    {
        Lifecycle::stats.reset();
        {
            Maybe<Lifecycle> a = some<Lifecycle>();
            Maybe<Lifecycle> b = {};
            TEST(Lifecycle::stats.alive == 1);
            b = a;
            TEST(a.has);
            TEST(b.has);
            TEST(Lifecycle::stats.alive == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // Move assign (empty → empty): no-op
    {
        Lifecycle::stats.reset();
        Maybe<Lifecycle> a = {};
        Maybe<Lifecycle> b = {};
        b = std::move(a);
        TEST(!a.has);
        TEST(!b.has);
        TEST(Lifecycle::stats.alive == 0);
    }

    // Value constructor from lvalue: one copy, zero moves
    {
        Lifecycle::stats.reset();
        {
            Lifecycle val;
            TEST(Lifecycle::stats.alive == 1);
            Maybe<Lifecycle> m = val;
            TEST(m.has);
            TEST(Lifecycle::stats.alive == 2);
            TEST(Lifecycle::stats.copies == 1);
            TEST(Lifecycle::stats.moves == 0);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // Value constructor from rvalue: zero copies, one move
    {
        Lifecycle::stats.reset();
        {
            Lifecycle val;
            TEST(Lifecycle::stats.alive == 1);
            Maybe<Lifecycle> m = std::move(val);
            TEST(m.has);
            TEST(Lifecycle::stats.alive == 1);
            TEST(Lifecycle::stats.copies == 0);
            TEST(Lifecycle::stats.moves == 1);
        }
        TEST(Lifecycle::stats.alive == 0);
    }
}


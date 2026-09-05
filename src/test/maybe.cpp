#include "tests.hpp"
#include "hg/maybe.hpp"

using namespace hg;

TEST(testMaybeCreateFilled)
{
    Maybe<u32> m = 42;
    ASSERT(m.has);
    ASSERT(m.val == 42);
}

TEST(testMaybeEmpty)
{
    Maybe<i64> m = {};
    ASSERT(!m.has);
}

TEST(testMaybeFloat)
{
    Maybe<f32> m = 3.14f;
    ASSERT(m.has);
    ASSERT(std::abs(m.val - 3.14f) <= FLT_EPSILON);
}

TEST(testMaybeSomeDefault)
{
    Maybe<bool> m = some<bool>();
    ASSERT(m.has);
    ASSERT(m.val == bool{});
}

TEST(testMaybeOrElsePresent)
{
    Maybe<i32> m = 42;
    i32 result = m.orElse(-1);
    ASSERT(result == 42);
    ASSERT(!m.has);
}

TEST(testMaybeOrElseEmpty)
{
    Maybe<i32> m = {};
    i32 result = m.orElse(-1);
    ASSERT(result == -1);
    ASSERT(!m.has);
}

TEST(testMaybeOrElseFloat)
{
    Maybe<f32> m = {};
    f32 result = m.orElse(1.0f);
    ASSERT(std::abs(result - 1.0f) <= FLT_EPSILON);
}

TEST(testMaybeOrElseConsumed)
{
    Maybe<i32> m = 42;
    m.orElse(-1);
    ASSERT(!m.has);

    i32 result = m.orElse(-2);
    ASSERT(result == -2);
}

TEST(testMaybeExpectPresent)
{
    Maybe<i32> m = 42;
    i32 result = m.expect("should have value");
    ASSERT(result == 42);
    ASSERT(!m.has);
}

TEST(testMaybeExpectString)
{
    ArenaScope arena = getScratch();
    Maybe<StringBuilder> m = some<StringBuilder>(arena, "hello");
    StringBuilder result = m.expect("string should exist");
    ASSERT(result == "hello");
    ASSERT(!m.has);
}

TEST(testMaybeCopyConstructFilled)
{
    Maybe<i32> a = 42;
    Maybe<i32> b{a};

    ASSERT(a.has);
    ASSERT(a.val == 42);
    ASSERT(b.has);
    ASSERT(b.val == 42);
}

TEST(testMaybeCopyConstructEmpty)
{
    Maybe<i32> a = {};
    Maybe<i32> b{a};

    ASSERT(!a.has);
    ASSERT(!b.has);
}

TEST(testMaybeCopyAssignFilled)
{
    Maybe<i32> a = 42;
    Maybe<i32> b = {};
    b = a;

    ASSERT(a.has);
    ASSERT(a.val == 42);
    ASSERT(b.has);
    ASSERT(b.val == 42);
}

TEST(testMaybeCopyAssignEmpty)
{
    Maybe<i32> a = {};
    Maybe<i32> b = 10;
    b = a;

    ASSERT(!a.has);
    ASSERT(!b.has);
}

TEST(testMaybeCopyAssignFilledToFilled)
{
    Maybe<i32> a = 42;
    Maybe<i32> b = 10;

    ASSERT(a.has && a.val == 42);
    ASSERT(b.has && b.val == 10);

    b = a;

    ASSERT(a.has && a.val == 42);
    ASSERT(b.has && b.val == 42);
}

TEST(testMaybeMoveConstructFilled)
{
    Maybe<i32> a = 42;
    Maybe<i32> b{std::move(a)};

    ASSERT(!a.has);
    ASSERT(b.has);
    ASSERT(b.val == 42);
}

TEST(testMaybeMoveConstructEmpty)
{
    Maybe<i32> a = {};
    Maybe<i32> b{std::move(a)};

    ASSERT(!a.has);
    ASSERT(!b.has);
}

TEST(testMaybeMoveAssignFilled)
{
    Maybe<i32> a = 42;
    Maybe<i32> b = {};
    b = std::move(a);

    ASSERT(!a.has);
    ASSERT(b.has);
    ASSERT(b.val == 42);
}

TEST(testMaybeMoveAssignEmpty)
{
    Maybe<i32> a = {};
    Maybe<i32> b = 10;
    b = std::move(a);

    ASSERT(!a.has);
    ASSERT(!b.has);
}

TEST(testMaybeStringCreate)
{
    Maybe<String> m = String::create("hello world");
    ASSERT(m.has);
    ASSERT(m.val == "hello world");
}

TEST(testMaybeStringMove)
{
    Maybe<String> a = String::create("move me");
    Maybe<String> b = std::move(a);

    ASSERT(!a.has);
    ASSERT(b.has);
    ASSERT(b.val == "move me");
}

TEST(testMaybeStringMoveAssign)
{
    Maybe<String> a = String::create("first");
    Maybe<String> b = String::create("second");
    b = std::move(a);

    ASSERT(!a.has);
    ASSERT(b.has);
    ASSERT(b.val == "first");
}

TEST(testMaybePodConstruct)
{
    struct Pod {
        i64 a;
        f32 b;
    };

    Maybe<Pod> m = Pod{-12, 3.14f};
    ASSERT(m.has);
    ASSERT(m.val.a == -12);
    ASSERT(std::abs(m.val.b - 3.14f) <= FLT_EPSILON);
}

TEST(testMaybePodSome)
{
    struct Pod {
        i64 a;
        f32 b;
    };

    Maybe<Pod> m = some<Pod>(-12, 3.14f);
    ASSERT(m.has);
    ASSERT(m.val.a == -12);
    ASSERT(std::abs(m.val.b - 3.14f) <= FLT_EPSILON);
}

TEST(testMaybePodEmpty)
{
    struct Pod {
        i64 a;
        f32 b;
    };

    Maybe<Pod> m = {};
    ASSERT(!m.has);
}

TEST(testMaybeLifecycleEmpty)
{
    Lifecycle::stats.reset();
    {
        Maybe<Lifecycle> m = {};
        ASSERT(!m.has);
        ASSERT(Lifecycle::stats.alive == 0);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMaybeLifecycleSome)
{
    Lifecycle::stats.reset();
    {
        Maybe<Lifecycle> m = some<Lifecycle>();
        ASSERT(m.has);
        ASSERT(Lifecycle::stats.alive == 1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMaybeLifecycleMoveConstructFilled)
{
    Lifecycle::stats.reset();
    {
        Maybe<Lifecycle> a = some<Lifecycle>();
        ASSERT(Lifecycle::stats.alive == 1);
        {
            Maybe<Lifecycle> b = std::move(a);
            ASSERT(!a.has);
            ASSERT(b.has);
            ASSERT(Lifecycle::stats.alive == 1);
        }
        ASSERT(Lifecycle::stats.alive == 0);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMaybeLifecycleMoveConstructEmpty)
{
    Lifecycle::stats.reset();
    Maybe<Lifecycle> a = {};
    Maybe<Lifecycle> b = std::move(a);
    ASSERT(!a.has);
    ASSERT(!b.has);
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMaybeLifecycleMoveAssignFilledToEmpty)
{
    Lifecycle::stats.reset();
    {
        Maybe<Lifecycle> a = some<Lifecycle>();
        Maybe<Lifecycle> b = {};
        ASSERT(Lifecycle::stats.alive == 1);
        b = std::move(a);
        ASSERT(!a.has);
        ASSERT(b.has);
        ASSERT(Lifecycle::stats.alive == 1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMaybeLifecycleMoveAssignFilledToFilled)
{
    Lifecycle::stats.reset();
    {
        Maybe<Lifecycle> a = some<Lifecycle>();
        Maybe<Lifecycle> b = some<Lifecycle>();
        ASSERT(Lifecycle::stats.alive == 2);
        b = std::move(a);
        ASSERT(!a.has);
        ASSERT(b.has);
        ASSERT(Lifecycle::stats.alive == 1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMaybeLifecycleCopyConstruct)
{
    Lifecycle::stats.reset();
    {
        Maybe<Lifecycle> a = some<Lifecycle>();
        ASSERT(Lifecycle::stats.alive == 1);
        {
            Maybe<Lifecycle> b{a};
            ASSERT(a.has);
            ASSERT(b.has);
            ASSERT(Lifecycle::stats.alive == 2);
        }
        ASSERT(Lifecycle::stats.alive == 1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMaybeLifecycleCopyAssignFilledToFilled)
{
    Lifecycle::stats.reset();
    {
        Maybe<Lifecycle> a = some<Lifecycle>();
        Maybe<Lifecycle> b = some<Lifecycle>();
        ASSERT(Lifecycle::stats.alive == 2);
        b = a;
        ASSERT(a.has);
        ASSERT(b.has);
        ASSERT(Lifecycle::stats.alive == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMaybeLifecycleCopyAssignEmptyToFilled)
{
    Lifecycle::stats.reset();
    {
        Maybe<Lifecycle> a = {};
        Maybe<Lifecycle> b = some<Lifecycle>();
        ASSERT(Lifecycle::stats.alive == 1);
        b = a;
        ASSERT(!a.has);
        ASSERT(!b.has);
        ASSERT(Lifecycle::stats.alive == 0);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMaybeLifecycleCopyAssignFilledToEmpty)
{
    Lifecycle::stats.reset();
    {
        Maybe<Lifecycle> a = some<Lifecycle>();
        Maybe<Lifecycle> b = {};
        ASSERT(Lifecycle::stats.alive == 1);
        b = a;
        ASSERT(a.has);
        ASSERT(b.has);
        ASSERT(Lifecycle::stats.alive == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMaybeLifecycleMoveAssignEmptyToEmpty)
{
    Lifecycle::stats.reset();
    Maybe<Lifecycle> a = {};
    Maybe<Lifecycle> b = {};
    b = std::move(a);
    ASSERT(!a.has);
    ASSERT(!b.has);
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMaybeLifecycleValueFromLvalue)
{
    Lifecycle::stats.reset();
    {
        Lifecycle val;
        ASSERT(Lifecycle::stats.alive == 1);
        Maybe<Lifecycle> m = val;
        ASSERT(m.has);
        ASSERT(Lifecycle::stats.alive == 2);
        ASSERT(Lifecycle::stats.copies == 1);
        ASSERT(Lifecycle::stats.moves == 0);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMaybeLifecycleValueFromRvalue)
{
    Lifecycle::stats.reset();
    {
        Lifecycle val;
        ASSERT(Lifecycle::stats.alive == 1);
        Maybe<Lifecycle> m = std::move(val);
        ASSERT(m.has);
        ASSERT(Lifecycle::stats.alive == 1);
        ASSERT(Lifecycle::stats.copies == 0);
        ASSERT(Lifecycle::stats.moves == 1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

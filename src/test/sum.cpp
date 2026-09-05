#include "tests.hpp"
#include "hg/sum.hpp"

using namespace hg;

TEST(testSumDefault)
{
    Sum<i32, f64> s;
    ASSERT(s.tag == s.count);
}

TEST(testSumConstruction)
{
    Sum<i32, f64> a{42};
    ASSERT(a.is<i32>());
    ASSERT(a.get<i32>() == 42);

    Sum<i32, f64> b{3.14};
    ASSERT(b.is<f64>());
    ASSERT(b.get<f64>() == 3.14);

    Sum<i32, f64, char> c{'A'};
    ASSERT(c.is<char>());
    ASSERT(c.get<char>() == 'A');
}

TEST(testSumIsGet)
{
    Sum<i32, f64> s{42};
    ASSERT(s.is<i32>());
    ASSERT(!s.is<f64>());
    ASSERT(s.get<i32>() == 42);
}

TEST(testSumIsNGetN)
{
    Sum<i32, f64> a{42};
    ASSERT(a.isN<0>());
    ASSERT(!a.isN<1>());

    Sum<i32, f64> b{3.14};
    ASSERT(b.getN<1>() == 3.14);
}

TEST(testSumEmplaceN)
{
    Sum<i32, f64> a{42};
    ASSERT(a.is<i32>());
    f64& ref = a.emplaceN<1>(3.14);
    ASSERT(!a.is<i32>());
    ASSERT(a.is<f64>());
    ASSERT(a.get<f64>() == 3.14);
    ASSERT(ref == 3.14);

    Sum<i32, f64> b{3.14};
    i32& ref2 = b.emplaceN<0>(99);
    ASSERT(b.is<i32>());
    ASSERT(b.get<i32>() == 99);
    ASSERT(ref2 == 99);
}

TEST(testSumCall)
{
    Sum<i32, f64> a{42};
    bool called = false;
    a.call([&](auto& val)
    {
        called = true;
        using T = std::remove_cvref_t<decltype(val)>;
        ASSERT((std::same_as<T, i32>));
        ASSERT(val == 42);
    });
    ASSERT(called);

    Sum<i32, f64> b{3.14};
    called = false;
    b.call([&](auto& val)
    {
        called = true;
        using T = std::remove_cvref_t<decltype(val)>;
        ASSERT((std::same_as<T, f64>));
        ASSERT(val == 3.14);
    });
    ASSERT(called);
}

TEST(testSumMatch)
{
    Sum<i32, f64> s{3.14};
    bool matched = false;
    s.match(
        [&](i32) { ASSERT(false); },
        [&](f64 v)
        {
            matched = true;
            ASSERT(v == 3.14);
        }
    );
    ASSERT(matched);
}

TEST(testSumCopyConstruction)
{
    Sum<i32, f64> a{42};
    Sum<i32, f64> b{a};
    ASSERT(b.is<i32>());
    ASSERT(b.get<i32>() == 42);
    ASSERT(a.is<i32>());
    ASSERT(a.get<i32>() == 42);
}

TEST(testSumCopyAssignment)
{
    Sum<i32, f64> a{42};
    Sum<i32, f64> b{3.14};
    b = a;
    ASSERT(b.is<i32>());
    ASSERT(b.get<i32>() == 42);
    ASSERT(a.is<i32>());
}

TEST(testSumMoveConstruction)
{
    Sum<i32, f64> a{42};
    Sum<i32, f64> b{std::move(a)};
    ASSERT(b.is<i32>());
    ASSERT(b.get<i32>() == 42);
    ASSERT(a.tag == a.count);
}

TEST(testSumMoveAssignment)
{
    Sum<i32, f64> a{42};
    Sum<i32, f64> b{3.14};
    b = std::move(a);
    ASSERT(b.is<i32>());
    ASSERT(b.get<i32>() == 42);
    ASSERT(a.tag == a.count);
}

TEST(testSumTypeIdx)
{
    static_assert(Sum<i32, f64, u64>::typeIdx<i32> == 0);
    static_assert(Sum<i32, f64, u64>::typeIdx<f64> == 1);
    static_assert(Sum<i32, f64, u64>::typeIdx<u64> == 2);
}

TEST(testSumSingleVariant)
{
    Sum<i32> s{42};
    ASSERT(s.is<i32>());
    ASSERT(s.get<i32>() == 42);
    ASSERT(s.count == 1);
}

TEST(testSumLifecycleDestroy)
{
    Lifecycle::stats.reset();
    {
        Sum<Lifecycle, i32> s{Lifecycle{}};
        ASSERT(s.is<Lifecycle>());
        ASSERT(Lifecycle::stats.alive == 1);
    }
    ASSERT(Lifecycle::stats.dtors == 1);
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testSumLifecycleEmplace)
{
    Lifecycle::stats.reset();
    {
        Sum<Lifecycle, i32> s{Lifecycle{}};
        ASSERT(Lifecycle::stats.alive == 1);
        s.template emplaceN<1>(42);
        ASSERT(s.is<i32>());
        ASSERT(Lifecycle::stats.alive == 0);
    }
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testSumLifecycleEmplaceBack)
{
    Lifecycle::stats.reset();
    {
        Sum<Lifecycle, i32> s{i32{42}};
        ASSERT(s.is<i32>());
        s.template emplaceN<0>();
        ASSERT(s.is<Lifecycle>());
        ASSERT(Lifecycle::stats.alive == 1);
    }
    ASSERT(Lifecycle::stats.dtors == 1);
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testSumLifecycleCopy)
{
    Lifecycle::stats.reset();
    {
        Sum<Lifecycle, i32> a{Lifecycle{}};
        Sum<Lifecycle, i32> b{a};
        ASSERT(a.is<Lifecycle>());
        ASSERT(b.is<Lifecycle>());
        ASSERT(Lifecycle::stats.alive == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 2);
}

TEST(testSumLifecycleMove)
{
    Lifecycle::stats.reset();
    {
        Sum<Lifecycle, i32> a{Lifecycle{}};
        Sum<Lifecycle, i32> b{std::move(a)};
        ASSERT(b.is<Lifecycle>());
        ASSERT(a.tag == a.count);
        ASSERT(Lifecycle::stats.alive == 1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testSumIsEmptyDefault)
{
    Sum<i32, f64> s;
    ASSERT(s.isEmpty());
}

TEST(testSumIsEmptyAfterConstruct)
{
    Sum<i32, f64> a{42};
    ASSERT(!a.isEmpty());

    Sum<i32, f64> b{3.14};
    ASSERT(!b.isEmpty());
}

TEST(testSumIsNAllVariants)
{
    Sum<i32, f64, char> a{42};
    ASSERT(a.isN<0>());
    ASSERT(!a.isN<1>());
    ASSERT(!a.isN<2>());

    Sum<i32, f64, char> b{3.14};
    ASSERT(!b.isN<0>());
    ASSERT(b.isN<1>());
    ASSERT(!b.isN<2>());

    Sum<i32, f64, char> c{'A'};
    ASSERT(!c.isN<0>());
    ASSERT(!c.isN<1>());
    ASSERT(c.isN<2>());
}

TEST(testSumConstGetN)
{
    const Sum<i32, f64> a{42};
    ASSERT(a.getN<0>() == 42);

    const Sum<i32, f64> b{3.14};
    ASSERT(b.getN<1>() == 3.14);
}

TEST(testSumConstGet)
{
    const Sum<i32, f64> a{42};
    ASSERT(a.get<i32>() == 42);

    const Sum<i32, f64> b{3.14};
    ASSERT(b.get<f64>() == 3.14);
}

TEST(testSumEmplaceByType)
{
    Sum<i32, f64> a{42};
    ASSERT(a.is<i32>());
    f64& ref = a.emplace<f64>(3.14);
    ASSERT(a.is<f64>());
    ASSERT(a.get<f64>() == 3.14);
    ASSERT(ref == 3.14);

    i32& ref2 = a.emplace<i32>(99);
    ASSERT(a.is<i32>());
    ASSERT(a.get<i32>() == 99);
    ASSERT(ref2 == 99);
}

TEST(testSumEmplaceOverwrites)
{
    Lifecycle::stats = {};
    {
        Sum<Lifecycle, i32, f64> s{Lifecycle{}};
        ASSERT(Lifecycle::stats.ctors == 1);
        s.emplace<i32>(42);
        ASSERT(Lifecycle::stats.dtors == 1);
        ASSERT(s.get<i32>() == 42);
    }
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testSumCopyAssignDifferentVariant)
{
    Sum<i32, f64> a{3.14};
    Sum<i32, f64> b{42};
    ASSERT(b.is<i32>());
    b = a;
    ASSERT(b.is<f64>());
    ASSERT(b.get<f64>() == 3.14);
    ASSERT(a.is<f64>());
}

TEST(testSumMoveAssignDifferentVariant)
{
    Sum<i32, f64> a{3.14};
    Sum<i32, f64> b{42};
    ASSERT(b.is<i32>());
    b = std::move(a);
    ASSERT(b.is<f64>());
    ASSERT(b.get<f64>() == 3.14);
    ASSERT(a.tag == a.count);
}

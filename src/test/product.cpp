#include "tests.hpp"
#include "hg/product.hpp"

using namespace hg;

TEST(testProductDefault)
{
    Product<> p;
    (void)p;
}

TEST(testProductSingleElement)
{
    Product<i32> p;
    ASSERT(p.count == 1);
}

TEST(testProductMultipleElements)
{
    Product<i32, f64, u64> p;
    ASSERT(p.count == 3);
    ASSERT(p.first == 0);
    ASSERT(p.rest.first == 0.0);
    ASSERT(p.rest.rest.first == 0u);
}

TEST(testProductValueConstruction)
{
    Product<i32> p{42};
    ASSERT(p.first == 42);

    Product<i32, f64, u64> q{1, 2.0, 3u};
    ASSERT(q.first == 1);
    ASSERT(q.rest.first == 2.0);
    ASSERT(q.rest.rest.first == 3u);
}

TEST(testProductGet)
{
    Product<i32, f64, u64> p{10, 20.0, 30u};
    ASSERT(p.get<0>() == 10);
    ASSERT(p.get<1>() == 20.0);
    ASSERT(p.get<2>() == 30u);

    p.get<0>() = 100;
    ASSERT(p.get<0>() == 100);
    ASSERT(p.first == 100);

    p.get<1>() = 200.0;
    ASSERT(p.get<1>() == 200.0);
    ASSERT(p.rest.first == 200.0);
}

TEST(testProductSet)
{
    Product<i32, f64> p{1, 2.0};
    i32& ref = p.set<0>(99);
    ASSERT(ref == 99);
    ASSERT(p.first == 99);
    ASSERT(&ref == &p.first);

    f64& ref2 = p.set<1>(3.0);
    ASSERT(ref2 == 3.0);
    ASSERT(p.rest.first == 3.0);
}

TEST(testProductEdgeCases)
{
    Product<i32, f64, u64, bool> p{1, 2.0, 3u, true};
    ASSERT(p.get<0>() == 1);
    ASSERT(p.get<3>() == true);
    p.set<3>(false);
    ASSERT(p.get<3>() == false);

    Product<f64> q{3.14};
    ASSERT(q.get<0>() == 3.14);
    q.set<0>(2.71);
    ASSERT(q.get<0>() == 2.71);
}

TEST(testProductStaticCount)
{
    static_assert(Product<i32>::count == 1);
    static_assert(Product<i32, f64, u64>::count == 3);
}

TEST(testProductForEachEmpty)
{
    Product<> p;
    p.forEach([](auto) { HG_PANIC("should not be called"); });
    const Product<> cp;
    cp.forEach([](auto) { HG_PANIC("should not be called"); });
}

TEST(testProductForEachSingle)
{
    Product<i32> p{42};
    i32 sum = 0;
    p.forEach([&](i32 x) { sum += x; });
    ASSERT(sum == 42);
}

TEST(testProductForEachMulti)
{
    Product<i32, f64, u64> p{1, 2.0, 3u};
    i32 count = 0;
    p.forEach([&](auto&) { count++; });
    ASSERT(count == 3);
}

TEST(testProductSetRvalue)
{
    Product<i32> p{10};
    i32 val = 99;
    p.set<0>(std::move(val));
    ASSERT(p.get<0>() == 99);

    Product<i32, f64> q{1, 2.0};
    q.set<1>(3.14);
    ASSERT(q.get<1>() == 3.14);
}

TEST(testProductConstGet)
{
    const Product<i32, f64, u64> p{1, 2.0, 3u};
    ASSERT(p.get<0>() == 1);
    ASSERT(p.get<1>() == 2.0);
    ASSERT(p.get<2>() == 3u);
}

TEST(testProductCopyConstruct)
{
    Product<i32, f64, u64> p{1, 2.0, 3u};
    Product<i32, f64, u64> q{p};
    ASSERT(q.get<0>() == 1);
    ASSERT(q.get<1>() == 2.0);
    ASSERT(q.get<2>() == 3u);
    p.set<0>(99);
    ASSERT(q.get<0>() == 1);
}

TEST(testProductMoveConstruct)
{
    Product<i32, f64, u64> p{1, 2.0, 3u};
    Product<i32, f64, u64> q{std::move(p)};
    ASSERT(q.get<0>() == 1);
    ASSERT(q.get<1>() == 2.0);
    ASSERT(q.get<2>() == 3u);
}

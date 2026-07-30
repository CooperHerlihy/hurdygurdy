#include "tests.hpp"
#include "hg/product.hpp"

void testProduct()
{
    // ============================================================================
    // Product
    // ============================================================================
    //
    // Product<Ts...> is a heterogeneous compile-time tuple (unnamed struct).
    // Provides index-based access via get<N>() and set<N>().
    // count() returns the number of elements at compile time.

    // Default-constructed Product<> is empty
    {
        Product<> p;
        (void)p;
    }

    // Product with a single element
    {
        Product<i32> p;
        TEST(p.count == 1);
    }

    // Product with multiple elements of different types
    {
        Product<i32, f64, u64> p;
        TEST(p.count == 3);
        TEST(p.first == 0);
        TEST(p.rest.first == 0.0);
        TEST(p.rest.rest.first == 0u);
    }

    // Value construction with one element
    {
        Product<i32> p{42};
        TEST(p.first == 42);
    }

    // Value construction with multiple elements
    {
        Product<i32, f64, u64> p{1, 2.0, 3u};
        TEST(p.first == 1);
        TEST(p.rest.first == 2.0);
        TEST(p.rest.rest.first == 3u);
    }

    // get<N>() returns a mutable reference to the Nth element
    {
        Product<i32, f64, u64> p{10, 20.0, 30u};
        TEST(p.get<0>() == 10);
        TEST(p.get<1>() == 20.0);
        TEST(p.get<2>() == 30u);

        p.get<0>() = 100;
        TEST(p.get<0>() == 100);
        TEST(p.first == 100);

        p.get<1>() = 200.0;
        TEST(p.get<1>() == 200.0);
        TEST(p.rest.first == 200.0);
    }

    // set<N>(val) assigns and returns a reference to the element
    {
        Product<i32, f64> p{1, 2.0};
        i32& ref = p.set<0>(99);
        TEST(ref == 99);
        TEST(p.first == 99);
        TEST(&ref == &p.first);

        f64& ref2 = p.set<1>(3.0);
        TEST(ref2 == 3.0);
        TEST(p.rest.first == 3.0);
    }

    // Edge case: get and set at the first and last index
    {
        Product<i32, f64, u64, bool> p{1, 2.0, 3u, true};
        TEST(p.get<0>() == 1);
        TEST(p.get<3>() == true);
        p.set<3>(false);
        TEST(p.get<3>() == false);
    }

    // Edge case: single-element Product access
    {
        Product<f64> p{3.14};
        TEST(p.get<0>() == 3.14);
        p.set<0>(2.71);
        TEST(p.get<0>() == 2.71);
    }

    // count() is a static constexpr member on non-empty products
    {
        static_assert(Product<i32>::count == 1);
        static_assert(Product<i32, f64, u64>::count == 3);
    }
}


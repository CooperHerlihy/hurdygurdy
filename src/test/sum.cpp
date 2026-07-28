#include "tests.hpp"

void testSum()
{
    // ============================================================================
    // Sum
    // ============================================================================
    //
    // Sum<Ts...> is a tagged union (discriminated union). It holds a tag
    // indicating which variant is active, and an untagged union for storage.
    // Supports construction from any variant type, emplacement, visitor-style
    // matching via call/match, and copy/move semantics.

    // Default-constructed Sum has no active variant (tag == count)
    {
        Sum<i32, f64> s;
        TEST(s.tag == s.count);
    }

    // Construction from the first variant type
    {
        Sum<i32, f64> s{42};
        TEST(s.is<i32>());
        TEST(!s.is<f64>());
        TEST(s.get<i32>() == 42);
    }

    // Construction from the second variant type
    {
        Sum<i32, f64> s{3.14};
        TEST(!s.is<i32>());
        TEST(s.is<f64>());
        TEST(s.get<f64>() == 3.14);
    }

    // Sum with three variants
    {
        Sum<i32, f64, char> s{'A'};
        TEST(s.is<char>());
        TEST(s.get<char>() == 'A');
    }

    // hasN<N>() checks the active variant by index
    {
        Sum<i32, f64> s{42};
        TEST(s.isN<0>());
        TEST(!s.isN<1>());
    }

    // getN<N>() accesses the active variant by index
    {
        Sum<i32, f64> s{3.14};
        TEST(s.getN<1>() == 3.14);
    }

    // emplaceN<N>() switches the variant and constructs the new value in place
    {
        Sum<i32, f64> s{42};
        TEST(s.is<i32>());

        f64& ref = s.emplaceN<1>(3.14);
        TEST(!s.is<i32>());
        TEST(s.is<f64>());
        TEST(s.get<f64>() == 3.14);
        TEST(ref == 3.14);
    }

    // emplaceN<N>() at the first index
    {
        Sum<i32, f64> s{3.14};
        i32& ref = s.emplaceN<0>(99);
        TEST(s.is<i32>());
        TEST(s.get<i32>() == 99);
        TEST(ref == 99);
    }

    // call(F) dispatches a generic lambda to the active variant
    {
        Sum<i32, f64> s{42};
        bool called = false;
        s.call([&](auto& val)
        {
            called = true;
            using T = std::remove_cvref_t<decltype(val)>;
            TEST((std::same_as<T, i32>));
            TEST(val == 42);
        });
        TEST(called);
    }

    // call(F) dispatches for the second variant
    {
        Sum<i32, f64> s{3.14};
        bool called = false;
        s.call([&](auto& val)
        {
            called = true;
            using T = std::remove_cvref_t<decltype(val)>;
            TEST((std::same_as<T, f64>));
            TEST(val == 3.14);
        });
        TEST(called);
    }

    // match(Fs...) dispatches an overload set to the active variant
    {
        Sum<i32, f64> s{3.14};
        bool matched = false;
        s.match(
            [&](i32) { TEST(false); },
            [&](f64 v)
            {
                matched = true;
                TEST(v == 3.14);
            }
        );
        TEST(matched);
    }

    // Copy construction preserves the active variant and value
    {
        Sum<i32, f64> a{42};
        Sum<i32, f64> b{a};
        TEST(b.is<i32>());
        TEST(b.get<i32>() == 42);
        TEST(a.is<i32>());
        TEST(a.get<i32>() == 42);
    }

    // Copy assignment from a different variant
    {
        Sum<i32, f64> a{42};
        Sum<i32, f64> b{3.14};
        b = a;
        TEST(b.is<i32>());
        TEST(b.get<i32>() == 42);
        TEST(a.is<i32>());
    }

    // Move construction transfers ownership; source tag is reset to count
    {
        Sum<i32, f64> a{42};
        Sum<i32, f64> b{std::move(a)};
        TEST(b.is<i32>());
        TEST(b.get<i32>() == 42);
        TEST(a.tag == a.count);
    }

    // Move assignment from a different variant
    {
        Sum<i32, f64> a{42};
        Sum<i32, f64> b{3.14};
        b = std::move(a);
        TEST(b.is<i32>());
        TEST(b.get<i32>() == 42);
        TEST(a.tag == a.count);
    }

    // typeToTag maps each type to its variant index at compile time
    {
        static_assert(Sum<i32, f64, u64>::typeIdx<i32> == 0);
        static_assert(Sum<i32, f64, u64>::typeIdx<f64> == 1);
        static_assert(Sum<i32, f64, u64>::typeIdx<u64> == 2);
    }

    // Sum with a single variant type
    {
        Sum<i32> s{42};
        TEST(s.is<i32>());
        TEST(s.get<i32>() == 42);
        TEST(s.count == 1);
    }

    // Non-trivial type: Sum destructor destroys the active variant
    {
        Lifecycle::stats.reset();
        {
            Sum<Lifecycle, i32> s{Lifecycle{}};
            TEST(s.is<Lifecycle>());
            TEST(Lifecycle::stats.alive == 1);
        }
        TEST(Lifecycle::stats.dtors == 1);
        TEST(Lifecycle::stats.alive == 0);
    }

    // Non-trivial type: emplaceN to a different variant destroys the old value
    {
        Lifecycle::stats.reset();
        {
            Sum<Lifecycle, i32> s{Lifecycle{}};
            TEST(Lifecycle::stats.alive == 1);
            s.template emplaceN<1>(42);
            TEST(s.is<i32>());
            TEST(Lifecycle::stats.alive == 0);
        }
        // No extra destruction from ~Sum (i32 variant active)
        TEST(Lifecycle::stats.dtors == 1);
    }

    // Non-trivial type: emplaceN back to the non-trivial variant
    {
        Lifecycle::stats.reset();
        {
            Sum<Lifecycle, i32> s{i32{42}};
            TEST(s.is<i32>());
            s.template emplaceN<0>();
            TEST(s.is<Lifecycle>());
            TEST(Lifecycle::stats.alive == 1);
        }
        TEST(Lifecycle::stats.dtors == 1);
        TEST(Lifecycle::stats.alive == 0);
    }

    // Non-trivial type: copy construction preserves the value
    {
        Lifecycle::stats.reset();
        {
            Sum<Lifecycle, i32> a{Lifecycle{}};
            Sum<Lifecycle, i32> b{a};
            TEST(a.is<Lifecycle>());
            TEST(b.is<Lifecycle>());
            TEST(Lifecycle::stats.alive == 2); // a and b each own a Lifecycle
        }
        // Both destroyed when scope exits
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 2);
    }

    // Non-trivial type: move construction transfers ownership; source is empty
    {
        Lifecycle::stats.reset();
        {
            Sum<Lifecycle, i32> a{Lifecycle{}};
            Sum<Lifecycle, i32> b{std::move(a)};
            TEST(b.is<Lifecycle>());
            TEST(a.tag == a.count);
            TEST(Lifecycle::stats.alive == 1);
        }
        // b destroyed, a was empty
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 1);
    }
}


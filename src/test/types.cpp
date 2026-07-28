#include "tests.hpp"

void testTypes()
{
    // ============================================================================
    // StringView operators
    // ============================================================================
    //
    // StringView is a non-owning view into a string (chars + length).
    // Equality compares length first, then memcmp.
    //
    // Functions covered:
    // - operator==(StringView, StringView)
    // - operator!=(StringView, StringView)

    // Equal strings
    {
        StringView a{"hello"};
        StringView b{"hello"};
        TEST(a == b);
        TEST(!(a != b));
    }

    // Different strings
    {
        StringView a{"hello"};
        StringView b{"world"};
        TEST(a != b);
        TEST(!(a == b));
    }

    // Empty strings are equal
    {
        StringView a{};
        StringView b{};
        TEST(a == b);
    }

    // Empty vs non-empty
    {
        StringView a{};
        StringView b{"x"};
        TEST(a != b);
    }

    // Same content, different pointer (should still compare equal)
    {
        const char* s1 = "abcdef";
        const char* s2 = "abcdef";
        StringView a{s1, 3};
        StringView b{s2, 3};
        TEST(a == b);
    }

    // Different lengths, same prefix
    {
        StringView a{"hello", 5};
        StringView b{"hello world", 5};
        TEST(a == b);
    }

    // Different lengths, same prefix (one longer)
    {
        StringView a{"hello world", 11};
        StringView b{"hello", 5};
        TEST(a != b);
    }

    // nullptr handling — constructing from nullptr gives empty view
    {
        const char* nullStr = nullptr;
        StringView sv{nullStr};
        TEST(sv.chars == nullptr);
        TEST(sv.length == 0);

        StringView empty{};
        TEST(sv == empty);
    }

    // StringView from (ptr, length) with zero length
    {
        const char* data = "hello";
        StringView sv{data, u64{0}};
        TEST(sv.length == 0);
        TEST(StringView{} == sv);
    }

    // StringView from begin/end pointers
    {
        const char* data = "hello world";
        StringView sv{data + 6, data + 11};
        TEST(sv == "world");
    }

    // StringView from begin/end with equal pointers (empty range)
    {
        const char* data = "hello";
        StringView sv{data, data};
        TEST(sv.length == 0);
    }

    // StringView indexing
    {
        StringView sv{"hello"};
        TEST(sv[0] == 'h');
        TEST(sv[1] == 'e');
        TEST(sv[2] == 'l');
        TEST(sv[3] == 'l');
        TEST(sv[4] == 'o');
    }

    // StringView range-for
    {
        StringView sv{"abc"};
        char result[4]{};
        u64 i = 0;
        for (char c : sv)
        {
            result[i] = c;
            ++i;
        }
        result[i] = '\0';
        TEST(StringView{result} == "abc");
    }

    // StringView from const char* implicit conversion (long string)
    {
        StringView sv{"this is a fairly long string that should work fine"};
        TEST(sv.length == 50);
        TEST(sv == "this is a fairly long string that should work fine");
    }

    // ============================================================================
    // BinaryView
    // ============================================================================
    //
    // BinaryView is a non-owning view into binary data (data pointer + size).
    // read() and read<T>() copy bytes out at an offset.
    //
    // Functions covered:
    // - BinaryView() — default constructor
    // - BinaryView(void*, u64) — ptr+size constructor
    // - read(u64, void*, u64)
    // - read<T>(u64)

    // Default-constructed BinaryView is empty
    {
        BinaryView bv{};
        TEST(bv.data == nullptr);
        TEST(bv.size == 0);
    }

    // Create from pointer and size
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        TEST(bv.data == &val);
        TEST(bv.size == sizeof(val));
    }

    // read<T>() copies typed data
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        u32 result = bv.read<u32>(0);
        TEST(result == 42);
    }

    // read() copies raw data
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        u32 result = 0;
        bv.read(0, &result, sizeof(result));
        TEST(result == 42);
    }

    // read<T>() at offset
    {
        u8 data[8] = {0, 0, 0, 0, 0xAA, 0xBB, 0xCC, 0xDD};
        BinaryView bv{data, 8};
        u32 result = bv.read<u32>(4);
        TEST(result == 0xDDCCBBAA);
    }

    // ============================================================================
    // Span<T>
    // ============================================================================
    //
    // Span is a non-owning typed view (pointer + count). Supports array,
    // ptr+count, begin+end constructors, indexing, and range-for.
    //
    // Functions covered:
    // - Span() — default
    // - Span(T*, u64) — ptr + count
    // - Span(T*, T*) — begin + end
    // - Span(T (&)[N]) — array constructor
    // - operator[]
    // - begin() / end()

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

    // ============================================================================
    // Maybe
    // ============================================================================
    //
    // Maybe<T> is a lightweight optional type used for recoverable error
    // handling. It holds a boolean `has` and a union containing the value.
    // orElse(default) returns the value or a fallback; expect(msg) returns
    // the value or panics.
    //
    // Functions covered:
    // - some<T>(args...): create a filled Maybe
    // - has: check whether a value is present
    // - val: access the value (direct union access)
    // - orElse(T): unwrap or return default
    // - expect(StringView): unwrap or panic
    // - copy construction and assignment
    // - move construction and assignment
    {
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
}


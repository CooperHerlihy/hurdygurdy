#include "tests.hpp"
#include "hg/map.hpp"

void testMap()
{
    // ============================================================================
    // Map
    // ============================================================================
    //
    // Map is a move-only, heap-allocated open-addressing hash map (Robin Hood).
    // Supports add, get, has, remove, reset, resize, and forEach.

    // Default-constructed map is empty
    {
        Map<u32, f32> map;
        TEST(map.hasVal == nullptr);
        TEST(map.capacity == 0);
        TEST(map.count == 0);
    }

    // Construct with initial capacity
    {
        Map<u32, f32> map{16};
        TEST(map.capacity == 16);
        TEST(map.count == 0);
    }

    // add, get, has
    {
        Map<u32, f32> map;
        map.add(1, 1.5f);
        map.add(2, 2.5f);
        map.add(3, 3.5f);
        TEST(map.count == 3);
        TEST(map.has(1));
        TEST(map.has(2));
        TEST(map.has(3));
        TEST(!map.has(4));
        f32* v = map.get(1);
        TEST(v != nullptr);
        TEST(*v == 1.5f);
        TEST(map.get(4) == nullptr);
    }

    // Duplicate key overwrites but count stays same
    {
        Map<u32, f32> map;
        map.add(1, 1.0f);
        map.add(1, 2.0f);
        TEST(map.count == 1);
        TEST(*map.get(1) == 2.0f);
    }

    // Remove a key-value pair
    {
        Map<u32, f32> map;
        map.add(1, 1.0f);
        map.add(2, 2.0f);
        TEST(map.remove(1));
        TEST(!map.has(1));
        TEST(map.has(2));
        TEST(!map.remove(99)); // non-existent
    }

    // Remove with value output
    {
        Map<u32, f32> map;
        map.add(42, 3.14f);
        f32 out = 0;
        TEST(map.remove(42, &out));
        TEST(out == 3.14f);
    }

    // Collision handling with sequential keys hashing to same slot
    {
        Map<u32, f32> map{8};
        map.add(0, 0.0f);
        map.add(8, 8.0f);  // collides
        map.add(16, 16.0f); // collides
        TEST(map.count == 3);
        TEST(*map.get(0) == 0.0f);
        TEST(*map.get(8) == 8.0f);
        TEST(*map.get(16) == 16.0f);

        map.remove(8);
        TEST(!map.has(8));
        TEST(*map.get(0) == 0.0f);
        TEST(*map.get(16) == 16.0f);
    }

    // reset clears all entries
    {
        Lifecycle::stats.reset();
        Map<u32, Lifecycle> map;
        map.add(1, Lifecycle{});
        map.add(2, Lifecycle{});
        TEST(Lifecycle::stats.alive == 2);
        map.reset();
        TEST(map.count == 0);
        TEST(Lifecycle::stats.alive == 0);
    }

    // Resize grows the map
    {
        Map<u32, f32> map{4};
        map.add(1, 1.0f);
        map.add(2, 2.0f);
        map.resize(32);
        TEST(map.capacity == 32);
        TEST(map.count == 2);
        TEST(*map.get(1) == 1.0f);
        TEST(*map.get(2) == 2.0f);
    }

    // forEach visits all key-value pairs
    {
        Map<u32, f32> map;
        map.add(1, 10.0f);
        map.add(2, 20.0f);
        f64 sum = 0;
        map.forEach([&](u32* k, f32* v) { sum += static_cast<f64>(*k) + *v; });
        TEST(sum == 33.0);
    }

    // Move construct
    {
        Map<u32, f32> a;
        a.add(1, 1.0f);
        bool* oldHasVal = a.hasVal;
        Map<u32, f32> b = std::move(a);
        TEST(a.hasVal == nullptr);
        TEST(b.hasVal == oldHasVal);
        TEST(b.count == 1);
        TEST(*b.get(1) == 1.0f);
    }

    // Move assign
    {
        Map<u32, f32> a;
        a.add(5, 5.0f);
        Map<u32, f32> b;
        b = std::move(a);
        TEST(*b.get(5) == 5.0f);
    }

    // ============================================================================
    // MapTemp
    // ============================================================================
    //
    // MapTemp is an arena-allocated open-addressing hash map.

    // Default-constructed MapTemp is empty
    {
        MapTemp<u32, f32> map;
        TEST(map.arena == nullptr);
        TEST(map.capacity == 0);
        TEST(map.count == 0);
    }

    // Create with arena and initial capacity
    {
        ArenaScope arena = getScratch();
        MapTemp<u32, f32> map{arena, 16};
        TEST(map.arena == arena);
        TEST(map.capacity == 16);
        TEST(map.count == 0);
    }

    // add, get, has, remove
    {
        ArenaScope arena = getScratch();
        MapTemp<u32, f32> map{arena, 0};
        map.add(1, 1.5f);
        map.add(2, 2.5f);
        TEST(map.count == 2);
        TEST(map.has(1));
        TEST(*map.get(1) == 1.5f);
        map.remove(1);
        TEST(!map.has(1));
        TEST(map.has(2));
    }

    // forEach
    {
        ArenaScope arena = getScratch();
        MapTemp<u32, f32> map{arena, 0};
        map.add(1, 10.0f);
        map.add(2, 20.0f);
        f64 sum = 0;
        map.forEach([&](u32* k, f32* v) { sum += static_cast<f64>(*k) + *v; });
        TEST(sum == 33.0);
    }

    // ============================================================================
    // Map<String, V> — heterogenous lookups with owning string keys
    // ============================================================================
    //
    // String is move-only, so add() must use the rvalue overload. Lookups via
    // get/has/ to use the new template form and must accept String, StringView,
    // const char*, StringBuilder, and string literals, all hashing and
    // comparing consistently.

    // Add and look up via every key representation
    {
        Map<String, u32> map{16};
        map.add(String::create("alpha"), 1u);
        map.add(String::create("beta"), 2u);
        map.add(String::create("gamma"), 3u);
        TEST(map.count == 3);

        // by owning String
        TEST(map.has(String::create("beta")));
        TEST(*map.get(String::create("beta")) == 2u);

        // by StringView
        TEST(map.has(StringView{"alpha"}));
        TEST(*map.get(StringView{"gamma"}) == 3u);

        // by const char* variable
        const char* b = "beta";
        TEST(map.has(b));
        TEST(*map.get(b) == 2u);

        // by StringBuilder
        ArenaScope arena = getScratch();
        TEST(map.has(StringBuilder{arena, "alpha"}));
        TEST(*map.get(StringBuilder{arena, "gamma"}) == 3u);

        // by string literal (const char[N])
        TEST(map.has("alpha"));
        TEST(*map.get("beta") == 2u);

        // unknown key across all representations
        TEST(!map.has("delta"));
        TEST(map.get("delta") == nullptr);
        TEST(!map.has(StringView{"delta"}));

        // remove by StringView with value output
        u32 out = 0;
        TEST(map.remove(StringView{"beta"}, &out));
        TEST(out == 2u);
        TEST(!map.has("beta"));
        TEST(map.count == 2);

        // remove by literal
        TEST(map.remove("alpha"));
        TEST(!map.has(StringView{"alpha"}));
        TEST(map.count == 1);

        // remaining key still reachable by every representation
        TEST(map.has("gamma"));
        TEST(map.has(StringView{"gamma"}));
        TEST(*map.get(StringBuilder{arena, "gamma"}) == 3u);
    }

    // MapTemp<String, u32> mirrors the heap version
    {
        ArenaScope arena = getScratch();
        MapTemp<String, u32> map{arena, 16};
        map.add(String::create("alpha"), 1u);
        map.add(String::create("beta"), 2u);
        map.add(String::create("gamma"), 3u);
        TEST(map.count == 3);

        TEST(map.has(StringView{"alpha"}));
        TEST(*map.get(StringView{"gamma"}) == 3u);

        const char* b = "beta";
        TEST(map.has(b));
        TEST(*map.get(b) == 2u);

        TEST(map.has(StringBuilder{arena, "alpha"}));
        TEST(map.has("gamma"));
        TEST(!map.has("delta"));

        u32 out = 0;
        TEST(map.remove(StringView{"beta"}, &out));
        TEST(out == 2u);
        TEST(!map.has("beta"));
        TEST(map.count == 2);

        TEST(map.remove("alpha"));
        TEST(!map.has(StringView{"alpha"}));
        TEST(map.count == 1);
        TEST(map.has("gamma"));
    }

    // Map<String, String> — move-only key and value, no copies
    {
        Map<String, String> map{16};
        map.add(String::create("k1"), String::create("v1"));
        map.add(String::create("k2"), String::create("v2"));
        TEST(map.count == 2);
        TEST(map.has("k1"));
        TEST(map.has("k2"));
        TEST(*map.get("k1") == "v1");
        TEST(*map.get("k2") == "v2");

        String out = std::move(*map.get("k1"));
        TEST(out == "v1");

        TEST(map.remove("k2"));
        TEST(!map.has("k2"));
        TEST(map.count == 1);
    }

    // MapTemp<String, String> — move-only key and value in arena
    {
        ArenaScope arena = getScratch();
        MapTemp<String, String> map{arena, 16};
        map.add(String::create("k1"), String::create("v1"));
        map.add(String::create("k2"), String::create("v2"));
        TEST(map.count == 2);
        TEST(map.has(StringView{"k1"}));
        TEST(*map.get("k2") == "v2");
        TEST(map.remove("k1"));
        TEST(!map.has("k1"));
        TEST(map.count == 1);
    }

    // ============================================================================
    // Map<StringView, V> — non-owning keys, looked up by every representation
    // ============================================================================
    //
    // Stored StringViews point at stable literals, so hashing at add and at
    // lookup (including during robin-hood shifts) reads identical memory.

    // Heap Map<StringView, u32>
    {
        Map<StringView, u32> map{16};
        map.add("alpha", 1u);
        map.add("beta", 2u);
        map.add("gamma", 3u);
        TEST(map.count == 3);

        TEST(map.has(StringView{"alpha"}));
        TEST(*map.get(StringView{"gamma"}) == 3u);

        const char* b = "beta";
        TEST(map.has(b));
        TEST(*map.get(b) == 2u);

        ArenaScope arena = getScratch();
        TEST(map.has(StringBuilder{arena, "alpha"}));
        TEST(*map.get(StringBuilder{arena, "gamma"}) == 3u);

        TEST(map.has("alpha"));
        TEST(*map.get("beta") == 2u);
        TEST(!map.has("delta"));

        u32 out = 0;
        TEST(map.remove(StringView{"beta"}, &out));
        TEST(out == 2u);
        TEST(!map.has("beta"));
        TEST(map.count == 2);
        TEST(map.has("gamma"));
    }

    // MapTemp<StringView, u32>
    {
        ArenaScope arena = getScratch();
        MapTemp<StringView, u32> map{arena, 16};
        map.add("alpha", 1u);
        map.add("beta", 2u);
        map.add("gamma", 3u);
        TEST(map.count == 3);

        TEST(map.has(StringView{"alpha"}));
        TEST(*map.get(StringView{"gamma"}) == 3u);
        TEST(map.has("beta"));
        TEST(*map.get(StringBuilder{arena, "alpha"}) == 1u);
        TEST(!map.has("delta"));

        TEST(map.remove("beta"));
        TEST(!map.has(StringView{"beta"}));
        TEST(map.count == 2);
        TEST(map.has("gamma"));
    }

    // Collision pressure: many string keys in a tiny map stay reachable
    {
        Map<String, u32> map{4};
        const char* keys[] = {
            "red", "green", "blue", "cyan", "magenta",
            "yellow", "black", "white", "orange", "pink"
        };
        for (u32 i = 0; i < 10; ++i)
            map.add(String::create(keys[i]), static_cast<u32>(i));
        TEST(map.count == 10);
        for (u32 i = 0; i < 10; ++i)
        {
            TEST(map.has(keys[i]));
            TEST(*map.get(StringView{keys[i]}) == i);
            TEST(map.has(String::create(keys[i])));
        }
        TEST(!map.has("purple"));
    }
}


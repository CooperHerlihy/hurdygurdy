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
}


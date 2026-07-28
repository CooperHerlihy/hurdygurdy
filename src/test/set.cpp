#include "tests.hpp"

void testSet()
{
    // ============================================================================
    // Set
    // ============================================================================
    //
    // Set is a move-only, heap-allocated open-addressing hash set using
    // Robin Hood hashing. Supports add, has, remove, reset, resize, and forEach.

    // Default-constructed set is empty
    {
        Set<u32> set;
        TEST(set.hasVal == nullptr);
        TEST(set.capacity == 0);
        TEST(set.count == 0);
    }

    // Construct with initial capacity
    {
        Set<u32> set{16};
        TEST(set.capacity == 16);
        TEST(set.count == 0);
    }

    // add and has
    {
        Set<u32> set;
        set.add(10);
        set.add(20);
        set.add(30);
        TEST(set.count == 3);
        TEST(set.has(10));
        TEST(set.has(20));
        TEST(set.has(30));
        TEST(!set.has(40));
    }

    // Duplicate add does not increase count
    {
        Set<u32> set;
        set.add(42);
        set.add(42);
        TEST(set.count == 1);
    }

    // Remove an element
    {
        Set<u32> set;
        set.add(1);
        set.add(2);
        set.add(3);
        set.remove(2);
        TEST(set.count == 2);
        TEST(!set.has(2));
        TEST(set.has(1));
        TEST(set.has(3));
    }

    // Remove non-existent element is safe
    {
        Set<u32> set;
        set.add(5);
        set.remove(99);
        TEST(set.count == 1);
    }

    // Add and remove multiple elements with collision
    {
        Set<u32> set{8};
        set.add(0);
        set.add(8);  // collides with 0 in capacity=8
        set.add(16); // collides with both
        TEST(set.count == 3);
        TEST(set.has(0));
        TEST(set.has(8));
        TEST(set.has(16));

        set.remove(8);
        TEST(!set.has(8));
        TEST(set.has(0));
        TEST(set.has(16));

        set.remove(0);
        TEST(!set.has(0));
        TEST(set.has(16));
    }

    // reset clears all elements
    {
        Lifecycle::stats.reset();
        Set<Lifecycle> set;
        set.add(Lifecycle{});
        set.add(Lifecycle{});
        TEST(Lifecycle::stats.alive == 2);
        set.reset();
        TEST(set.count == 0);
        TEST(Lifecycle::stats.alive == 0);
    }

    // Resize grows the set
    {
        Set<u32> set{4};
        set.add(1);
        set.add(2);
        set.resize(32);
        TEST(set.capacity == 32);
        TEST(set.count == 2);
        TEST(set.has(1));
        TEST(set.has(2));
    }

    // forEach visits all elements
    {
        Set<u32> set;
        set.add(10);
        set.add(20);
        set.add(30);
        u64 sum = 0;
        set.forEach([&](u32* v) { sum += *v; });
        TEST(sum == 60);
    }

    // Move construct
    {
        Set<u32> a;
        a.add(1);
        a.add(2);
        bool* oldHasVal = a.hasVal;
        Set<u32> b = std::move(a);
        TEST(a.hasVal == nullptr);
        TEST(b.hasVal == oldHasVal);
        TEST(b.count == 2);
        TEST(b.has(1));
        TEST(b.has(2));
    }

    // Move assign
    {
        Set<u32> a;
        a.add(99);
        Set<u32> b;
        b = std::move(a);
        TEST(b.has(99));
    }

    // ============================================================================
    // SetTemp
    // ============================================================================
    //
    // SetTemp is an arena-allocated open-addressing hash set.

    // Default-constructed SetTemp is empty
    {
        SetTemp<u32> set;
        TEST(set.arena == nullptr);
        TEST(set.capacity == 0);
        TEST(set.count == 0);
    }

    // Create with arena and initial capacity
    {
        ArenaScope arena = getScratch();
        SetTemp<u32> set{arena, 16};
        TEST(set.arena == arena);
        TEST(set.capacity == 16);
        TEST(set.count == 0);
    }

    // add, has, remove
    {
        ArenaScope arena = getScratch();
        SetTemp<u32> set{arena, 0};
        set.add(10);
        set.add(20);
        TEST(set.count == 2);
        TEST(set.has(10));
        TEST(set.has(20));
        set.remove(10);
        TEST(!set.has(10));
        TEST(set.has(20));
    }

    // forEach visits all elements
    {
        ArenaScope arena = getScratch();
        SetTemp<u32> set{arena, 0};
        set.add(5);
        set.add(10);
        u64 sum = 0;
        set.forEach([&](u32* v) { sum += *v; });
        TEST(sum == 15);
    }
}


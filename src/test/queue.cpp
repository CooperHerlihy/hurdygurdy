#include "tests.hpp"

void testQueue()
{
    // ============================================================================
    // Queue
    // ============================================================================
    //
    // Queue is a move-only, heap-allocated double-ended ring buffer.
    // Supports pushFront, pushBack, popFront, popBack, and reserve.

    // Default-constructed queue is empty
    {
        Queue<u32> q;
        TEST(q.vals == nullptr);
        TEST(q.count == 0);
        TEST(q.capacity == 0);
    }

    // Construct with initial capacity
    {
        Queue<u32> q{16};
        TEST(q.vals != nullptr);
        TEST(q.capacity == 16);
        TEST(q.count == 0);
    }

    // pushBack and popFront (FIFO order)
    {
        Queue<u32> q;
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        TEST(q.count == 3);
        TEST(q.popFront() == 1);
        TEST(q.popFront() == 2);
        TEST(q.popFront() == 3);
        TEST(q.count == 0);
    }

    // pushFront and popBack (LIFO order from front)
    {
        Queue<u32> q;
        q.pushFront(1);
        q.pushFront(2);
        q.pushFront(3);
        TEST(q.count == 3);
        TEST(q.popBack() == 1);
        TEST(q.popBack() == 2);
        TEST(q.popBack() == 3);
        TEST(q.count == 0);
    }

    // pushFront and popFront (reversed order)
    {
        Queue<u32> q;
        q.pushFront(10);
        q.pushFront(20);
        TEST(q.popFront() == 20);
        TEST(q.popFront() == 10);
    }

    // pushBack and popBack (stack order)
    {
        Queue<u32> q;
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        TEST(q.popBack() == 3);
        TEST(q.popBack() == 2);
        TEST(q.popBack() == 1);
    }

    // Wrap-around behavior: popFront followed by pushBack reuses slots
    {
        Queue<u32> q;
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        TEST(q.popFront() == 1);
        TEST(q.popFront() == 2);
        q.pushBack(4);
        q.pushBack(5);
        TEST(q.count == 3);
        TEST(q.popFront() == 3);
        TEST(q.popFront() == 4);
        TEST(q.popFront() == 5);
    }

    // Wrap-around with pushFront
    {
        Queue<u32> q;
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        q.popFront(); // 1
        q.pushFront(0);
        TEST(q.count == 3);
        TEST(q.popFront() == 0);
        TEST(q.popFront() == 2);
        TEST(q.popFront() == 3);
    }

    // Reserve grows capacity
    {
        Queue<u32> q{4};
        q.pushBack(1);
        q.pushBack(2);
        q.reserve(16);
        TEST(q.capacity >= 16);
        TEST(q.count == 2);
        TEST(q.popFront() == 1);
        TEST(q.popFront() == 2);
    }

    // Move construct
    {
        Queue<u32> a;
        a.pushBack(1);
        a.pushBack(2);
        u32* oldVals = a.vals;
        Queue<u32> b = std::move(a);
        TEST(a.vals == nullptr);
        TEST(b.vals == oldVals);
        TEST(b.popFront() == 1);
        TEST(b.popFront() == 2);
    }

    // Move assign
    {
        Queue<u32> a;
        a.pushBack(10);
        Queue<u32> b;
        b.pushBack(20);
        b.popFront();
        b = std::move(a);
        TEST(b.popFront() == 10);
    }

    // pushBack const ref copies (0 moves)
    {
        Lifecycle::stats.reset();
        {
            Queue<Lifecycle> q;
            Lifecycle a;
            q.pushBack(a);
            TEST(a.valid);
            TEST(q.count == 1);
            TEST(Lifecycle::stats.copies == 1);
            TEST(Lifecycle::stats.moves == 0);
            q.popFront();
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // pushBack rvalue ref moves (0 copies)
    {
        Lifecycle::stats.reset();
        {
            Queue<Lifecycle> q;
            Lifecycle a;
            q.pushBack(std::move(a));
            TEST(!a.valid);
            TEST(q.count == 1);
            TEST(Lifecycle::stats.copies == 0);
            TEST(Lifecycle::stats.moves == 1);
            q.popFront();
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // pushFront const ref copies (0 moves)
    {
        Lifecycle::stats.reset();
        {
            Queue<Lifecycle> q;
            Lifecycle a;
            q.pushFront(a);
            TEST(a.valid);
            TEST(Lifecycle::stats.copies == 1);
            TEST(Lifecycle::stats.moves == 0);
            q.popBack();
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // pushFront rvalue ref moves (0 copies)
    {
        Lifecycle::stats.reset();
        {
            Queue<Lifecycle> q;
            Lifecycle a;
            q.pushFront(std::move(a));
            TEST(!a.valid);
            TEST(Lifecycle::stats.copies == 0);
            TEST(Lifecycle::stats.moves == 1);
            q.popBack();
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // popFront pops by move
    {
        Lifecycle::stats.reset();
        {
            Queue<Lifecycle> q;
            q.pushBack();
            Lifecycle val = q.popFront();
            TEST(val.valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 1);
        TEST(Lifecycle::stats.copies == 0);
    }

    // popBack pops by move
    {
        Lifecycle::stats.reset();
        {
            Queue<Lifecycle> q;
            q.pushBack();
            Lifecycle val = q.popBack();
            TEST(val.valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 1);
        TEST(Lifecycle::stats.copies == 0);
    }

    // ============================================================================
    // QueueTemp
    // ============================================================================
    //
    // QueueTemp is an arena-allocated double-ended ring buffer.

    // Default-constructed QueueTemp is empty
    {
        QueueTemp<u32> q;
        TEST(q.arena == nullptr);
        TEST(q.vals == nullptr);
        TEST(q.count == 0);
    }

    // Create with arena and initial capacity
    {
        ArenaScope arena = getScratch();
        QueueTemp<u32> q{arena, 16};
        TEST(q.arena == arena);
        TEST(q.vals != nullptr);
        TEST(q.capacity == 16);
        TEST(q.count == 0);
    }

    // pushBack and popFront
    {
        ArenaScope arena = getScratch();
        QueueTemp<u32> q{arena, 0};
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        TEST(q.count == 3);
        TEST(q.popFront() == 1);
        TEST(q.popFront() == 2);
        TEST(q.popFront() == 3);
    }

    // pushFront and popBack
    {
        ArenaScope arena = getScratch();
        QueueTemp<u32> q{arena, 0};
        q.pushFront(10);
        q.pushFront(20);
        TEST(q.popBack() == 10);
        TEST(q.popBack() == 20);
    }

    // Wrap-around behavior
    {
        ArenaScope arena = getScratch();
        QueueTemp<u32> q{arena, 4};
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        q.popFront(); // 1
        q.popFront(); // 2
        q.pushBack(4);
        q.pushBack(5);
        TEST(q.count == 3);
        TEST(q.popFront() == 3);
        TEST(q.popFront() == 4);
        TEST(q.popFront() == 5);
    }

    // Reserve grows capacity (non-extend path)
    {
        ArenaScope arena = getScratch();
        QueueTemp<u32> q{arena, 4};
        q.pushBack(1);
        q.pushBack(2);
        q.reserve(20);
        TEST(q.capacity >= 20);
        TEST(q.popFront() == 1);
        TEST(q.popFront() == 2);
    }

    // Move construct
    {
        ArenaScope arena = getScratch();
        QueueTemp<u32> a{arena, 0};
        a.pushBack(99);
        QueueTemp<u32> b = std::move(a);
        TEST(a.vals == nullptr);
        TEST(b.popFront() == 99);
    }
}

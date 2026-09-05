#include "tests.hpp"
#include "hg/queue.hpp"

using namespace hg;

TEST(testQueueDefault)
{
    Queue<u32> q;
    ASSERT(q.vals == nullptr);
    ASSERT(q.count == 0);
    ASSERT(q.capacity == 0);
}

TEST(testQueueInitialCapacity)
{
    Queue<u32> q{16};
    ASSERT(q.vals != nullptr);
    ASSERT(q.capacity == 16);
    ASSERT(q.count == 0);
}

TEST(testQueuePushBackPopFront)
{
    Queue<u32> q;
    q.pushBack(1);
    q.pushBack(2);
    q.pushBack(3);
    ASSERT(q.count == 3);
    ASSERT(q.popFront() == 1);
    ASSERT(q.popFront() == 2);
    ASSERT(q.popFront() == 3);
    ASSERT(q.count == 0);
}

TEST(testQueuePushFrontPopBack)
{
    Queue<u32> q;
    q.pushFront(1);
    q.pushFront(2);
    q.pushFront(3);
    ASSERT(q.count == 3);
    ASSERT(q.popBack() == 1);
    ASSERT(q.popBack() == 2);
    ASSERT(q.popBack() == 3);
    ASSERT(q.count == 0);
}

TEST(testQueuePushFrontPopFront)
{
    Queue<u32> q;
    q.pushFront(10);
    q.pushFront(20);
    ASSERT(q.popFront() == 20);
    ASSERT(q.popFront() == 10);
}

TEST(testQueuePushBackPopBack)
{
    Queue<u32> q;
    q.pushBack(1);
    q.pushBack(2);
    q.pushBack(3);
    ASSERT(q.popBack() == 3);
    ASSERT(q.popBack() == 2);
    ASSERT(q.popBack() == 1);
}

TEST(testQueueWrapAround)
{
    Queue<u32> q;
    q.pushBack(1);
    q.pushBack(2);
    q.pushBack(3);
    ASSERT(q.popFront() == 1);
    ASSERT(q.popFront() == 2);
    q.pushBack(4);
    q.pushBack(5);
    ASSERT(q.count == 3);
    ASSERT(q.popFront() == 3);
    ASSERT(q.popFront() == 4);
    ASSERT(q.popFront() == 5);
}

TEST(testQueueWrapAroundPushFront)
{
    Queue<u32> q;
    q.pushBack(1);
    q.pushBack(2);
    q.pushBack(3);
    q.popFront();
    q.pushFront(0);
    ASSERT(q.count == 3);
    ASSERT(q.popFront() == 0);
    ASSERT(q.popFront() == 2);
    ASSERT(q.popFront() == 3);
}

TEST(testQueueReserve)
{
    Queue<u32> q{4};
    q.pushBack(1);
    q.pushBack(2);
    q.reserve(16);
    ASSERT(q.capacity >= 16);
    ASSERT(q.count == 2);
    ASSERT(q.popFront() == 1);
    ASSERT(q.popFront() == 2);
}

TEST(testQueueMoveConstruct)
{
    Queue<u32> a;
    a.pushBack(1);
    a.pushBack(2);
    u32* oldVals = a.vals;
    Queue<u32> b = std::move(a);
    ASSERT(a.vals == nullptr);
    ASSERT(b.vals == oldVals);
    ASSERT(b.popFront() == 1);
    ASSERT(b.popFront() == 2);
}

TEST(testQueueMoveAssign)
{
    Queue<u32> a;
    a.pushBack(10);
    Queue<u32> b;
    b.pushBack(20);
    b.popFront();
    b = std::move(a);
    ASSERT(b.popFront() == 10);
}

TEST(testQueuePushBackCopy)
{
    Lifecycle::stats.reset();
    {
        Queue<Lifecycle> q;
        Lifecycle a;
        q.pushBack(a);
        ASSERT(a.valid);
        ASSERT(q.count == 1);
        ASSERT(Lifecycle::stats.copies == 1);
        ASSERT(Lifecycle::stats.moves == 0);
        q.popFront();
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testQueuePushBackMove)
{
    Lifecycle::stats.reset();
    {
        Queue<Lifecycle> q;
        Lifecycle a;
        q.pushBack(std::move(a));
        ASSERT(!a.valid);
        ASSERT(q.count == 1);
        ASSERT(Lifecycle::stats.copies == 0);
        ASSERT(Lifecycle::stats.moves == 1);
        q.popFront();
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testQueuePushFrontCopy)
{
    Lifecycle::stats.reset();
    {
        Queue<Lifecycle> q;
        Lifecycle a;
        q.pushFront(a);
        ASSERT(a.valid);
        ASSERT(Lifecycle::stats.copies == 1);
        ASSERT(Lifecycle::stats.moves == 0);
        q.popBack();
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testQueuePushFrontMove)
{
    Lifecycle::stats.reset();
    {
        Queue<Lifecycle> q;
        Lifecycle a;
        q.pushFront(std::move(a));
        ASSERT(!a.valid);
        ASSERT(Lifecycle::stats.copies == 0);
        ASSERT(Lifecycle::stats.moves == 1);
        q.popBack();
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testQueuePopFrontMove)
{
    Lifecycle::stats.reset();
    {
        Queue<Lifecycle> q;
        q.pushBack();
        Lifecycle val = q.popFront();
        ASSERT(val.valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 1);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testQueuePopBackMove)
{
    Lifecycle::stats.reset();
    {
        Queue<Lifecycle> q;
        q.pushBack();
        Lifecycle val = q.popBack();
        ASSERT(val.valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 1);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testQueueTempDefault)
{
    QueueTemp<u32> q;
    ASSERT(q.arena == nullptr);
    ASSERT(q.vals == nullptr);
    ASSERT(q.count == 0);
}

TEST(testQueueTempCreate)
{
    ArenaScope arena = getScratch();
    QueueTemp<u32> q{arena, 16};
    ASSERT(q.arena == arena);
    ASSERT(q.vals != nullptr);
    ASSERT(q.capacity == 16);
    ASSERT(q.count == 0);
}

TEST(testQueueTempPushBackPopFront)
{
    ArenaScope arena = getScratch();
    QueueTemp<u32> q{arena, 0};
    q.pushBack(1);
    q.pushBack(2);
    q.pushBack(3);
    ASSERT(q.count == 3);
    ASSERT(q.popFront() == 1);
    ASSERT(q.popFront() == 2);
    ASSERT(q.popFront() == 3);
}

TEST(testQueueTempPushFrontPopBack)
{
    ArenaScope arena = getScratch();
    QueueTemp<u32> q{arena, 0};
    q.pushFront(10);
    q.pushFront(20);
    ASSERT(q.popBack() == 10);
    ASSERT(q.popBack() == 20);
}

TEST(testQueueTempWrapAround)
{
    ArenaScope arena = getScratch();
    QueueTemp<u32> q{arena, 4};
    q.pushBack(1);
    q.pushBack(2);
    q.pushBack(3);
    q.popFront();
    q.popFront();
    q.pushBack(4);
    q.pushBack(5);
    ASSERT(q.count == 3);
    ASSERT(q.popFront() == 3);
    ASSERT(q.popFront() == 4);
    ASSERT(q.popFront() == 5);
}

TEST(testQueueTempReserve)
{
    ArenaScope arena = getScratch();
    QueueTemp<u32> q{arena, 4};
    q.pushBack(1);
    q.pushBack(2);
    q.reserve(20);
    ASSERT(q.capacity >= 20);
    ASSERT(q.popFront() == 1);
    ASSERT(q.popFront() == 2);
}

TEST(testQueueTempMoveConstruct)
{
    ArenaScope arena = getScratch();
    QueueTemp<u32> a{arena, 0};
    a.pushBack(99);
    QueueTemp<u32> b = std::move(a);
    ASSERT(a.vals == nullptr);
    ASSERT(b.popFront() == 99);
}

TEST(testQueueSingleElement)
{
    Queue<u32> q;
    q.pushBack(42);
    ASSERT(q.count == 1);
    ASSERT(q.popFront() == 42);
    ASSERT(q.count == 0);
}

TEST(testQueuePushPopAlternating)
{
    Queue<u32> q;
    for (u32 i = 0; i < 16; ++i)
    {
        q.pushBack(i);
        ASSERT(q.popFront() == i);
        ASSERT(q.count == 0);
    }
}

TEST(testQueueReserveDoesNotShrink)
{
    Queue<u32> q{16};
    q.pushBack(1);
    q.reserve(4);
    ASSERT(q.capacity == 16);
    ASSERT(q.popFront() == 1);
}

TEST(testQueueReserveZero)
{
    Queue<u32> q{8};
    q.reserve(0);
    ASSERT(q.capacity == 8);
}

TEST(testQueuePushBackExceedCapacity)
{
    Queue<u32> q{4};
    for (u32 i = 0; i < 16; ++i)
        q.pushBack(i);
    ASSERT(q.capacity >= 16);
    ASSERT(q.count == 16);
    for (u32 i = 0; i < 16; ++i)
        ASSERT(q.popFront() == i);
}

TEST(testQueuePushFrontExceedCapacity)
{
    Queue<u32> q{4};
    for (u32 i = 0; i < 16; ++i)
        q.pushFront(i);
    ASSERT(q.capacity >= 16);
    ASSERT(q.count == 16);
    for (u32 i = 0; i < 16; ++i)
        ASSERT(q.popBack() == i);
}

TEST(testQueueTempSingleElement)
{
    ArenaScope arena = getScratch();
    QueueTemp<u32> q{arena, 0};
    q.pushBack(42);
    ASSERT(q.count == 1);
    ASSERT(q.popFront() == 42);
    ASSERT(q.count == 0);
}

TEST(testQueueTempPushPopAlternating)
{
    ArenaScope arena = getScratch();
    QueueTemp<u32> q{arena, 0};
    for (u32 i = 0; i < 16; ++i)
    {
        q.pushBack(i);
        ASSERT(q.popFront() == i);
        ASSERT(q.count == 0);
    }
}

TEST(testQueueTempReserveDoesNotShrink)
{
    ArenaScope arena = getScratch();
    QueueTemp<u32> q{arena, 16};
    q.pushBack(1);
    q.reserve(4);
    ASSERT(q.capacity == 16);
    ASSERT(q.popFront() == 1);
}

TEST(testQueueTempMoveAssign)
{
    ArenaScope arena = getScratch();
    QueueTemp<u32> a{arena, 0};
    a.pushBack(10);
    QueueTemp<u32> b{arena, 0};
    b.pushBack(20);
    b.popFront();
    b = std::move(a);
    ASSERT(a.vals == nullptr);
    ASSERT(b.popFront() == 10);
}

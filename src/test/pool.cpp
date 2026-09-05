#include "tests.hpp"
#include "hg/pool.hpp"

using namespace hg;

TEST(testPoolDefault)
{
    Pool<Lifecycle> pool;
    ASSERT(pool.prealloc.count == 0);
    ASSERT(pool.inactive.count == 0);
}

TEST(testPoolAllocFree)
{
    Lifecycle::stats.reset();
    Pool<Lifecycle> pool;
    Lifecycle* a = pool.alloc();
    ASSERT(a != nullptr);
    ASSERT(a->valid);
    ASSERT(Lifecycle::stats.alive == 1);
    pool.free(a);
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 1);
}

TEST(testPoolFreeReuse)
{
    Pool<u32> pool;
    u32* a = pool.alloc(10u);
    u32* b = pool.alloc(20u);
    ASSERT(*a == 10);
    ASSERT(*b == 20);
    pool.free(a);
    pool.free(b);
    u32* c = pool.alloc(30u);
    ASSERT(c == b);
    ASSERT(*c == 30u);
    u32* d = pool.alloc(40u);
    ASSERT(d == a);
    pool.free(c);
    pool.free(d);
}

TEST(testPoolMultipleBlocks)
{
    Pool<u32> pool;
    static constexpr u32 n = 2500;
    u32* pts[2500];
    for (u32 i = 0; i < n; ++i)
    {
        pts[i] = pool.alloc(i);
        ASSERT(*pts[i] == i);
    }
    ASSERT(pool.prealloc.count >= 3);
    for (u32 i = 0; i < n; ++i)
        pool.free(pts[i]);
}

TEST(testPoolFreeNullptr)
{
    Pool<u32> pool;
    pool.free(nullptr);
}

TEST(testHandlePoolCreate)
{
    HandlePool pool = HandlePool{};
    ASSERT(nullHandle.id == (u32)-1);
    ASSERT(pool.handles.count == 0);
}

TEST(testHandlePoolAlloc)
{
    HandlePool pool = HandlePool{};
    Handle a = pool.alloc();
    Handle b = pool.alloc();
    ASSERT(pool.alive(a));
    ASSERT(pool.alive(b));
    ASSERT(a.idx() == 0);
    ASSERT(b.idx() == 1);
}

TEST(testHandlePoolFree)
{
    HandlePool pool = HandlePool{};
    Handle a = pool.alloc();
    ASSERT(pool.alive(a));
    pool.free(a);
    ASSERT(!pool.alive(a));
}

TEST(testHandlePoolGeneration)
{
    HandlePool pool = HandlePool{};
    Handle a = pool.alloc();
    u32 idx = a.idx();
    pool.free(a);
    Handle b = pool.alloc();
    ASSERT(b.idx() == idx);
    ASSERT(b.id != a.id);
    ASSERT(pool.alive(b));
    ASSERT(!pool.alive(a));
}

TEST(testHandlePoolReset)
{
    HandlePool pool = HandlePool{};
    Handle a = pool.alloc();
    Handle b = pool.alloc();
    pool.reset();
    ASSERT(!pool.alive(a));
    ASSERT(!pool.alive(b));
    Handle c = pool.alloc();
    ASSERT(c.idx() == 0);
}

TEST(testPoolAllocConstructorArgs)
{
    Pool<u32> pool;
    u32* a = pool.alloc(42u);
    u32* b = pool.alloc(100u);
    u32* c = pool.alloc(0u);
    ASSERT(*a == 42);
    ASSERT(*b == 100);
    ASSERT(*c == 0);
    pool.free(a);
    pool.free(b);
    pool.free(c);
}

TEST(testPoolAllocFreeCycle)
{
    Lifecycle::stats.reset();
    Pool<Lifecycle> pool;
    for (u32 i = 0; i < 100; ++i)
    {
        Lifecycle* a = pool.alloc();
        Lifecycle* b = pool.alloc();
        pool.free(a);
        pool.free(b);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.ctors == 200);
    ASSERT(Lifecycle::stats.dtors == 200);
}

TEST(testPoolAllocManyThenFreeAll)
{
    Lifecycle::stats.reset();
    Pool<Lifecycle> pool;
    Lifecycle* objs[100];
    for (u32 i = 0; i < 100; ++i)
        objs[i] = pool.alloc();
    ASSERT(Lifecycle::stats.alive == 100);
    for (u32 i = 0; i < 100; ++i)
        pool.free(objs[i]);
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 100);
}

TEST(testHandleIdx)
{
    Handle h0{0};
    Handle h1{1};
    Handle h2{(1 << 24) + 5};
    ASSERT(h0.idx() == 0);
    ASSERT(h1.idx() == 1);
    ASSERT(h2.idx() == 5);
}

TEST(testHandleGeneration)
{
    Handle h0{0};
    Handle h1{1};
    Handle h2{(1 << 24) + 5};
    Handle h3{(2 << 24) + 3};
    ASSERT(h0.generation() == 0);
    ASSERT(h1.generation() == 0);
    ASSERT(h2.generation() == (u32)(1 << 24));
    ASSERT(h3.generation() == (u32)(2 << 24));
}

TEST(testHandleNextGeneration)
{
    Handle h{42};
    Handle next = h.nextGeneration();
    ASSERT(next.idx() == 42);
    ASSERT(next.generation() == (u32)(1 << 24));
    Handle next2 = next.nextGeneration();
    ASSERT(next2.idx() == 42);
    ASSERT(next2.generation() == (u32)(2 << 24));
}

TEST(testHandlePoolManyAllocs)
{
    HandlePool pool = HandlePool{};
    static constexpr u32 n = 100;
    Handle handles[n];
    for (u32 i = 0; i < n; ++i)
    {
        handles[i] = pool.alloc();
        ASSERT(pool.alive(handles[i]));
        ASSERT(handles[i].idx() == i);
    }
    for (u32 i = 0; i < n; ++i)
        ASSERT(pool.alive(handles[i]));
}

TEST(testHandlePoolAllocFreeAllocMany)
{
    HandlePool pool = HandlePool{};
    static constexpr u32 n = 50;
    Handle first[n];
    Handle second[n];
    for (u32 i = 0; i < n; ++i)
        first[i] = pool.alloc();
    for (u32 i = 0; i < n; ++i)
    {
        pool.free(first[i]);
        second[i] = pool.alloc();
        ASSERT(second[i].idx() == first[i].idx());
        ASSERT(second[i].generation() > first[i].generation());
        ASSERT(pool.alive(second[i]));
        ASSERT(!pool.alive(first[i]));
    }
}

TEST(testPoolMoveConstruct)
{
    Lifecycle::stats.reset();
    Pool<Lifecycle> pool;
    Lifecycle* a = pool.alloc();
    Lifecycle* b = pool.alloc();
    ASSERT(Lifecycle::stats.alive == 2);
    Pool<Lifecycle> moved{std::move(pool)};
    ASSERT(Lifecycle::stats.alive == 2);
    ASSERT(pool.prealloc.count == 0);
    ASSERT(pool.inactive.count == 0);
    moved.free(a);
    moved.free(b);
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testPoolMoveAssign)
{
    Lifecycle::stats.reset();
    Pool<Lifecycle> pool;
    Lifecycle* a = pool.alloc();
    Lifecycle* b = pool.alloc();
    ASSERT(Lifecycle::stats.alive == 2);
    Pool<Lifecycle> target;
    target = std::move(pool);
    ASSERT(Lifecycle::stats.alive == 2);
    ASSERT(pool.prealloc.count == 0);
    ASSERT(pool.inactive.count == 0);
    target.free(a);
    target.free(b);
    ASSERT(Lifecycle::stats.alive == 0);
}

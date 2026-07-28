#include "tests.hpp"

void testPool()
{
    // ============================================================================
    // Pool
    // ============================================================================
    //
    // Pool is a fixed-block object pool allocator. Allocates in blocks of 1024
    // objects, reuses freed slots. Move-only.

    // Default-constructed pool is empty
    {
        Pool<Lifecycle> pool;
        TEST(pool.prealloc.count == 0);
        TEST(pool.inactive.count == 0);
    }

    // alloc creates a new object, free returns it to the pool
    {
        Lifecycle::stats.reset();
        Pool<Lifecycle> pool;
        Lifecycle* a = pool.alloc();
        TEST(a != nullptr);
        TEST(a->valid);
        TEST(Lifecycle::stats.alive == 1);
        pool.free(a);
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 1);
    }

    // Free returns memory to inactive pool for reuse
    {
        Pool<u32> pool;
        u32* a = pool.alloc(10u);
        u32* b = pool.alloc(20u);
        TEST(*a == 10);
        TEST(*b == 20);
        pool.free(a);
        pool.free(b);
        u32* c = pool.alloc(30u);
        TEST(c == b);
        TEST(*c == 30u);
        u32* d = pool.alloc(40u);
        TEST(d == a);
        pool.free(c);
        pool.free(d);
    }

    // Multiple blocks allocated beyond 1024 items
    {
        Pool<u32> pool;
        static constexpr u32 n = 2500;
        u32* pts[2500];
        for (u32 i = 0; i < n; ++i)
        {
            pts[i] = pool.alloc(i);
            TEST(*pts[i] == i);
        }
        TEST(pool.prealloc.count >= 3);
        for (u32 i = 0; i < n; ++i)
            pool.free(pts[i]);
    }

    // Free nullptr is safe
    {
        Pool<u32> pool;
        pool.free(nullptr);
    }

    // ============================================================================
    // HandlePool
    // ============================================================================
    //
    // HandlePool provides generation-counted handle allocation. Handles are
    // 32-bit values with 24-bit index and 8-bit generation.

    // handlePoolCreate returns a valid pool with handle 0 reserved (null handle)
    {
        HandlePool pool = handlePoolCreate();
        HG_DEFER(handlePoolDestroy(&pool));
        TEST(handleNull.id == 0);
        TEST(pool.handles.count == 1);
    }

    // handlePoolAlloc returns sequential handles
    {
        HandlePool pool = handlePoolCreate();
        HG_DEFER(handlePoolDestroy(&pool));
        Handle a = handlePoolAlloc(&pool);
        Handle b = handlePoolAlloc(&pool);
        TEST(handlePoolAlive(&pool, a));
        TEST(handlePoolAlive(&pool, b));
        TEST(handleIdx(a) == 1);
        TEST(handleIdx(b) == 2);
    }

    // handlePoolFree invalidates handle
    {
        HandlePool pool = handlePoolCreate();
        HG_DEFER(handlePoolDestroy(&pool));
        Handle a = handlePoolAlloc(&pool);
        TEST(handlePoolAlive(&pool, a));
        handlePoolFree(&pool, a);
        TEST(!handlePoolAlive(&pool, a));
    }

    // Freed handle is reused with incremented generation
    {
        HandlePool pool = handlePoolCreate();
        HG_DEFER(handlePoolDestroy(&pool));
        Handle a = handlePoolAlloc(&pool);
        u32 idx = handleIdx(a);
        handlePoolFree(&pool, a);
        Handle b = handlePoolAlloc(&pool);
        TEST(handleIdx(b) == idx);
        TEST(b.id != a.id);
        TEST(handlePoolAlive(&pool, b));
        TEST(!handlePoolAlive(&pool, a));
    }

    // handlePoolReset invalidates all handles
    {
        HandlePool pool = handlePoolCreate();
        HG_DEFER(handlePoolDestroy(&pool));
        Handle a = handlePoolAlloc(&pool);
        Handle b = handlePoolAlloc(&pool);
        handlePoolReset(&pool);
        TEST(!handlePoolAlive(&pool, a));
        TEST(!handlePoolAlive(&pool, b));
        Handle c = handlePoolAlloc(&pool);
        TEST(handleIdx(c) == 1);
    }
}


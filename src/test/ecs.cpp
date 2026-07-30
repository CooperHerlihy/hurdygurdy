#include "tests.hpp"

void testEcs()
{
    // ============================================================================
    // Entity
    // ============================================================================

    {
        Entity e;
        TEST(e == nullEntity);
        TEST(e.handle == nullHandle);
    }

    {
        Entity a{Handle{1}};
        Entity b{Handle{1}};
        Entity c{Handle{2}};
        TEST(a == b);
        TEST(a != c);
    }

    // ============================================================================
    // spawn / alive / despawn / reset
    // ============================================================================

    {
        Ecs<> ecs = Ecs<>::create();
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        TEST(a != nullEntity);
        TEST(b != nullEntity);
        TEST(a != b);
        TEST(a.handle.idx() == 0);
        TEST(b.handle.idx() == 1);
    }

    {
        Ecs<> ecs = Ecs<>::create();
        TEST(!ecs.alive(nullEntity));
    }

    {
        Ecs<> ecs = Ecs<>::create();
        Entity e = ecs.spawn();
        TEST(ecs.alive(e));
        ecs.despawn(e);
        TEST(!ecs.alive(e));
    }

    {
        Ecs<> ecs = Ecs<>::create();
        Entity a = ecs.spawn();
        u32 idx = a.handle.idx();
        ecs.despawn(a);
        Entity b = ecs.spawn();
        TEST(b.handle.idx() == idx);
        TEST(b.handle.id != a.handle.id);
        TEST(ecs.alive(b));
        TEST(!ecs.alive(a));
    }

    {
        Ecs<> ecs = Ecs<>::create();
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        ecs.reset();
        TEST(!ecs.alive(a));
        TEST(!ecs.alive(b));
    }

    // ============================================================================
    // Single component: add / has / get / remove
    // ============================================================================

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e = ecs.spawn();
        TEST(!ecs.has<u32>(e));
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e = ecs.spawn();
        u32& c = ecs.add<u32>(e);
        c = 42;
        TEST(ecs.has<u32>(e));
        TEST(ecs.get<u32>(e) == 42);
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 7;
        ecs.get<u32>(e) = 99;
        TEST(ecs.get<u32>(e) == 99);
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e = ecs.spawn();
        ecs.add<u32>(e);
        TEST(ecs.has<u32>(e));
        ecs.remove<u32>(e);
        TEST(!ecs.has<u32>(e));
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 10;
        ecs.remove<u32>(e);
        ecs.add<u32>(e) = 20;
        TEST(ecs.get<u32>(e) == 20);
    }

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 1;
        ecs.add<f32>(e) = 2.0f;
        ecs.despawn(e);
        TEST(!ecs.alive(e));
        TEST(ecs.count<u32>() == 0);
        TEST(ecs.count<f32>() == 0);
    }

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1);
        ecs.add<f32>(e2);
        ecs.reset();
        TEST(ecs.count<u32>() == 0);
        TEST(ecs.count<f32>() == 0);
        { Entity e3 = ecs.spawn(); TEST(ecs.alive(e3)); }
    }

    // ============================================================================
    // count / getEntities / getComponents / getEntity
    // ============================================================================

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1);
        ecs.add<u32>(e2);
        TEST(ecs.count<u32>() == 2);
        ecs.remove<u32>(e1);
        TEST(ecs.count<u32>() == 1);
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1);
        ecs.add<u32>(e2);
        Span<const Entity> ents = ecs.getEntities<u32>();
        TEST(ents.count == 2);
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;
        Span<u32> comps = ecs.getComponents<u32>();
        TEST(comps.count == 2);
        u64 sum = 0;
        for (u32 i = 0; i < comps.count; ++i)
            sum += comps[i];
        TEST(sum == 30);
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e = ecs.spawn();
        u32& c = ecs.add<u32>(e);
        Entity back = ecs.getEntity(c);
        TEST(back == e);
    }

    // ============================================================================
    // Multiple component types on same entity
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        Entity e3 = ecs.spawn();

        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 1.0f;
        ecs.add<u32>(e2) = 2;

        TEST(ecs.has<u32>(e1));
        TEST(ecs.has<f32>(e1));
        TEST(ecs.has<u32>(e2));
        TEST(!ecs.has<f32>(e2));
        TEST(!ecs.has<u32>(e3));
        TEST(!ecs.has<f32>(e3));

        TEST(ecs.get<u32>(e1) == 1);
        TEST(ecs.get<f32>(e1) == 1.0f);
        TEST(ecs.get<u32>(e2) == 2);
    }

    // ============================================================================
    // hasAll / hasAny
    // ============================================================================

    {
        Ecs<u32, f32, u64> ecs = Ecs<u32, f32, u64>::create();
        Entity e = ecs.spawn();
        ecs.add<u32>(e);
        ecs.add<f32>(e);
        TEST((ecs.hasAll<u32, f32>(e)));
        TEST((!ecs.hasAll<u32, u64>(e)));
        TEST((!ecs.hasAll<u32, f32, u64>(e)));
    }

    {
        Ecs<u32, f32, u64> ecs = Ecs<u32, f32, u64>::create();
        Entity e = ecs.spawn();
        ecs.add<u32>(e);
        TEST((ecs.hasAny<u32, f32>(e)));
        TEST((ecs.hasAny<u32, u64>(e)));
        TEST((!ecs.hasAny<f32, u64>(e)));
    }

    // ============================================================================
    // forEach single component type
    // ============================================================================

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        u64 sum = 0;
        auto fn = [&](Entity e, u32& v)
        {
            sum += v;
            (void)e;
        };
        ecs.forEach<decltype(fn), u32>(fn);
        TEST(sum == 30);
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        u64 sum = 0;
        auto fn = [&](Entity e)
        {
            sum += ecs.get<u32>(e);
        };
        ecs.forEach<decltype(fn), u32>(fn);
        TEST(sum == 30);
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        u64 sum = 0;
        auto fn = [&](u32& v)
        {
            sum += v;
        };
        ecs.forEach<decltype(fn), u32>(fn);
        TEST(sum == 30);
    }

    // ============================================================================
    // forEachPar single component type
    // ============================================================================

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        hg::SpinLock lk;
        u64 sum = 0;
        auto fn = [&](Entity e, u32& v)
        {
            hg::SpinLockScope scope{&lk};
            sum += v;
            (void)e;
        };
        ecs.forEachPar<decltype(fn), u32>(fn);
        TEST(sum == 30);
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        hg::SpinLock lk;
        u64 sum = 0;
        auto fn = [&](Entity e)
        {
            hg::SpinLockScope scope{&lk};
            sum += ecs.get<u32>(e);
        };
        ecs.forEachPar<decltype(fn), u32>(fn);
        TEST(sum == 30);
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        hg::SpinLock lk;
        u64 sum = 0;
        auto fn = [&](u32& v)
        {
            hg::SpinLockScope scope{&lk};
            sum += v;
        };
        ecs.forEachPar<decltype(fn), u32>(fn);
        TEST(sum == 30);
    }

    // ============================================================================
    // forEach multiple component types
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        Entity e3 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;
        ecs.add<u32>(e3) = 3;

        u64 sum = 0;
        auto fn = [&](Entity e, u32& a, f32& b)
        {
            sum += a + (u64)b;
            (void)e;
        };
        ecs.forEach<decltype(fn), u32, f32>(fn);
        TEST(sum == 33);
    }

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;

        u64 sum = 0;
        auto fn = [&](Entity e)
        {
            sum += ecs.get<u32>(e);
        };
        ecs.forEach<decltype(fn), u32, f32>(fn);
        TEST(sum == 3);
    }

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;

        u64 sum = 0;
        auto fn = [&](u32& a, f32& b)
        {
            sum += a + (u64)b;
        };
        ecs.forEach<decltype(fn), u32, f32>(fn);
        TEST(sum == 33);
    }

    // ============================================================================
    // forEachPar multiple component types
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;

        hg::SpinLock lk;
        u64 sum = 0;
        auto fn = [&](Entity e, u32& a, f32& b)
        {
            hg::SpinLockScope scope{&lk};
            sum += a + (u64)b;
            (void)e;
        };
        ecs.forEachPar<decltype(fn), u32, f32>(fn);
        TEST(sum == 33);
    }

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;

        hg::SpinLock lk;
        u64 sum = 0;
        auto fn = [&](Entity e)
        {
            hg::SpinLockScope scope{&lk};
            sum += ecs.get<u32>(e);
        };
        ecs.forEachPar<decltype(fn), u32, f32>(fn);
        TEST(sum == 3);
    }

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;

        hg::SpinLock lk;
        u64 sum = 0;
        auto fn = [&](u32& a, f32& b)
        {
            hg::SpinLockScope scope{&lk};
            sum += a + (u64)b;
        };
        ecs.forEachPar<decltype(fn), u32, f32>(fn);
        TEST(sum == 33);
    }

    // ============================================================================
    // Component lifecycle with Lifecycle type
    // ============================================================================

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs = Ecs<Lifecycle>::create();
            Entity e = ecs.spawn();
            ecs.add<Lifecycle>(e);
            TEST(Lifecycle::stats.alive == 1);
            TEST(Lifecycle::stats.ctors == 1);
        }
    }

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs = Ecs<Lifecycle>::create();
            Entity e = ecs.spawn();
            ecs.add<Lifecycle>(e);
            TEST(Lifecycle::stats.alive == 1);
            ecs.remove<Lifecycle>(e);
            TEST(Lifecycle::stats.alive == 0);
            TEST(Lifecycle::stats.dtors == 1);
        }
    }

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs = Ecs<Lifecycle>::create();
            Entity e1 = ecs.spawn();
            Entity e2 = ecs.spawn();
            ecs.add<Lifecycle>(e1);
            ecs.add<Lifecycle>(e2);
            TEST(Lifecycle::stats.alive == 2);
            ecs.reset();
            TEST(Lifecycle::stats.alive == 0);
            TEST(Lifecycle::stats.dtors == 2);
        }
    }

    // ============================================================================
    // Large number of entities
    // ============================================================================

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        static constexpr u32 n = 100;
        Entity entities[100];
        for (u32 i = 0; i < n; ++i)
        {
            entities[i] = ecs.spawn();
            ecs.add<u32>(entities[i]) = i;
        }
        TEST(ecs.count<u32>() == n);

        for (u32 i = 0; i < n; ++i)
            TEST(ecs.get<u32>(entities[i]) == i);

        for (u32 i = 0; i < n; i += 2)
            ecs.remove<u32>(entities[i]);
        TEST(ecs.count<u32>() == n / 2);
    }

    // ============================================================================
    // Entity reuse after despawn with multiple component types
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 5;
        ecs.add<f32>(e) = 3.14f;
        ecs.despawn(e);

        Entity f = ecs.spawn();
        ecs.add<u32>(f) = 10;
        ecs.add<f32>(f) = 2.71f;
        TEST(ecs.get<u32>(f) == 10);
        TEST(ecs.get<f32>(f) == 2.71f);
        TEST(ecs.count<u32>() == 1);
        TEST(ecs.count<f32>() == 1);
    }

    // ============================================================================
    // Remove and re-add with different entity in between (swap bookkeeping)
    // ============================================================================

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        ecs.add<u32>(a) = 1;
        ecs.add<u32>(b) = 2;
        ecs.remove<u32>(a);
        TEST(!ecs.has<u32>(a));
        TEST(ecs.has<u32>(b));
        TEST(ecs.get<u32>(b) == 2);
        TEST(ecs.count<u32>() == 1);
    }

    // ============================================================================
    // Repeated add/remove cycles on same entity
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e = ecs.spawn();
        for (u32 i = 0; i < 10; ++i)
        {
            ecs.add<u32>(e) = i;
            TEST(ecs.has<u32>(e));
            TEST(ecs.get<u32>(e) == i);
            ecs.remove<u32>(e);
            TEST(!ecs.has<u32>(e));
        }
    }

    // ============================================================================
    // Despawn entity with no components (edge case)
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e = ecs.spawn();
        ecs.despawn(e);
        TEST(!ecs.alive(e));
    }

    // ============================================================================
    // Query zero-component state
    // ============================================================================

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        TEST(ecs.count<u32>() == 0);
        Span<const Entity> ents = ecs.getEntities<u32>();
        TEST(ents.count == 0);
        Span<u32> comps = ecs.getComponents<u32>();
        TEST(comps.count == 0);
    }

    // ============================================================================
    // Remove from middle vs end to exercise swap bookkeeping
    // ============================================================================

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        Entity c = ecs.spawn();
        ecs.add<u32>(a) = 1;
        ecs.add<u32>(b) = 2;
        ecs.add<u32>(c) = 3;
        // Remove from middle (b): removes index 1, swaps c into index 1
        ecs.remove<u32>(b);
        TEST(!ecs.has<u32>(b));
        TEST(ecs.has<u32>(a));
        TEST(ecs.has<u32>(c));
        TEST(ecs.get<u32>(a) == 1);
        TEST(ecs.get<u32>(c) == 3);
        // Remove last (end): no swap needed
        ecs.remove<u32>(c);
        TEST(!ecs.has<u32>(c));
        TEST(ecs.has<u32>(a));
        TEST(ecs.get<u32>(a) == 1);
        // Remove first: the only remaining
        ecs.remove<u32>(a);
        TEST(ecs.count<u32>() == 0);
    }

    // ============================================================================
    // Direct EcsComponent<T> API
    // ============================================================================

    {
        HandlePool pool = HandlePool::create();
        Entity e = {pool.alloc()};
        EcsComponent<u32> cs;
        u32& ref = cs.add(e);
        ref = 42;
        TEST(cs.has(e));
        TEST(cs.get(e) == 42);
        Entity back = cs.getEntity(ref);
        TEST(back == e);
        cs.remove(e);
        TEST(!cs.has(e));
    }

    // ============================================================================
    // getSmallestEntities
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        Entity c = ecs.spawn();
        ecs.add<u32>(a);
        ecs.add<u32>(b);
        ecs.add<f32>(b);
        ecs.add<f32>(c);
        // u32 has 2 entities (a,b), f32 has 2 entities (b,c)
        // smallest should be 2
        Span<Entity> smallest = ecs.getSmallestEntities<u32, f32>();
        TEST(smallest.count == 2);
        // With no entities, should return empty
        Ecs<u32> empty = Ecs<u32>::create();
        Span<Entity> emptySmallest = empty.getSmallestEntities<u32>();
        TEST(emptySmallest.count == 0);
    }

    // ============================================================================
    // forEach multi: verify only matching entities are visited
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        Entity e3 = ecs.spawn();
        Entity e4 = ecs.spawn();
        // e1: u32 only, e2: both, e3: f32 only, e4: both
        ecs.add<u32>(e1) = 1;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;
        ecs.add<f32>(e3) = 30.0f;
        ecs.add<u32>(e4) = 4;
        ecs.add<f32>(e4) = 40.0f;
        u64 visitCount = 0;
        u64 sum = 0;
        auto fn = [&](Entity e, u32& a, f32& b)
        {
            visitCount++;
            sum += a + (u64)b;
            (void)e;
        };
        ecs.forEach<decltype(fn), u32, f32>(fn);
        TEST(visitCount == 2);  // only e2 and e4
        TEST(sum == (2 + 20) + (4 + 40));
    }

    // ============================================================================
    // getSmallestEntities with single type returns all entities
    // ============================================================================

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        Entity c = ecs.spawn();
        ecs.add<u32>(a);
        ecs.add<u32>(b);
        ecs.add<u32>(c);
        ecs.remove<u32>(b);
        auto ents = ecs.getSmallestEntities<u32>();
        TEST(ents.count == 2);
    }

    // ============================================================================
    // Lifecycle: multiple add/remove cycles track correctly
    // ============================================================================

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs = Ecs<Lifecycle>::create();
            Entity e = ecs.spawn();
            for (u32 i = 0; i < 5; ++i)
            {
                ecs.add<Lifecycle>(e);
                ecs.remove<Lifecycle>(e);
            }
            TEST(Lifecycle::stats.alive == 0);
            TEST(Lifecycle::stats.dtors == 5);
            TEST(Lifecycle::stats.ctors == 5);
        }
    }

    // ============================================================================
    // Despawn with already-freed entity handle (should assert)
    // ============================================================================

    // Note: despawn calls entities.free() which asserts alive;
    // then the component check c.has() uses old entity idx.
    // This is caller error, and the assert catches it.

    // ============================================================================
    // forEach: component arrays are traversed in packed order
    // ============================================================================

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        Entity c = ecs.spawn();
        ecs.add<u32>(a) = 10;
        ecs.add<u32>(b) = 20;
        ecs.add<u32>(c) = 30;
        ecs.remove<u32>(b);
        u64 sum = 0;
        auto fn = [&](Entity e, u32& v)
        {
            sum += v;
            (void)e;
        };
        ecs.forEach<decltype(fn), u32>(fn);
        TEST(sum == 40);
    }

    // ============================================================================
    // Pack add: default-construct multiple types at once
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e = ecs.spawn();
        ecs.add<u32, f32>(e);
        TEST(ecs.has<u32>(e));
        TEST(ecs.has<f32>(e));
        TEST(ecs.count<u32>() == 1);
        TEST(ecs.count<f32>() == 1);
    }

    // ============================================================================
    // Pack add: forward-construct multiple types at once
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e = ecs.spawn();
        ecs.add<u32, f32>(e, 42u, 3.14f);
        TEST(ecs.get<u32>(e) == 42);
        TEST(ecs.get<f32>(e) == 3.14f);
    }

    // ============================================================================
    // Pack remove: remove multiple types at once
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 1;
        ecs.add<f32>(e) = 2.0f;
        ecs.remove<u32, f32>(e);
        TEST(!ecs.has<u32>(e));
        TEST(!ecs.has<f32>(e));
    }

    // ============================================================================
    // EcsComponent direct: copy and move add
    // ============================================================================

    {
        HandlePool pool = HandlePool::create();
        Entity e = {pool.alloc()};
        EcsComponent<Lifecycle> cs;

        Lifecycle::stats.reset();
        {
            Lifecycle src;
            cs.add(e, src);
            TEST(src.valid);
            TEST(cs.has(e));
            TEST(cs.get(e).valid);
            TEST(Lifecycle::stats.copies == 1);
            TEST(Lifecycle::stats.moves == 0);
            cs.remove(e);
        }
        TEST(Lifecycle::stats.alive == 0);

        Lifecycle::stats.reset();
        {
            Lifecycle src;
            cs.add(e, std::move(src));
            TEST(!src.valid);
            TEST(cs.has(e));
            TEST(cs.get(e).valid);
            TEST(Lifecycle::stats.copies == 0);
            TEST(Lifecycle::stats.moves == 1);
            cs.remove(e);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // ============================================================================
    // Ecs copy and move add via Ecs
    // ============================================================================

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs = Ecs<Lifecycle>::create();
            Entity e = ecs.spawn();
            Lifecycle src;
            ecs.add<Lifecycle>(e, src);
            TEST(src.valid);
            TEST(Lifecycle::stats.copies == 1);
            TEST(Lifecycle::stats.moves == 0);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs = Ecs<Lifecycle>::create();
            Entity e = ecs.spawn();
            Lifecycle src;
            ecs.add<Lifecycle>(e, std::move(src));
            TEST(!src.valid);
            TEST(Lifecycle::stats.copies == 0);
            TEST(Lifecycle::stats.moves == 1);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // ============================================================================
    // Pack add with Lifecycle types -- forward-construct two at once
    // ============================================================================

    {
        Lifecycle::stats.reset();
        {
            Ecs<u32, Lifecycle> ecs = Ecs<u32, Lifecycle>::create();
            Entity e = ecs.spawn();
            Lifecycle a;
            ecs.add<u32, Lifecycle>(e, 42u, std::move(a));
            TEST(!a.valid);
            TEST(ecs.has<u32>(e));
            TEST(ecs.has<Lifecycle>(e));
            TEST(ecs.get<u32>(e) == 42);
            TEST(ecs.get<Lifecycle>(e).valid);
            TEST(Lifecycle::stats.copies == 0);
            TEST(Lifecycle::stats.moves == 1);
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // ============================================================================
    // const Ecs: query methods work on const objects
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 7;
        ecs.add<f32>(e) = 3.0f;
        const Ecs<u32, f32>& cecs = ecs;

        bool all = cecs.hasAll<u32, f32>(e);
        TEST(all);
        bool any = cecs.hasAny<u32, f32>(e);
        TEST(any);
        TEST(cecs.alive(e));
        TEST(cecs.has<u32>(e));
        TEST(cecs.get<u32>(e) == 7);
        TEST(cecs.get<f32>(e) == 3.0f);
        TEST(cecs.count<u32>() == 1);
        TEST(cecs.count<f32>() == 1);
    }

    // ============================================================================
    // const EcsComponent: query methods work on const objects
    // ============================================================================

    {
        HandlePool pool = HandlePool::create();
        Entity e = {pool.alloc()};
        EcsComponent<u32> cs;
        cs.add(e) = 42;
        const EcsComponent<u32>& ccs = cs;
        TEST(ccs.has(e));
        TEST(ccs.get(e) == 42);
    }

    // ============================================================================
    // const forEach single: lambda receives const refs
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<f32>(e1) = 1.0f;
        ecs.add<u32>(e2) = 20;
        ecs.add<f32>(e2) = 2.0f;
        const Ecs<u32, f32>& cecs = ecs;

        u64 sum = 0;
        auto fn1 = [&](Entity en, const u32& v)
        {
            sum += v;
            (void)en;
        };
        cecs.forEach<decltype(fn1), u32>(fn1);
        TEST(sum == 30);

        f32 fsum = 0;
        auto fn2 = [&](Entity en, const f32& v)
        {
            fsum += v;
            (void)en;
        };
        cecs.forEach<decltype(fn2), f32>(fn2);
        TEST(fsum == 3.0f);
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;
        const Ecs<u32>& cecs = ecs;

        u64 sum = 0;
        auto fn = [&](Entity e)
        {
            sum += cecs.get<u32>(e);
        };
        cecs.forEach<decltype(fn), u32>(fn);
        TEST(sum == 30);
    }

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;
        const Ecs<u32>& cecs = ecs;

        u64 sum = 0;
        auto fn = [&](const u32& v)
        {
            sum += v;
        };
        cecs.forEach<decltype(fn), u32>(fn);
        TEST(sum == 30);
    }

    // ============================================================================
    // const forEach multi: lambda receives const refs
    // ============================================================================

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 5;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 15;
        ecs.add<f32>(e2) = 20.0f;
        const Ecs<u32, f32>& cecs = ecs;

        u64 sum = 0;
        auto fn = [&](Entity en, const u32& a, const f32& b)
        {
            sum += a + (u64)b;
            (void)en;
        };
        cecs.forEach<decltype(fn), u32, f32>(fn);
        TEST(sum == (5 + 10) + (15 + 20));
    }

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 5;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 15;
        ecs.add<f32>(e2) = 20.0f;
        const Ecs<u32, f32>& cecs = ecs;

        u64 sum = 0;
        auto fn = [&](Entity e)
        {
            sum += cecs.get<u32>(e);
        };
        cecs.forEach<decltype(fn), u32, f32>(fn);
        TEST(sum == 20);
    }

    {
        Ecs<u32, f32> ecs = Ecs<u32, f32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 5;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 15;
        ecs.add<f32>(e2) = 20.0f;
        const Ecs<u32, f32>& cecs = ecs;

        u64 sum = 0;
        auto fn = [&](const u32& a, const f32& b)
        {
            sum += a + (u64)b;
        };
        cecs.forEach<decltype(fn), u32, f32>(fn);
        TEST(sum == (5 + 10) + (15 + 20));
    }

    // ============================================================================
    // const getComponentSystem returns const ref
    // ============================================================================

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 99;
        const Ecs<u32>& cecs = ecs;
        const EcsComponent<u32>& ccs = cecs.getComponentSystem<u32>();
        TEST(ccs.has(e));
        TEST(ccs.get(e) == 99);
    }

    // ============================================================================
    // Span<T> converts implicitly to Span<const T>
    // ============================================================================

    {
        u32 arr[] = {1, 2, 3};
        Span<u32> s{arr, 3};
        Span<const u32> cs = s;
        TEST(cs.data == arr);
        TEST(cs.count == 3);
        TEST(cs[0] == 1);
        TEST(cs[1] == 2);
        TEST(cs[2] == 3);
    }

    // ============================================================================
    // getComponents on const Ecs returns Span<const T>
    // ============================================================================

    {
        Ecs<u32> ecs = Ecs<u32>::create();
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;
        const Ecs<u32>& cecs = ecs;
        Span<const u32> comps = cecs.getComponents<u32>();
        TEST(comps.count == 2);
        u64 sum = 0;
        for (u64 i = 0; i < comps.count; ++i)
            sum += comps[i];
        TEST(sum == 30);
    }
}

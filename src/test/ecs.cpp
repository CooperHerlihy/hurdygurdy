#include "tests.hpp"
#include "hg/ecs.hpp"

using namespace hg;

struct EntityRefComp {
    Entity target;
    u32 data;
};

namespace hg {

template<>
void ecsSerialize(Serializer* s, EntityRefComp* val, EntitySerializer* ecs)
{
    ecsSerialize(s, &val->target, ecs);
    serialize(s, &val->data);
}

} // namespace hg

TEST(testEcs)
{
    // ============================================================================
    // Entity
    // ============================================================================

    {
        Entity e;
        ASSERT(e == nullEntity);
        ASSERT(e.handle == nullHandle);
    }

    {
        Entity a{Handle{1}};
        Entity b{Handle{1}};
        Entity c{Handle{2}};
        ASSERT(a == b);
        ASSERT(a != c);
    }

    // ============================================================================
    // spawn / alive / despawn / reset
    // ============================================================================

    {
        Ecs<> ecs{};
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        ASSERT(a != nullEntity);
        ASSERT(b != nullEntity);
        ASSERT(a != b);
        ASSERT(a.handle.idx() == 0);
        ASSERT(b.handle.idx() == 1);
    }

    {
        Ecs<> ecs{};
        ASSERT(!ecs.alive(nullEntity));
    }

    {
        Ecs<> ecs{};
        Entity e = ecs.spawn();
        ASSERT(ecs.alive(e));
        ecs.despawn(e);
        ASSERT(!ecs.alive(e));
    }

    {
        Ecs<> ecs{};
        Entity a = ecs.spawn();
        u32 idx = a.handle.idx();
        ecs.despawn(a);
        Entity b = ecs.spawn();
        ASSERT(b.handle.idx() == idx);
        ASSERT(b.handle.id != a.handle.id);
        ASSERT(ecs.alive(b));
        ASSERT(!ecs.alive(a));
    }

    {
        Ecs<> ecs{};
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        ecs.reset();
        ASSERT(!ecs.alive(a));
        ASSERT(!ecs.alive(b));
    }

    // ============================================================================
    // Single component: add / has / get / remove
    // ============================================================================

    {
        Ecs<u32> ecs{};
        Entity e = ecs.spawn();
        ASSERT(!ecs.has<u32>(e));
    }

    {
        Ecs<u32> ecs{};
        Entity e = ecs.spawn();
        u32& c = ecs.add<u32>(e);
        c = 42;
        ASSERT(ecs.has<u32>(e));
        ASSERT(ecs.get<u32>(e) == 42);
    }

    {
        Ecs<u32> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 7;
        ecs.get<u32>(e) = 99;
        ASSERT(ecs.get<u32>(e) == 99);
    }

    {
        Ecs<u32> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32>(e);
        ASSERT(ecs.has<u32>(e));
        ecs.remove<u32>(e);
        ASSERT(!ecs.has<u32>(e));
    }

    {
        Ecs<u32> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 10;
        ecs.remove<u32>(e);
        ecs.add<u32>(e) = 20;
        ASSERT(ecs.get<u32>(e) == 20);
    }

    {
        Ecs<u32, f32> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 1;
        ecs.add<f32>(e) = 2.0f;
        ecs.despawn(e);
        ASSERT(!ecs.alive(e));
        ASSERT(ecs.count<u32>() == 0);
        ASSERT(ecs.count<f32>() == 0);
    }

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1);
        ecs.add<f32>(e2);
        ecs.reset();
        ASSERT(ecs.count<u32>() == 0);
        ASSERT(ecs.count<f32>() == 0);
        { Entity e3 = ecs.spawn(); ASSERT(ecs.alive(e3)); }
    }

    // ============================================================================
    // count / getEntities / getComponents / getEntity
    // ============================================================================

    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1);
        ecs.add<u32>(e2);
        ASSERT(ecs.count<u32>() == 2);
        ecs.remove<u32>(e1);
        ASSERT(ecs.count<u32>() == 1);
    }

    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1);
        ecs.add<u32>(e2);
        Span<const Entity> ents = ecs.getEntities<u32>();
        ASSERT(ents.count == 2);
    }

    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;
        Span<u32> comps = ecs.getComponents<u32>();
        ASSERT(comps.count == 2);
        u64 sum = 0;
        for (u32 i = 0; i < comps.count; ++i)
            sum += comps[i];
        ASSERT(sum == 30);
    }

    {
        Ecs<u32> ecs{};
        Entity e = ecs.spawn();
        u32& c = ecs.add<u32>(e);
        Entity back = ecs.getEntity(c);
        ASSERT(back == e);
    }

    // ============================================================================
    // Multiple component types on same entity
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        Entity e3 = ecs.spawn();

        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 1.0f;
        ecs.add<u32>(e2) = 2;

        ASSERT(ecs.has<u32>(e1));
        ASSERT(ecs.has<f32>(e1));
        ASSERT(ecs.has<u32>(e2));
        ASSERT(!ecs.has<f32>(e2));
        ASSERT(!ecs.has<u32>(e3));
        ASSERT(!ecs.has<f32>(e3));

        ASSERT(ecs.get<u32>(e1) == 1);
        ASSERT(ecs.get<f32>(e1) == 1.0f);
        ASSERT(ecs.get<u32>(e2) == 2);
    }

    // ============================================================================
    // hasAll / hasAny
    // ============================================================================

    {
        Ecs<u32, f32, u64> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32>(e);
        ecs.add<f32>(e);
        ASSERT((ecs.hasAll<u32, f32>(e)));
        ASSERT((!ecs.hasAll<u32, u64>(e)));
        ASSERT((!ecs.hasAll<u32, f32, u64>(e)));
    }

    {
        Ecs<u32, f32, u64> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32>(e);
        ASSERT((ecs.hasAny<u32, f32>(e)));
        ASSERT((ecs.hasAny<u32, u64>(e)));
        ASSERT((!ecs.hasAny<f32, u64>(e)));
    }

    // ============================================================================
    // forEach single component type
    // ============================================================================

    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        u64 sum = 0;
        ecs.forEach<u32>([&](Entity e, u32& v)
        {
            sum += v;
            (void)e;
        });
        ASSERT(sum == 30);
    }

    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        u64 sum = 0;
        ecs.forEach<u32>([&](Entity e)
        {
            sum += ecs.get<u32>(e);
        });
        ASSERT(sum == 30);
    }

    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        u64 sum = 0;
        ecs.forEach<u32>([&](u32& v)
        {
            sum += v;
        });
        ASSERT(sum == 30);
    }

    // ============================================================================
    // forEachPar single component type
    // ============================================================================

    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        hg::SpinLock lk;
        u64 sum = 0;
        ecs.forEachPar<u32>([&](Entity e, u32& v)
        {
            hg::SpinLockScope scope{&lk};
            sum += v;
            (void)e;
        });
        ASSERT(sum == 30);
    }

    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        hg::SpinLock lk;
        u64 sum = 0;
        ecs.forEachPar<u32>([&](Entity e)
        {
            hg::SpinLockScope scope{&lk};
            sum += ecs.get<u32>(e);
        });
        ASSERT(sum == 30);
    }

    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        hg::SpinLock lk;
        u64 sum = 0;
        ecs.forEachPar<u32>([&](u32& v)
        {
            hg::SpinLockScope scope{&lk};
            sum += v;
        });
        ASSERT(sum == 30);
    }

    // ============================================================================
    // forEach multiple component types
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        Entity e3 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;
        ecs.add<u32>(e3) = 3;

        u64 sum = 0;
        ecs.forEach<u32, f32>([&](Entity e, u32& a, f32& b)
        {
            sum += a + (u64)b;
            (void)e;
        });
        ASSERT(sum == 33);
    }

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;

        u64 sum = 0;
        ecs.forEach<u32, f32>([&](Entity e)
        {
            sum += ecs.get<u32>(e);
        });
        ASSERT(sum == 3);
    }

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;

        u64 sum = 0;
        ecs.forEach<u32, f32>([&](u32& a, f32& b)
        {
            sum += a + (u64)b;
        });
        ASSERT(sum == 33);
    }

    // ============================================================================
    // forEachPar multiple component types
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;

        hg::SpinLock lk;
        u64 sum = 0;
        ecs.forEachPar<u32, f32>([&](Entity e, u32& a, f32& b)
        {
            hg::SpinLockScope scope{&lk};
            sum += a + (u64)b;
            (void)e;
        });
        ASSERT(sum == 33);
    }

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;

        hg::SpinLock lk;
        u64 sum = 0;
        ecs.forEachPar<u32, f32>([&](Entity e)
        {
            hg::SpinLockScope scope{&lk};
            sum += ecs.get<u32>(e);
        });
        ASSERT(sum == 3);
    }

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;

        hg::SpinLock lk;
        u64 sum = 0;
        ecs.forEachPar<u32, f32>([&](u32& a, f32& b)
        {
            hg::SpinLockScope scope{&lk};
            sum += a + (u64)b;
        });
        ASSERT(sum == 33);
    }

    // ============================================================================
    // Component lifecycle with Lifecycle type
    // ============================================================================

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs{};
            Entity e = ecs.spawn();
            ecs.add<Lifecycle>(e);
            ASSERT(Lifecycle::stats.alive == 1);
            ASSERT(Lifecycle::stats.ctors == 1);
        }
    }

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs{};
            Entity e = ecs.spawn();
            ecs.add<Lifecycle>(e);
            ASSERT(Lifecycle::stats.alive == 1);
            ecs.remove<Lifecycle>(e);
            ASSERT(Lifecycle::stats.alive == 0);
            ASSERT(Lifecycle::stats.dtors == 1);
        }
    }

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs{};
            Entity e1 = ecs.spawn();
            Entity e2 = ecs.spawn();
            ecs.add<Lifecycle>(e1);
            ecs.add<Lifecycle>(e2);
            ASSERT(Lifecycle::stats.alive == 2);
            ecs.reset();
            ASSERT(Lifecycle::stats.alive == 0);
            ASSERT(Lifecycle::stats.dtors == 2);
        }
    }

    // ============================================================================
    // Large number of entities
    // ============================================================================

    {
        Ecs<u32> ecs{};
        static constexpr u32 n = 100;
        Entity entities[100];
        for (u32 i = 0; i < n; ++i)
        {
            entities[i] = ecs.spawn();
            ecs.add<u32>(entities[i]) = i;
        }
        ASSERT(ecs.count<u32>() == n);

        for (u32 i = 0; i < n; ++i)
            ASSERT(ecs.get<u32>(entities[i]) == i);

        for (u32 i = 0; i < n; i += 2)
            ecs.remove<u32>(entities[i]);
        ASSERT(ecs.count<u32>() == n / 2);
    }

    // ============================================================================
    // Entity reuse after despawn with multiple component types
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 5;
        ecs.add<f32>(e) = 3.14f;
        ecs.despawn(e);

        Entity f = ecs.spawn();
        ecs.add<u32>(f) = 10;
        ecs.add<f32>(f) = 2.71f;
        ASSERT(ecs.get<u32>(f) == 10);
        ASSERT(ecs.get<f32>(f) == 2.71f);
        ASSERT(ecs.count<u32>() == 1);
        ASSERT(ecs.count<f32>() == 1);
    }

    // ============================================================================
    // Remove and re-add with different entity in between (swap bookkeeping)
    // ============================================================================

    {
        Ecs<u32> ecs{};
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        ecs.add<u32>(a) = 1;
        ecs.add<u32>(b) = 2;
        ecs.remove<u32>(a);
        ASSERT(!ecs.has<u32>(a));
        ASSERT(ecs.has<u32>(b));
        ASSERT(ecs.get<u32>(b) == 2);
        ASSERT(ecs.count<u32>() == 1);
    }

    // ============================================================================
    // Repeated add/remove cycles on same entity
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e = ecs.spawn();
        for (u32 i = 0; i < 10; ++i)
        {
            ecs.add<u32>(e) = i;
            ASSERT(ecs.has<u32>(e));
            ASSERT(ecs.get<u32>(e) == i);
            ecs.remove<u32>(e);
            ASSERT(!ecs.has<u32>(e));
        }
    }

    // ============================================================================
    // Despawn entity with no components (edge case)
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e = ecs.spawn();
        ecs.despawn(e);
        ASSERT(!ecs.alive(e));
    }

    // ============================================================================
    // Query zero-component state
    // ============================================================================

    {
        Ecs<u32> ecs{};
        ASSERT(ecs.count<u32>() == 0);
        Span<const Entity> ents = ecs.getEntities<u32>();
        ASSERT(ents.count == 0);
        Span<u32> comps = ecs.getComponents<u32>();
        ASSERT(comps.count == 0);
    }

    // ============================================================================
    // Remove from middle vs end to exercise swap bookkeeping
    // ============================================================================

    {
        Ecs<u32> ecs{};
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        Entity c = ecs.spawn();
        ecs.add<u32>(a) = 1;
        ecs.add<u32>(b) = 2;
        ecs.add<u32>(c) = 3;
        // Remove from middle (b): removes index 1, swaps c into index 1
        ecs.remove<u32>(b);
        ASSERT(!ecs.has<u32>(b));
        ASSERT(ecs.has<u32>(a));
        ASSERT(ecs.has<u32>(c));
        ASSERT(ecs.get<u32>(a) == 1);
        ASSERT(ecs.get<u32>(c) == 3);
        // Remove last (end): no swap needed
        ecs.remove<u32>(c);
        ASSERT(!ecs.has<u32>(c));
        ASSERT(ecs.has<u32>(a));
        ASSERT(ecs.get<u32>(a) == 1);
        // Remove first: the only remaining
        ecs.remove<u32>(a);
        ASSERT(ecs.count<u32>() == 0);
    }

    // ============================================================================
    // Direct EcsComponent<T> API
    // ============================================================================

    {
        HandlePool pool{};
        Entity e = {pool.alloc()};
        EcsComponent<u32> cs;
        u32& ref = cs.add(e);
        ref = 42;
        ASSERT(cs.has(e));
        ASSERT(cs.get(e) == 42);
        Entity back = cs.getEntity(ref);
        ASSERT(back == e);
        cs.remove(e);
        ASSERT(!cs.has(e));
    }

    // ============================================================================
    // getSmallestEntities
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
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
        ASSERT(smallest.count == 2);
        // With no entities, should return empty
        Ecs<u32> empty{};
        Span<Entity> emptySmallest = empty.getSmallestEntities<u32>();
        ASSERT(emptySmallest.count == 0);
    }

    // ============================================================================
    // forEach multi: verify only matching entities are visited
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
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
        ecs.forEach<u32, f32>([&](Entity e, u32& a, f32& b)
        {
            visitCount++;
            sum += a + (u64)b;
            (void)e;
        });
        ASSERT(visitCount == 2);  // only e2 and e4
        ASSERT(sum == (2 + 20) + (4 + 40));
    }

    // ============================================================================
    // getSmallestEntities with single type returns all entities
    // ============================================================================

    {
        Ecs<u32> ecs{};
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        Entity c = ecs.spawn();
        ecs.add<u32>(a);
        ecs.add<u32>(b);
        ecs.add<u32>(c);
        ecs.remove<u32>(b);
        auto ents = ecs.getSmallestEntities<u32>();
        ASSERT(ents.count == 2);
    }

    // ============================================================================
    // Lifecycle: multiple add/remove cycles track correctly
    // ============================================================================

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs{};
            Entity e = ecs.spawn();
            for (u32 i = 0; i < 5; ++i)
            {
                ecs.add<Lifecycle>(e);
                ecs.remove<Lifecycle>(e);
            }
            ASSERT(Lifecycle::stats.alive == 0);
            ASSERT(Lifecycle::stats.dtors == 5);
            ASSERT(Lifecycle::stats.ctors == 5);
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
        Ecs<u32> ecs{};
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        Entity c = ecs.spawn();
        ecs.add<u32>(a) = 10;
        ecs.add<u32>(b) = 20;
        ecs.add<u32>(c) = 30;
        ecs.remove<u32>(b);
        u64 sum = 0;
        ecs.forEach<u32>([&](Entity e, u32& v)
        {
            sum += v;
            (void)e;
        });
        ASSERT(sum == 40);
    }

    // ============================================================================
    // Pack add: default-construct multiple types at once
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32, f32>(e);
        ASSERT(ecs.has<u32>(e));
        ASSERT(ecs.has<f32>(e));
        ASSERT(ecs.count<u32>() == 1);
        ASSERT(ecs.count<f32>() == 1);
    }

    // ============================================================================
    // Pack add: forward-construct multiple types at once
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32, f32>(e, 42u, 3.14f);
        ASSERT(ecs.get<u32>(e) == 42);
        ASSERT(ecs.get<f32>(e) == 3.14f);
    }

    // ============================================================================
    // Pack remove: remove multiple types at once
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 1;
        ecs.add<f32>(e) = 2.0f;
        ecs.remove<u32, f32>(e);
        ASSERT(!ecs.has<u32>(e));
        ASSERT(!ecs.has<f32>(e));
    }

    // ============================================================================
    // EcsComponent direct: copy and move add
    // ============================================================================

    {
        HandlePool pool{};
        Entity e = {pool.alloc()};
        EcsComponent<Lifecycle> cs;

        Lifecycle::stats.reset();
        {
            Lifecycle src;
            cs.add(e, src);
            ASSERT(src.valid);
            ASSERT(cs.has(e));
            ASSERT(cs.get(e).valid);
            ASSERT(Lifecycle::stats.copies == 1);
            ASSERT(Lifecycle::stats.moves == 0);
            cs.remove(e);
        }
        ASSERT(Lifecycle::stats.alive == 0);

        Lifecycle::stats.reset();
        {
            Lifecycle src;
            cs.add(e, std::move(src));
            ASSERT(!src.valid);
            ASSERT(cs.has(e));
            ASSERT(cs.get(e).valid);
            ASSERT(Lifecycle::stats.copies == 0);
            ASSERT(Lifecycle::stats.moves == 1);
            cs.remove(e);
        }
        ASSERT(Lifecycle::stats.alive == 0);
    }

    // ============================================================================
    // Ecs copy and move add via Ecs
    // ============================================================================

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs{};
            Entity e = ecs.spawn();
            Lifecycle src;
            ecs.add<Lifecycle>(e, src);
            ASSERT(src.valid);
            ASSERT(Lifecycle::stats.copies == 1);
            ASSERT(Lifecycle::stats.moves == 0);
        }
        ASSERT(Lifecycle::stats.alive == 0);
    }

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs{};
            Entity e = ecs.spawn();
            Lifecycle src;
            ecs.add<Lifecycle>(e, std::move(src));
            ASSERT(!src.valid);
            ASSERT(Lifecycle::stats.copies == 0);
            ASSERT(Lifecycle::stats.moves == 1);
        }
        ASSERT(Lifecycle::stats.alive == 0);
    }

    // ============================================================================
    // Pack add with Lifecycle types -- forward-construct two at once
    // ============================================================================

    {
        Lifecycle::stats.reset();
        {
            Ecs<u32, Lifecycle> ecs{};
            Entity e = ecs.spawn();
            Lifecycle a;
            ecs.add<u32, Lifecycle>(e, 42u, std::move(a));
            ASSERT(!a.valid);
            ASSERT(ecs.has<u32>(e));
            ASSERT(ecs.has<Lifecycle>(e));
            ASSERT(ecs.get<u32>(e) == 42);
            ASSERT(ecs.get<Lifecycle>(e).valid);
            ASSERT(Lifecycle::stats.copies == 0);
            ASSERT(Lifecycle::stats.moves == 1);
        }
        ASSERT(Lifecycle::stats.alive == 0);
    }

    // ============================================================================
    // const Ecs: query methods work on const objects
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 7;
        ecs.add<f32>(e) = 3.0f;
        const Ecs<u32, f32>& cecs = ecs;

        bool all = cecs.hasAll<u32, f32>(e);
        ASSERT(all);
        bool any = cecs.hasAny<u32, f32>(e);
        ASSERT(any);
        ASSERT(cecs.alive(e));
        ASSERT(cecs.has<u32>(e));
        ASSERT(cecs.get<u32>(e) == 7);
        ASSERT(cecs.get<f32>(e) == 3.0f);
        ASSERT(cecs.count<u32>() == 1);
        ASSERT(cecs.count<f32>() == 1);
    }

    // ============================================================================
    // const EcsComponent: query methods work on const objects
    // ============================================================================

    {
        HandlePool pool{};
        Entity e = {pool.alloc()};
        EcsComponent<u32> cs;
        cs.add(e) = 42;
        const EcsComponent<u32>& ccs = cs;
        ASSERT(ccs.has(e));
        ASSERT(ccs.get(e) == 42);
    }

    // ============================================================================
    // const forEach single: lambda receives const refs
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<f32>(e1) = 1.0f;
        ecs.add<u32>(e2) = 20;
        ecs.add<f32>(e2) = 2.0f;
        const Ecs<u32, f32>& cecs = ecs;

        u64 sum = 0;
        cecs.forEach<u32>([&](Entity en, const u32& v)
        {
            sum += v;
            (void)en;
        });
        ASSERT(sum == 30);

        f32 fsum = 0;
        cecs.forEach<f32>([&](Entity en, const f32& v)
        {
            fsum += v;
            (void)en;
        });
        ASSERT(fsum == 3.0f);
    }

    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;
        const Ecs<u32>& cecs = ecs;

        u64 sum = 0;
        cecs.forEach<u32>([&](Entity e)
        {
            sum += cecs.get<u32>(e);
        });
        ASSERT(sum == 30);
    }

    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;
        const Ecs<u32>& cecs = ecs;

        u64 sum = 0;
        cecs.forEach<u32>([&](const u32& v)
        {
            sum += v;
        });
        ASSERT(sum == 30);
    }

    // ============================================================================
    // const forEach multi: lambda receives const refs
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 5;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 15;
        ecs.add<f32>(e2) = 20.0f;
        const Ecs<u32, f32>& cecs = ecs;

        u64 sum = 0;
        cecs.forEach<u32, f32>([&](Entity en, const u32& a, const f32& b)
        {
            sum += a + (u64)b;
            (void)en;
        });
        ASSERT(sum == (5 + 10) + (15 + 20));
    }

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 5;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 15;
        ecs.add<f32>(e2) = 20.0f;
        const Ecs<u32, f32>& cecs = ecs;

        u64 sum = 0;
        cecs.forEach<u32, f32>([&](Entity e)
        {
            sum += cecs.get<u32>(e);
        });
        ASSERT(sum == 20);
    }

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 5;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 15;
        ecs.add<f32>(e2) = 20.0f;
        const Ecs<u32, f32>& cecs = ecs;

        u64 sum = 0;
        cecs.forEach<u32, f32>([&](const u32& a, const f32& b)
        {
            sum += a + (u64)b;
        });
        ASSERT(sum == (5 + 10) + (15 + 20));
    }

    // ============================================================================
    // const getComponentSystem returns const ref
    // ============================================================================

    {
        Ecs<u32> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 99;
        const Ecs<u32>& cecs = ecs;
        const EcsComponent<u32>& ccs = cecs.getComponentSystem<u32>();
        ASSERT(ccs.has(e));
        ASSERT(ccs.get(e) == 99);
    }

    // ============================================================================
    // Span<T> converts implicitly to Span<const T>
    // ============================================================================

    {
        u32 arr[] = {1, 2, 3};
        Span<u32> s{arr, 3};
        Span<const u32> cs = s;
        ASSERT(cs.data == arr);
        ASSERT(cs.count == 3);
        ASSERT(cs[0] == 1);
        ASSERT(cs[1] == 2);
        ASSERT(cs[2] == 3);
    }

    // ============================================================================
    // getComponents on const Ecs returns Span<const T>
    // ============================================================================

    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;
        const Ecs<u32>& cecs = ecs;
        Span<const u32> comps = cecs.getComponents<u32>();
        ASSERT(comps.count == 2);
        u64 sum = 0;
        for (u64 i = 0; i < comps.count; ++i)
            sum += comps[i];
        ASSERT(sum == 30);
    }

    // ============================================================================
    // ECS Serialization: Empty
    // ============================================================================

    // Empty ECS (no component types)
    {
        Ecs<> ecs{};
        Ecs<> copy{};
        ArenaScope arena = getScratch();
        Serializer w = serialWriter(arena);
        serialize(&w, &ecs);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
    }

    // Empty ECS with component types (no entities)
    {
        Ecs<u32, f32> ecs{};
        Ecs<u32, f32> copy{};
        ArenaScope arena = getScratch();
        Serializer w = serialWriter(arena);
        serialize(&w, &ecs);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);
        ASSERT(copy.count<u32>() == 0);
        ASSERT(copy.count<f32>() == 0);
    }

    // ============================================================================
    // ECS Serialization: Single component
    // ============================================================================

    // Single entity, single component
    {
        Ecs<u32> ecs{};
        Entity e = ecs.spawn();
        ecs.add<u32>(e) = 42;

        Ecs<u32> copy{};
        ArenaScope arena = getScratch();
        Serializer w = serialWriter(arena);
        serialize(&w, &ecs);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);

        ASSERT(copy.count<u32>() == 1);
        u32 found = 0;
        copy.forEach<u32>([&](u32& v) { found = v; });
        ASSERT(found == 42);
    }

    // Multiple entities, single component
    {
        Ecs<u32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 10;
        ecs.add<u32>(e2) = 20;

        Ecs<u32> copy{};
        ArenaScope arena = getScratch();
        Serializer w = serialWriter(arena);
        serialize(&w, &ecs);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);

        ASSERT(copy.count<u32>() == 2);
        u64 sum = 0;
        copy.forEach<u32>([&](u32& v) { sum += v; });
        ASSERT(sum == 30);
    }

    // ============================================================================
    // ECS Serialization: Multiple component types
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;

        Ecs<u32, f32> copy{};
        ArenaScope arena = getScratch();
        Serializer w = serialWriter(arena);
        serialize(&w, &ecs);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);

        ASSERT(copy.count<u32>() == 2);
        ASSERT(copy.count<f32>() == 2);
        u64 sum = 0;
        copy.forEach<u32, f32>([&](Entity ce, u32& a, f32& b)
        {
            sum += a + (u64)b;
            (void)ce;
        });
        ASSERT(sum == 33);
    }

    // ============================================================================
    // ECS Serialization: Sparse entities (after despawn)
    // ============================================================================

    {
        Ecs<u32> ecs{};
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        Entity c = ecs.spawn();
        ecs.add<u32>(a) = 1;
        ecs.add<u32>(b) = 2;
        ecs.add<u32>(c) = 3;
        ecs.despawn(b);

        Ecs<u32> copy{};
        ArenaScope arena = getScratch();
        Serializer w = serialWriter(arena);
        serialize(&w, &ecs);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);

        ASSERT(copy.count<u32>() == 2);
        u64 sum = 0;
        copy.forEach<u32>([&](u32& v) { sum += v; });
        ASSERT(sum == 4);
    }

    // ============================================================================
    // ECS Serialization: Partial component coverage
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        Entity e3 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;
        ecs.add<f32>(e3) = 30.0f;

        Ecs<u32, f32> copy{};
        ArenaScope arena = getScratch();
        Serializer w = serialWriter(arena);
        serialize(&w, &ecs);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);

        ASSERT(copy.count<u32>() == 2);
        ASSERT(copy.count<f32>() == 2);

        {
            u64 sumU = 0;
            copy.forEach<u32>([&](u32& v) { sumU += v; });
            ASSERT(sumU == 3);
        }

        {
            f64 sumF = 0;
            copy.forEach<f32>([&](f32& v) { sumF += v; });
            ASSERT(sumF == 50.0);
        }
    }

    // ============================================================================
    // ECS Serialization: Entity references in components
    // ============================================================================

    {
        Ecs<EntityRefComp> ecs{};
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        ecs.add<EntityRefComp>(a, EntityRefComp{b, 42});

        Ecs<EntityRefComp> copy{};
        ArenaScope arena = getScratch();
        Serializer w = serialWriter(arena);
        serialize(&w, &ecs);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);

        ASSERT(copy.count<EntityRefComp>() == 1);
        u32 foundData = 0;
        bool targetAlive = false;
        copy.forEach<EntityRefComp>([&](Entity ce, EntityRefComp& c)
        {
            foundData = c.data;
            targetAlive = copy.alive(c.target);
            (void)ce;
        });
        ASSERT(foundData == 42);
        ASSERT(targetAlive);
    }

    // ============================================================================
    // ECS Serialization: No-op entity reference (Entity component)
    // ============================================================================

    {
        Ecs<Entity> ecs{};
        Entity a = ecs.spawn();
        Entity b = ecs.spawn();
        ecs.add<Entity>(a, b);

        Ecs<Entity> copy{};
        ArenaScope arena = getScratch();
        Serializer w = serialWriter(arena);
        serialize(&w, &ecs);
        Serializer r = serialReader(arena, w.current);
        serialize(&r, &copy);

        ASSERT(copy.count<Entity>() == 1);
        bool targetAlive = false;
        copy.forEach<Entity>([&](Entity ce, Entity& e)
        {
            targetAlive = copy.alive(e);
            (void)ce;
        });
        ASSERT(targetAlive);
    }

    // ============================================================================
    // ECS Serialization: Lifecycle component
    // ============================================================================

    {
        Lifecycle::stats.reset();
        {
            Ecs<Lifecycle> ecs{};
            Entity e1 = ecs.spawn();
            Entity e2 = ecs.spawn();
            ecs.add<Lifecycle>(e1);
            ecs.add<Lifecycle>(e2);

            Ecs<Lifecycle> copy{};
            ArenaScope arena = getScratch();
            Serializer w = serialWriter(arena);
            serialize(&w, &ecs);
            Serializer r = serialReader(arena, w.current);
            serialize(&r, &copy);

            ASSERT(copy.count<Lifecycle>() == 2);
            ASSERT(Lifecycle::stats.alive == 4); // 2 original + 2 copy

            // Verify copied values are valid (serialization preserves Lifecycle fields)
            u64 validCount = 0;
            copy.forEach<Lifecycle>([&](Lifecycle& l)
            {
                if (l.valid)
                    validCount++;
            });
            ASSERT(validCount == 2);
        }
        ASSERT(Lifecycle::stats.alive == 0);
    }

    // ============================================================================
    // ECS Serialization: Binary format round-trip
    // ============================================================================

    {
        Ecs<u32, f32> ecs{};
        Entity e1 = ecs.spawn();
        Entity e2 = ecs.spawn();
        ecs.add<u32>(e1) = 1;
        ecs.add<f32>(e1) = 10.0f;
        ecs.add<u32>(e2) = 2;
        ecs.add<f32>(e2) = 20.0f;

        ArenaScope arena = getScratch();
        Serializer w = serialWriter(arena);
        serialize(&w, &ecs);
        BinaryView bin = writeSerialBinary(arena, &w);
        Serializer r = readSerialBinary(arena, bin);
        Ecs<u32, f32> copy{};
        serialize(&r, &copy);

        ASSERT(copy.count<u32>() == 2);
        ASSERT(copy.count<f32>() == 2);
        u64 sum = 0;
        copy.forEach<u32, f32>([&](Entity ce, u32& a, f32& b)
        {
            sum += a + (u64)b;
            (void)ce;
        });
        ASSERT(sum == 33);
    }
}

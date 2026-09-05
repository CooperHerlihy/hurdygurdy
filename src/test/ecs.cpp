#include "tests.hpp"
#include "hg/ecs.hpp"

using namespace hg;

TEST(testEntityDefault)
{
    Entity e;
    ASSERT(e == nullEntity);
    ASSERT(e.handle == nullHandle);
}

TEST(testEntityEquality)
{
    Entity a{Handle{1}};
    Entity b{Handle{1}};
    Entity c{Handle{2}};
    ASSERT(a == b);
    ASSERT(a != c);
}

TEST(testSpawnReturnsDistinct)
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

TEST(testAliveNullEntity)
{
    Ecs<> ecs{};
    ASSERT(!ecs.alive(nullEntity));
}

TEST(testSpawnDespawnAlive)
{
    Ecs<> ecs{};
    Entity e = ecs.spawn();
    ASSERT(ecs.alive(e));
    ecs.despawn(e);
    ASSERT(!ecs.alive(e));
}

TEST(testDespawnReusesIndex)
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

TEST(testResetClearsAll)
{
    Ecs<> ecs{};
    Entity a = ecs.spawn();
    Entity b = ecs.spawn();
    ecs.reset();
    ASSERT(!ecs.alive(a));
    ASSERT(!ecs.alive(b));
}

TEST(testHasComponentFalseInitially)
{
    Ecs<u32> ecs{};
    Entity e = ecs.spawn();
    ASSERT(!ecs.has<u32>(e));
}

TEST(testAddAndGetComponent)
{
    Ecs<u32> ecs{};
    Entity e = ecs.spawn();
    u32& c = ecs.add<u32>(e);
    c = 42;
    ASSERT(ecs.has<u32>(e));
    ASSERT(ecs.get<u32>(e) == 42);
}

TEST(testGetComponentMutate)
{
    Ecs<u32> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32>(e) = 7;
    ecs.get<u32>(e) = 99;
    ASSERT(ecs.get<u32>(e) == 99);
}

TEST(testRemoveComponent)
{
    Ecs<u32> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32>(e);
    ASSERT(ecs.has<u32>(e));
    ecs.remove<u32>(e);
    ASSERT(!ecs.has<u32>(e));
}

TEST(testRemoveThenReAdd)
{
    Ecs<u32> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32>(e) = 10;
    ecs.remove<u32>(e);
    ecs.add<u32>(e) = 20;
    ASSERT(ecs.get<u32>(e) == 20);
}

TEST(testDespawnClearsComponents)
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

TEST(testResetClearsComponents)
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

TEST(testCountComponents)
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

TEST(testGetEntities)
{
    Ecs<u32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1);
    ecs.add<u32>(e2);
    Span<const Entity> ents = ecs.getEntities<u32>();
    ASSERT(ents.count == 2);
}

TEST(testGetComponents)
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

TEST(testGetEntityFromComponent)
{
    Ecs<u32> ecs{};
    Entity e = ecs.spawn();
    u32& c = ecs.add<u32>(e);
    Entity back = ecs.getEntity(c);
    ASSERT(back == e);
}

TEST(testMultipleComponentTypes)
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

TEST(testHasAll)
{
    Ecs<u32, f32, u64> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32>(e);
    ecs.add<f32>(e);
    ASSERT((ecs.hasAll<u32, f32>(e)));
    ASSERT((!ecs.hasAll<u32, u64>(e)));
    ASSERT((!ecs.hasAll<u32, f32, u64>(e)));
}

TEST(testHasAny)
{
    Ecs<u32, f32, u64> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32>(e);
    ASSERT((ecs.hasAny<u32, f32>(e)));
    ASSERT((ecs.hasAny<u32, u64>(e)));
    ASSERT((!ecs.hasAny<f32, u64>(e)));
}

TEST(testLifecycleAdd)
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

TEST(testLifecycleRemove)
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

TEST(testLifecycleReset)
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

TEST(testLargeNumberOfEntities)
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

TEST(testEntityReuseAfterDespawn)
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

TEST(testRemoveAndReAddSwap)
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

TEST(testRepeatedAddRemoveCycles)
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

TEST(testDespawnNoComponents)
{
    Ecs<u32, f32> ecs{};
    Entity e = ecs.spawn();
    ecs.despawn(e);
    ASSERT(!ecs.alive(e));
}

TEST(testQueryZeroComponentState)
{
    Ecs<u32> ecs{};
    ASSERT(ecs.count<u32>() == 0);
    Span<const Entity> ents = ecs.getEntities<u32>();
    ASSERT(ents.count == 0);
    Span<u32> comps = ecs.getComponents<u32>();
    ASSERT(comps.count == 0);
}

TEST(testRemoveFromMiddleAndEnd)
{
    Ecs<u32> ecs{};
    Entity a = ecs.spawn();
    Entity b = ecs.spawn();
    Entity c = ecs.spawn();
    ecs.add<u32>(a) = 1;
    ecs.add<u32>(b) = 2;
    ecs.add<u32>(c) = 3;
    ecs.remove<u32>(b);
    ASSERT(!ecs.has<u32>(b));
    ASSERT(ecs.has<u32>(a));
    ASSERT(ecs.has<u32>(c));
    ASSERT(ecs.get<u32>(a) == 1);
    ASSERT(ecs.get<u32>(c) == 3);
    ecs.remove<u32>(c);
    ASSERT(!ecs.has<u32>(c));
    ASSERT(ecs.has<u32>(a));
    ASSERT(ecs.get<u32>(a) == 1);
    ecs.remove<u32>(a);
    ASSERT(ecs.count<u32>() == 0);
}

TEST(testDirectEcsComponentApi)
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

TEST(testGetSmallestEntities)
{
    Ecs<u32, f32> ecs{};
    Entity a = ecs.spawn();
    Entity b = ecs.spawn();
    Entity c = ecs.spawn();
    ecs.add<u32>(a);
    ecs.add<u32>(b);
    ecs.add<f32>(b);
    ecs.add<f32>(c);
    Span<Entity> smallest = ecs.getSmallestEntities<u32, f32>();
    ASSERT(smallest.count == 2);
    Ecs<u32> empty{};
    Span<Entity> emptySmallest = empty.getSmallestEntities<u32>();
    ASSERT(emptySmallest.count == 0);
}

TEST(testGetSmallestEntitiesSingleType)
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

TEST(testLifecycleMultipleAddRemove)
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

TEST(testPackAddDefault)
{
    Ecs<u32, f32> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32, f32>(e);
    ASSERT(ecs.has<u32>(e));
    ASSERT(ecs.has<f32>(e));
    ASSERT(ecs.count<u32>() == 1);
    ASSERT(ecs.count<f32>() == 1);
}

TEST(testPackAddForward)
{
    Ecs<u32, f32> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32, f32>(e, 42u, 3.14f);
    ASSERT(ecs.get<u32>(e) == 42);
    ASSERT(ecs.get<f32>(e) == 3.14f);
}

TEST(testPackRemove)
{
    Ecs<u32, f32> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32>(e) = 1;
    ecs.add<f32>(e) = 2.0f;
    ecs.remove<u32, f32>(e);
    ASSERT(!ecs.has<u32>(e));
    ASSERT(!ecs.has<f32>(e));
}

TEST(testEcsComponentCopyAdd)
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
}

TEST(testEcsComponentMoveAdd)
{
    HandlePool pool{};
    Entity e = {pool.alloc()};
    EcsComponent<Lifecycle> cs;

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

TEST(testEcsCopyAdd)
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

TEST(testEcsMoveAdd)
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

TEST(testPackAddWithLifecycle)
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

TEST(testConstEcsQuery)
{
    Ecs<u32, f32> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32>(e) = 7;
    ecs.add<f32>(e) = 3.0f;
    const Ecs<u32, f32>& cecs = ecs;

    ASSERT((cecs.hasAll<u32, f32>(e)));
    ASSERT((cecs.hasAny<u32, f32>(e)));
    ASSERT(cecs.alive(e));
    ASSERT(cecs.has<u32>(e));
    ASSERT(cecs.get<u32>(e) == 7);
    ASSERT(cecs.get<f32>(e) == 3.0f);
    ASSERT(cecs.count<u32>() == 1);
    ASSERT(cecs.count<f32>() == 1);
}

TEST(testConstEcsComponent)
{
    HandlePool pool{};
    Entity e = {pool.alloc()};
    EcsComponent<u32> cs;
    cs.add(e) = 42;
    const EcsComponent<u32>& ccs = cs;
    ASSERT(ccs.has(e));
    ASSERT(ccs.get(e) == 42);
}

TEST(testConstGetComponentSystem)
{
    Ecs<u32> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32>(e) = 99;
    const Ecs<u32>& cecs = ecs;
    const EcsComponent<u32>& ccs = cecs.getComponentSystem<u32>();
    ASSERT(ccs.has(e));
    ASSERT(ccs.get(e) == 99);
}

TEST(testSpanImplicitConversion)
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

TEST(testConstGetComponents)
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

TEST(testForEachPackedOrder)
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

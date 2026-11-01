#include "tests.hpp"
#include "hg/ecs.hpp"

using namespace hg;

TEST(testEntity)
{
    Entity a;
    ASSERT(a == nullEntity);
    ASSERT(a.handle == nullHandle);
    Entity b{Handle{1}};
    Entity c{Handle{1}};
    Entity d{Handle{2}};
    ASSERT(b == c);
    ASSERT(b != d);
    ASSERT(a != b);
}

TEST(testSpawnDespawn)
{
    Ecs<> ecs{};
    ASSERT(!ecs.alive(nullEntity));
    Entity a = ecs.spawn();
    Entity b = ecs.spawn();
    ASSERT(a != nullEntity);
    ASSERT(b != nullEntity);
    ASSERT(a != b);
    ASSERT(a.handle.idx() == 0);
    ASSERT(b.handle.idx() == 1);
    ASSERT(ecs.alive(a));
    ASSERT(ecs.alive(b));
    ecs.despawn(a);
    ASSERT(!ecs.alive(a));
    ASSERT(ecs.alive(b));
    Entity c = ecs.spawn();
    ASSERT(c.handle.idx() == a.handle.idx());
    ASSERT(c.handle.id != a.handle.id);
    ASSERT(!ecs.alive(a));
    ASSERT(ecs.alive(c));
}

TEST(testDespawnComponents)
{
    Ecs<u32, f32> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32>(e) = 5;
    ecs.add<f32>(e) = 3.14f;
    ecs.despawn(e);
    ASSERT(!ecs.alive(e));
    ASSERT(ecs.count<u32>() == 0);
    ASSERT(ecs.count<f32>() == 0);

    Entity f = ecs.spawn();
    ASSERT(!ecs.has<u32>(f));
    ASSERT(!ecs.has<f32>(f));
    ecs.add<u32>(f) = 10;
    ASSERT(ecs.count<u32>() == 1);
    ASSERT(ecs.get<u32>(f) == 10);
}

TEST(testEcsReset)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 1;
    ecs.add<f32>(e2) = 2.0f;
    ecs.reset();
    ASSERT(!ecs.alive(e1));
    ASSERT(!ecs.alive(e2));
    ASSERT(ecs.count<u32>() == 0);
    ASSERT(ecs.count<f32>() == 0);
    Entity e3 = ecs.spawn();
    ASSERT(ecs.alive(e3));
}

TEST(testAddGetRemove)
{
    Ecs<u32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ASSERT(!ecs.has<u32>(e1));
    ecs.add<u32>(e1) = 42;
    ASSERT(ecs.has<u32>(e1));
    ecs.get<u32>(e1) = 99;
    ASSERT(ecs.get<u32>(e1) == 99);
    ecs.add<u32>(e2) = 7;
    ASSERT(ecs.count<u32>() == 2);
    ecs.remove<u32>(e1);
    ASSERT(!ecs.has<u32>(e1));
    ASSERT(ecs.has<u32>(e2));
    ASSERT(ecs.get<u32>(e2) == 7);
    ASSERT(ecs.count<u32>() == 1);
    ecs.add<u32>(e1) = 55;
    ASSERT(ecs.get<u32>(e1) == 55);
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

TEST(testHasAllHasAny)
{
    Ecs<u32, f32, u64> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32>(e);
    ecs.add<f32>(e);
    ASSERT((ecs.hasAll<u32, f32>(e)));
    ASSERT((!ecs.hasAll<u32, u64>(e)));
    ASSERT((!ecs.hasAll<u32, f32, u64>(e)));
    ASSERT((ecs.hasAny<u32, f32>(e)));
    ASSERT((ecs.hasAny<u32, u64>(e)));
    ASSERT((ecs.hasAny<f32, u64>(e)));
    Entity empty = ecs.spawn();
    ASSERT((!ecs.hasAny<u32, u64>(empty)));
    ASSERT((!ecs.hasAll<u32, f32>(empty)));
}

TEST(testGetEntityFromComponent)
{
    Ecs<u32> ecs{};
    Entity e = ecs.spawn();
    u32& c = ecs.add<u32>(e);
    c = 42;
    ASSERT(ecs.getEntity(c) == e);
}

TEST(testQuerySpans)
{
    Ecs<u32> ecs{};
    ASSERT(ecs.count<u32>() == 0);
    ASSERT(ecs.getEntities<u32>().count == 0);
    ASSERT(ecs.getComponents<u32>().count == 0);

    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<u32>(e2) = 20;

    Span<const Entity> ents = ecs.getEntities<u32>();
    ASSERT(ents.count == 2);
    ASSERT(ents[0] == e1);
    ASSERT(ents[1] == e2);

    Span<u32> comps = ecs.getComponents<u32>();
    ASSERT(comps.count == 2);
    comps[0] = 99;
    ASSERT(ecs.get<u32>(e1) == 99);

    const Ecs<u32>& cecs = ecs;
    Span<const u32> ccomps = cecs.getComponents<u32>();
    ASSERT(ccomps.count == 2);
    ASSERT(ccomps[0] == 99);
    ASSERT(ccomps[1] == 20);
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

    ecs.remove<u32>(b);
    smallest = ecs.getSmallestEntities<u32, f32>();
    ASSERT(smallest.count == 1);

    Ecs<u32> empty{};
    ASSERT(empty.getSmallestEntities<u32>().count == 0);
}

TEST(testEcsComponentApi)
{
    HandlePool pool{};
    Entity e = {pool.alloc()};
    EcsComponent<u32> cs;
    u32& ref = cs.add(e);
    ref = 42;
    ASSERT(cs.has(e));
    ASSERT(cs.get(e) == 42);
    ASSERT(cs.getEntity(cs.get(e)) == e);
    cs.remove(e);
    ASSERT(!cs.has(e));

    cs.add(e) = 7;
    const EcsComponent<u32>& ccs = cs;
    ASSERT(ccs.has(e));
    ASSERT(ccs.get(e) == 7);
}

TEST(testPackAdd)
{
    Ecs<u32, f32> ecs{};
    Entity e = ecs.spawn();
    ecs.add<u32, f32>(e);
    ASSERT(ecs.has<u32>(e));
    ASSERT(ecs.has<f32>(e));
    ASSERT(ecs.count<u32>() == 1);
    ASSERT(ecs.count<f32>() == 1);

    Entity e2 = ecs.spawn();
    ecs.add<u32, f32>(e2, 33u, 2.5f);
    ASSERT(ecs.get<u32>(e2) == 33);
    ASSERT(ecs.get<f32>(e2) == 2.5f);
}

TEST(testPackAddMove)
{
    Lifecycle::stats.reset();
    {
        Ecs<u32, Lifecycle> ecs{};
        Entity e = ecs.spawn();
        Lifecycle src;
        ecs.add<u32, Lifecycle>(e, 42u, std::move(src));
        ASSERT(!src.valid);
        ASSERT(ecs.has<u32>(e));
        ASSERT(ecs.get<u32>(e) == 42);
        ASSERT(ecs.get<Lifecycle>(e).valid);
        ASSERT(Lifecycle::stats.copies == 0);
        ASSERT(Lifecycle::stats.moves == 1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
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

TEST(testEcsLifecycle)
{
    Lifecycle::stats.reset();
    {
        Ecs<Lifecycle> ecs{};
        Entity e = ecs.spawn();
        ecs.add<Lifecycle>(e);
        ASSERT(Lifecycle::stats.alive == 1);
        ASSERT(Lifecycle::stats.ctors == 1);
        ecs.remove<Lifecycle>(e);
        ASSERT(Lifecycle::stats.alive == 0);
        ASSERT(Lifecycle::stats.dtors == 1);

        Lifecycle src;
        ecs.add<Lifecycle>(e, src);
        ASSERT(src.valid);
        ASSERT(Lifecycle::stats.alive == 2);
        ASSERT(Lifecycle::stats.copies == 1);
        ecs.remove<Lifecycle>(e);
        ASSERT(Lifecycle::stats.alive == 1);

        ecs.add<Lifecycle>(e, std::move(src));
        ASSERT(!src.valid);
        ASSERT(Lifecycle::stats.alive == 1);
        ASSERT(Lifecycle::stats.copies == 1);
        ecs.remove<Lifecycle>(e);
        ASSERT(Lifecycle::stats.alive == 0);
        ASSERT(Lifecycle::stats.dtors == 3);

        for (u32 i = 0; i < 3; ++i)
        {
            ecs.add<Lifecycle>(e);
            ASSERT(ecs.has<Lifecycle>(e));
            ecs.remove<Lifecycle>(e);
            ASSERT(!ecs.has<Lifecycle>(e));
        }
        ASSERT(Lifecycle::stats.alive == 0);
        ASSERT(Lifecycle::stats.ctors == 5);

        Entity e2 = ecs.spawn();
        ecs.add<Lifecycle>(e2);
        ASSERT(Lifecycle::stats.alive == 1);
        ecs.reset();
        ASSERT(Lifecycle::stats.alive == 0);
        ASSERT(Lifecycle::stats.dtors == 7);
        ASSERT(Lifecycle::stats.copies == 1);
        ASSERT(Lifecycle::stats.moves == 7);
    }
    ASSERT(Lifecycle::stats.alive == 0);
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

TEST(testForEachSingleType)
{
    Ecs<u32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    Entity e3 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<u32>(e2) = 20;
    ecs.add<u32>(e3) = 30;
    ecs.remove<u32>(e2);

    u64 sum = 0;
    ecs.forEach<u32>([&](u32& v) { sum += v; });
    ASSERT(sum == 40);

    sum = 0;
    ecs.forEach<u32>([&](Entity e, u32& v)
    {
        sum += v;
        (void)e;
    });
    ASSERT(sum == 40);

    sum = 0;
    u64 visits = 0;
    ecs.forEach<u32>([&](Entity e)
    {
        sum += ecs.get<u32>(e);
        visits++;
    });
    ASSERT(sum == 40);
    ASSERT(visits == 2);
}

TEST(testForEachMultiType)
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
    u64 visits = 0;
    ecs.forEach<u32, f32>([&](Entity e, u32& a, f32& b)
    {
        sum += a + (u64)b;
        visits++;
        (void)e;
    });
    ASSERT(sum == 33);
    ASSERT(visits == 2);

    sum = 0;
    ecs.forEach<u32, f32>([&](u32& a, f32& b) { sum += a + (u64)b; });
    ASSERT(sum == 33);

    sum = 0;
    ecs.forEach<u32, f32>([&](Entity e)
    {
        sum += ecs.get<u32>(e);
        (void)e;
    });
    ASSERT(sum == 3);
}

TEST(testForEachParSingleType)
{
    Ecs<u32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<u32>(e2) = 20;

    SpinLock lk;
    u64 sum = 0;
    ecs.forEachPar<u32>([&](Entity e, u32& v)
    {
        SpinLockScope scope{lk};
        sum += v;
        (void)e;
    });
    ASSERT(sum == 30);

    sum = 0;
    ecs.forEachPar<u32>([&](u32& v)
    {
        SpinLockScope scope{lk};
        sum += v;
    });
    ASSERT(sum == 30);

    sum = 0;
    ecs.forEachPar<u32>([&](Entity e)
    {
        SpinLockScope scope{lk};
        sum += ecs.get<u32>(e);
    });
    ASSERT(sum == 30);
}

TEST(testForEachParMultiType)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 1;
    ecs.add<f32>(e1) = 10.0f;
    ecs.add<u32>(e2) = 2;
    ecs.add<f32>(e2) = 20.0f;

    SpinLock lk;
    u64 sum = 0;
    ecs.forEachPar<u32, f32>([&](Entity e, u32& a, f32& b)
    {
        SpinLockScope scope{lk};
        sum += a + (u64)b;
        (void)e;
    });
    ASSERT(sum == 33);

    sum = 0;
    ecs.forEachPar<u32, f32>([&](Entity e)
    {
        SpinLockScope scope{lk};
        sum += ecs.get<u32>(e);
    });
    ASSERT(sum == 3);
}

TEST(testConstQuery)
{
    Ecs<u32, f32> ecs{};
    Entity e1 = ecs.spawn();
    Entity e2 = ecs.spawn();
    ecs.add<u32>(e1) = 10;
    ecs.add<f32>(e1) = 1.0f;
    ecs.add<u32>(e2) = 20;
    ecs.add<f32>(e2) = 2.0f;
    const Ecs<u32, f32>& cecs = ecs;

    ASSERT(cecs.alive(e1));
    ASSERT(cecs.has<u32>(e1));
    ASSERT((cecs.hasAll<u32, f32>(e1)));
    ASSERT((cecs.hasAny<u32, f32>(e1)));
    ASSERT(cecs.get<u32>(e1) == 10);
    ASSERT(cecs.get<f32>(e2) == 2.0f);
    ASSERT(cecs.count<u32>() == 2);
    ASSERT(cecs.count<f32>() == 2);
    Span<const u32> comps = cecs.getComponents<u32>();
    ASSERT(comps.count == 2);
    const EcsComponent<u32>& cs = cecs.getComponentSystem<u32>();
    ASSERT(cs.has(e1));
    ASSERT(cs.get(e1) == 10);

    u64 sum = 0;
    cecs.forEach<u32>([&](const u32& v) { sum += v; });
    ASSERT(sum == 30);

    sum = 0;
    cecs.forEach<u32>([&](Entity e, const u32& v)
    {
        sum += v;
        (void)e;
    });
    ASSERT(sum == 30);

    sum = 0;
    cecs.forEach<u32>([&](Entity e)
    {
        sum += cecs.get<u32>(e);
    });
    ASSERT(sum == 30);

    sum = 0;
    cecs.forEach<u32, f32>([&](Entity e, const u32& a, const f32& b)
    {
        sum += a + (u64)b;
        (void)e;
    });
    ASSERT(sum == (10 + 1) + (20 + 2));
}

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

TEST(testSerializeEcsEmpty)
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

TEST(testSerializeEcsRoundtrip)
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

TEST(testSerializeEcsSparse)
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

TEST(testSerializeEcsPartialCoverage)
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

TEST(testSerializeEcsEntityReferences)
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

TEST(testSerializeEcsEntityComponent)
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

TEST(testSerializeEcsBinaryRoundtrip)
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

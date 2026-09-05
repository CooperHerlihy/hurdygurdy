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

TEST(testSerializeEcsEmptyNoTypes)
{
    Ecs<> ecs{};
    Ecs<> copy{};
    ArenaScope arena = getScratch();
    Serializer w = serialWriter(arena);
    serialize(&w, &ecs);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
}

TEST(testSerializeEcsEmptyWithTypes)
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

TEST(testSerializeEcsSingleComponent)
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

TEST(testSerializeEcsMultipleEntitiesSingleComponent)
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

TEST(testSerializeEcsMultipleComponentTypes)
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

TEST(testSerializeEcsSparseEntities)
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

TEST(testSerializeEcsLifecycle)
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
        ASSERT(Lifecycle::stats.alive == 4);

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

TEST(testSerializeEcsBinaryRoundTrip)
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

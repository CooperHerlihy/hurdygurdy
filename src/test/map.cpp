#include "tests.hpp"
#include "hg/map.hpp"

using namespace hg;

TEST(testMapDefault)
{
    Map<u32, f32> map;
    ASSERT(map.hasVal == nullptr);
    ASSERT(map.capacity == 0);
    ASSERT(map.count == 0);
}

TEST(testMapInitialCapacity)
{
    Map<u32, f32> map{16};
    ASSERT(map.capacity == 16);
    ASSERT(map.count == 0);
}

TEST(testMapAddGetHas)
{
    Map<u32, f32> map;
    map.add(1, 1.5f);
    map.add(2, 2.5f);
    map.add(3, 3.5f);
    ASSERT(map.count == 3);
    ASSERT(map.has(1));
    ASSERT(map.has(2));
    ASSERT(map.has(3));
    ASSERT(!map.has(4));
    f32* v = map.get(1);
    ASSERT(v != nullptr);
    ASSERT(*v == 1.5f);
    ASSERT(map.get(4) == nullptr);
}

TEST(testMapDuplicateOverwrite)
{
    Map<u32, f32> map;
    map.add(1, 1.0f);
    map.add(1, 2.0f);
    ASSERT(map.count == 1);
    ASSERT(*map.get(1) == 2.0f);
}

TEST(testMapRemove)
{
    Map<u32, f32> map;
    map.add(1, 1.0f);
    map.add(2, 2.0f);
    ASSERT(map.remove(1));
    ASSERT(!map.has(1));
    ASSERT(map.has(2));
    ASSERT(!map.remove(99));
}

TEST(testMapRemoveWithValue)
{
    Map<u32, f32> map;
    map.add(42, 3.14f);
    f32 out = 0;
    ASSERT(map.remove(42, &out));
    ASSERT(out == 3.14f);
}

TEST(testMapCollision)
{
    Map<u32, f32> map{8};
    map.add(0, 0.0f);
    map.add(8, 8.0f);
    map.add(16, 16.0f);
    ASSERT(map.count == 3);
    ASSERT(*map.get(0) == 0.0f);
    ASSERT(*map.get(8) == 8.0f);
    ASSERT(*map.get(16) == 16.0f);

    map.remove(8);
    ASSERT(!map.has(8));
    ASSERT(*map.get(0) == 0.0f);
    ASSERT(*map.get(16) == 16.0f);
}

TEST(testMapReset)
{
    Lifecycle::stats.reset();
    Map<u32, Lifecycle> map;
    map.add(1, Lifecycle{});
    map.add(2, Lifecycle{});
    ASSERT(Lifecycle::stats.alive == 2);
    map.reset();
    ASSERT(map.count == 0);
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMapResize)
{
    Map<u32, f32> map{4};
    map.add(1, 1.0f);
    map.add(2, 2.0f);
    map.resize(32);
    ASSERT(map.capacity == 32);
    ASSERT(map.count == 2);
    ASSERT(*map.get(1) == 1.0f);
    ASSERT(*map.get(2) == 2.0f);
}

TEST(testMapForEach)
{
    Map<u32, f32> map;
    map.add(1, 10.0f);
    map.add(2, 20.0f);
    f64 sum = 0;
    map.forEach([&](const u32& k, f32& v) { sum += static_cast<f64>(k) + v; });
    ASSERT(sum == 33.0);
}

TEST(testMapMoveConstruct)
{
    Map<u32, f32> a;
    a.add(1, 1.0f);
    bool* oldHasVal = a.hasVal;
    Map<u32, f32> b = std::move(a);
    ASSERT(a.hasVal == nullptr);
    ASSERT(b.hasVal == oldHasVal);
    ASSERT(b.count == 1);
    ASSERT(*b.get(1) == 1.0f);
}

TEST(testMapMoveAssign)
{
    Map<u32, f32> a;
    a.add(5, 5.0f);
    Map<u32, f32> b;
    b = std::move(a);
    ASSERT(*b.get(5) == 5.0f);
}

TEST(testMapTempDefault)
{
    MapTemp<u32, f32> map;
    ASSERT(map.arena == nullptr);
    ASSERT(map.capacity == 0);
    ASSERT(map.count == 0);
}

TEST(testMapTempCreate)
{
    ArenaScope arena = getScratch();
    MapTemp<u32, f32> map{arena, 16};
    ASSERT(map.arena == arena);
    ASSERT(map.capacity == 16);
    ASSERT(map.count == 0);
}

TEST(testMapTempAddGetHasRemove)
{
    ArenaScope arena = getScratch();
    MapTemp<u32, f32> map{arena, 0};
    map.add(1, 1.5f);
    map.add(2, 2.5f);
    ASSERT(map.count == 2);
    ASSERT(map.has(1));
    ASSERT(*map.get(1) == 1.5f);
    map.remove(1);
    ASSERT(!map.has(1));
    ASSERT(map.has(2));
}

TEST(testMapTempForEach)
{
    ArenaScope arena = getScratch();
    MapTemp<u32, f32> map{arena, 0};
    map.add(1, 10.0f);
    map.add(2, 20.0f);
    f64 sum = 0;
    map.forEach([&](const u32& k, f32& v) { sum += static_cast<f64>(k) + v; });
    ASSERT(sum == 33.0);
}

TEST(testMapStringHeap)
{
    Map<String, u32> map{16};
    map.add(String::create("alpha"), 1u);
    map.add(String::create("beta"), 2u);
    map.add(String::create("gamma"), 3u);
    ASSERT(map.count == 3);

    ASSERT(map.has(String::create("beta")));
    ASSERT(*map.get(String::create("beta")) == 2u);

    ASSERT(map.has(StringView{"alpha"}));
    ASSERT(*map.get(StringView{"gamma"}) == 3u);

    const char* b = "beta";
    ASSERT(map.has(b));
    ASSERT(*map.get(b) == 2u);

    ArenaScope arena = getScratch();
    ASSERT(map.has(StringBuilder{arena, "alpha"}));
    ASSERT(*map.get(StringBuilder{arena, "gamma"}) == 3u);

    ASSERT(map.has("alpha"));
    ASSERT(*map.get("beta") == 2u);

    ASSERT(!map.has("delta"));
    ASSERT(map.get("delta") == nullptr);
    ASSERT(!map.has(StringView{"delta"}));

    u32 out = 0;
    ASSERT(map.remove(StringView{"beta"}, &out));
    ASSERT(out == 2u);
    ASSERT(!map.has("beta"));
    ASSERT(map.count == 2);

    ASSERT(map.remove("alpha"));
    ASSERT(!map.has(StringView{"alpha"}));
    ASSERT(map.count == 1);

    ASSERT(map.has("gamma"));
    ASSERT(map.has(StringView{"gamma"}));
    ASSERT(*map.get(StringBuilder{arena, "gamma"}) == 3u);
}

TEST(testMapStringTemp)
{
    ArenaScope arena = getScratch();
    MapTemp<String, u32> map{arena, 16};
    map.add(String::create("alpha"), 1u);
    map.add(String::create("beta"), 2u);
    map.add(String::create("gamma"), 3u);
    ASSERT(map.count == 3);

    ASSERT(map.has(StringView{"alpha"}));
    ASSERT(*map.get(StringView{"gamma"}) == 3u);

    const char* b = "beta";
    ASSERT(map.has(b));
    ASSERT(*map.get(b) == 2u);

    ASSERT(map.has(StringBuilder{arena, "alpha"}));
    ASSERT(map.has("gamma"));
    ASSERT(!map.has("delta"));

    u32 out = 0;
    ASSERT(map.remove(StringView{"beta"}, &out));
    ASSERT(out == 2u);
    ASSERT(!map.has("beta"));
    ASSERT(map.count == 2);

    ASSERT(map.remove("alpha"));
    ASSERT(!map.has(StringView{"alpha"}));
    ASSERT(map.count == 1);
    ASSERT(map.has("gamma"));
}

TEST(testMapStringMoveOnly)
{
    Map<String, String> map{16};
    map.add(String::create("k1"), String::create("v1"));
    map.add(String::create("k2"), String::create("v2"));
    ASSERT(map.count == 2);
    ASSERT(map.has("k1"));
    ASSERT(map.has("k2"));
    ASSERT(*map.get("k1") == "v1");
    ASSERT(*map.get("k2") == "v2");

    String out = std::move(*map.get("k1"));
    ASSERT(out == "v1");

    ASSERT(map.remove("k2"));
    ASSERT(!map.has("k2"));
    ASSERT(map.count == 1);
}

TEST(testMapStringTempMoveOnly)
{
    ArenaScope arena = getScratch();
    MapTemp<String, String> map{arena, 16};
    map.add(String::create("k1"), String::create("v1"));
    map.add(String::create("k2"), String::create("v2"));
    ASSERT(map.count == 2);
    ASSERT(map.has(StringView{"k1"}));
    ASSERT(*map.get("k2") == "v2");
    ASSERT(map.remove("k1"));
    ASSERT(!map.has("k1"));
    ASSERT(map.count == 1);
}

TEST(testMapViewHeap)
{
    Map<StringView, u32> map{16};
    map.add("alpha", 1u);
    map.add("beta", 2u);
    map.add("gamma", 3u);
    ASSERT(map.count == 3);

    ASSERT(map.has(StringView{"alpha"}));
    ASSERT(*map.get(StringView{"gamma"}) == 3u);

    const char* b = "beta";
    ASSERT(map.has(b));
    ASSERT(*map.get(b) == 2u);

    ArenaScope arena = getScratch();
    ASSERT(map.has(StringBuilder{arena, "alpha"}));
    ASSERT(*map.get(StringBuilder{arena, "gamma"}) == 3u);

    ASSERT(map.has("alpha"));
    ASSERT(*map.get("beta") == 2u);
    ASSERT(!map.has("delta"));

    u32 out = 0;
    ASSERT(map.remove(StringView{"beta"}, &out));
    ASSERT(out == 2u);
    ASSERT(!map.has("beta"));
    ASSERT(map.count == 2);
    ASSERT(map.has("gamma"));
}

TEST(testMapViewTemp)
{
    ArenaScope arena = getScratch();
    MapTemp<StringView, u32> map{arena, 16};
    map.add("alpha", 1u);
    map.add("beta", 2u);
    map.add("gamma", 3u);
    ASSERT(map.count == 3);

    ASSERT(map.has(StringView{"alpha"}));
    ASSERT(*map.get(StringView{"gamma"}) == 3u);
    ASSERT(map.has("beta"));
    ASSERT(*map.get(StringBuilder{arena, "alpha"}) == 1u);
    ASSERT(!map.has("delta"));

    ASSERT(map.remove("beta"));
    ASSERT(!map.has(StringView{"beta"}));
    ASSERT(map.count == 2);
    ASSERT(map.has("gamma"));
}

TEST(testMapStringCollision)
{
    Map<String, u32> map{4};
    const char* keys[] = {
        "red", "green", "blue", "cyan", "magenta",
        "yellow", "black", "white", "orange", "pink"
    };
    for (u32 i = 0; i < 10; ++i)
        map.add(String::create(keys[i]), static_cast<u32>(i));
    ASSERT(map.count == 10);
    for (u32 i = 0; i < 10; ++i)
    {
        ASSERT(map.has(keys[i]));
        ASSERT(*map.get(StringView{keys[i]}) == i);
        ASSERT(map.has(String::create(keys[i])));
    }
    ASSERT(!map.has("purple"));
}

TEST(testMapForEachModify)
{
    Map<u32, f32> map;
    map.add(1, 10.0f);
    map.add(2, 20.0f);
    map.add(3, 30.0f);
    map.forEach([](const u32& k, f32& v) { v += static_cast<f32>(k); });
    ASSERT(*map.get(1) == 11.0f);
    ASSERT(*map.get(2) == 22.0f);
    ASSERT(*map.get(3) == 33.0f);
}

TEST(testMapResizeSmaller)
{
    Map<u32, f32> map{64};
    map.add(1, 1.0f);
    map.add(2, 2.0f);
    map.add(3, 3.0f);
    map.resize(8);
    ASSERT(map.capacity == 8);
    ASSERT(map.count == 3);
    ASSERT(*map.get(1) == 1.0f);
    ASSERT(*map.get(2) == 2.0f);
    ASSERT(*map.get(3) == 3.0f);
}

TEST(testMapResizeSame)
{
    Map<u32, f32> map{16};
    map.add(1, 1.0f);
    map.add(2, 2.0f);
    u64 oldCapacity = map.capacity;
    map.resize(16);
    ASSERT(map.capacity == oldCapacity);
    ASSERT(map.count == 2);
    ASSERT(*map.get(1) == 1.0f);
    ASSERT(*map.get(2) == 2.0f);
}

TEST(testMapResetEmpty)
{
    Map<u32, f32> map;
    map.reset();
    ASSERT(map.count == 0);
    ASSERT(map.capacity == 0);
    Map<u32, f32> map2{8};
    map2.reset();
    ASSERT(map2.count == 0);
    ASSERT(map2.capacity == 8);
}

TEST(testMapAddManyThenRemoveAll)
{
    Map<u32, u32> map;
    for (u32 i = 0; i < 100; ++i)
        map.add(i, i * 10);
    ASSERT(map.count == 100);
    for (u32 i = 0; i < 100; ++i)
        ASSERT(map.remove(i));
    ASSERT(map.count == 0);
    for (u32 i = 0; i < 100; ++i)
        ASSERT(!map.has(i));
}

TEST(testMapGetExistingKey)
{
    Map<u32, f32> map;
    map.add(42, 3.14f);
    f32* ptr = map.get(42);
    ASSERT(ptr != nullptr);
    ASSERT(*ptr == 3.14f);
    *ptr = 6.28f;
    ASSERT(*map.get(42) == 6.28f);
}

TEST(testMapRemoveNonExistentKey)
{
    Map<u32, f32> map{16};
    map.add(1, 1.0f);
    ASSERT(!map.remove(99));
    ASSERT(!map.remove(0));
    ASSERT(map.count == 1);
}

TEST(testMapTempDuplicateAdd)
{
    ArenaScope arena = getScratch();
    MapTemp<u32, f32> map{arena, 16};
    map.add(1, 1.0f);
    map.add(1, 2.0f);
    ASSERT(map.count == 1);
    ASSERT(*map.get(1) == 2.0f);
}

TEST(testMapTempRemoveNonExistent)
{
    ArenaScope arena = getScratch();
    MapTemp<u32, f32> map{arena, 16};
    map.add(1, 1.0f);
    ASSERT(!map.remove(99));
    ASSERT(!map.remove(0));
    ASSERT(map.count == 1);
}

TEST(testMapTempReset)
{
    ArenaScope arena = getScratch();
    MapTemp<u32, f32> map{arena, 16};
    map.add(1, 1.0f);
    map.add(2, 2.0f);
    map.reset();
    ASSERT(map.count == 0);
    ASSERT(map.capacity == 16);
    ASSERT(!map.has(1));
    ASSERT(!map.has(2));
}

TEST(testMapStringViewComparison)
{
    Map<StringView, u32> map{16};
    map.add("alpha", 1u);
    map.add("beta", 2u);
    ASSERT(map.has(StringView{"alpha"}));
    ASSERT(map.has(StringView{"beta"}));
    ASSERT(!map.has(StringView{"gamma"}));

    u32 out = 0;
    ASSERT(map.remove(StringView{"alpha"}, &out));
    ASSERT(out == 1u);
    ASSERT(!map.has(StringView{"alpha"}));
    ASSERT(map.has(StringView{"beta"}));

    u32* v = map.get(StringView{"beta"});
    ASSERT(v != nullptr);
    ASSERT(*v == 2u);
}

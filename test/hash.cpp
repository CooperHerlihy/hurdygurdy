#include "tests.hpp"
#include "hg/hash.hpp"
#include "hg/strings.hpp"

#include <string>
#include <string_view>

using namespace hg;

TEST(testHashStringConsistency)
{
    const char* cstr = "hello";
    std::string_view sv{cstr};
    std::string s = std::string{sv};
    std::string sb{sv};

    u64 hSv = hash(sv);
    u64 hStr = hash(s);
    u64 hCstr = hash(cstr);
    u64 hSb = hash(sb);
    u64 hLit = hash("hello");

    ASSERT(hSv == hStr);
    ASSERT(hSv == hCstr);
    ASSERT(hSv == hSb);
    ASSERT(hSv == hLit);
}

TEST(testHashEmptyString)
{
    std::string_view sv{};
    std::string s = std::string{sv};
    u64 hSv = hash(sv);
    u64 hStr = hash(s);
    u64 hLit = hash("");
    ASSERT(hSv == hStr);
    ASSERT(hSv == hLit);
}

TEST(testHashCArray)
{
    char buf[] = "abc";
    u64 hArr = hash(buf);
    u64 hPtr = hash((const char*)buf);
    ASSERT(hArr == hPtr);
}

TEST(testHashStringViewLength)
{
    const char* data = "hello world";
    std::string_view sv{data, 5};
    u64 hLen = hash(sv);
    u64 hTerm = hash(std::string_view{"hello"});
    ASSERT(hLen == hTerm);
}

TEST(testHashDistinct)
{
    ASSERT(hash(std::string_view{"abc"}) != hash(std::string_view{"abd"}));
    ASSERT(hash(std::string_view{""}) != hash(std::string_view{"a"}));
}

TEST(testHashInteger)
{
    ASSERT(hash(static_cast<u32>(5)) == hash(static_cast<u32>(5)));
    ASSERT(hash(static_cast<u32>(5)) != hash(static_cast<u32>(6)));
    ASSERT(hash(static_cast<i32>(-1)) == hash(static_cast<i32>(-1)));
    ASSERT(hash(static_cast<i32>(-1)) != hash(static_cast<i32>(1)));
}

TEST(testHashU8)
{
    ASSERT(hash(static_cast<u8>(0)) == static_cast<u64>(0));
    ASSERT(hash(static_cast<u8>(255)) == static_cast<u64>(255));
    ASSERT(hash(static_cast<u8>(42)) == hash(static_cast<u8>(42)));
    ASSERT(hash(static_cast<u8>(42)) != hash(static_cast<u8>(43)));
}

TEST(testHashU16)
{
    ASSERT(hash(static_cast<u16>(0)) == static_cast<u64>(0));
    ASSERT(hash(static_cast<u16>(65535)) == static_cast<u64>(65535));
    ASSERT(hash(static_cast<u16>(123)) == hash(static_cast<u16>(123)));
    ASSERT(hash(static_cast<u16>(123)) != hash(static_cast<u16>(124)));
}

TEST(testHashU64)
{
    ASSERT(hash(static_cast<u64>(0)) == static_cast<u64>(0));
    ASSERT(hash(static_cast<u64>(0xFFFFFFFFFFFFFFFF)) == static_cast<u64>(0xFFFFFFFFFFFFFFFF));
    ASSERT(hash(static_cast<u64>(999)) == hash(static_cast<u64>(999)));
    ASSERT(hash(static_cast<u64>(999)) != hash(static_cast<u64>(1000)));
}

TEST(testHashI8)
{
    ASSERT(hash(static_cast<i8>(0)) == static_cast<u64>(0));
    ASSERT(hash(static_cast<i8>(-1)) == static_cast<u64>(static_cast<u64>(-1)));
    ASSERT(hash(static_cast<i8>(-128)) == hash(static_cast<i8>(-128)));
    ASSERT(hash(static_cast<i8>(127)) == hash(static_cast<i8>(127)));
    ASSERT(hash(static_cast<i8>(-1)) != hash(static_cast<i8>(1)));
}

TEST(testHashI16)
{
    ASSERT(hash(static_cast<i16>(0)) == static_cast<u64>(0));
    ASSERT(hash(static_cast<i16>(-1)) == static_cast<u64>(static_cast<u64>(-1)));
    ASSERT(hash(static_cast<i16>(-32768)) == hash(static_cast<i16>(-32768)));
    ASSERT(hash(static_cast<i16>(32767)) == hash(static_cast<i16>(32767)));
    ASSERT(hash(static_cast<i16>(-32768)) != hash(static_cast<i16>(32767)));
}

TEST(testHashI64)
{
    ASSERT(hash(static_cast<i64>(0)) == static_cast<u64>(0));
    ASSERT(hash(static_cast<i64>(-1)) == static_cast<u64>(static_cast<u64>(-1)));
    ASSERT(hash(static_cast<i64>(-9223372036854775807LL - 1)) == hash(static_cast<i64>(-9223372036854775807LL - 1)));
    ASSERT(hash(static_cast<i64>(-1)) != hash(static_cast<i64>(1)));
}

TEST(testHashFloat)
{
    ASSERT(hash(static_cast<f32>(0.0f)) == hash(static_cast<f32>(0.0f)));
    ASSERT(hash(static_cast<f32>(1.0f)) == hash(static_cast<f32>(1.0f)));
    ASSERT(hash(static_cast<f32>(1.0f)) != hash(static_cast<f32>(2.0f)));
    ASSERT(hash(static_cast<f32>(-1.0f)) == hash(static_cast<f32>(-1.0f)));
    ASSERT(hash(static_cast<f32>(0.0f)) != hash(static_cast<f32>(-0.0f)));
}

TEST(testHashDouble)
{
    ASSERT(hash(static_cast<f64>(0.0)) == hash(static_cast<f64>(0.0)));
    ASSERT(hash(static_cast<f64>(1.0)) == hash(static_cast<f64>(1.0)));
    ASSERT(hash(static_cast<f64>(1.0)) != hash(static_cast<f64>(2.0)));
    ASSERT(hash(static_cast<f64>(-1.0)) == hash(static_cast<f64>(-1.0)));
    ASSERT(hash(static_cast<f64>(0.0)) != hash(static_cast<f64>(-0.0)));
}

TEST(testHashPointer)
{
    int a = 1;
    int b = 2;
    void* pa = &a;
    void* pb = &b;
    ASSERT(hash(pa) == hash(pa));
    ASSERT(hash(pa) != hash(pb));
    ASSERT(hash((void*)nullptr) == hash((void*)nullptr));
    ASSERT(hash(pa) != hash((void*)nullptr));
}

TEST(testHashStringViewEmpty)
{
    std::string_view sv{};
    u64 h = hash(sv);
    ASSERT(h == hash(std::string_view{}));
    ASSERT(h == 0);
}

TEST(testHashStringViewNonEmpty)
{
    const char* data = "test";
    std::string_view sv{data};
    u64 hSv = hash(sv);
    u64 hContent = hash(std::string_view{"test"});
    ASSERT(hSv == hContent);
    ASSERT(hSv != 0);
}

TEST(testHashDistinctStrings)
{
    ASSERT(hash(std::string_view{"aaa"}) != hash(std::string_view{"aab"}));
    ASSERT(hash(std::string_view{"a"}) != hash(std::string_view{"b"}));
    ASSERT(hash(std::string_view{"ab"}) != hash(std::string_view{"ba"}));
    ASSERT(hash(std::string_view{"hello"}) != hash(std::string_view{"world"}));
    ASSERT(hash(std::string_view{""}) != hash(std::string_view{"x"}));
}

TEST(testSetDefault)
{
    HashSet<u32> set;
    ASSERT(set.hasVal == nullptr);
    ASSERT(set.capacity == 0);
    ASSERT(set.count == 0);
}

TEST(testSetInitialCapacity)
{
    HashSet<u32> set{16};
    ASSERT(set.capacity == 16);
    ASSERT(set.count == 0);
}

TEST(testSetAddHas)
{
    HashSet<u32> set;
    set.add(10);
    set.add(20);
    set.add(30);
    ASSERT(set.count == 3);
    ASSERT(set.has(10u));
    ASSERT(set.has(20u));
    ASSERT(set.has(30u));
    ASSERT(!set.has(40u));
}

TEST(testSetDuplicateAdd)
{
    HashSet<u32> set;
    set.add(42);
    set.add(42);
    ASSERT(set.count == 1);
}

TEST(testSetRemove)
{
    HashSet<u32> set;
    set.add(1);
    set.add(2);
    set.add(3);
    set.remove(2u);
    ASSERT(set.count == 2);
    ASSERT(!set.has(2u));
    ASSERT(set.has(1u));
    ASSERT(set.has(3u));
}

TEST(testSetRemoveNonExistent)
{
    HashSet<u32> set;
    set.add(5u);
    set.remove(99u);
    ASSERT(set.count == 1);
}

TEST(testSetCollision)
{
    HashSet<u32> set{8};
    set.add(0u);
    set.add(8u);
    set.add(16u);
    ASSERT(set.count == 3);
    ASSERT(set.has(0u));
    ASSERT(set.has(8u));
    ASSERT(set.has(16u));

    set.remove(8u);
    ASSERT(!set.has(8u));
    ASSERT(set.has(0u));
    ASSERT(set.has(16u));

    set.remove(0u);
    ASSERT(!set.has(0u));
    ASSERT(set.has(16u));
}

TEST(testSetClear)
{
    Lifecycle::stats.reset();
    HashSet<Lifecycle> set;
    set.add(Lifecycle{});
    set.add(Lifecycle{});
    ASSERT(Lifecycle::stats.alive == 2);
    set.clear();
    ASSERT(set.count == 0);
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testSetResize)
{
    HashSet<u32> set{4};
    set.add(1);
    set.add(2);
    set.reserve(32);
    ASSERT(set.capacity == 32);
    ASSERT(set.count == 2);
    ASSERT(set.has(1u));
    ASSERT(set.has(2u));
}

TEST(testSetForEach)
{
    HashSet<u32> set;
    set.add(10);
    set.add(20);
    set.add(30);
    u64 sum = 0;
    set.forEach([&](const u32& v) { sum += v; });
    ASSERT(sum == 60);
}

TEST(testSetForEachConst)
{
    HashSet<u32> set;
    set.add(10);
    set.add(20);
    set.add(30);
    u64 sum = 0;
    const HashSet<u32>& cset = set;
    cset.forEach([&](const u32& v) { sum += v; });
    ASSERT(sum == 60);
}

TEST(testSetMoveConstruct)
{
    HashSet<u32> a;
    a.add(1);
    a.add(2);
    bool* oldHasVal = a.hasVal;
    HashSet<u32> b = std::move(a);
    ASSERT(a.hasVal == nullptr);
    ASSERT(b.hasVal == oldHasVal);
    ASSERT(b.count == 2);
    ASSERT(b.has(1u));
    ASSERT(b.has(2u));
}

TEST(testSetMoveAssign)
{
    HashSet<u32> a;
    a.add(99);
    HashSet<u32> b;
    b = std::move(a);
    ASSERT(b.has(99u));
}

TEST(testSetString)
{
    HashSet<std::string> set{16};
    set.add(std::string{"apple"});
    set.add(std::string{"banana"});
    set.add(std::string{"cherry"});
    ASSERT(set.count == 3);

    ASSERT(set.has(std::string{"banana"}));
    ASSERT(set.has(std::string_view{"apple"}));

    const char* b = "banana";
    ASSERT(set.has(b));

    ASSERT(set.has("cherry"));
    ASSERT(!set.has("durian"));

    set.remove(std::string_view{"banana"});
    ASSERT(!set.has("banana"));
    ASSERT(set.count == 2);

    set.remove("apple");
    ASSERT(!set.has(std::string_view{"apple"}));
    ASSERT(set.count == 1);
    ASSERT(set.has("cherry"));
}

TEST(testSetStringView)
{
    HashSet<std::string_view> set{16};
    set.add("apple");
    set.add("banana");
    set.add("cherry");
    ASSERT(set.count == 3);

    ASSERT(set.has(std::string_view{"apple"}));
    const char* b = "banana";
    ASSERT(set.has(b));

    ASSERT(set.has("cherry"));
    ASSERT(!set.has("durian"));

    set.remove(std::string_view{"banana"});
    ASSERT(!set.has("banana"));
    ASSERT(set.count == 2);

    set.remove("apple");
    ASSERT(!set.has(std::string_view{"apple"}));
    ASSERT(set.count == 1);
    ASSERT(set.has("cherry"));
}

TEST(testSetStringCollision)
{
    HashSet<std::string> set{4};
    const char* vals[] = {
        "red", "green", "blue", "cyan", "magenta",
        "yellow", "black", "white", "orange", "pink"
    };
    for (u32 i = 0; i < 10; ++i)
        set.add(std::string{vals[i]});
    ASSERT(set.count == 10);
    for (u32 i = 0; i < 10; ++i)
    {
        ASSERT(set.has(vals[i]));
        ASSERT(set.has(std::string_view{vals[i]}));
        ASSERT(set.has(std::string{vals[i]}));
    }
    ASSERT(!set.has("purple"));
}

TEST(testSetForEachModify)
{
    HashSet<u32> set;
    set.add(10);
    set.add(20);
    set.add(30);
    u64 count = 0;
    set.forEach([&](const u32& v) {
        ASSERT(v > 0);
        ++count;
    });
    ASSERT(count == 3);
}

TEST(testSetResizeSame)
{
    HashSet<u32> set{8};
    set.add(1);
    set.add(2);
    set.reserve(8);
    ASSERT(set.capacity == 8);
    ASSERT(set.count == 2);
    ASSERT(set.has(1u));
    ASSERT(set.has(2u));
}

TEST(testSetClearEmpty)
{
    HashSet<u32> set{8};
    set.clear();
    ASSERT(set.count == 0);
    ASSERT(!set.has(0u));
}

TEST(testSetAddManyThenRemoveAll)
{
    HashSet<u32> set;
    for (u32 i = 0; i < 100; ++i)
        set.add(i);
    ASSERT(set.count == 100);
    for (u32 i = 0; i < 100; ++i)
        set.remove(i);
    ASSERT(set.count == 0);
    for (u32 i = 0; i < 100; ++i)
        ASSERT(!set.has(i));
}

TEST(testSetStringViewComparison)
{
    HashSet<std::string_view> set{16};
    set.add("alpha");
    set.add("beta");
    set.add("gamma");

    ASSERT(set.has(std::string_view{"alpha"}));
    ASSERT(set.has(std::string_view{"beta"}));
    ASSERT(set.has(std::string_view{"gamma"}));
    ASSERT(!set.has(std::string_view{"delta"}));

    set.remove(std::string_view{"beta"});
    ASSERT(!set.has("beta"));
    ASSERT(set.count == 2);

    ASSERT(set.has("alpha"));
    ASSERT(!set.has("beta"));

    set.add("beta");
    ASSERT(set.has(std::string_view{"beta"}));
    ASSERT(set.count == 3);
}

TEST(testMapDefault)
{
    HashMap<u32, f32> map;
    ASSERT(map.hasVal == nullptr);
    ASSERT(map.capacity == 0);
    ASSERT(map.count == 0);
}

TEST(testMapInitialCapacity)
{
    HashMap<u32, f32> map{16};
    ASSERT(map.capacity == 16);
    ASSERT(map.count == 0);
}

TEST(testMapAddGetHas)
{
    HashMap<u32, f32> map;
    map.add(1, 1.5f);
    map.add(2, 2.5f);
    map.add(3, 3.5f);
    ASSERT(map.count == 3);
    ASSERT(map.has(1u));
    ASSERT(map.has(2u));
    ASSERT(map.has(3u));
    ASSERT(!map.has(4u));
    f32* v = map.get(1u);
    ASSERT(v != nullptr);
    ASSERT(*v == 1.5f);
    ASSERT(map.get(4u) == nullptr);
}

TEST(testMapDuplicateOverwrite)
{
    HashMap<u32, f32> map;
    map.add(1, 1.0f);
    map.add(1, 2.0f);
    ASSERT(map.count == 1);
    ASSERT(*map.get(1u) == 2.0f);
}

TEST(testMapRemove)
{
    HashMap<u32, f32> map;
    map.add(1, 1.0f);
    map.add(2, 2.0f);
    ASSERT(map.remove(1u));
    ASSERT(!map.has(1u));
    ASSERT(map.has(2u));
    ASSERT(!map.remove(99u));
}

TEST(testMapRemoveWithValue)
{
    HashMap<u32, f32> map;
    map.add(42, 3.14f);
    f32 out = 0;
    ASSERT(map.remove(42u, &out));
    ASSERT(out == 3.14f);
}

TEST(testMapCollision)
{
    HashMap<u32, f32> map{8};
    map.add(0, 0.0f);
    map.add(8, 8.0f);
    map.add(16, 16.0f);
    ASSERT(map.count == 3);
    ASSERT(*map.get(0u) == 0.0f);
    ASSERT(*map.get(8u) == 8.0f);
    ASSERT(*map.get(16u) == 16.0f);

    map.remove(8u);
    ASSERT(!map.has(8u));
    ASSERT(*map.get(0u) == 0.0f);
    ASSERT(*map.get(16u) == 16.0f);
}

TEST(testMapClear)
{
    Lifecycle::stats.reset();
    HashMap<u32, Lifecycle> map;
    map.add(1, Lifecycle{});
    map.add(2, Lifecycle{});
    ASSERT(Lifecycle::stats.alive == 2);
    map.clear();
    ASSERT(map.count == 0);
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testMapResize)
{
    HashMap<u32, f32> map{4};
    map.add(1, 1.0f);
    map.add(2, 2.0f);
    map.reserve(32);
    ASSERT(map.capacity == 32);
    ASSERT(map.count == 2);
    ASSERT(*map.get(1u) == 1.0f);
    ASSERT(*map.get(2u) == 2.0f);
}

TEST(testMapForEach)
{
    HashMap<u32, f32> map;
    map.add(1, 10.0f);
    map.add(2, 20.0f);
    f64 sum = 0;
    map.forEach([&](const u32& k, f32& v) { sum += static_cast<f64>(k) + v; });
    ASSERT(sum == 33.0);
}

TEST(testMapForEachConst)
{
    HashMap<u32, f32> map;
    map.add(1, 10.0f);
    map.add(2, 20.0f);
    u64 count = 0;
    f64 sum = 0;
    const HashMap<u32, f32>& cmap = map;
    cmap.forEach([&](const u32& k, const f32& v)
    {
        ++count;
        sum += static_cast<f64>(k) + v;
    });
    ASSERT(count == 2);
    ASSERT(sum == 33.0);
}

TEST(testMapMoveConstruct)
{
    HashMap<u32, f32> a;
    a.add(1, 1.0f);
    bool* oldHasVal = a.hasVal;
    HashMap<u32, f32> b = std::move(a);
    ASSERT(a.hasVal == nullptr);
    ASSERT(b.hasVal == oldHasVal);
    ASSERT(b.count == 1);
    ASSERT(*b.get(1u) == 1.0f);
}

TEST(testMapMoveAssign)
{
    HashMap<u32, f32> a;
    a.add(5, 5.0f);
    HashMap<u32, f32> b;
    b = std::move(a);
    ASSERT(*b.get(5u) == 5.0f);
}

TEST(testMapForEachModify)
{
    HashMap<u32, f32> map;
    map.add(1, 10.0f);
    map.add(2, 20.0f);
    map.add(3, 30.0f);
    map.forEach([](const u32& k, f32& v) { v += static_cast<f32>(k); });
    ASSERT(*map.get(1u) == 11.0f);
    ASSERT(*map.get(2u) == 22.0f);
    ASSERT(*map.get(3u) == 33.0f);
}

TEST(testMapResizeSame)
{
    HashMap<u32, f32> map{16};
    map.add(1, 1.0f);
    map.add(2, 2.0f);
    u64 oldCapacity = map.capacity;
    map.reserve(16);
    ASSERT(map.capacity == oldCapacity);
    ASSERT(map.count == 2);
    ASSERT(*map.get(1u) == 1.0f);
    ASSERT(*map.get(2u) == 2.0f);
}

TEST(testMapClearEmpty)
{
    HashMap<u32, f32> map;
    map.clear();
    ASSERT(map.count == 0);
    ASSERT(map.capacity == 0);
    HashMap<u32, f32> map2{8};
    map2.clear();
    ASSERT(map2.count == 0);
    ASSERT(map2.capacity == 8);
}

TEST(testMapAddManyThenRemoveAll)
{
    HashMap<u32, u32> map;
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
    HashMap<u32, f32> map;
    map.add(42, 3.14f);
    f32* ptr = map.get(42u);
    ASSERT(ptr != nullptr);
    ASSERT(*ptr == 3.14f);
    *ptr = 6.28f;
    ASSERT(*map.get(42u) == 6.28f);
}

TEST(testMapRemoveNonExistentKey)
{
    HashMap<u32, f32> map{16};
    map.add(1, 1.0f);
    ASSERT(!map.remove(99u));
    ASSERT(!map.remove(0u));
    ASSERT(map.count == 1);
}

TEST(testMapLifecycleValues)
{
    Lifecycle::stats.reset();
    HashMap<u32, Lifecycle> map;
    map.add(1, Lifecycle{});
    map.add(2, Lifecycle{});
    map.add(3, Lifecycle{});
    ASSERT(Lifecycle::stats.alive == 3);
    ASSERT(Lifecycle::stats.copies + Lifecycle::stats.moves >= 3);
    map.remove(2u);
    ASSERT(Lifecycle::stats.alive == 2);
    map.clear();
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors >= 3);
}

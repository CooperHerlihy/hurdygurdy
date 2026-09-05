#include "tests.hpp"
#include "hg/set.hpp"

using namespace hg;

TEST(testSetDefault)
{
    Set<u32> set;
    ASSERT(set.hasVal == nullptr);
    ASSERT(set.capacity == 0);
    ASSERT(set.count == 0);
}

TEST(testSetInitialCapacity)
{
    Set<u32> set{16};
    ASSERT(set.capacity == 16);
    ASSERT(set.count == 0);
}

TEST(testSetAddHas)
{
    Set<u32> set;
    set.add(10);
    set.add(20);
    set.add(30);
    ASSERT(set.count == 3);
    ASSERT(set.has(10));
    ASSERT(set.has(20));
    ASSERT(set.has(30));
    ASSERT(!set.has(40));
}

TEST(testSetDuplicateAdd)
{
    Set<u32> set;
    set.add(42);
    set.add(42);
    ASSERT(set.count == 1);
}

TEST(testSetRemove)
{
    Set<u32> set;
    set.add(1);
    set.add(2);
    set.add(3);
    set.remove(2);
    ASSERT(set.count == 2);
    ASSERT(!set.has(2));
    ASSERT(set.has(1));
    ASSERT(set.has(3));
}

TEST(testSetRemoveNonExistent)
{
    Set<u32> set;
    set.add(5);
    set.remove(99);
    ASSERT(set.count == 1);
}

TEST(testSetCollision)
{
    Set<u32> set{8};
    set.add(0);
    set.add(8);
    set.add(16);
    ASSERT(set.count == 3);
    ASSERT(set.has(0));
    ASSERT(set.has(8));
    ASSERT(set.has(16));

    set.remove(8);
    ASSERT(!set.has(8));
    ASSERT(set.has(0));
    ASSERT(set.has(16));

    set.remove(0);
    ASSERT(!set.has(0));
    ASSERT(set.has(16));
}

TEST(testSetReset)
{
    Lifecycle::stats.reset();
    Set<Lifecycle> set;
    set.add(Lifecycle{});
    set.add(Lifecycle{});
    ASSERT(Lifecycle::stats.alive == 2);
    set.reset();
    ASSERT(set.count == 0);
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testSetResize)
{
    Set<u32> set{4};
    set.add(1);
    set.add(2);
    set.resize(32);
    ASSERT(set.capacity == 32);
    ASSERT(set.count == 2);
    ASSERT(set.has(1));
    ASSERT(set.has(2));
}

TEST(testSetForEach)
{
    Set<u32> set;
    set.add(10);
    set.add(20);
    set.add(30);
    u64 sum = 0;
    set.forEach([&](const u32& v) { sum += v; });
    ASSERT(sum == 60);
}

TEST(testSetMoveConstruct)
{
    Set<u32> a;
    a.add(1);
    a.add(2);
    bool* oldHasVal = a.hasVal;
    Set<u32> b = std::move(a);
    ASSERT(a.hasVal == nullptr);
    ASSERT(b.hasVal == oldHasVal);
    ASSERT(b.count == 2);
    ASSERT(b.has(1));
    ASSERT(b.has(2));
}

TEST(testSetMoveAssign)
{
    Set<u32> a;
    a.add(99);
    Set<u32> b;
    b = std::move(a);
    ASSERT(b.has(99));
}

TEST(testSetTempDefault)
{
    SetTemp<u32> set;
    ASSERT(set.arena == nullptr);
    ASSERT(set.capacity == 0);
    ASSERT(set.count == 0);
}

TEST(testSetTempCreate)
{
    ArenaScope arena = getScratch();
    SetTemp<u32> set{arena, 16};
    ASSERT(set.arena == arena);
    ASSERT(set.capacity == 16);
    ASSERT(set.count == 0);
}

TEST(testSetTempAddHasRemove)
{
    ArenaScope arena = getScratch();
    SetTemp<u32> set{arena, 0};
    set.add(10);
    set.add(20);
    ASSERT(set.count == 2);
    ASSERT(set.has(10));
    ASSERT(set.has(20));
    set.remove(10);
    ASSERT(!set.has(10));
    ASSERT(set.has(20));
}

TEST(testSetTempForEach)
{
    ArenaScope arena = getScratch();
    SetTemp<u32> set{arena, 0};
    set.add(5);
    set.add(10);
    u64 sum = 0;
    set.forEach([&](const u32& v) { sum += v; });
    ASSERT(sum == 15);
}

TEST(testSetStringHeap)
{
    Set<String> set{16};
    set.add(String::create("apple"));
    set.add(String::create("banana"));
    set.add(String::create("cherry"));
    ASSERT(set.count == 3);

    ASSERT(set.has(String::create("banana")));
    ASSERT(set.has(StringView{"apple"}));

    const char* b = "banana";
    ASSERT(set.has(b));

    ArenaScope arena = getScratch();
    ASSERT(set.has(StringBuilder{arena, "apple"}));
    ASSERT(set.has("cherry"));
    ASSERT(!set.has("durian"));

    set.remove(StringView{"banana"});
    ASSERT(!set.has("banana"));
    ASSERT(set.count == 2);

    set.remove("apple");
    ASSERT(!set.has(StringView{"apple"}));
    ASSERT(set.count == 1);
    ASSERT(set.has("cherry"));
}

TEST(testSetStringTemp)
{
    ArenaScope arena = getScratch();
    SetTemp<String> set{arena, 16};
    set.add(String::create("apple"));
    set.add(String::create("banana"));
    set.add(String::create("cherry"));
    ASSERT(set.count == 3);

    ASSERT(set.has(StringView{"apple"}));
    ASSERT(set.has("banana"));
    ASSERT(set.has(StringBuilder{arena, "cherry"}));
    ASSERT(!set.has("durian"));

    set.remove(StringView{"banana"});
    ASSERT(!set.has("banana"));
    ASSERT(set.count == 2);

    set.remove("apple");
    ASSERT(!set.has(StringView{"apple"}));
    ASSERT(set.count == 1);
    ASSERT(set.has("cherry"));
}

TEST(testSetStringViewHeap)
{
    Set<StringView> set{16};
    set.add("apple");
    set.add("banana");
    set.add("cherry");
    ASSERT(set.count == 3);

    ASSERT(set.has(StringView{"apple"}));
    const char* b = "banana";
    ASSERT(set.has(b));

    ArenaScope arena = getScratch();
    ASSERT(set.has(StringBuilder{arena, "apple"}));
    ASSERT(set.has("cherry"));
    ASSERT(!set.has("durian"));

    set.remove(StringView{"banana"});
    ASSERT(!set.has("banana"));
    ASSERT(set.count == 2);

    set.remove("apple");
    ASSERT(!set.has(StringView{"apple"}));
    ASSERT(set.count == 1);
    ASSERT(set.has("cherry"));
}

TEST(testSetStringViewTemp)
{
    ArenaScope arena = getScratch();
    SetTemp<StringView> set{arena, 16};
    set.add("apple");
    set.add("banana");
    set.add("cherry");
    ASSERT(set.count == 3);

    ASSERT(set.has(StringView{"apple"}));
    ASSERT(set.has("banana"));
    ASSERT(set.has(StringBuilder{arena, "cherry"}));
    ASSERT(!set.has("durian"));

    set.remove(StringView{"banana"});
    ASSERT(!set.has("banana"));
    ASSERT(set.count == 2);

    set.remove("apple");
    ASSERT(!set.has(StringView{"apple"}));
    ASSERT(set.count == 1);
    ASSERT(set.has("cherry"));
}

TEST(testSetStringCollision)
{
    Set<String> set{4};
    const char* vals[] = {
        "red", "green", "blue", "cyan", "magenta",
        "yellow", "black", "white", "orange", "pink"
    };
    for (u32 i = 0; i < 10; ++i)
        set.add(String::create(vals[i]));
    ASSERT(set.count == 10);
    for (u32 i = 0; i < 10; ++i)
    {
        ASSERT(set.has(vals[i]));
        ASSERT(set.has(StringView{vals[i]}));
        ASSERT(set.has(String::create(vals[i])));
    }
    ASSERT(!set.has("purple"));
}

TEST(testSetForEachModify)
{
    Set<u32> set;
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

TEST(testSetResizeSmaller)
{
    Set<u32> set{16};
    set.add(1);
    set.add(2);
    set.resize(4);
    ASSERT(set.capacity == 4);
    ASSERT(set.count == 2);
    ASSERT(set.has(1));
    ASSERT(set.has(2));
}

TEST(testSetResizeSame)
{
    Set<u32> set{8};
    set.add(1);
    set.add(2);
    set.resize(8);
    ASSERT(set.capacity == 8);
    ASSERT(set.count == 2);
    ASSERT(set.has(1));
    ASSERT(set.has(2));
}

TEST(testSetResetEmpty)
{
    Set<u32> set{8};
    set.reset();
    ASSERT(set.count == 0);
    ASSERT(!set.has(0));
}

TEST(testSetAddManyThenRemoveAll)
{
    Set<u32> set;
    for (u32 i = 0; i < 100; ++i)
        set.add(i);
    ASSERT(set.count == 100);
    for (u32 i = 0; i < 100; ++i)
        set.remove(i);
    ASSERT(set.count == 0);
    for (u32 i = 0; i < 100; ++i)
        ASSERT(!set.has(i));
}

TEST(testSetTempDuplicateAdd)
{
    ArenaScope arena = getScratch();
    SetTemp<u32> set{arena, 0};
    set.add(42);
    set.add(42);
    ASSERT(set.count == 1);
    ASSERT(set.has(42));
}

TEST(testSetTempRemoveNonExistent)
{
    ArenaScope arena = getScratch();
    SetTemp<u32> set{arena, 0};
    set.add(5);
    set.remove(99);
    ASSERT(set.count == 1);
    ASSERT(set.has(5));
    ASSERT(!set.has(99));
}

TEST(testSetTempReset)
{
    ArenaScope arena = getScratch();
    SetTemp<u32> set{arena, 0};
    set.add(1);
    set.add(2);
    set.add(3);
    set.reset();
    ASSERT(set.count == 0);
    ASSERT(!set.has(1));
    ASSERT(!set.has(2));
    ASSERT(!set.has(3));
}

TEST(testSetStringViewComparison)
{
    Set<StringView> set{16};
    set.add("alpha");
    set.add("beta");
    set.add("gamma");

    ASSERT(set.has(StringView{"alpha"}));
    ASSERT(set.has(StringView{"beta"}));
    ASSERT(set.has(StringView{"gamma"}));
    ASSERT(!set.has(StringView{"delta"}));

    set.remove(StringView{"beta"});
    ASSERT(!set.has("beta"));
    ASSERT(set.count == 2);

    ArenaScope arena = getScratch();
    ASSERT(set.has(StringBuilder{arena, "alpha"}));
    ASSERT(!set.has(StringBuilder{arena, "beta"}));

    set.add("beta");
    ASSERT(set.has(StringView{"beta"}));
    ASSERT(set.count == 3);
}

TEST(testSetStringMoveOnly)
{
    Set<String> set{16};
    set.add(String::create("one"));
    set.add(String::create("two"));
    set.add(String::create("three"));
    ASSERT(set.count == 3);

    ASSERT(set.has("one"));
    ASSERT(set.has("two"));
    ASSERT(set.has("three"));

    set.remove("two");
    ASSERT(!set.has("two"));
    ASSERT(set.count == 2);

    set.add(String::create("four"));
    ASSERT(set.has("four"));
    ASSERT(set.count == 3);

    set.reset();
    ASSERT(set.count == 0);
    ASSERT(!set.has("one"));
    ASSERT(!set.has("three"));
    ASSERT(!set.has("four"));
}

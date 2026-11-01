#include "tests.hpp"
#include "hg/array.hpp"

using namespace hg;

TEST(testArrayDefault)
{
    Array<u32> arr;
    ASSERT(arr.vals == nullptr);
    ASSERT(arr.count == 0);
    ASSERT(arr.capacity == 0);
}

TEST(testArrayInitialCount)
{
    Array<u32> arr{3, 8};
    ASSERT(arr.count == 3);
    ASSERT(arr.capacity == 8);
    ASSERT(arr[0] == 0);
    ASSERT(arr[1] == 0);
    ASSERT(arr[2] == 0);
}

TEST(testArrayPush)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    u32& ref = arr.push(30);
    ref = 99;
    ASSERT(arr.count == 3);
    ASSERT(arr.capacity >= 3);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 20);
    ASSERT(arr[2] == 99);
}

TEST(testArrayPop)
{
    Array<u32> arr;
    arr.push(1);
    arr.push(2);
    arr.push(3);
    u32 val = arr.pop();
    ASSERT(val == 3);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 1);
    ASSERT(arr[1] == 2);
}

TEST(testArrayLifecycle)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        Lifecycle lc;
        arr.push();
        arr.push(lc);
        ASSERT(arr[1].valid);
        ASSERT(&arr[1] != &arr[0]);
        arr.push(std::move(lc));
        ASSERT(!lc.valid);
        ASSERT(arr[2].valid);
        ASSERT(arr.count == 3);
        Lifecycle popped = arr.pop();
        ASSERT(popped.valid);
        ASSERT(arr.count == 2);
        ASSERT(Lifecycle::stats.moves >= 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 1);
    ASSERT(Lifecycle::stats.dtors == 3);
}

TEST(testArrayReserve)
{
    Array<u32> arr;
    arr.reserve(100);
    ASSERT(arr.capacity >= 100);
    ASSERT(arr.count == 0);
    arr.reserve(50);
    ASSERT(arr.capacity >= 100);
    arr.push(42);
    ASSERT(arr.count == 1);
    ASSERT(arr[0] == 42);
}

TEST(testArrayResizeGrow)
{
    Array<u32> arr;
    arr.push(1);
    arr.push(2);
    arr.resize(5);
    ASSERT(arr.count == 5);
    ASSERT(arr[0] == 1);
    ASSERT(arr[1] == 2);
    ASSERT(arr[4] == 0);
    u32* oldVals = arr.vals;
    arr.resize(5);
    ASSERT(arr.vals == oldVals);
    ASSERT(arr.count == 5);
}

TEST(testArrayResizeShrinkToZero)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push();
        arr.push();
        arr.push();
        ASSERT(Lifecycle::stats.alive == 3);
        arr.resize(1);
        ASSERT(arr.count == 1);
        ASSERT(arr[0].valid);
        ASSERT(Lifecycle::stats.alive == 1);
        ASSERT(Lifecycle::stats.dtors == 2);
        arr.resize(0);
        ASSERT(arr.count == 0);
        ASSERT(Lifecycle::stats.alive == 0);
        ASSERT(Lifecycle::stats.dtors == 3);
    }
    ASSERT(Lifecycle::stats.dtors == 3);
}

TEST(testArrayReset)
{
    Lifecycle::stats.reset();
    Array<Lifecycle> arr;
    arr.push();
    arr.push();
    arr.push();
    ASSERT(Lifecycle::stats.alive == 3);
    arr.reset();
    ASSERT(arr.count == 0);
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 3);
    ASSERT(arr.capacity >= 3);
    arr.push();
    ASSERT(arr.count == 1);
    ASSERT(Lifecycle::stats.ctors >= 4);
}

TEST(testArrayInsertShift)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    u32& r = arr.insertShift(1, 99);
    ASSERT(arr.count == 4);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 99);
    ASSERT(arr[2] == 20);
    ASSERT(arr[3] == 30);
    ASSERT(r == 30);
    r = 55;
    ASSERT(arr[3] == 55);
    arr.insertShift(4, 77);
    ASSERT(arr.count == 5);
    ASSERT(arr[4] == 77);
    ASSERT(arr[0] == 10);
    ASSERT(arr[3] == 55);
}

TEST(testArrayRemoveShift)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    ASSERT(arr.removeShift(1) == 20);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 30);
    ASSERT(arr.removeShift(0) == 10);
    ASSERT(arr.removeShift(0) == 30);
    ASSERT(arr.count == 0);

    Array<u32> arr2;
    arr2.push(10);
    arr2.push(20);
    arr2.push(30);
    ASSERT(arr2.removeShift(2) == 30);
    ASSERT(arr2.count == 2);
    ASSERT(arr2[0] == 10);
    ASSERT(arr2[1] == 20);
}

TEST(testArrayInsertShiftLifecycle)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push(); arr.push(); arr.push();
        Lifecycle val;
        arr.insertShift(0, val);
        ASSERT(val.valid);
        ASSERT(arr[0].valid);
        ASSERT(arr.count == 4);
        ASSERT(Lifecycle::stats.copies == 1);
        ASSERT(Lifecycle::stats.moves == 3);
    }
    ASSERT(Lifecycle::stats.alive == 0);

    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push(); arr.push();
        Lifecycle val;
        arr.insertShift(0, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr.count == 3);
        ASSERT(Lifecycle::stats.moves == 3);
        ASSERT(Lifecycle::stats.copies == 0);
    }
    ASSERT(Lifecycle::stats.alive == 0);

    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push(); arr.push();
        Lifecycle val;
        arr.insertShift(2, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr.count == 3);
        ASSERT(Lifecycle::stats.moves == 1);
        ASSERT(Lifecycle::stats.copies == 0);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testArrayRemoveShiftLifecycle)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push(); arr.push(); arr.push();
        Lifecycle removed = arr.removeShift(0);
        ASSERT(removed.valid);
        ASSERT(arr.count == 2);
        ASSERT(Lifecycle::stats.moves == 3);
        ASSERT(Lifecycle::stats.copies == 0);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testArrayInsertSwap)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    u32& r = arr.insertSwap(1, 99);
    ASSERT(arr.count == 4);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 99);
    ASSERT(arr[3] == 20);
    ASSERT(r == 20);
    r = 55;
    ASSERT(arr[3] == 55);

    Array<u32> arr2;
    arr2.push(10);
    arr2.push(20);
    arr2.insertSwap(2, 30);
    ASSERT(arr2.count == 3);
    ASSERT(arr2[0] == 10);
    ASSERT(arr2[1] == 20);
    ASSERT(arr2[2] == 30);
}

TEST(testArrayInsertSwapLifecycle)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push(); arr.push(); arr.push();
        Lifecycle val;
        arr.insertSwap(0, val);
        ASSERT(val.valid);
        ASSERT(arr.count == 4);
        ASSERT(Lifecycle::stats.copies == 1);
        ASSERT(Lifecycle::stats.moves == 1);
    }
    ASSERT(Lifecycle::stats.alive == 0);

    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push(); arr.push();
        Lifecycle val;
        arr.insertSwap(1, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr.count == 3);
        ASSERT(Lifecycle::stats.moves == 2);
        ASSERT(Lifecycle::stats.copies == 0);
    }
    ASSERT(Lifecycle::stats.alive == 0);

    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push(); arr.push();
        Lifecycle val;
        arr.insertSwap(2, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr.count == 3);
        ASSERT(Lifecycle::stats.moves == 1);
        ASSERT(Lifecycle::stats.copies == 0);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testArrayRemoveSwap)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    ASSERT(arr.removeSwap(0) == 10);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 30);
    ASSERT(arr[1] == 20);

    Array<u32> arr2;
    arr2.push(10);
    arr2.push(20);
    arr2.push(30);
    ASSERT(arr2.removeSwap(2) == 30);
    ASSERT(arr2.count == 2);
    ASSERT(arr2[0] == 10);
    ASSERT(arr2[1] == 20);

    Array<u32> arr3;
    arr3.push(99);
    ASSERT(arr3.removeSwap(0) == 99);
    ASSERT(arr3.count == 0);
}

TEST(testArrayRemoveSwapLifecycle)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push(); arr.push(); arr.push();
        Lifecycle removed = arr.removeSwap(0);
        ASSERT(removed.valid);
        ASSERT(arr.count == 2);
        ASSERT(Lifecycle::stats.moves == 2);
        ASSERT(Lifecycle::stats.copies == 0);
    }
    ASSERT(Lifecycle::stats.alive == 0);
}

TEST(testArrayRangeFor)
{
    Array<u32> arr;
    arr.push(1);
    arr.push(2);
    arr.push(3);
    u64 sum = 0;
    for (u32 v : arr)
        sum += v;
    ASSERT(sum == 6);
}

TEST(testArrayRangeForConst)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    const Array<u32>& carr = arr;
    u64 sum = 0;
    for (const u32& v : carr)
        sum += v;
    ASSERT(sum == 30);
    ASSERT(carr.begin() == arr.vals);
    ASSERT(*carr.begin() == 10);
}

TEST(testArrayConstIndex)
{
    Array<u32> arr;
    arr.push(42);
    const Array<u32>& carr = arr;
    ASSERT(carr[0] == 42);
}

TEST(testArraySpanConversion)
{
    Array<u32> arr;
    arr.push(42);
    Span<u32> s = arr;
    ASSERT(s.data == arr.vals);
    ASSERT(s.count == arr.count);
    s[0] = 99;
    ASSERT(arr[0] == 99);
}

TEST(testArraySpanConversionConst)
{
    Array<u32> arr;
    arr.push(99);
    const Array<u32>& carr = arr;
    Span<const u32> cs = carr;
    ASSERT(cs.data == arr.vals);
    ASSERT(cs.count == arr.count);
    ASSERT(cs[0] == 99);
}

TEST(testArrayMoveConstruct)
{
    Array<u32> a;
    a.push(1);
    a.push(2);
    u32* oldVals = a.vals;
    Array<u32> b = std::move(a);
    ASSERT(a.vals == nullptr);
    ASSERT(a.count == 0);
    ASSERT(a.capacity == 0);
    ASSERT(b.vals == oldVals);
    ASSERT(b.count == 2);
    ASSERT(b[0] == 1);
    ASSERT(b[1] == 2);
}

TEST(testArrayMoveAssign)
{
    Array<u32> a;
    a.push(10);
    a.push(20);
    Array<u32> b;
    b.push(30);
    b = std::move(a);
    ASSERT(b[0] == 10);
    ASSERT(b[1] == 20);
    ASSERT(a.vals == nullptr);
}

TEST(testArrayMoveAssignSelf)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    Array<u32>& ref = arr;
    ref = std::move(arr);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 20);
}

TEST(testArrayPushManyThenPopAll)
{
    Array<u32> arr;
    for (u32 i = 0; i < 100; ++i)
        arr.push(i);
    ASSERT(arr.count == 100);
    for (u32 i = 0; i < 100; ++i)
    {
        u32 v = arr.pop();
        ASSERT(v == 99 - i);
    }
    ASSERT(arr.count == 0);
}

TEST(testArrayTempCreate)
{
    ArrayTemp<u32> def;
    ASSERT(def.arena == nullptr);
    ASSERT(def.vals == nullptr);
    ASSERT(def.count == 0);
    ASSERT(def.capacity == 0);

    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 3, 8};
    ASSERT(arr.arena == arena);
    ASSERT(arr.vals != nullptr);
    ASSERT(arr.count == 3);
    ASSERT(arr.capacity == 8);
    ASSERT(arr[0] == 0);
    ASSERT(arr[2] == 0);
}

TEST(testArrayTempPush)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(1);
    arr.push(2);
    arr.push(3);
    ASSERT(arr.count == 3);
    ASSERT(arr[0] == 1);
    ASSERT(arr[1] == 2);
    ASSERT(arr[2] == 3);
}

TEST(testArrayTempReserve)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.reserve(100);
    ASSERT(arr.capacity >= 100);
    ASSERT(arr.count == 0);
    arr.push(42);
    ASSERT(arr[0] == 42);
}

TEST(testArrayTempMoveConstruct)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> a{arena, 0, 8};
    a.push(7);
    u32* oldVals = a.vals;
    ArrayTemp<u32> b = std::move(a);
    ASSERT(a.arena == nullptr);
    ASSERT(a.vals == nullptr);
    ASSERT(b.vals == oldVals);
    ASSERT(b[0] == 7);
}

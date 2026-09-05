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
    arr.push(30);
    ASSERT(arr.count == 3);
    ASSERT(arr.capacity >= 3);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 20);
    ASSERT(arr[2] == 30);
}

TEST(testArrayPushReturnRef)
{
    Array<u32> arr;
    u32& ref = arr.push(42);
    ASSERT(ref == 42);
    ref = 99;
    ASSERT(arr[0] == 99);
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

TEST(testArrayPushDefaultLifecycle)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        Lifecycle& p1 = arr.push();
        ASSERT(arr.count == 1);
        ASSERT(p1.valid);
        Lifecycle& p2 = arr.push();
        ASSERT(arr.count == 2);
        ASSERT(p2.valid);
        ASSERT(&p2 != &p1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 2);
    ASSERT(Lifecycle::stats.copies == 0);
    ASSERT(Lifecycle::stats.moves == 0);
}

TEST(testArrayPushRvalueLifecycle)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        Lifecycle lc;
        arr.push(std::move(lc));
        ASSERT(arr.count == 1);
        ASSERT(!lc.valid);
        ASSERT(arr[0].valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 1);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testArrayPushConstRefLifecycle)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        Lifecycle lc;
        arr.push(lc);
        ASSERT(arr.count == 1);
        ASSERT(lc.valid);
        ASSERT(arr[0].valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 1);
    ASSERT(Lifecycle::stats.moves == 0);
}

TEST(testArrayPopMoveLifecycle)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push();
        Lifecycle val = arr.pop();
        ASSERT(val.valid);
        ASSERT(Lifecycle::stats.alive == 1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 1);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testArrayReserve)
{
    Array<u32> arr;
    arr.reserve(100);
    ASSERT(arr.capacity >= 100);
    ASSERT(arr.count == 0);
}

TEST(testArrayReserveSmaller)
{
    Array<u32> arr;
    arr.reserve(100);
    u64 cap = arr.capacity;
    arr.reserve(50);
    ASSERT(arr.capacity == cap);
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
    ASSERT(arr[2] == 0);
    ASSERT(arr[3] == 0);
    ASSERT(arr[4] == 0);
}

TEST(testArrayResizeShrink)
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

TEST(testArrayInsertShiftConstRef)
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
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 1);
    ASSERT(Lifecycle::stats.moves == 3);
}

TEST(testArrayInsertShiftAtEnd)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push();
        Lifecycle val;
        arr.insertShift(1, val);
        ASSERT(arr.count == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 1);
    ASSERT(Lifecycle::stats.moves == 0);
}

TEST(testArrayInsertShiftAtEndRvalue)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push();
        Lifecycle val;
        arr.insertShift(1, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr.count == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 1);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testArrayInsertShiftRvalue)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push();
        Lifecycle val;
        arr.insertShift(0, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr.count == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 2);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testArrayInsertShiftMiddleRvalue)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push(); arr.push(); arr.push();
        Lifecycle val;
        arr.insertShift(1, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr[0].valid);
        ASSERT(arr[1].valid);
        ASSERT(arr.count == 4);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 3);
    ASSERT(Lifecycle::stats.copies == 0);
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
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 0);
    ASSERT(Lifecycle::stats.moves == 3);
}

TEST(testArrayRemoveShiftSingle)
{
    Array<u32> arr;
    arr.push(99);
    u32 v = arr.removeShift(0);
    ASSERT(v == 99);
    ASSERT(arr.count == 0);
}

TEST(testArrayInsertSwapConstRef)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push(); arr.push(); arr.push();
        Lifecycle val;
        arr.insertSwap(0, val);
        ASSERT(arr.count == 4);
        ASSERT(val.valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 1);
    ASSERT(Lifecycle::stats.moves == 1);
}

TEST(testArrayInsertSwapRvalue)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push(); arr.push(); arr.push();
        Lifecycle val;
        arr.insertSwap(0, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr.count == 4);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 2);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testArrayInsertSwapAtEnd)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push(); arr.push();
        Lifecycle val;
        arr.insertSwap(2, val);
        ASSERT(arr.count == 3);
        ASSERT(val.valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 1);
    ASSERT(Lifecycle::stats.moves == 0);
}

TEST(testArrayInsertSwapAtEndRvalue)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push();
        Lifecycle val;
        arr.insertSwap(1, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr.count == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 1);
    ASSERT(Lifecycle::stats.copies == 0);
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
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 0);
    ASSERT(Lifecycle::stats.moves == 2);
}

TEST(testArrayRemoveSwapMiddle)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    u32 removed = arr.removeSwap(1);
    ASSERT(removed == 20);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 30);
}

TEST(testArrayRemoveSwapEnd)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    u32 removed = arr.removeSwap(2);
    ASSERT(removed == 30);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 20);
}

TEST(testArrayRemoveSwapSingle)
{
    Array<u32> arr;
    arr.push(99);
    u32 v = arr.removeSwap(0);
    ASSERT(v == 99);
    ASSERT(arr.count == 0);
}

TEST(testArrayInsertShiftBeginning)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    arr.insertShift(0, 5);
    ASSERT(arr.count == 4);
    ASSERT(arr[0] == 5);
    ASSERT(arr[1] == 10);
    ASSERT(arr[2] == 20);
    ASSERT(arr[3] == 30);
}

TEST(testArrayInsertShiftMiddle)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    arr.insertShift(1, 15);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 15);
    ASSERT(arr[2] == 20);
    ASSERT(arr[3] == 30);
}

TEST(testArrayInsertShiftEnd)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.insertShift(2, 30);
    ASSERT(arr.count == 3);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 20);
    ASSERT(arr[2] == 30);
}

TEST(testArrayRemoveShiftBeginning)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    u32 removed = arr.removeShift(0);
    ASSERT(removed == 10);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 20);
    ASSERT(arr[1] == 30);
}

TEST(testArrayRemoveShiftMiddle)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    u32 removed = arr.removeShift(1);
    ASSERT(removed == 20);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 30);
}

TEST(testArrayRemoveShiftEnd)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    u32 removed = arr.removeShift(2);
    ASSERT(removed == 30);
    ASSERT(arr.count == 2);
}

TEST(testArrayInsertSwapBeginning)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    arr.insertSwap(0, 5);
    ASSERT(arr.count == 4);
    ASSERT(arr[0] == 5);
    ASSERT(arr[3] == 10);
}

TEST(testArrayInsertSwapMiddle)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    arr.insertSwap(1, 99);
    ASSERT(arr.count == 4);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 99);
    ASSERT(arr[3] == 20);
}

TEST(testArrayRemoveSwapSwapWithLast)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    u32 removed = arr.removeSwap(0);
    ASSERT(removed == 10);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 30);
    ASSERT(arr[1] == 20);
}

TEST(testArrayInsertShiftReturnValue)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    u32& r = arr.insertShift(1, 99);
    ASSERT(r == 20);
    r = 77;
    ASSERT(arr[2] == 77);
}

TEST(testArrayInsertSwapReturnValue)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    u32& r = arr.insertSwap(0, 99);
    ASSERT(r == 10);
    r = 55;
    ASSERT(arr[2] == 55);
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
    arr.push(1);
    arr.push(2);
    arr.push(3);
    const Array<u32>& carr = arr;
    u64 sum = 0;
    for (u32 v : carr)
        sum += v;
    ASSERT(sum == 6);
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
    u64 oldCount = a.count;
    u64 oldCap = a.capacity;
    u32* oldVals = a.vals;
    Array<u32> b = std::move(a);
    ASSERT(a.vals == nullptr);
    ASSERT(a.count == 0);
    ASSERT(a.capacity == 0);
    ASSERT(b.vals == oldVals);
    ASSERT(b.count == oldCount);
    ASSERT(b.capacity == oldCap);
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

TEST(testArrayTempDefault)
{
    ArrayTemp<u32> arr;
    ASSERT(arr.arena == nullptr);
    ASSERT(arr.vals == nullptr);
    ASSERT(arr.count == 0);
    ASSERT(arr.capacity == 0);
}

TEST(testArrayTempCreate)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 16};
    ASSERT(arr.arena == arena);
    ASSERT(arr.vals != nullptr);
    ASSERT(arr.count == 0);
    ASSERT(arr.capacity == 16);
}

TEST(testArrayTempInitialCount)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 3, 8};
    ASSERT(arr.count == 3);
    ASSERT(arr.capacity == 8);
    ASSERT(arr[0] == 0);
    ASSERT(arr[1] == 0);
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

TEST(testArrayTempPushReturnRef)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    u32& ref = arr.push(42);
    ASSERT(ref == 42);
    ref = 99;
    ASSERT(arr[0] == 99);
}

TEST(testArrayTempPop)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(10);
    arr.push(20);
    u32 v = arr.pop();
    ASSERT(v == 20);
    ASSERT(arr.count == 1);
}

TEST(testArrayTempPushDefaultLifecycle)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        Lifecycle& p1 = arr.push();
        ASSERT(arr.count == 1);
        ASSERT(p1.valid);
        Lifecycle& p2 = arr.push();
        ASSERT(arr.count == 2);
        ASSERT(p2.valid);
        ASSERT(&p2 != &p1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.dtors == 2);
    ASSERT(Lifecycle::stats.copies == 0);
    ASSERT(Lifecycle::stats.moves == 0);
}

TEST(testArrayTempPushRvalueLifecycle)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        Lifecycle lc;
        arr.push(std::move(lc));
        ASSERT(arr.count == 1);
        ASSERT(!lc.valid);
        ASSERT(arr[0].valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 1);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testArrayTempPushConstRefLifecycle)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        Lifecycle lc;
        arr.push(lc);
        ASSERT(arr.count == 1);
        ASSERT(lc.valid);
        ASSERT(arr[0].valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 1);
    ASSERT(Lifecycle::stats.moves == 0);
}

TEST(testArrayTempPopMoveLifecycle)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push();
        Lifecycle val = arr.pop();
        ASSERT(val.valid);
        ASSERT(Lifecycle::stats.alive == 1);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 1);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testArrayTempReserve)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.reserve(100);
    ASSERT(arr.capacity >= 100);
    ASSERT(arr.count == 0);
}

TEST(testArrayTempReserveSmaller)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.reserve(100);
    u64 cap = arr.capacity;
    arr.reserve(50);
    ASSERT(arr.capacity == cap);
}

TEST(testArrayTempResizeGrow)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(5);
    arr.resize(4);
    ASSERT(arr.count == 4);
    ASSERT(arr[0] == 5);
    ASSERT(arr[1] == 0);
}

TEST(testArrayTempResizeShrink)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push();
        arr.push();
        arr.push();
        ASSERT(Lifecycle::stats.alive == 3);
        arr.resize(1);
        ASSERT(arr.count == 1);
        ASSERT(arr[0].valid);
        ASSERT(Lifecycle::stats.alive == 1);
        ASSERT(Lifecycle::stats.dtors == 2);
    }
    ASSERT(Lifecycle::stats.dtors == 3);
}

TEST(testArrayTempReset)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push();
        arr.push();
        ASSERT(Lifecycle::stats.alive == 2);
        arr.reset();
        ASSERT(arr.count == 0);
        ASSERT(Lifecycle::stats.alive == 0);
    }
    ASSERT(Lifecycle::stats.dtors == 2);
}

TEST(testArrayTempInsertShiftConstRef)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push(); arr.push(); arr.push();
        Lifecycle val;
        arr.insertShift(0, val);
        ASSERT(val.valid);
        ASSERT(arr[0].valid);
        ASSERT(arr.count == 4);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 1);
    ASSERT(Lifecycle::stats.moves == 3);
}

TEST(testArrayTempInsertShiftRvalue)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push();
        Lifecycle val;
        arr.insertShift(0, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr.count == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 2);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testArrayTempInsertShiftAtEnd)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(10);
    arr.push(20);
    arr.insertShift(2, 30);
    ASSERT(arr.count == 3);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 20);
    ASSERT(arr[2] == 30);
}

TEST(testArrayTempInsertShiftAtEndRvalue)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push();
        Lifecycle val;
        arr.insertShift(1, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr.count == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 1);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testArrayTempInsertShiftMiddleRvalue)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push(); arr.push(); arr.push();
        Lifecycle val;
        arr.insertShift(1, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr[0].valid);
        ASSERT(arr[1].valid);
        ASSERT(arr.count == 4);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 3);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testArrayTempRemoveShift)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push(); arr.push(); arr.push();
        Lifecycle removed = arr.removeShift(0);
        ASSERT(removed.valid);
        ASSERT(arr.count == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 0);
    ASSERT(Lifecycle::stats.moves == 3);
}

TEST(testArrayTempRemoveShiftSingle)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(99);
    u32 v = arr.removeShift(0);
    ASSERT(v == 99);
    ASSERT(arr.count == 0);
}

TEST(testArrayTempRemoveShiftSequence)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(10);
    arr.push(20);
    arr.push(30);
    ASSERT(arr.removeShift(0) == 10);
    ASSERT(arr[0] == 20);
    ASSERT(arr.removeShift(0) == 20);
    ASSERT(arr[0] == 30);
    ASSERT(arr.removeShift(0) == 30);
    ASSERT(arr.count == 0);
}

TEST(testArrayTempInsertSwapConstRef)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push(); arr.push(); arr.push();
        Lifecycle val;
        arr.insertSwap(0, val);
        ASSERT(arr.count == 4);
        ASSERT(val.valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 1);
    ASSERT(Lifecycle::stats.moves == 1);
}

TEST(testArrayTempInsertSwapRvalue)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push(); arr.push(); arr.push();
        Lifecycle val;
        arr.insertSwap(0, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr.count == 4);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 2);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testArrayTempInsertSwapAtEnd)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push(); arr.push();
        Lifecycle val;
        arr.insertSwap(2, val);
        ASSERT(arr.count == 3);
        ASSERT(val.valid);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 1);
    ASSERT(Lifecycle::stats.moves == 0);
}

TEST(testArrayTempInsertSwapAtEndRvalue)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push();
        Lifecycle val;
        arr.insertSwap(1, std::move(val));
        ASSERT(!val.valid);
        ASSERT(arr.count == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.moves == 1);
    ASSERT(Lifecycle::stats.copies == 0);
}

TEST(testArrayTempInsertSwapBeginning)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(10);
    arr.push(20);
    arr.push(30);
    arr.insertSwap(0, 5);
    ASSERT(arr.count == 4);
    ASSERT(arr[0] == 5);
    ASSERT(arr[3] == 10);
}

TEST(testArrayTempInsertSwapMiddle)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(10);
    arr.push(20);
    arr.push(30);
    arr.insertSwap(1, 99);
    ASSERT(arr.count == 4);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 99);
    ASSERT(arr[3] == 20);
}

TEST(testArrayTempRemoveSwap)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push(); arr.push(); arr.push();
        Lifecycle removed = arr.removeSwap(0);
        ASSERT(removed.valid);
        ASSERT(arr.count == 2);
    }
    ASSERT(Lifecycle::stats.alive == 0);
    ASSERT(Lifecycle::stats.copies == 0);
    ASSERT(Lifecycle::stats.moves == 2);
}

TEST(testArrayTempRemoveSwapBeginning)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(10);
    arr.push(20);
    arr.push(30);
    u32 removed = arr.removeSwap(0);
    ASSERT(removed == 10);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 30);
    ASSERT(arr[1] == 20);
}

TEST(testArrayTempRemoveSwapMiddle)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(10);
    arr.push(20);
    arr.push(30);
    u32 removed = arr.removeSwap(1);
    ASSERT(removed == 20);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 30);
}

TEST(testArrayTempRemoveSwapEnd)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(10);
    arr.push(20);
    arr.push(30);
    u32 removed = arr.removeSwap(2);
    ASSERT(removed == 30);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 20);
}

TEST(testArrayTempRemoveSwapSingle)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(99);
    u32 v = arr.removeSwap(0);
    ASSERT(v == 99);
    ASSERT(arr.count == 0);
}

TEST(testArrayTempInsertShiftReturnValue)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(10);
    arr.push(20);
    u32& r = arr.insertShift(1, 99);
    ASSERT(r == 20);
    r = 77;
    ASSERT(arr[2] == 77);
}

TEST(testArrayTempInsertSwapReturnValue)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(10);
    arr.push(20);
    u32& r = arr.insertSwap(0, 99);
    ASSERT(r == 10);
    r = 55;
    ASSERT(arr[2] == 55);
}

TEST(testArrayTempRangeFor)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(1);
    arr.push(2);
    u64 sum = 0;
    for (u32 v : arr)
        sum += v;
    ASSERT(sum == 3);
}

TEST(testArrayTempRangeForConst)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(1);
    arr.push(2);
    const ArrayTemp<u32>& carr = arr;
    u64 sum = 0;
    for (u32 v : carr)
        sum += v;
    ASSERT(sum == 3);
}

TEST(testArrayTempConstIndex)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(42);
    const ArrayTemp<u32>& carr = arr;
    ASSERT(carr[0] == 42);
}

TEST(testArrayTempSpanConversion)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(42);
    Span<u32> s = arr;
    ASSERT(s.data == arr.vals);
    ASSERT(s.count == arr.count);
    s[0] = 99;
    ASSERT(arr[0] == 99);
}

TEST(testArrayTempSpanConversionConst)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(99);
    const ArrayTemp<u32>& carr = arr;
    Span<const u32> cs = carr;
    ASSERT(cs.data == arr.vals);
    ASSERT(cs.count == arr.count);
    ASSERT(cs[0] == 99);
}

TEST(testArrayTempMoveConstruct)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> a{arena, 0, 8};
    a.push(7);
    u32* oldVals = a.vals;
    ArrayTemp<u32> b = std::move(a);
    ASSERT(a.vals == nullptr);
    ASSERT(b.vals == oldVals);
    ASSERT(b[0] == 7);
}

TEST(testArrayTempMoveAssign)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> a{arena, 0, 8};
    a.push(10);
    a.push(20);
    ArrayTemp<u32> b{arena, 0, 8};
    b.push(30);
    b = std::move(a);
    ASSERT(b[0] == 10);
    ASSERT(b[1] == 20);
    ASSERT(a.vals == nullptr);
}

TEST(testArrayResizeToZero)
{
    Lifecycle::stats.reset();
    {
        Array<Lifecycle> arr;
        arr.push();
        arr.push();
        ASSERT(Lifecycle::stats.alive == 2);
        arr.resize(0);
        ASSERT(arr.count == 0);
        ASSERT(Lifecycle::stats.alive == 0);
        ASSERT(Lifecycle::stats.dtors == 2);
    }
    ASSERT(Lifecycle::stats.dtors == 2);
}

TEST(testArrayResizeSameSize)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    arr.push(30);
    u64 oldVals = (u64)arr.vals;
    arr.resize(3);
    ASSERT(arr.count == 3);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 20);
    ASSERT(arr[2] == 30);
    ASSERT((u64)arr.vals == oldVals);
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

TEST(testArrayConstRangeFor)
{
    Array<u32> arr;
    arr.push(1);
    arr.push(2);
    arr.push(3);
    const Array<u32>& carr = arr;
    u64 sum = 0;
    for (const u32& v : carr)
        sum += v;
    ASSERT(sum == 6);
}

TEST(testArrayConstBeginEnd)
{
    Array<u32> arr;
    arr.push(10);
    arr.push(20);
    const Array<u32>& carr = arr;
    ASSERT(carr.begin() == arr.vals);
    ASSERT(carr.end() == arr.vals + 2);
    ASSERT(*carr.begin() == 10);
    ASSERT(*(carr.end() - 1) == 20);
}

TEST(testArrayResetThenPush)
{
    Lifecycle::stats.reset();
    Array<Lifecycle> arr;
    arr.push();
    arr.push();
    arr.reset();
    ASSERT(arr.count == 0);
    arr.push();
    ASSERT(arr.count == 1);
    ASSERT(arr[0].valid);
}

TEST(testArrayTempResizeToZero)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push();
        arr.push();
        ASSERT(Lifecycle::stats.alive == 2);
        arr.resize(0);
        ASSERT(arr.count == 0);
        ASSERT(Lifecycle::stats.alive == 0);
        ASSERT(Lifecycle::stats.dtors == 2);
    }
    ASSERT(Lifecycle::stats.dtors == 2);
}

TEST(testArrayTempResizeSameSize)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(10);
    arr.push(20);
    arr.push(30);
    arr.resize(3);
    ASSERT(arr.count == 3);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 20);
    ASSERT(arr[2] == 30);
}

TEST(testArrayTempPushManyThenPopAll)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
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

TEST(testArrayTempConstRangeFor)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(1);
    arr.push(2);
    arr.push(3);
    const ArrayTemp<u32>& carr = arr;
    u64 sum = 0;
    for (const u32& v : carr)
        sum += v;
    ASSERT(sum == 6);
}

TEST(testArrayTempResetThenPush)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();
    {
        ArrayTemp<Lifecycle> arr{arena, 0, 0};
        arr.push();
        arr.push();
        arr.reset();
        ASSERT(arr.count == 0);
        arr.push();
        ASSERT(arr.count == 1);
        ASSERT(arr[0].valid);
    }
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

TEST(testArrayTempMoveAssignSelf)
{
    ArenaScope arena = getScratch();
    ArrayTemp<u32> arr{arena, 0, 0};
    arr.push(10);
    arr.push(20);
    ArrayTemp<u32>& ref = arr;
    ref = std::move(arr);
    ASSERT(arr.count == 2);
    ASSERT(arr[0] == 10);
    ASSERT(arr[1] == 20);
}

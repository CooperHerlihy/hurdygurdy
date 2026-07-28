#include "tests.hpp"

void testArray()
{
    // ============================================================================
    // Array
    // ============================================================================
    //
    // Array is a move-only, heap-allocated dynamic array (std::vector-like).
    // Supports push, pop, insertShift, removeShift, insertSwap, removeSwap,
    // resize, reserve, reset, range-for, and implicit conversion to Span.

    // Default-constructed array is empty
    {
        Array<u32> arr;
        TEST(arr.vals == nullptr);
        TEST(arr.count == 0);
        TEST(arr.capacity == 0);
    }

    // push appends elements, grows capacity as needed
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        arr.push(30);
        TEST(arr.count == 3);
        TEST(arr.capacity >= 3);
        TEST(arr[0] == 10);
        TEST(arr[1] == 20);
        TEST(arr[2] == 30);
    }

    // pop removes and returns the last element
    {
        Array<u32> arr;
        arr.push(1);
        arr.push(2);
        arr.push(3);
        u32 val = arr.pop();
        TEST(val == 3);
        TEST(arr.count == 2);
        TEST(arr[0] == 1);
        TEST(arr[1] == 2);
    }

    // push default-constructs in place (0 copies, 0 moves)
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            Lifecycle* p1 = arr.push();
            TEST(arr.count == 1);
            TEST(p1->valid);
            Lifecycle* p2 = arr.push();
            TEST(arr.count == 2);
            TEST(p2->valid);
            TEST(p2 != p1);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 2);
        TEST(Lifecycle::stats.copies == 0);
        TEST(Lifecycle::stats.moves == 0);
    }

    // push by rvalue reference (1 move, 0 copies)
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            Lifecycle lc;
            arr.push(std::move(lc));
            TEST(arr.count == 1);
            TEST(!lc.valid); // moved-from
            TEST(arr[0].valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 1);
        TEST(Lifecycle::stats.copies == 0);
    }

    // push by const reference (1 copy, 0 moves)
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            Lifecycle lc;
            arr.push(lc);
            TEST(arr.count == 1);
            TEST(lc.valid); // not moved-from
            TEST(arr[0].valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 1);
        TEST(Lifecycle::stats.moves == 0);
    }

    // pop returns by move (0 copies)
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push();
            Lifecycle val = arr.pop();
            TEST(val.valid);
            TEST(Lifecycle::stats.alive == 1); // val lives outside array
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 1);   // 1 move-construct in pop
        TEST(Lifecycle::stats.copies == 0);
    }

    // resize grows the array, default-constructing new elements
    {
        Array<u32> arr;
        arr.push(1);
        arr.push(2);
        arr.resize(5);
        TEST(arr.count == 5);
        TEST(arr[0] == 1);
        TEST(arr[1] == 2);
        TEST(arr[2] == 0);
        TEST(arr[3] == 0);
        TEST(arr[4] == 0);
    }

    // reserve increases capacity without changing count
    {
        Array<u32> arr;
        arr.reserve(100);
        TEST(arr.capacity >= 100);
        TEST(arr.count == 0);
    }

    // reset destroys all elements without freeing memory
    {
        Lifecycle::stats.reset();
        Array<Lifecycle> arr;
        arr.push();
        arr.push();
        arr.push();
        TEST(Lifecycle::stats.alive == 3);
        arr.reset();
        TEST(arr.count == 0);
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 3);
        TEST(arr.capacity >= 3);
        arr.push();
        TEST(arr.count == 1);
        TEST(Lifecycle::stats.ctors >= 4);
    }

    // insertShift with const ref (copies, no extra moves beyond shift)
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push(); arr.push(); arr.push();
            Lifecycle val;
            arr.insertShift(0, val);
            TEST(val.valid);
            TEST(arr[0].valid);
            TEST(arr.count == 4);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 1); // 1 copy of val into slot
        TEST(Lifecycle::stats.moves == 3);  // 3 elements shifted right
    }

    // insertShift at end appends without shifting
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push();
            Lifecycle val;
            arr.insertShift(1, val);
            TEST(arr.count == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 1); // 1 copy of val
        TEST(Lifecycle::stats.moves == 0);  // no shift needed
    }

    // insertShift with rvalue ref (no copies, just moves)
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push();
            Lifecycle val;
            arr.insertShift(0, std::move(val));
            TEST(!val.valid);
            TEST(arr.count == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 0);
        TEST(Lifecycle::stats.moves == 2);  // 1 shift + 1 move-construct
    }

    // removeShift moves elements (0 copies)
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push(); arr.push(); arr.push();
            Lifecycle removed = arr.removeShift(0);
            TEST(removed.valid);
            TEST(arr.count == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 0);
        TEST(Lifecycle::stats.moves == 3);  // 1 move-out + 2 shift
    }

    // insertSwap with const ref (copies)
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push(); arr.push(); arr.push();
            Lifecycle val;
            arr.insertSwap(0, val);
            TEST(arr.count == 4);
            TEST(val.valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 1); // 1 copy of val
        TEST(Lifecycle::stats.moves == 1);  // 1 displaced element moved
    }

    // removeSwap moves only
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push(); arr.push(); arr.push();
            Lifecycle removed = arr.removeSwap(0);
            TEST(removed.valid);
            TEST(arr.count == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 0);
        TEST(Lifecycle::stats.moves == 2);  // 1 move-out + 1 swap-move
    }

    // insertShift at beginning shifts all elements right
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        arr.push(30);
        arr.insertShift(0, 5);
        TEST(arr.count == 4);
        TEST(arr[0] == 5);
        TEST(arr[1] == 10);
        TEST(arr[2] == 20);
        TEST(arr[3] == 30);
    }

    // insertShift at middle
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        arr.push(30);
        arr.insertShift(1, 15);
        TEST(arr[0] == 10);
        TEST(arr[1] == 15);
        TEST(arr[2] == 20);
        TEST(arr[3] == 30);
    }

    // insertShift at end appends
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        arr.insertShift(2, 30);
        TEST(arr.count == 3);
        TEST(arr[0] == 10);
        TEST(arr[1] == 20);
        TEST(arr[2] == 30);
    }

    // removeShift at beginning shifts elements left
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        arr.push(30);
        u32 removed = arr.removeShift(0);
        TEST(removed == 10);
        TEST(arr.count == 2);
        TEST(arr[0] == 20);
        TEST(arr[1] == 30);
    }

    // removeShift at middle
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        arr.push(30);
        u32 removed = arr.removeShift(1);
        TEST(removed == 20);
        TEST(arr[0] == 10);
        TEST(arr[1] == 30);
    }

    // removeShift at end
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        arr.push(30);
        u32 removed = arr.removeShift(2);
        TEST(removed == 30);
        TEST(arr.count == 2);
    }

    // insertSwap at beginning moves old value to end
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        arr.push(30);
        arr.insertSwap(0, 5);
        TEST(arr.count == 4);
        TEST(arr[0] == 5);
        TEST(arr[3] == 10);
    }

    // removeSwap swaps with last element then pops
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        arr.push(30);
        u32 removed = arr.removeSwap(0);
        TEST(removed == 10);
        TEST(arr.count == 2);
        TEST(arr[0] == 30);
        TEST(arr[1] == 20);
    }

    // range-for iteration
    {
        Array<u32> arr;
        arr.push(1);
        arr.push(2);
        arr.push(3);
        u64 sum = 0;
        for (u32 v : arr)
            sum += v;
        TEST(sum == 6);
    }

    // Implicit conversion to Span
    {
        Array<u32> arr;
        arr.push(42);
        Span<u32> s = arr;
        TEST(s.data == arr.vals);
        TEST(s.count == arr.count);
        TEST(s[0] == 42);
    }

    // Implicit conversion to Span<const T>
    {
        Array<u32> arr;
        arr.push(99);
        Span<const u32> cs = arr;
        TEST(cs.data == arr.vals);
        TEST(cs.count == arr.count);
    }

    // Move construct
    {
        Array<u32> a;
        a.push(1);
        a.push(2);
        u64 oldCount = a.count;
        u64 oldCap = a.capacity;
        u32* oldVals = a.vals;
        Array<u32> b = std::move(a);
        TEST(a.vals == nullptr);
        TEST(a.count == 0);
        TEST(a.capacity == 0);
        TEST(b.vals == oldVals);
        TEST(b.count == oldCount);
        TEST(b.capacity == oldCap);
        TEST(b[0] == 1);
        TEST(b[1] == 2);
    }

    // Move assign
    {
        Array<u32> a;
        a.push(10);
        a.push(20);
        Array<u32> b;
        b.push(30);
        b = std::move(a);
        TEST(b[0] == 10);
        TEST(b[1] == 20);
        TEST(a.vals == nullptr);
    }

    // ============================================================================
    // ArrayTemp
    // ============================================================================
    //
    // ArrayTemp is an arena-allocated dynamic array. Memory is managed by the
    // arena rather than heap. Has the same API as Array.

    // Default-constructed ArrayTemp is empty
    {
        ArrayTemp<u32> arr;
        TEST(arr.arena == nullptr);
        TEST(arr.vals == nullptr);
        TEST(arr.count == 0);
        TEST(arr.capacity == 0);
    }

    // Create with arena and initial capacity
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 16};
        TEST(arr.arena == arena);
        TEST(arr.vals != nullptr);
        TEST(arr.count == 0);
        TEST(arr.capacity == 16);
    }

    // push appends elements
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(1);
        arr.push(2);
        arr.push(3);
        TEST(arr.count == 3);
        TEST(arr[0] == 1);
        TEST(arr[1] == 2);
        TEST(arr[2] == 3);
    }

    // pop removes and returns last element
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(10);
        arr.push(20);
        u32 v = arr.pop();
        TEST(v == 20);
        TEST(arr.count == 1);
    }

    // resize grows the array
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(5);
        arr.resize(4);
        TEST(arr.count == 4);
        TEST(arr[0] == 5);
        TEST(arr[1] == 0);
    }

    // reset destroys all elements
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push();
            arr.push();
            TEST(Lifecycle::stats.alive == 2);
            arr.reset();
            TEST(arr.count == 0);
            TEST(Lifecycle::stats.alive == 0);
        }
        TEST(Lifecycle::stats.dtors == 2);
    }

    // insertShift and removeShift
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(10);
        arr.push(20);
        arr.push(30);
        arr.insertShift(0, 5);
        TEST(arr[0] == 5);
        TEST(arr[1] == 10);
        u32 r = arr.removeShift(1);
        TEST(r == 10);
        TEST(arr[1] == 20);
        TEST(arr[2] == 30);
    }

    // insertSwap and removeSwap
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(10);
        arr.push(20);
        arr.insertSwap(0, 99);
        TEST(arr[0] == 99);
        TEST(arr[2] == 10);
        u32 r = arr.removeSwap(0);
        TEST(r == 99);
        TEST(arr[0] == 10);
        TEST(arr[1] == 20);
    }

    // range-for iteration
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(1);
        arr.push(2);
        u64 sum = 0;
        for (u32 v : arr)
            sum += v;
        TEST(sum == 3);
    }

    // Implicit conversion to Span
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(42);
        Span<u32> s = arr;
        TEST(s.data == arr.vals);
        TEST(s.count == arr.count);
    }

    // Move construct
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> a{arena, 0, 8};
        a.push(7);
        u32* oldVals = a.vals;
        ArrayTemp<u32> b = std::move(a);
        TEST(a.vals == nullptr);
        TEST(b.vals == oldVals);
        TEST(b[0] == 7);
    }
}


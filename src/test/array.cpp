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

    // Construct with initial count and capacity
    {
        Array<u32> arr{3, 8};
        TEST(arr.count == 3);
        TEST(arr.capacity == 8);
        TEST(arr[0] == 0);
        TEST(arr[1] == 0);
        TEST(arr[2] == 0);
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

    // push return value references the pushed element
    {
        Array<u32> arr;
        u32& ref = arr.push(42);
        TEST(ref == 42);
        ref = 99;
        TEST(arr[0] == 99);
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
            Lifecycle& p1 = arr.push();
            TEST(arr.count == 1);
            TEST(p1.valid);
            Lifecycle& p2 = arr.push();
            TEST(arr.count == 2);
            TEST(p2.valid);
            TEST(&p2 != &p1);
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

    // reserve increases capacity without changing count
    {
        Array<u32> arr;
        arr.reserve(100);
        TEST(arr.capacity >= 100);
        TEST(arr.count == 0);
    }

    // reserve smaller than capacity is a no-op
    {
        Array<u32> arr;
        arr.reserve(100);
        u64 cap = arr.capacity;
        arr.reserve(50);
        TEST(arr.capacity == cap);
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

    // resize shrinks the array, destroying excess elements
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push();
            arr.push();
            arr.push();
            TEST(Lifecycle::stats.alive == 3);
            arr.resize(1);
            TEST(arr.count == 1);
            TEST(arr[0].valid);
            TEST(Lifecycle::stats.alive == 1);
            TEST(Lifecycle::stats.dtors == 2);
        }
        TEST(Lifecycle::stats.dtors == 3);
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

    // insertShift at end with rvalue
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push();
            Lifecycle val;
            arr.insertShift(1, std::move(val));
            TEST(!val.valid);
            TEST(arr.count == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 1);  // 1 move-construct
        TEST(Lifecycle::stats.copies == 0);
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
        TEST(Lifecycle::stats.moves == 2);  // 1 shift + 1 move-construct
        TEST(Lifecycle::stats.copies == 0);
    }

    // insertShift at middle with rvalue
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push(); arr.push(); arr.push();
            Lifecycle val;
            arr.insertShift(1, std::move(val));
            TEST(!val.valid);
            TEST(arr[0].valid);
            TEST(arr[1].valid);
            TEST(arr.count == 4);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 3);  // 1 move-construct + 1 shift + 1 move-assign
        TEST(Lifecycle::stats.copies == 0);
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

    // removeShift single element (last remaining)
    {
        Array<u32> arr;
        arr.push(99);
        u32 v = arr.removeShift(0);
        TEST(v == 99);
        TEST(arr.count == 0);
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

    // insertSwap with rvalue ref
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push(); arr.push(); arr.push();
            Lifecycle val;
            arr.insertSwap(0, std::move(val));
            TEST(!val.valid);
            TEST(arr.count == 4);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 2);  // 1 displaced + 1 move-construct
        TEST(Lifecycle::stats.copies == 0);
    }

    // insertSwap at end appends without swapping
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push(); arr.push();
            Lifecycle val;
            arr.insertSwap(2, val);
            TEST(arr.count == 3);
            TEST(val.valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 1);
        TEST(Lifecycle::stats.moves == 0);  // no swap, just append
    }

    // insertSwap at end with rvalue
    {
        Lifecycle::stats.reset();
        {
            Array<Lifecycle> arr;
            arr.push();
            Lifecycle val;
            arr.insertSwap(1, std::move(val));
            TEST(!val.valid);
            TEST(arr.count == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 1);
        TEST(Lifecycle::stats.copies == 0);
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

    // removeSwap at middle
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        arr.push(30);
        u32 removed = arr.removeSwap(1);
        TEST(removed == 20);
        TEST(arr.count == 2);
        TEST(arr[0] == 10);
        TEST(arr[1] == 30);   // last element swapped into slot 1
    }

    // removeSwap at end (no swap, just pop)
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        arr.push(30);
        u32 removed = arr.removeSwap(2);
        TEST(removed == 30);
        TEST(arr.count == 2);
        TEST(arr[0] == 10);
        TEST(arr[1] == 20);
    }

    // removeSwap single element (last remaining)
    {
        Array<u32> arr;
        arr.push(99);
        u32 v = arr.removeSwap(0);
        TEST(v == 99);
        TEST(arr.count == 0);
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

    // insertSwap at middle
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        arr.push(30);
        arr.insertSwap(1, 99);
        TEST(arr.count == 4);
        TEST(arr[0] == 10);
        TEST(arr[1] == 99);
        TEST(arr[3] == 20);  // displaced to end
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

    // insertShift return value is reference to displaced element
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        u32& r = arr.insertShift(1, 99);
        TEST(r == 20); // displaced element (old last) moved to end
        r = 77;
        TEST(arr[2] == 77); // end slot was modified
    }

    // insertSwap return value is reference to displaced element
    {
        Array<u32> arr;
        arr.push(10);
        arr.push(20);
        u32& r = arr.insertSwap(0, 99);
        TEST(r == 10); // displaced element moved to end
        r = 55;
        TEST(arr[2] == 55); // end slot was modified
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

    // range-for over const ref
    {
        Array<u32> arr;
        arr.push(1);
        arr.push(2);
        arr.push(3);
        const Array<u32>& carr = arr;
        u64 sum = 0;
        for (u32 v : carr)
            sum += v;
        TEST(sum == 6);
    }

    // const operator[] works on const ref
    {
        Array<u32> arr;
        arr.push(42);
        const Array<u32>& carr = arr;
        TEST(carr[0] == 42);
    }

    // Implicit conversion to Span (non-const)
    {
        Array<u32> arr;
        arr.push(42);
        Span<u32> s = arr;
        TEST(s.data == arr.vals);
        TEST(s.count == arr.count);
        s[0] = 99;
        TEST(arr[0] == 99);
    }

    // Implicit conversion to Span<const T> from const ref
    {
        Array<u32> arr;
        arr.push(99);
        const Array<u32>& carr = arr;
        Span<const u32> cs = carr;
        TEST(cs.data == arr.vals);
        TEST(cs.count == arr.count);
        TEST(cs[0] == 99);
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

    // Create with arena, initial count and capacity
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 3, 8};
        TEST(arr.count == 3);
        TEST(arr.capacity == 8);
        TEST(arr[0] == 0);
        TEST(arr[1] == 0);
        TEST(arr[2] == 0);
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

    // push return value references the pushed element
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        u32& ref = arr.push(42);
        TEST(ref == 42);
        ref = 99;
        TEST(arr[0] == 99);
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

    // push default-constructs in place (0 copies, 0 moves)
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            Lifecycle& p1 = arr.push();
            TEST(arr.count == 1);
            TEST(p1.valid);
            Lifecycle& p2 = arr.push();
            TEST(arr.count == 2);
            TEST(p2.valid);
            TEST(&p2 != &p1);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 2);
        TEST(Lifecycle::stats.copies == 0);
        TEST(Lifecycle::stats.moves == 0);
    }

    // push by rvalue reference (1 move, 0 copies)
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            Lifecycle lc;
            arr.push(std::move(lc));
            TEST(arr.count == 1);
            TEST(!lc.valid);
            TEST(arr[0].valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 1);
        TEST(Lifecycle::stats.copies == 0);
    }

    // push by const reference (1 copy, 0 moves)
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            Lifecycle lc;
            arr.push(lc);
            TEST(arr.count == 1);
            TEST(lc.valid);
            TEST(arr[0].valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 1);
        TEST(Lifecycle::stats.moves == 0);
    }

    // pop returns by move (0 copies)
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push();
            Lifecycle val = arr.pop();
            TEST(val.valid);
            TEST(Lifecycle::stats.alive == 1);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 1);
        TEST(Lifecycle::stats.copies == 0);
    }

    // reserve increases capacity without changing count
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.reserve(100);
        TEST(arr.capacity >= 100);
        TEST(arr.count == 0);
    }

    // reserve smaller than capacity is a no-op
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.reserve(100);
        u64 cap = arr.capacity;
        arr.reserve(50);
        TEST(arr.capacity == cap);
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

    // resize shrinks the array
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push();
            arr.push();
            arr.push();
            TEST(Lifecycle::stats.alive == 3);
            arr.resize(1);
            TEST(arr.count == 1);
            TEST(arr[0].valid);
            TEST(Lifecycle::stats.alive == 1);
            TEST(Lifecycle::stats.dtors == 2);
        }
        TEST(Lifecycle::stats.dtors == 3);
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

    // insertShift with const ref
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push(); arr.push(); arr.push();
            Lifecycle val;
            arr.insertShift(0, val);
            TEST(val.valid);
            TEST(arr[0].valid);
            TEST(arr.count == 4);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 1);
        TEST(Lifecycle::stats.moves == 3);
    }

    // insertShift with rvalue ref
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push();
            Lifecycle val;
            arr.insertShift(0, std::move(val));
            TEST(!val.valid);
            TEST(arr.count == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 2);
        TEST(Lifecycle::stats.copies == 0);
    }

    // insertShift at end appends (const ref)
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(10);
        arr.push(20);
        arr.insertShift(2, 30);
        TEST(arr.count == 3);
        TEST(arr[0] == 10);
        TEST(arr[1] == 20);
        TEST(arr[2] == 30);
    }

    // insertShift at end with rvalue
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push();
            Lifecycle val;
            arr.insertShift(1, std::move(val));
            TEST(!val.valid);
            TEST(arr.count == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 1);
        TEST(Lifecycle::stats.copies == 0);
    }

    // insertShift at middle with rvalue
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push(); arr.push(); arr.push();
            Lifecycle val;
            arr.insertShift(1, std::move(val));
            TEST(!val.valid);
            TEST(arr[0].valid);
            TEST(arr[1].valid);
            TEST(arr.count == 4);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 3);
        TEST(Lifecycle::stats.copies == 0);
    }

    // removeShift moves elements
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push(); arr.push(); arr.push();
            Lifecycle removed = arr.removeShift(0);
            TEST(removed.valid);
            TEST(arr.count == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 0);
        TEST(Lifecycle::stats.moves == 3);
    }

    // removeShift single element
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(99);
        u32 v = arr.removeShift(0);
        TEST(v == 99);
        TEST(arr.count == 0);
    }

    // removeShift at beginning, middle, end
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(10);
        arr.push(20);
        arr.push(30);
        TEST(arr.removeShift(0) == 10);
        TEST(arr[0] == 20);
        TEST(arr.removeShift(0) == 20);
        TEST(arr[0] == 30);
        TEST(arr.removeShift(0) == 30);
        TEST(arr.count == 0);
    }

    // insertSwap with const ref
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push(); arr.push(); arr.push();
            Lifecycle val;
            arr.insertSwap(0, val);
            TEST(arr.count == 4);
            TEST(val.valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 1);
        TEST(Lifecycle::stats.moves == 1);
    }

    // insertSwap with rvalue ref
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push(); arr.push(); arr.push();
            Lifecycle val;
            arr.insertSwap(0, std::move(val));
            TEST(!val.valid);
            TEST(arr.count == 4);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 2);
        TEST(Lifecycle::stats.copies == 0);
    }

    // insertSwap at end appends without swapping
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push(); arr.push();
            Lifecycle val;
            arr.insertSwap(2, val);
            TEST(arr.count == 3);
            TEST(val.valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 1);
        TEST(Lifecycle::stats.moves == 0);
    }

    // insertSwap at end with rvalue
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push();
            Lifecycle val;
            arr.insertSwap(1, std::move(val));
            TEST(!val.valid);
            TEST(arr.count == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 1);
        TEST(Lifecycle::stats.copies == 0);
    }

    // insertSwap at beginning moves old value to end
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(10);
        arr.push(20);
        arr.push(30);
        arr.insertSwap(0, 5);
        TEST(arr.count == 4);
        TEST(arr[0] == 5);
        TEST(arr[3] == 10);
    }

    // insertSwap at middle
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(10);
        arr.push(20);
        arr.push(30);
        arr.insertSwap(1, 99);
        TEST(arr.count == 4);
        TEST(arr[0] == 10);
        TEST(arr[1] == 99);
        TEST(arr[3] == 20);
    }

    // removeSwap moves only
    {
        Lifecycle::stats.reset();
        ArenaScope arena = getScratch();
        {
            ArrayTemp<Lifecycle> arr{arena, 0, 0};
            arr.push(); arr.push(); arr.push();
            Lifecycle removed = arr.removeSwap(0);
            TEST(removed.valid);
            TEST(arr.count == 2);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.copies == 0);
        TEST(Lifecycle::stats.moves == 2);
    }

    // removeSwap at beginning (with 3 elements)
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(10);
        arr.push(20);
        arr.push(30);
        u32 removed = arr.removeSwap(0);
        TEST(removed == 10);
        TEST(arr.count == 2);
        TEST(arr[0] == 30);
        TEST(arr[1] == 20);
    }

    // removeSwap at middle
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(10);
        arr.push(20);
        arr.push(30);
        u32 removed = arr.removeSwap(1);
        TEST(removed == 20);
        TEST(arr.count == 2);
        TEST(arr[0] == 10);
        TEST(arr[1] == 30);
    }

    // removeSwap at end
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(10);
        arr.push(20);
        arr.push(30);
        u32 removed = arr.removeSwap(2);
        TEST(removed == 30);
        TEST(arr.count == 2);
        TEST(arr[0] == 10);
        TEST(arr[1] == 20);
    }

    // removeSwap single element (last remaining)
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(99);
        u32 v = arr.removeSwap(0);
        TEST(v == 99);
        TEST(arr.count == 0);
    }

    // insertShift return value is reference to displaced element
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(10);
        arr.push(20);
        u32& r = arr.insertShift(1, 99);
        TEST(r == 20);
        r = 77;
        TEST(arr[2] == 77);
    }

    // insertSwap return value is reference to displaced element
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(10);
        arr.push(20);
        u32& r = arr.insertSwap(0, 99);
        TEST(r == 10);
        r = 55;
        TEST(arr[2] == 55);
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

    // range-for over const ref
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(1);
        arr.push(2);
        const ArrayTemp<u32>& carr = arr;
        u64 sum = 0;
        for (u32 v : carr)
            sum += v;
        TEST(sum == 3);
    }

    // const operator[] works on const ref
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(42);
        const ArrayTemp<u32>& carr = arr;
        TEST(carr[0] == 42);
    }

    // Implicit conversion to Span (non-const)
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(42);
        Span<u32> s = arr;
        TEST(s.data == arr.vals);
        TEST(s.count == arr.count);
        s[0] = 99;
        TEST(arr[0] == 99);
    }

    // Implicit conversion to Span<const T> from const ref
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> arr{arena, 0, 0};
        arr.push(99);
        const ArrayTemp<u32>& carr = arr;
        Span<const u32> cs = carr;
        TEST(cs.data == arr.vals);
        TEST(cs.count == arr.count);
        TEST(cs[0] == 99);
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

    // Move assign
    {
        ArenaScope arena = getScratch();
        ArrayTemp<u32> a{arena, 0, 8};
        a.push(10);
        a.push(20);
        ArrayTemp<u32> b{arena, 0, 8};
        b.push(30);
        b = std::move(a);
        TEST(b[0] == 10);
        TEST(b[1] == 20);
        TEST(a.vals == nullptr);
    }
}

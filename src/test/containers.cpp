#include "tests.hpp"

void testContainers()
{
    // ============================================================================
    // BinaryBuilder
    // ============================================================================
    //
    // BinaryBuilder is an arena-backed builder for binary data. Supports
    // resize, append, overwrite, read, and implicit BinaryView conversion.
    //
    // Functions covered:
    // - BinaryBuilder() — default
    // - BinaryBuilder(Arena*, u64) — arena + optional initial size
    // - operator BinaryView()
    // - read(u64, void*, u64)
    // - read<T>(u64)
    // - resize(u64)
    // - overwrite(u64, const void*, u64)
    // - overwrite<T>(u64, const T&)
    // - append(const void*, u64)
    // - append<T>(const T&)

    // Default-constructed builder has null arena
    {
        BinaryBuilder bb;
        TEST(bb.arena == nullptr);
        TEST(bb.data == nullptr);
        TEST(bb.size == 0);
    }

    // Create with arena and initial size
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 8};
        TEST(bb.arena == arena);
        TEST(bb.data != nullptr);
        TEST(bb.size == 8);
    }

    // Create with arena and zero size
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 0};
        TEST(bb.arena == arena);
        TEST(bb.data != nullptr); // alloc(0,1) returns a valid pointer
        TEST(bb.size == 0);
    }

    // resize grows the builder
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 4};
        bb.resize(8);
        TEST(bb.size == 8);
    }

    // append raw data
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena};
        u32 val = 42;
        bb.append(&val, sizeof(val));
        TEST(bb.size == sizeof(val));
        u32 result = bb.read<u32>(0);
        TEST(result == 42);
    }

    // append<T> typed data
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena};
        u32 val = 0xDEADBEEF;
        bb.append(val);
        TEST(bb.size == sizeof(val));
        u32 result = bb.read<u32>(0);
        TEST(result == 0xDEADBEEF);
    }

    // overwrite existing data
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 4};
        u32 val = 123;
        bb.overwrite(0, &val, sizeof(val));
        u32 result = bb.read<u32>(0);
        TEST(result == 123);
    }

    // overwrite<T> typed data
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 4};
        bb.overwrite(0, static_cast<u32>(789));
        u32 result = bb.read<u32>(0);
        TEST(result == 789);
    }

    // Read raw data
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 4};
        bb.overwrite(0, static_cast<u32>(0xAABBCCDD));
        u32 result = 0;
        bb.read(0, &result, sizeof(result));
        TEST(result == 0xAABBCCDD);
    }

    // Implicit conversion to BinaryView
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena, 4};
        BinaryView bv = bb;
        TEST(bv.data == bb.data);
        TEST(bv.size == bb.size);
    }

    // Append multiple values
    {
        ArenaScope arena = getScratch();
        BinaryBuilder bb{arena};
        u8 a = 0xAA;
        u8 b = 0xBB;
        bb.append(a);
        bb.append(b);
        TEST(bb.size == 2);
        u8 ra = bb.read<u8>(0);
        u8 rb = bb.read<u8>(1);
        TEST(ra == 0xAA);
        TEST(rb == 0xBB);
    }

    // ============================================================================
    // Binary
    // ============================================================================
    //
    // Binary is an owning, heap-allocated, move-only binary data block.
    //
    // Functions covered:
    // - Binary() — default
    // - Binary::create(BinaryView) — factory
    // - ~Binary() — destructor
    // - Binary(Binary&&) — move construct
    // - Binary& operator=(Binary&&) — move assign
    // - read(u64, void*, u64)
    // - read<T>(u64)
    // - operator BinaryView()

    // Default-constructed Binary is empty
    {
        Binary b;
        TEST(b.data == nullptr);
        TEST(b.size == 0);
    }

    // Create from BinaryView
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        Binary b = Binary::create(bv);
        TEST(b.size == sizeof(val));
        u32 result = b.read<u32>(0);
        TEST(result == 42);
    }

    // Create from empty BinaryView
    {
        BinaryView bv{};
        Binary b = Binary::create(bv);
        TEST(b.size == 0);
        // data may be non-null (heapAlloc(0,1) returns valid pointer)
    }

    // Move construct
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        Binary a = Binary::create(bv);
        Binary b = std::move(a);
        TEST(a.data == nullptr);
        TEST(a.size == 0);
        TEST(b.size == sizeof(val));
        u32 result = b.read<u32>(0);
        TEST(result == 42);
    }

    // Move assign
    {
        u32 val1 = 42;
        u32 val2 = 99;
        BinaryView bv1{&val1, sizeof(val1)};
        BinaryView bv2{&val2, sizeof(val2)};
        Binary a = Binary::create(bv1);
        Binary b = Binary::create(bv2);
        b = std::move(a);
        TEST(a.data == nullptr);
        TEST(a.size == 0);
        TEST(b.size == sizeof(val1));
        u32 result = b.read<u32>(0);
        TEST(result == 42);
    }

    // Implicit conversion to BinaryView
    {
        u32 val = 0xCAFEBABE;
        BinaryView bv{&val, sizeof(val)};
        Binary b = Binary::create(bv);
        BinaryView bv2 = b;
        TEST(bv2.size == b.size);
        u32 result = bv2.read<u32>(0);
        TEST(result == 0xCAFEBABE);
    }

    // Create, scope-exit destroys (no double-free crash)
    {
        u32 val = 12345;
        BinaryView bv{&val, sizeof(val)};
        Binary b = Binary::create(bv);
        TEST(b.read<u32>(0) == 12345);
    }
    // Allocate after — if heap is corrupt, we crash
    {
        Binary b2 = Binary::create(BinaryView{});
        TEST(b2.size == 0);
    }

    // ============================================================================
    // UniquePtr
    // ============================================================================
    //
    // UniquePtr is a move-only heap-allocated smart pointer with unique ownership.

    // Default construction yields null pointer
    {
        UniquePtr<i32> ptr;
        TEST(ptr.ptr == nullptr);
        TEST((i32*)ptr == nullptr);
    }

    // nullptr construction yields null pointer
    {
        UniquePtr<i32> ptr{nullptr};
        TEST(ptr.ptr == nullptr);
    }

    // makeUnique creates a new object on the heap
    {
        Lifecycle::stats.reset();
        {
            UniquePtr<Lifecycle> ptr = makeUnique<Lifecycle>();
            TEST(ptr.ptr != nullptr);
            TEST(ptr->valid);
            TEST((*ptr).valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 1);
    }

    // Move construct transfers ownership
    {
        Lifecycle::stats.reset();
        UniquePtr<Lifecycle> a = makeUnique<Lifecycle>();
        Lifecycle* addr = a.ptr;
        UniquePtr<Lifecycle> b = std::move(a);
        TEST(a.ptr == nullptr);
        TEST(b.ptr == addr);
    }
    TEST(Lifecycle::stats.alive == 0);
    TEST(Lifecycle::stats.dtors == 1);

    // Move assign transfers ownership and frees old
    {
        Lifecycle::stats.reset();
        UniquePtr<Lifecycle> a = makeUnique<Lifecycle>();
        UniquePtr<Lifecycle> b = makeUnique<Lifecycle>();
        Lifecycle* addr = a.ptr;
        b = std::move(a);
        TEST(a.ptr == nullptr);
        TEST(b.ptr == addr);
    }
    TEST(Lifecycle::stats.alive == 0);
    TEST(Lifecycle::stats.dtors == 2);

    // ============================================================================
    // SharedPtr
    // ============================================================================
    //
    // SharedPtr is a reference-counted, move-only heap-allocated smart pointer
    // with shared ownership. Clone via clone() to increment ref count.

    // Default construction yields null pointer
    {
        SharedPtr<i32> ptr;
        TEST(ptr.ptr == nullptr);
        TEST((i32*)ptr == nullptr);
    }

    // nullptr construction yields null pointer
    {
        SharedPtr<i32> ptr{nullptr};
        TEST(ptr.ptr == nullptr);
    }

    // makeShared creates a new object that is alive
    {
        Lifecycle::stats.reset();
        {
            SharedPtr<Lifecycle> ptr = makeShared<Lifecycle>();
            TEST(ptr.ptr != nullptr);
            TEST(ptr->valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 1);
    }

    // Cloning keeps object alive until last clone is destroyed
    {
        Lifecycle::stats.reset();
        {
            SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
            TEST(a.ptr != nullptr);
            {
                SharedPtr<Lifecycle> b = a.clone();
                TEST(b.ptr == a.ptr);
                TEST(b->valid);
            }
            // a still valid after b destroyed
            TEST(a->valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 1);
    }

    // Move construct transfers ownership without destroying
    {
        Lifecycle::stats.reset();
        {
            SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
            SharedPtr<Lifecycle> b = std::move(a);
            TEST(a.ptr == nullptr);
            TEST(b.ptr != nullptr);
            TEST(b->valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 1);
    }

    // Move assign transfers and destroys old target
    {
        Lifecycle::stats.reset();
        {
            SharedPtr<Lifecycle> a = makeShared<Lifecycle>();
            SharedPtr<Lifecycle> b = makeShared<Lifecycle>();
            b = std::move(a);
            TEST(a.ptr == nullptr);
            TEST(b->valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 2);
    }

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

    // ============================================================================
    // Queue
    // ============================================================================
    //
    // Queue is a move-only, heap-allocated double-ended ring buffer.
    // Supports pushFront, pushBack, popFront, popBack, and reserve.

    // Default-constructed queue is empty
    {
        Queue<u32> q;
        TEST(q.vals == nullptr);
        TEST(q.count == 0);
        TEST(q.capacity == 0);
    }

    // Construct with initial capacity
    {
        Queue<u32> q{16};
        TEST(q.vals != nullptr);
        TEST(q.capacity == 16);
        TEST(q.count == 0);
    }

    // pushBack and popFront (FIFO order)
    {
        Queue<u32> q;
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        TEST(q.count == 3);
        TEST(q.popFront() == 1);
        TEST(q.popFront() == 2);
        TEST(q.popFront() == 3);
        TEST(q.count == 0);
    }

    // pushFront and popBack (LIFO order from front)
    {
        Queue<u32> q;
        q.pushFront(1);
        q.pushFront(2);
        q.pushFront(3);
        TEST(q.count == 3);
        TEST(q.popBack() == 1);
        TEST(q.popBack() == 2);
        TEST(q.popBack() == 3);
        TEST(q.count == 0);
    }

    // pushFront and popFront (reversed order)
    {
        Queue<u32> q;
        q.pushFront(10);
        q.pushFront(20);
        TEST(q.popFront() == 20);
        TEST(q.popFront() == 10);
    }

    // pushBack and popBack (stack order)
    {
        Queue<u32> q;
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        TEST(q.popBack() == 3);
        TEST(q.popBack() == 2);
        TEST(q.popBack() == 1);
    }

    // Wrap-around behavior: popFront followed by pushBack reuses slots
    {
        Queue<u32> q;
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        TEST(q.popFront() == 1);
        TEST(q.popFront() == 2);
        q.pushBack(4);
        q.pushBack(5);
        TEST(q.count == 3);
        TEST(q.popFront() == 3);
        TEST(q.popFront() == 4);
        TEST(q.popFront() == 5);
    }

    // Wrap-around with pushFront
    {
        Queue<u32> q;
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        q.popFront(); // 1
        q.pushFront(0);
        TEST(q.count == 3);
        TEST(q.popFront() == 0);
        TEST(q.popFront() == 2);
        TEST(q.popFront() == 3);
    }

    // Reserve grows capacity
    {
        Queue<u32> q{4};
        q.pushBack(1);
        q.pushBack(2);
        q.reserve(16);
        TEST(q.capacity >= 16);
        TEST(q.count == 2);
        TEST(q.popFront() == 1);
        TEST(q.popFront() == 2);
    }

    // Move construct
    {
        Queue<u32> a;
        a.pushBack(1);
        a.pushBack(2);
        u32* oldVals = a.vals;
        Queue<u32> b = std::move(a);
        TEST(a.vals == nullptr);
        TEST(b.vals == oldVals);
        TEST(b.popFront() == 1);
        TEST(b.popFront() == 2);
    }

    // Move assign
    {
        Queue<u32> a;
        a.pushBack(10);
        Queue<u32> b;
        b.pushBack(20);
        b.popFront();
        b = std::move(a);
        TEST(b.popFront() == 10);
    }

    // pushBack const ref copies (0 moves)
    {
        Lifecycle::stats.reset();
        {
            Queue<Lifecycle> q;
            Lifecycle a;
            q.pushBack(a);
            TEST(a.valid);
            TEST(q.count == 1);
            TEST(Lifecycle::stats.copies == 1);
            TEST(Lifecycle::stats.moves == 0);
            q.popFront();
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // pushBack rvalue ref moves (0 copies)
    {
        Lifecycle::stats.reset();
        {
            Queue<Lifecycle> q;
            Lifecycle a;
            q.pushBack(std::move(a));
            TEST(!a.valid);
            TEST(q.count == 1);
            TEST(Lifecycle::stats.copies == 0);
            TEST(Lifecycle::stats.moves == 1);
            q.popFront();
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // pushFront const ref copies (0 moves)
    {
        Lifecycle::stats.reset();
        {
            Queue<Lifecycle> q;
            Lifecycle a;
            q.pushFront(a);
            TEST(a.valid);
            TEST(Lifecycle::stats.copies == 1);
            TEST(Lifecycle::stats.moves == 0);
            q.popBack();
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // pushFront rvalue ref moves (0 copies)
    {
        Lifecycle::stats.reset();
        {
            Queue<Lifecycle> q;
            Lifecycle a;
            q.pushFront(std::move(a));
            TEST(!a.valid);
            TEST(Lifecycle::stats.copies == 0);
            TEST(Lifecycle::stats.moves == 1);
            q.popBack();
        }
        TEST(Lifecycle::stats.alive == 0);
    }

    // popFront pops by move
    {
        Lifecycle::stats.reset();
        {
            Queue<Lifecycle> q;
            q.pushBack();
            Lifecycle val = q.popFront();
            TEST(val.valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 1);
        TEST(Lifecycle::stats.copies == 0);
    }

    // popBack pops by move
    {
        Lifecycle::stats.reset();
        {
            Queue<Lifecycle> q;
            q.pushBack();
            Lifecycle val = q.popBack();
            TEST(val.valid);
        }
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.moves == 1);
        TEST(Lifecycle::stats.copies == 0);
    }

    // ============================================================================
    // QueueTemp
    // ============================================================================
    //
    // QueueTemp is an arena-allocated double-ended ring buffer.

    // Default-constructed QueueTemp is empty
    {
        QueueTemp<u32> q;
        TEST(q.arena == nullptr);
        TEST(q.vals == nullptr);
        TEST(q.count == 0);
    }

    // Create with arena and initial capacity
    {
        ArenaScope arena = getScratch();
        QueueTemp<u32> q{arena, 16};
        TEST(q.arena == arena);
        TEST(q.vals != nullptr);
        TEST(q.capacity == 16);
        TEST(q.count == 0);
    }

    // pushBack and popFront
    {
        ArenaScope arena = getScratch();
        QueueTemp<u32> q{arena, 0};
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        TEST(q.count == 3);
        TEST(q.popFront() == 1);
        TEST(q.popFront() == 2);
        TEST(q.popFront() == 3);
    }

    // pushFront and popBack
    {
        ArenaScope arena = getScratch();
        QueueTemp<u32> q{arena, 0};
        q.pushFront(10);
        q.pushFront(20);
        TEST(q.popBack() == 10);
        TEST(q.popBack() == 20);
    }

    // Wrap-around behavior
    {
        ArenaScope arena = getScratch();
        QueueTemp<u32> q{arena, 4};
        q.pushBack(1);
        q.pushBack(2);
        q.pushBack(3);
        q.popFront(); // 1
        q.popFront(); // 2
        q.pushBack(4);
        q.pushBack(5);
        TEST(q.count == 3);
        TEST(q.popFront() == 3);
        TEST(q.popFront() == 4);
        TEST(q.popFront() == 5);
    }

    // Reserve grows capacity (non-extend path)
    {
        ArenaScope arena = getScratch();
        QueueTemp<u32> q{arena, 4};
        q.pushBack(1);
        q.pushBack(2);
        q.reserve(20);
        TEST(q.capacity >= 20);
        TEST(q.popFront() == 1);
        TEST(q.popFront() == 2);
    }

    // Move construct
    {
        ArenaScope arena = getScratch();
        QueueTemp<u32> a{arena, 0};
        a.pushBack(99);
        QueueTemp<u32> b = std::move(a);
        TEST(a.vals == nullptr);
        TEST(b.popFront() == 99);
    }

    // ============================================================================
    // Set
    // ============================================================================
    //
    // Set is a move-only, heap-allocated open-addressing hash set using
    // Robin Hood hashing. Supports add, has, remove, reset, resize, and forEach.

    // Default-constructed set is empty
    {
        Set<u32> set;
        TEST(set.hasVal == nullptr);
        TEST(set.capacity == 0);
        TEST(set.count == 0);
    }

    // Construct with initial capacity
    {
        Set<u32> set{16};
        TEST(set.capacity == 16);
        TEST(set.count == 0);
    }

    // add and has
    {
        Set<u32> set;
        set.add(10);
        set.add(20);
        set.add(30);
        TEST(set.count == 3);
        TEST(set.has(10));
        TEST(set.has(20));
        TEST(set.has(30));
        TEST(!set.has(40));
    }

    // Duplicate add does not increase count
    {
        Set<u32> set;
        set.add(42);
        set.add(42);
        TEST(set.count == 1);
    }

    // Remove an element
    {
        Set<u32> set;
        set.add(1);
        set.add(2);
        set.add(3);
        set.remove(2);
        TEST(set.count == 2);
        TEST(!set.has(2));
        TEST(set.has(1));
        TEST(set.has(3));
    }

    // Remove non-existent element is safe
    {
        Set<u32> set;
        set.add(5);
        set.remove(99);
        TEST(set.count == 1);
    }

    // Add and remove multiple elements with collision
    {
        Set<u32> set{8};
        set.add(0);
        set.add(8);  // collides with 0 in capacity=8
        set.add(16); // collides with both
        TEST(set.count == 3);
        TEST(set.has(0));
        TEST(set.has(8));
        TEST(set.has(16));

        set.remove(8);
        TEST(!set.has(8));
        TEST(set.has(0));
        TEST(set.has(16));

        set.remove(0);
        TEST(!set.has(0));
        TEST(set.has(16));
    }

    // reset clears all elements
    {
        Lifecycle::stats.reset();
        Set<Lifecycle> set;
        set.add(Lifecycle{});
        set.add(Lifecycle{});
        TEST(Lifecycle::stats.alive == 2);
        set.reset();
        TEST(set.count == 0);
        TEST(Lifecycle::stats.alive == 0);
    }

    // Resize grows the set
    {
        Set<u32> set{4};
        set.add(1);
        set.add(2);
        set.resize(32);
        TEST(set.capacity == 32);
        TEST(set.count == 2);
        TEST(set.has(1));
        TEST(set.has(2));
    }

    // forEach visits all elements
    {
        Set<u32> set;
        set.add(10);
        set.add(20);
        set.add(30);
        u64 sum = 0;
        set.forEach([&](u32* v) { sum += *v; });
        TEST(sum == 60);
    }

    // Move construct
    {
        Set<u32> a;
        a.add(1);
        a.add(2);
        bool* oldHasVal = a.hasVal;
        Set<u32> b = std::move(a);
        TEST(a.hasVal == nullptr);
        TEST(b.hasVal == oldHasVal);
        TEST(b.count == 2);
        TEST(b.has(1));
        TEST(b.has(2));
    }

    // Move assign
    {
        Set<u32> a;
        a.add(99);
        Set<u32> b;
        b = std::move(a);
        TEST(b.has(99));
    }

    // ============================================================================
    // SetTemp
    // ============================================================================
    //
    // SetTemp is an arena-allocated open-addressing hash set.

    // Default-constructed SetTemp is empty
    {
        SetTemp<u32> set;
        TEST(set.arena == nullptr);
        TEST(set.capacity == 0);
        TEST(set.count == 0);
    }

    // Create with arena and initial capacity
    {
        ArenaScope arena = getScratch();
        SetTemp<u32> set{arena, 16};
        TEST(set.arena == arena);
        TEST(set.capacity == 16);
        TEST(set.count == 0);
    }

    // add, has, remove
    {
        ArenaScope arena = getScratch();
        SetTemp<u32> set{arena, 0};
        set.add(10);
        set.add(20);
        TEST(set.count == 2);
        TEST(set.has(10));
        TEST(set.has(20));
        set.remove(10);
        TEST(!set.has(10));
        TEST(set.has(20));
    }

    // forEach visits all elements
    {
        ArenaScope arena = getScratch();
        SetTemp<u32> set{arena, 0};
        set.add(5);
        set.add(10);
        u64 sum = 0;
        set.forEach([&](u32* v) { sum += *v; });
        TEST(sum == 15);
    }

    // ============================================================================
    // Map
    // ============================================================================
    //
    // Map is a move-only, heap-allocated open-addressing hash map (Robin Hood).
    // Supports add, get, has, remove, reset, resize, and forEach.

    // Default-constructed map is empty
    {
        Map<u32, f32> map;
        TEST(map.hasVal == nullptr);
        TEST(map.capacity == 0);
        TEST(map.count == 0);
    }

    // Construct with initial capacity
    {
        Map<u32, f32> map{16};
        TEST(map.capacity == 16);
        TEST(map.count == 0);
    }

    // add, get, has
    {
        Map<u32, f32> map;
        map.add(1, 1.5f);
        map.add(2, 2.5f);
        map.add(3, 3.5f);
        TEST(map.count == 3);
        TEST(map.has(1));
        TEST(map.has(2));
        TEST(map.has(3));
        TEST(!map.has(4));
        f32* v = map.get(1);
        TEST(v != nullptr);
        TEST(*v == 1.5f);
        TEST(map.get(4) == nullptr);
    }

    // Duplicate key overwrites but count stays same
    {
        Map<u32, f32> map;
        map.add(1, 1.0f);
        map.add(1, 2.0f);
        TEST(map.count == 1);
        TEST(*map.get(1) == 2.0f);
    }

    // Remove a key-value pair
    {
        Map<u32, f32> map;
        map.add(1, 1.0f);
        map.add(2, 2.0f);
        TEST(map.remove(1));
        TEST(!map.has(1));
        TEST(map.has(2));
        TEST(!map.remove(99)); // non-existent
    }

    // Remove with value output
    {
        Map<u32, f32> map;
        map.add(42, 3.14f);
        f32 out = 0;
        TEST(map.remove(42, &out));
        TEST(out == 3.14f);
    }

    // Collision handling with sequential keys hashing to same slot
    {
        Map<u32, f32> map{8};
        map.add(0, 0.0f);
        map.add(8, 8.0f);  // collides
        map.add(16, 16.0f); // collides
        TEST(map.count == 3);
        TEST(*map.get(0) == 0.0f);
        TEST(*map.get(8) == 8.0f);
        TEST(*map.get(16) == 16.0f);

        map.remove(8);
        TEST(!map.has(8));
        TEST(*map.get(0) == 0.0f);
        TEST(*map.get(16) == 16.0f);
    }

    // reset clears all entries
    {
        Lifecycle::stats.reset();
        Map<u32, Lifecycle> map;
        map.add(1, Lifecycle{});
        map.add(2, Lifecycle{});
        TEST(Lifecycle::stats.alive == 2);
        map.reset();
        TEST(map.count == 0);
        TEST(Lifecycle::stats.alive == 0);
    }

    // Resize grows the map
    {
        Map<u32, f32> map{4};
        map.add(1, 1.0f);
        map.add(2, 2.0f);
        map.resize(32);
        TEST(map.capacity == 32);
        TEST(map.count == 2);
        TEST(*map.get(1) == 1.0f);
        TEST(*map.get(2) == 2.0f);
    }

    // forEach visits all key-value pairs
    {
        Map<u32, f32> map;
        map.add(1, 10.0f);
        map.add(2, 20.0f);
        f64 sum = 0;
        map.forEach([&](u32* k, f32* v) { sum += static_cast<f64>(*k) + *v; });
        TEST(sum == 33.0);
    }

    // Move construct
    {
        Map<u32, f32> a;
        a.add(1, 1.0f);
        bool* oldHasVal = a.hasVal;
        Map<u32, f32> b = std::move(a);
        TEST(a.hasVal == nullptr);
        TEST(b.hasVal == oldHasVal);
        TEST(b.count == 1);
        TEST(*b.get(1) == 1.0f);
    }

    // Move assign
    {
        Map<u32, f32> a;
        a.add(5, 5.0f);
        Map<u32, f32> b;
        b = std::move(a);
        TEST(*b.get(5) == 5.0f);
    }

    // ============================================================================
    // MapTemp
    // ============================================================================
    //
    // MapTemp is an arena-allocated open-addressing hash map.

    // Default-constructed MapTemp is empty
    {
        MapTemp<u32, f32> map;
        TEST(map.arena == nullptr);
        TEST(map.capacity == 0);
        TEST(map.count == 0);
    }

    // Create with arena and initial capacity
    {
        ArenaScope arena = getScratch();
        MapTemp<u32, f32> map{arena, 16};
        TEST(map.arena == arena);
        TEST(map.capacity == 16);
        TEST(map.count == 0);
    }

    // add, get, has, remove
    {
        ArenaScope arena = getScratch();
        MapTemp<u32, f32> map{arena, 0};
        map.add(1, 1.5f);
        map.add(2, 2.5f);
        TEST(map.count == 2);
        TEST(map.has(1));
        TEST(*map.get(1) == 1.5f);
        map.remove(1);
        TEST(!map.has(1));
        TEST(map.has(2));
    }

    // forEach
    {
        ArenaScope arena = getScratch();
        MapTemp<u32, f32> map{arena, 0};
        map.add(1, 10.0f);
        map.add(2, 20.0f);
        f64 sum = 0;
        map.forEach([&](u32* k, f32* v) { sum += static_cast<f64>(*k) + *v; });
        TEST(sum == 33.0);
    }

    // ============================================================================
    // Pool
    // ============================================================================
    //
    // Pool is a fixed-block object pool allocator. Allocates in blocks of 1024
    // objects, reuses freed slots. Move-only.

    // Default-constructed pool is empty
    {
        Pool<Lifecycle> pool;
        TEST(pool.prealloc.count == 0);
        TEST(pool.inactive.count == 0);
    }

    // alloc creates a new object, free returns it to the pool
    {
        Lifecycle::stats.reset();
        Pool<Lifecycle> pool;
        Lifecycle* a = pool.alloc();
        TEST(a != nullptr);
        TEST(a->valid);
        TEST(Lifecycle::stats.alive == 1);
        pool.free(a);
        TEST(Lifecycle::stats.alive == 0);
        TEST(Lifecycle::stats.dtors == 1);
    }

    // Free returns memory to inactive pool for reuse
    {
        Pool<u32> pool;
        u32* a = pool.alloc(10u);
        u32* b = pool.alloc(20u);
        TEST(*a == 10);
        TEST(*b == 20);
        pool.free(a);
        pool.free(b);
        u32* c = pool.alloc(30u);
        TEST(c == b);
        TEST(*c == 30u);
        u32* d = pool.alloc(40u);
        TEST(d == a);
        pool.free(c);
        pool.free(d);
    }

    // Multiple blocks allocated beyond 1024 items
    {
        Pool<u32> pool;
        static constexpr u32 n = 2500;
        u32* pts[2500];
        for (u32 i = 0; i < n; ++i)
        {
            pts[i] = pool.alloc(i);
            TEST(*pts[i] == i);
        }
        TEST(pool.prealloc.count >= 3);
        for (u32 i = 0; i < n; ++i)
            pool.free(pts[i]);
    }

    // Free nullptr is safe
    {
        Pool<u32> pool;
        pool.free(nullptr);
    }

    // ============================================================================
    // HandlePool
    // ============================================================================
    //
    // HandlePool provides generation-counted handle allocation. Handles are
    // 32-bit values with 24-bit index and 8-bit generation.

    // handlePoolCreate returns a valid pool with handle 0 reserved (null handle)
    {
        HandlePool pool = handlePoolCreate();
        HG_DEFER(handlePoolDestroy(&pool));
        TEST(handleNull.id == 0);
        TEST(pool.handles.count == 1);
    }

    // handlePoolAlloc returns sequential handles
    {
        HandlePool pool = handlePoolCreate();
        HG_DEFER(handlePoolDestroy(&pool));
        Handle a = handlePoolAlloc(&pool);
        Handle b = handlePoolAlloc(&pool);
        TEST(handlePoolAlive(&pool, a));
        TEST(handlePoolAlive(&pool, b));
        TEST(handleIdx(a) == 1);
        TEST(handleIdx(b) == 2);
    }

    // handlePoolFree invalidates handle
    {
        HandlePool pool = handlePoolCreate();
        HG_DEFER(handlePoolDestroy(&pool));
        Handle a = handlePoolAlloc(&pool);
        TEST(handlePoolAlive(&pool, a));
        handlePoolFree(&pool, a);
        TEST(!handlePoolAlive(&pool, a));
    }

    // Freed handle is reused with incremented generation
    {
        HandlePool pool = handlePoolCreate();
        HG_DEFER(handlePoolDestroy(&pool));
        Handle a = handlePoolAlloc(&pool);
        u32 idx = handleIdx(a);
        handlePoolFree(&pool, a);
        Handle b = handlePoolAlloc(&pool);
        TEST(handleIdx(b) == idx);
        TEST(b.id != a.id);
        TEST(handlePoolAlive(&pool, b));
        TEST(!handlePoolAlive(&pool, a));
    }

    // handlePoolReset invalidates all handles
    {
        HandlePool pool = handlePoolCreate();
        HG_DEFER(handlePoolDestroy(&pool));
        Handle a = handlePoolAlloc(&pool);
        Handle b = handlePoolAlloc(&pool);
        handlePoolReset(&pool);
        TEST(!handlePoolAlive(&pool, a));
        TEST(!handlePoolAlive(&pool, b));
        Handle c = handlePoolAlloc(&pool);
        TEST(handleIdx(c) == 1);
    }
}


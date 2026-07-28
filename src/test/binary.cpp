#include "tests.hpp"

void testBinary()
{
    // ============================================================================
    // BinaryView
    // ============================================================================
    //
    // BinaryView is a non-owning view into binary data (data pointer + size).
    // read() and read<T>() copy bytes out at an offset.

    // Default-constructed BinaryView is empty
    {
        BinaryView bv{};
        TEST(bv.data == nullptr);
        TEST(bv.size == 0);
    }

    // Create from pointer and size
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        TEST(bv.data == &val);
        TEST(bv.size == sizeof(val));
    }

    // read<T>() copies typed data
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        u32 result = bv.read<u32>(0);
        TEST(result == 42);
    }

    // read() copies raw data
    {
        u32 val = 42;
        BinaryView bv{&val, sizeof(val)};
        u32 result = 0;
        bv.read(0, &result, sizeof(result));
        TEST(result == 42);
    }

    // read<T>() at offset
    {
        u8 data[8] = {0, 0, 0, 0, 0xAA, 0xBB, 0xCC, 0xDD};
        BinaryView bv{data, 8};
        u32 result = bv.read<u32>(4);
        TEST(result == 0xDDCCBBAA);
    }

    // ============================================================================
    // BinaryBuilder
    // ============================================================================
    //
    // BinaryBuilder is an arena-backed builder for binary data. Supports
    // resize, append, overwrite, read, and implicit BinaryView conversion.

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
}


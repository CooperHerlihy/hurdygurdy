#include "tests.hpp"
#include "hg/binary.hpp"

using namespace hg;

TEST(testBinaryViewDefault)
{
    BinaryView bv{};
    ASSERT(bv.data == nullptr);
    ASSERT(bv.size == 0);
}

TEST(testBinaryViewFromPtr)
{
    u32 val = 42;
    BinaryView bv{&val, sizeof(val)};
    ASSERT(bv.data == &val);
    ASSERT(bv.size == sizeof(val));
}

TEST(testBinaryViewReadTyped)
{
    u32 val = 42;
    BinaryView bv{&val, sizeof(val)};
    u32 result = bv.read<u32>(0);
    ASSERT(result == 42);
}

TEST(testBinaryViewReadRaw)
{
    u32 val = 42;
    BinaryView bv{&val, sizeof(val)};
    u32 result = 0;
    bv.read(0, &result, sizeof(result));
    ASSERT(result == 42);
}

TEST(testBinaryViewReadAtOffset)
{
    u8 data[8] = {0, 0, 0, 0, 0xAA, 0xBB, 0xCC, 0xDD};
    BinaryView bv{data, 8};
    u32 result = bv.read<u32>(4);
    ASSERT(result == 0xDDCCBBAA);
}

TEST(testBinaryBuilderDefault)
{
    BinaryBuilder bb;
    ASSERT(bb.arena == nullptr);
    ASSERT(bb.data == nullptr);
    ASSERT(bb.size == 0);
}

TEST(testBinaryBuilderCreate)
{
    ArenaScope arena = getScratch();
    BinaryBuilder bb{arena, 8};
    ASSERT(bb.arena == arena);
    ASSERT(bb.data != nullptr);
    ASSERT(bb.size == 8);
}

TEST(testBinaryBuilderCreateZero)
{
    ArenaScope arena = getScratch();
    BinaryBuilder bb{arena, 0};
    ASSERT(bb.arena == arena);
    ASSERT(bb.data != nullptr);
    ASSERT(bb.size == 0);
}

TEST(testBinaryBuilderResize)
{
    ArenaScope arena = getScratch();
    BinaryBuilder bb{arena, 4};
    bb.resize(8);
    ASSERT(bb.size == 8);
}

TEST(testBinaryBuilderAppendRaw)
{
    ArenaScope arena = getScratch();
    BinaryBuilder bb{arena};
    u32 val = 42;
    bb.append(&val, sizeof(val));
    ASSERT(bb.size == sizeof(val));
    u32 result = bb.read<u32>(0);
    ASSERT(result == 42);
}

TEST(testBinaryBuilderAppendTyped)
{
    ArenaScope arena = getScratch();
    BinaryBuilder bb{arena};
    u32 val = 0xDEADBEEF;
    bb.append(val);
    ASSERT(bb.size == sizeof(val));
    u32 result = bb.read<u32>(0);
    ASSERT(result == 0xDEADBEEF);
}

TEST(testBinaryBuilderOverwriteRaw)
{
    ArenaScope arena = getScratch();
    BinaryBuilder bb{arena, 4};
    u32 val = 123;
    bb.overwrite(0, &val, sizeof(val));
    u32 result = bb.read<u32>(0);
    ASSERT(result == 123);
}

TEST(testBinaryBuilderOverwriteTyped)
{
    ArenaScope arena = getScratch();
    BinaryBuilder bb{arena, 4};
    bb.overwrite(0, static_cast<u32>(789));
    u32 result = bb.read<u32>(0);
    ASSERT(result == 789);
}

TEST(testBinaryBuilderReadRaw)
{
    ArenaScope arena = getScratch();
    BinaryBuilder bb{arena, 4};
    bb.overwrite(0, static_cast<u32>(0xAABBCCDD));
    u32 result = 0;
    bb.read(0, &result, sizeof(result));
    ASSERT(result == 0xAABBCCDD);
}

TEST(testBinaryBuilderImplicitConversion)
{
    ArenaScope arena = getScratch();
    BinaryBuilder bb{arena, 4};
    BinaryView bv = bb;
    ASSERT(bv.data == bb.data);
    ASSERT(bv.size == bb.size);
}

TEST(testBinaryBuilderAppendMultiple)
{
    ArenaScope arena = getScratch();
    BinaryBuilder bb{arena};
    u8 a = 0xAA;
    u8 b = 0xBB;
    bb.append(a);
    bb.append(b);
    ASSERT(bb.size == 2);
    u8 ra = bb.read<u8>(0);
    u8 rb = bb.read<u8>(1);
    ASSERT(ra == 0xAA);
    ASSERT(rb == 0xBB);
}

TEST(testBinaryDefault)
{
    Binary b;
    ASSERT(b.data == nullptr);
    ASSERT(b.size == 0);
}

TEST(testBinaryCreate)
{
    u32 val = 42;
    BinaryView bv{&val, sizeof(val)};
    Binary b = Binary::create(bv);
    ASSERT(b.size == sizeof(val));
    u32 result = b.read<u32>(0);
    ASSERT(result == 42);
}

TEST(testBinaryCreateEmpty)
{
    BinaryView bv{};
    Binary b = Binary::create(bv);
    ASSERT(b.size == 0);
}

TEST(testBinaryMoveConstruct)
{
    u32 val = 42;
    BinaryView bv{&val, sizeof(val)};
    Binary a = Binary::create(bv);
    Binary b = std::move(a);
    ASSERT(a.data == nullptr);
    ASSERT(a.size == 0);
    ASSERT(b.size == sizeof(val));
    u32 result = b.read<u32>(0);
    ASSERT(result == 42);
}

TEST(testBinaryMoveAssign)
{
    u32 val1 = 42;
    u32 val2 = 99;
    BinaryView bv1{&val1, sizeof(val1)};
    BinaryView bv2{&val2, sizeof(val2)};
    Binary a = Binary::create(bv1);
    Binary b = Binary::create(bv2);
    b = std::move(a);
    ASSERT(a.data == nullptr);
    ASSERT(a.size == 0);
    ASSERT(b.size == sizeof(val1));
    u32 result = b.read<u32>(0);
    ASSERT(result == 42);
}

TEST(testBinaryImplicitConversion)
{
    u32 val = 0xCAFEBABE;
    BinaryView bv{&val, sizeof(val)};
    Binary b = Binary::create(bv);
    BinaryView bv2 = b;
    ASSERT(bv2.size == b.size);
    u32 result = bv2.read<u32>(0);
    ASSERT(result == 0xCAFEBABE);
}

TEST(testBinaryScopeExit)
{
    u32 val = 12345;
    BinaryView bv{&val, sizeof(val)};
    Binary b = Binary::create(bv);
    ASSERT(b.read<u32>(0) == 12345);
    Binary b2 = Binary::create(BinaryView{});
    ASSERT(b2.size == 0);
}

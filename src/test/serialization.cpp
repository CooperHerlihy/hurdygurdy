#include "tests.hpp"
#include "hg/serialization.hpp"

using namespace hg;

TEST(testSerializeBool)
{
    bool val = true;
    bool copy = false;
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy == val);
}

TEST(testSerializeI64)
{
    i64 val = -1234567890123;
    i64 copy = 0;
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy == val);
}

TEST(testSerializeU32)
{
    u32 val = 0xDEADBEEF;
    u32 copy = 0;
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy == val);
}

TEST(testSerializeF64)
{
    f64 val = 3.14159265358979;
    f64 copy = 0.0;
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy == val);
}

TEST(testSerializeVec3)
{
    Vec3 val{1.0f, 2.0f, 3.0f};
    Vec3 copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy == val);
}

TEST(testSerializeMat4)
{
    Mat4 val{1.0f};
    Mat4 copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy == val);
}

TEST(testSerializeComplex)
{
    Complex val{3.0f, 4.0f};
    Complex copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy == val);
}

TEST(testSerializeQuat)
{
    Quat val{0.707f, 0.0f, 0.707f, 0.0f};
    Quat copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy == val);
}

TEST(testSerializeNested)
{
    ArenaScope arena = getScratch();
    i32 outer = 7;
    i32 inner = 42;
    i32 outerCopy = 0;
    i32 innerCopy = 0;

    Serializer w = serialWriter(arena);
    serializeBegin(&w);
    serialize(&w, &outer);
    serializeBegin(&w);
    serialize(&w, &inner);
    serializeEnd(&w);
    serializeEnd(&w);

    Serializer r = serialReader(arena, w.current);
    serializeBegin(&r);
    serialize(&r, &outerCopy);
    serializeBegin(&r);
    serialize(&r, &innerCopy);
    serializeEnd(&r);
    serializeEnd(&r);

    ASSERT(outerCopy == outer);
    ASSERT(innerCopy == inner);
}

TEST(testSerializeBeginWithSize)
{
    ArenaScope arena = getScratch();
    u32 childCount = 0;
    u32 val = 99;
    u32 copy = 0;

    Serializer w = serialWriter(arena);
    serializeBegin(&w);
    serialize(&w, &val);
    serializeEnd(&w);

    Serializer r = serialReader(arena, w.current);
    serializeBegin(&r, &childCount);
    serialize(&r, &copy);
    serializeEnd(&r);

    ASSERT(childCount == 1);
    ASSERT(copy == val);
}

TEST(testSerializeCompositeStruct)
{
    struct Data {
        i64 a;
        u16 b;
        f32 c;
        bool d;
        String e;
    };

    auto serializeData = [](Serializer* s, Data* val)
    {
        serializeObject(s,
            &val->a,
            &val->b,
            &val->c,
            &val->d,
            &val->e);
    };

    ArenaScope arena = getScratch();
    Data val{};
    val.a = -42;
    val.b = 99;
    val.c = 2.5f;
    val.d = true;
    val.e = String::create("composite");

    Data copy{};
    Serializer w = serialWriter(arena);
    serializeData(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serializeData(&r, &copy);
    ASSERT(copy.a == val.a);
    ASSERT(copy.b == val.b);
    ASSERT(copy.c == val.c);
    ASSERT(copy.d == val.d);
    ASSERT(copy.e == val.e);
}

TEST(testSerializeLifecycle)
{
    Lifecycle::stats.reset();
    ArenaScope arena = getScratch();

    Lifecycle val{};
    u64 savedId = val.id;

    Lifecycle copy{};
    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.id == savedId);
    ASSERT(copy.valid == true);
}

TEST(testSerializeCArray)
{
    u32 val[4] = {10, 20, 30, 40};
    u32 copy[4] = {};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy[0] == 10);
    ASSERT(copy[1] == 20);
    ASSERT(copy[2] == 30);
    ASSERT(copy[3] == 40);
}

TEST(testSerializeString)
{
    String val = String::create("hello world");
    String copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy == val);
}

TEST(testSerializeBinary)
{
    u8 raw[8] = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
    Binary val = Binary::create({raw, 8});
    Binary copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.size == val.size);
    ASSERT(memcmp(copy.data, val.data, val.size) == 0);
}

TEST(testSerializeUniquePtr)
{
    UniquePtr<i32> val = makeUnique<i32>(42);
    UniquePtr<i32> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(*copy == 42);
    ASSERT(copy.ptr != val.ptr);
}

TEST(testSerializeArray)
{
    ArenaScope arena = getScratch();
    Array<u32> val{};
    val.push(1);
    val.push(2);
    val.push(3);

    Array<u32> copy{};
    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.count == val.count);
    for (u32 i = 0; i < val.count; ++i)
        ASSERT(copy[i] == val[i]);
}

TEST(testSerializeSet)
{
    ArenaScope arena = getScratch();
    Set<u32> val{};
    val.add(10);
    val.add(20);
    val.add(30);

    Set<u32> copy{};
    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.count == val.count);
    ASSERT(copy.has(10));
    ASSERT(copy.has(20));
    ASSERT(copy.has(30));
}

TEST(testSerializeMap)
{
    ArenaScope arena = getScratch();
    Map<u32, f32> val{};
    val.add(1, 1.5f);
    val.add(2, 2.5f);
    val.add(3, 3.5f);

    Map<u32, f32> copy{};
    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.count == val.count);
    ASSERT(*copy.get(1) == 1.5f);
    ASSERT(*copy.get(2) == 2.5f);
    ASSERT(*copy.get(3) == 3.5f);
}

TEST(testBinaryRoundTripPrimitive)
{
    struct Data {
        i64 a;
        f64 b;
        bool c;
    };

    auto serializeData = [](Serializer* s, Data* val)
    {
        serializeObject(s,
            &val->a,
            &val->b,
            &val->c);
    };

    ArenaScope arena = getScratch();
    Data val{};
    val.a = -999;
    val.b = 3.14;
    val.c = true;

    Data copy{};
    Serializer w = serialWriter(arena);
    serializeData(&w, &val);
    BinaryView bin = writeSerialBinary(arena, &w);
    Serializer r = readSerialBinary(arena, bin);
    serializeData(&r, &copy);
    ASSERT(copy.a == val.a);
    ASSERT(copy.b == val.b);
    ASSERT(copy.c == val.c);
}

TEST(testBinaryRoundTripString)
{
    struct Data {
        i64 a;
        String b;
    };

    auto serializeData = [](Serializer* s, Data* val)
    {
        serializeObject(s,
            &val->a,
            &val->b);
    };

    ArenaScope arena = getScratch();
    Data val{};
    val.a = -999;
    val.b = String::create("binary-test");

    Data copy{};
    Serializer w = serialWriter(arena);
    serializeData(&w, &val);
    BinaryView bin = writeSerialBinary(arena, &w);
    Serializer r = readSerialBinary(arena, bin);
    serializeData(&r, &copy);
    ASSERT(copy.a == val.a);
    ASSERT(copy.b == val.b);
}

TEST(testBinaryRoundTripBinary)
{
    struct Data {
        i64 a;
        Binary b;
    };

    auto serializeData = [](Serializer* s, Data* val)
    {
        serializeObject(s,
            &val->a,
            &val->b);
    };

    ArenaScope arena = getScratch();
    u8 raw[4] = {0xDE, 0xAD, 0xBE, 0xEF};
    Data val{};
    val.a = -123;
    val.b = Binary::create({raw, 4});

    Data copy{};
    Serializer w = serialWriter(arena);
    serializeData(&w, &val);
    BinaryView bin = writeSerialBinary(arena, &w);
    Serializer r = readSerialBinary(arena, bin);
    serializeData(&r, &copy);
    ASSERT(copy.a == val.a);
    ASSERT(copy.b.size == val.b.size);
    ASSERT(memcmp(copy.b.data, val.b.data, val.b.size) == 0);
}

TEST(testProductRoundTripPrimitive)
{
    Product<i64, f64, bool> val{-42, 3.14, true};
    Product<i64, f64, bool> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.get<0>() == val.get<0>());
    ASSERT(copy.get<1>() == val.get<1>());
    ASSERT(copy.get<2>() == val.get<2>());
}

TEST(testProductRoundTripString)
{
    Product<i64, String> val{7, String::create("hello")};
    Product<i64, String> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.get<0>() == val.get<0>());
    ASSERT(copy.get<1>() == val.get<1>());
}

TEST(testProductRoundTripEmpty)
{
    Product<> val{};
    Product<> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
}

TEST(testProductRoundTripBinary)
{
    Product<i32, String, f32> val{42, String::create("binary"), 2.5f};
    Product<i32, String, f32> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    BinaryView bin = writeSerialBinary(arena, &w);
    Serializer r = readSerialBinary(arena, bin);
    serialize(&r, &copy);
    ASSERT(copy.get<0>() == val.get<0>());
    ASSERT(copy.get<1>() == val.get<1>());
    ASSERT(copy.get<2>() == val.get<2>());
}

TEST(testSumRoundTripFirstType)
{
    Sum<i64, f64, String> val{i64{-42}};
    Sum<i64, f64, String> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.is<i64>());
    ASSERT(copy.get<i64>() == -42);
}

TEST(testSumRoundTripSecondType)
{
    Sum<i64, f64, String> val{f64{3.14}};
    Sum<i64, f64, String> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.is<f64>());
    ASSERT(copy.get<f64>() == 3.14);
}

TEST(testSumRoundTripThirdType)
{
    Sum<i64, f64, String> val{String::create("world")};
    Sum<i64, f64, String> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.is<String>());
    ASSERT(copy.get<String>() == String::create("world"));
}

TEST(testSumRoundTripEmpty)
{
    Sum<i64, f64, String> val{};
    Sum<i64, f64, String> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(!copy.is<i64>());
    ASSERT(!copy.is<f64>());
    ASSERT(!copy.is<String>());
}

TEST(testSumRoundTripBinary)
{
    Sum<i64, f64, String> val{f64{99.9}};
    Sum<i64, f64, String> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    BinaryView bin = writeSerialBinary(arena, &w);
    Serializer r = readSerialBinary(arena, bin);
    serialize(&r, &copy);
    ASSERT(copy.is<f64>());
    ASSERT(copy.get<f64>() == 99.9);
}

TEST(testMaybeRoundTripWithValue)
{
    Maybe<i64> val{42};
    Maybe<i64> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.has);
    ASSERT(*copy == 42);
}

TEST(testMaybeRoundTripWithString)
{
    Maybe<String> val{String::create("maybe")};
    Maybe<String> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.has);
    ASSERT(*copy == val.val);
}

TEST(testMaybeRoundTripEmpty)
{
    Maybe<i64> val{};
    Maybe<i64> copy{Maybe<i64>{99}};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(!copy.has);
}

TEST(testMaybeRoundTripBinary)
{
    Maybe<f32> val{2.5f};
    Maybe<f32> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    BinaryView bin = writeSerialBinary(arena, &w);
    Serializer r = readSerialBinary(arena, bin);
    serialize(&r, &copy);
    ASSERT(copy.has);
    ASSERT(*copy == 2.5f);
}

TEST(testSerializeEmptyString)
{
    String val = String::create("");
    String copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy == val);
}

TEST(testSerializeEmptyBinary)
{
    Binary val = Binary::create({nullptr, 0});
    Binary copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.size == val.size);
}

TEST(testSerializeEmptyArray)
{
    ArenaScope arena = getScratch();
    Array<u32> val{};
    Array<u32> copy{};

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.count == 0);
}

TEST(testSerializeEmptySet)
{
    ArenaScope arena = getScratch();
    Set<u32> val{};
    Set<u32> copy{};

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.count == 0);
}

TEST(testSerializeEmptyMap)
{
    ArenaScope arena = getScratch();
    Map<u32, f32> val{};
    Map<u32, f32> copy{};

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.count == 0);
}

TEST(testSerializeEmptyMaybe)
{
    Maybe<i64> val{};
    Maybe<i64> copy{Maybe<i64>{99}};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(!copy.has);
}

TEST(testSerializeNestedProduct)
{
    Product<Product<i64, f64>, bool> val{Product<i64, f64>{-7, 2.5f}, true};
    Product<Product<i64, f64>, bool> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.get<0>().get<0>() == -7);
    ASSERT(copy.get<0>().get<1>() == 2.5f);
    ASSERT(copy.get<1>() == true);
}

TEST(testSerializeNestedMaybe)
{
    Maybe<String> val{String::create("nested")};
    Maybe<String> copy{};
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy.has);
    ASSERT(*copy == val.val);
}

TEST(testSerializeBoolFalse)
{
    bool val = false;
    bool copy = true;
    ArenaScope arena = getScratch();

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy == val);
}

TEST(testSerializeLargeString)
{
    ArenaScope arena = getScratch();
    StringBuilder builder{arena};
    for (u32 i = 0; i < 2048; ++i)
        builder.append("x");
    String val = String::create(builder);
    String copy{};

    Serializer w = serialWriter(arena);
    serialize(&w, &val);
    Serializer r = serialReader(arena, w.current);
    serialize(&r, &copy);
    ASSERT(copy == val);
    ASSERT(copy.length == 2048);
}

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
    ASSERT(copy.has(10u));
    ASSERT(copy.has(20u));
    ASSERT(copy.has(30u));
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
    ASSERT(*copy.get(1u) == 1.5f);
    ASSERT(*copy.get(2u) == 2.5f);
    ASSERT(*copy.get(3u) == 3.5f);
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

TEST(testWriteJsonEmptyObject)
{
    ArenaScope arena = getScratch();
    JsonObject obj{arena, 0};
    ASSERT(writeJson(arena, obj) == "{\n}");
}

TEST(testWriteJsonEmptyArray)
{
    ArenaScope arena = getScratch();
    JsonObject obj{arena, 0};
    JsonArray arr{arena};
    obj.add(StringBuilder{arena, "a"}, JsonValue{std::move(arr)});
    ASSERT(writeJson(arena, obj) == "{\n\t\"a\" : [\n\t]\n}");
}

TEST(testWriteJsonPrimitives)
{
    ArenaScope arena = getScratch();

    {
        JsonObject obj{arena, 0};
        obj.add(StringBuilder{arena, "b"}, JsonValue{true});
        ASSERT(writeJson(arena, obj) == "{\n\t\"b\" : true\n}");
    }
    {
        JsonObject obj{arena, 0};
        obj.add(StringBuilder{arena, "b"}, JsonValue{false});
        ASSERT(writeJson(arena, obj) == "{\n\t\"b\" : false\n}");
    }
    {
        JsonObject obj{arena, 0};
        obj.add(StringBuilder{arena, "f"}, JsonValue{f64{2.5}});
        ASSERT(writeJson(arena, obj) == "{\n\t\"f\" : 2.5\n}");
    }
    {
        JsonObject obj{arena, 0};
        obj.add(StringBuilder{arena, "f"}, JsonValue{f64{-2.5}});
        ASSERT(writeJson(arena, obj) == "{\n\t\"f\" : -2.5\n}");
    }
    {
        JsonObject obj{arena, 0};
        obj.add(StringBuilder{arena, "f"}, JsonValue{f64{0.0}});
        ASSERT(writeJson(arena, obj) == "{\n\t\"f\" : 0\n}");
    }
    {
        JsonObject obj{arena, 0};
        obj.add(StringBuilder{arena, "s"}, JsonValue{StringBuilder{arena, "hello"}});
        ASSERT(writeJson(arena, obj) == "{\n\t\"s\" : \"hello\"\n}");
    }
}

TEST(testWriteJsonArrays)
{
    ArenaScope arena = getScratch();

    {
        JsonObject obj{arena, 0};
        JsonArray arr{arena};
        arr.push(JsonValue{true});
        obj.add(StringBuilder{arena, "a"}, JsonValue{std::move(arr)});
        ASSERT(writeJson(arena, obj) == "{\n\t\"a\" : [\n\t\ttrue\n\t]\n}");
    }
    {
        JsonObject obj{arena, 0};
        JsonArray arr{arena};
        arr.push(JsonValue{f64{2.5}});
        obj.add(StringBuilder{arena, "a"}, JsonValue{std::move(arr)});
        ASSERT(writeJson(arena, obj) == "{\n\t\"a\" : [\n\t\t2.5\n\t]\n}");
    }
    {
        JsonObject obj{arena, 0};
        JsonArray arr{arena};
        arr.push(JsonValue{StringBuilder{arena, "one"}});
        arr.push(JsonValue{StringBuilder{arena, "two"}});
        obj.add(StringBuilder{arena, "a"}, JsonValue{std::move(arr)});
        ASSERT(writeJson(arena, obj) == "{\n\t\"a\" : [\n\t\t\"one\",\n\t\t\"two\"\n\t]\n}");
    }
}

TEST(testWriteJsonNested)
{
    ArenaScope arena = getScratch();

    {
        JsonObject inner{arena, 0};
        inner.add(StringBuilder{arena, "b"}, JsonValue{true});
        JsonObject outer{arena, 0};
        outer.add(StringBuilder{arena, "a"}, JsonValue{std::move(inner)});
        ASSERT(writeJson(arena, outer) == "{\n\t\"a\" : {\n\t\t\"b\" : true\n\t}\n}");
    }
    {
        JsonArray inner{arena};
        inner.push(JsonValue{true});
        JsonObject outer{arena, 0};
        outer.add(StringBuilder{arena, "a"}, JsonValue{std::move(inner)});
        ASSERT(writeJson(arena, outer) == "{\n\t\"a\" : [\n\t\ttrue\n\t]\n}");
    }
    {
        JsonObject inner{arena, 0};
        inner.add(StringBuilder{arena, "x"}, JsonValue{f64{2.5}});
        JsonArray mid{arena};
        mid.push(JsonValue{std::move(inner)});
        JsonObject outer{arena, 0};
        outer.add(StringBuilder{arena, "a"}, JsonValue{std::move(mid)});
        ASSERT(writeJson(arena, outer) == "{\n\t\"a\" : [\n\t\t{\n\t\t\t\"x\" : 2.5\n\t\t}\n\t]\n}");
    }
}

TEST(testWriteJsonDocument)
{
    ArenaScope arena = getScratch();

    JsonObject obj{arena, 0};
    obj.add(StringBuilder{arena, "name"}, JsonValue{StringBuilder{arena, "hurdy"}});
    obj.add(StringBuilder{arena, "count"}, JsonValue{f64{3.0}});
    obj.add(StringBuilder{arena, "valid"}, JsonValue{true});
    JsonArray list{arena};
    list.push(JsonValue{f64{1.0}});
    list.push(JsonValue{false});
    obj.add(StringBuilder{arena, "list"}, JsonValue{std::move(list)});

    StringView out = writeJson(arena, obj);

    auto contains = [](const StringView& hay, const char* needle)
    {
        u64 needleLen = 0;
        while (needle[needleLen] != '\0')
            ++needleLen;
        if (needleLen > hay.length)
            return false;
        for (u64 i = 0; i <= hay.length - needleLen; ++i)
        {
            bool match = true;
            for (u64 j = 0; j < needleLen; ++j)
            {
                if (hay[i + j] != needle[j])
                {
                    match = false;
                    break;
                }
            }
            if (match)
                return true;
        }
        return false;
    };

    ASSERT(out.length >= 2);
    ASSERT(out[0] == '{');
    ASSERT(out[out.length - 1] == '}');

    u64 openBrace = 0;
    u64 closeBrace = 0;
    u64 openBracket = 0;
    u64 closeBracket = 0;
    for (u64 i = 0; i < out.length; ++i)
    {
        switch (out[i])
        {
        case '{': ++openBrace; break;
        case '}': ++closeBrace; break;
        case '[': ++openBracket; break;
        case ']': ++closeBracket; break;
        default: break;
        }
    }
    ASSERT(openBrace == closeBrace);
    ASSERT(openBracket == closeBracket);

    bool trailingComma = false;
    for (u64 i = 0; i + 2 < out.length; ++i)
    {
        if (out[i] == ',' && out[i + 1] == '\n' && (out[i + 2] == '}' || out[i + 2] == ']'))
            trailingComma = true;
    }
    ASSERT(!trailingComma);

    ASSERT(contains(out, "\"name\" : \"hurdy\""));
    ASSERT(contains(out, "\"count\" : 3"));
    ASSERT(contains(out, "\"valid\" : true"));
    ASSERT(contains(out, "\"list\" : ["));
    ASSERT(contains(out, "1"));
    ASSERT(contains(out, "false"));
}

TEST(testWriteJsonEscaping)
{
    ArenaScope arena = getScratch();

    JsonObject obj{arena, 0};
    obj.add(StringBuilder{arena, "s"}, JsonValue{StringBuilder{arena, "a\"b\\c\bd\fe\nf\rg\th"}});
    ASSERT(writeJson(arena, obj) == "{\n\t\"s\" : \"a\\\"b\\\\c\\bd\\fe\\nf\\rg\\th\"\n}");
}

TEST(testReadJsonEmpty)
{
    ArenaScope arena = getScratch();

    ASSERT(readJson(arena, "{}").has);
    ASSERT(readJson(arena, "{ }").has);
    ASSERT(readJson(arena, "{\n}").has);
    ASSERT(readJson(arena, "{}")->count == 0);

    Maybe<JsonObject> result = readJson(arena, "{\n\t\"a\" : [\n\t]\n}");
    ASSERT(result.has);
    ASSERT(result->count == 1);
    ASSERT(result->get(StringBuilder{arena, "a"})->is<JsonArray>());
    ASSERT(result->get(StringBuilder{arena, "a"})->get<JsonArray>().count == 0);
}

TEST(testReadJsonPrimitives)
{
    ArenaScope arena = getScratch();

    Maybe<JsonObject> result = readJson(arena,
        "{ \"b\" : true , \"c\" : false , \"i\" : 12345 , \"n\" : -7 , \"f\" : 2.5 , \"g\" : -0.5 , \"s\" : \"hi\" }");
    ASSERT(result.has);
    ASSERT(result->count == 7);

    JsonValue* b = result->get(StringBuilder{arena, "b"});
    ASSERT(b != nullptr);
    ASSERT(b->is<bool>());
    ASSERT(b->get<bool>() == true);

    JsonValue* c = result->get(StringBuilder{arena, "c"});
    ASSERT(c != nullptr);
    ASSERT(c->is<bool>());
    ASSERT(c->get<bool>() == false);

    JsonValue* i = result->get(StringBuilder{arena, "i"});
    ASSERT(i != nullptr);
    ASSERT(i->is<f64>());
    ASSERT(i->get<f64>() == 12345.0);

    JsonValue* n = result->get(StringBuilder{arena, "n"});
    ASSERT(n != nullptr);
    ASSERT(n->is<f64>());
    ASSERT(n->get<f64>() == -7.0);

    JsonValue* f = result->get(StringBuilder{arena, "f"});
    ASSERT(f != nullptr);
    ASSERT(f->is<f64>());
    ASSERT(f->get<f64>() == 2.5);

    JsonValue* g = result->get(StringBuilder{arena, "g"});
    ASSERT(g != nullptr);
    ASSERT(g->is<f64>());
    ASSERT(g->get<f64>() == -0.5);

    JsonValue* s = result->get(StringBuilder{arena, "s"});
    ASSERT(s != nullptr);
    ASSERT(s->is<StringBuilder>());
    ASSERT(StringView{s->get<StringBuilder>()} == "hi");
}

TEST(testReadJsonWhitespace)
{
    ArenaScope arena = getScratch();

    Maybe<JsonObject> result = readJson(arena,
        "{\n\t\"a\" : 1,\n\t\"b\" : \"two words\"\n}");
    ASSERT(result.has);
    ASSERT(result->count == 2);
    ASSERT(result->get(StringBuilder{arena, "a"})->get<f64>() == 1.0);
    ASSERT(StringView{result->get(StringBuilder{arena, "b"})->get<StringBuilder>()} == "two words");

    Maybe<JsonObject> trailing = readJson(arena, "{ \"a\" : 1 , }");
    ASSERT(trailing.has);
    ASSERT(trailing->count == 1);
}

TEST(testReadJsonArrays)
{
    ArenaScope arena = getScratch();

    Maybe<JsonObject> result = readJson(arena,
        "{ \"a\" : [] , \"b\" : [ 1 ] , \"c\" : [ \"x\" , true ] }");
    ASSERT(result.has);
    ASSERT(result->count == 3);

    JsonValue* a = result->get(StringBuilder{arena, "a"});
    ASSERT(a != nullptr);
    ASSERT(a->is<JsonArray>());
    ASSERT(a->get<JsonArray>().count == 0);

    JsonValue* b = result->get(StringBuilder{arena, "b"});
    ASSERT(b != nullptr);
    ASSERT(b->get<JsonArray>().count == 1);
    ASSERT(b->get<JsonArray>()[0].get<f64>() == 1.0);

    JsonValue* c = result->get(StringBuilder{arena, "c"});
    ASSERT(c != nullptr);
    JsonArray& cArr = c->get<JsonArray>();
    ASSERT(cArr.count == 2);
    ASSERT(StringView{cArr[0].get<StringBuilder>()} == "x");
    ASSERT(cArr[1].get<bool>() == true);
}

TEST(testReadJsonNested)
{
    ArenaScope arena = getScratch();

    Maybe<JsonObject> result = readJson(arena,
        "{ \"child\" : { \"inner\" : true } , \"list\" : [ { \"id\" : 1 } , { \"id\" : 2 } ] }");
    ASSERT(result.has);

    JsonValue* child = result->get(StringBuilder{arena, "child"});
    ASSERT(child != nullptr);
    ASSERT(child->is<JsonObject>());
    ASSERT(child->get<JsonObject>().count == 1);
    JsonValue* inner = child->get<JsonObject>().get(StringBuilder{arena, "inner"});
    ASSERT(inner != nullptr);
    ASSERT(inner->is<bool>());
    ASSERT(inner->get<bool>() == true);

    JsonValue* list = result->get(StringBuilder{arena, "list"});
    ASSERT(list != nullptr);
    ASSERT(list->is<JsonArray>());
    JsonArray& listArr = list->get<JsonArray>();
    ASSERT(listArr.count == 2);
    for (u32 i = 0; i < 2; ++i)
    {
        ASSERT(listArr[i].is<JsonObject>());
        JsonValue* id = listArr[i].get<JsonObject>().get(StringBuilder{arena, "id"});
        ASSERT(id != nullptr);
        ASSERT(id->is<f64>());
        ASSERT(id->get<f64>() == static_cast<f64>(i + 1));
    }
}

TEST(testReadJsonEscapes)
{
    ArenaScope arena = getScratch();

    Maybe<JsonObject> result = readJson(arena,
        "{ \"s\" : \"a\\\"b\\\\c\\bd\\fe\\nf\\rg\\th\" }");
    ASSERT(result.has);

    String expected = String::create("a\"b\\c\bd\fe\nf\rg\th");
    JsonValue* s = result->get(StringBuilder{arena, "s"});
    ASSERT(s != nullptr);
    ASSERT(s->is<StringBuilder>());
    ASSERT(StringView{s->get<StringBuilder>()} == StringView{expected});

    Maybe<JsonObject> empty = readJson(arena, "{ \"s\" : \"\" }");
    ASSERT(empty.has);
    ASSERT(empty->get(StringBuilder{arena, "s"})->get<StringBuilder>().length == 0);
}

TEST(testReadJsonFailure)
{
    ArenaScope arena = getScratch();

    ASSERT(!readJson(arena, "").has);
    ASSERT(!readJson(arena, "junk").has);
    ASSERT(!readJson(arena, "\"a\" : 1").has);
    ASSERT(!readJson(arena, "[1,2]").has);
    ASSERT(!readJson(arena, "{").has);
    ASSERT(!readJson(arena, "{ \"a\" : }").has);
    ASSERT(!readJson(arena, "{ : 1 }").has);
    ASSERT(!readJson(arena, "{ \"a\" 1 }").has);
    ASSERT(!readJson(arena, "{ \"a\" : \"unterminated }").has);
    ASSERT(!readJson(arena, "{ \"a\" : 1, \"b\" }").has);
    ASSERT(!readJson(arena, "{ \"a\" : [ 1, 2 }").has);
    ASSERT(!readJson(arena, "{ \"a\" : [ 1, 2").has);
    ASSERT(!readJson(arena, "{ \"a\" : 1, \"b\" : }").has);
}

TEST(testWriteReadJsonRoundTrip)
{
    ArenaScope arena = getScratch();

    JsonObject obj{arena, 0};
    obj.add(StringBuilder{arena, "name"}, JsonValue{StringBuilder{arena, "hurdy"}});
    obj.add(StringBuilder{arena, "count"}, JsonValue{f64{42.0}});
    obj.add(StringBuilder{arena, "valid"}, JsonValue{true});
    obj.add(StringBuilder{arena, "empty"}, JsonValue{StringBuilder{arena, ""}});
    JsonArray list{arena};
    list.push(JsonValue{f64{1.0}});
    list.push(JsonValue{f64{-2.5}});
    list.push(JsonValue{false});
    list.push(JsonValue{StringBuilder{arena, "x"}});
    obj.add(StringBuilder{arena, "list"}, JsonValue{std::move(list)});
    JsonObject child{arena, 0};
    child.add(StringBuilder{arena, "pi"}, JsonValue{f64{3.5}});
    obj.add(StringBuilder{arena, "child"}, JsonValue{std::move(child)});

    StringView out = writeJson(arena, obj);
    ASSERT(out.length >= 2);

    Maybe<JsonObject> json = readJson(arena, out);
    ASSERT(json.has);
    ASSERT(json->count == 6);
    ASSERT(StringView{json->get(StringBuilder{arena, "name"})->get<StringBuilder>()} == "hurdy");
    ASSERT(json->get(StringBuilder{arena, "count"})->get<f64>() == 42.0);
    ASSERT(json->get(StringBuilder{arena, "valid"})->get<bool>() == true);
    ASSERT(json->get(StringBuilder{arena, "empty"})->get<StringBuilder>().length == 0);
    ASSERT(json->get(StringBuilder{arena, "list"})->get<JsonArray>().count == 4);
    ASSERT(json->get(StringBuilder{arena, "child"})->get<JsonObject>().count == 1);

    StringView out2 = writeJson(arena, *json);
    ASSERT(out2 == out);
}

TEST(testReadWriteJsonRoundTrip)
{
    ArenaScope arena = getScratch();

    Maybe<JsonObject> result = readJson(arena,
        "{ \"a\" : 1 , \"b\" : \"str\" , \"c\" : [ true, false, -2.5, \"x\" ] , \"d\" : { \"e\" : [] } }");
    ASSERT(result.has);

    StringView out = writeJson(arena, *result);
    ASSERT(out ==
        "{\n\t\"a\" : 1,\n\t\"b\" : \"str\",\n\t\"c\" : [\n\t\ttrue,\n\t\tfalse,\n\t\t-2.5,\n\t\t\"x\"\n\t],\n\t\"d\" : {\n\t\t\"e\" : [\n\t\t]\n\t}\n}");

    Maybe<JsonObject> json = readJson(arena, out);
    ASSERT(json.has);
    ASSERT(json->count == 4);
    ASSERT(json->get(StringBuilder{arena, "a"})->get<f64>() == 1.0);
    ASSERT(StringView{json->get(StringBuilder{arena, "b"})->get<StringBuilder>()} == "str");
    ASSERT(json->get(StringBuilder{arena, "c"})->get<JsonArray>().count == 4);
    ASSERT(json->get(StringBuilder{arena, "d"})->get<JsonObject>().count == 1);
    ASSERT(json->get(StringBuilder{arena, "d"})->get<JsonObject>().get(StringBuilder{arena, "e"})->get<JsonArray>().count == 0);
}


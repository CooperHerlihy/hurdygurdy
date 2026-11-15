#include "tests.hpp"
#include "hg/serialization.hpp"

#include <string>
#include <vector>

using namespace hg;

TEST(testSerializeBool)
{
    bool val = true;
    bool copy = false;

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeI64)
{
    i64 val = -1234567890123;
    i64 copy = 0;

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeU32)
{
    u32 val = 0xDEADBEEF;
    u32 copy = 0;

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeF64)
{
    f64 val = 3.14159265358979;
    f64 copy = 0.0;

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeVec3)
{
    Vec3 val{1.0f, 2.0f, 3.0f};
    Vec3 copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeMat4)
{
    Mat4 val{1.0f};
    Mat4 copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeComplex)
{
    Complex val{3.0f, 4.0f};
    Complex copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeQuat)
{
    Quat val{0.707f, 0.0f, 0.707f, 0.0f};
    Quat copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeNested)
{
    i32 outer = 7;
    i32 inner = 42;
    i32 outerCopy = 0;
    i32 innerCopy = 0;

    Serializer w = serialWriter();
    serializeBegin(&w);
    ASSERT(serialize(&w, &outer));
    serializeBegin(&w);
    ASSERT(serialize(&w, &inner));
    serializeEnd(&w);
    serializeEnd(&w);

    Serializer r = serialReader(std::move(w.root));
    serializeBegin(&r);
    ASSERT(serialize(&r, &outerCopy));
    serializeBegin(&r);
    ASSERT(serialize(&r, &innerCopy));
    serializeEnd(&r);
    serializeEnd(&r);

    ASSERT(outerCopy == outer);
    ASSERT(innerCopy == inner);
}

TEST(testSerializeBeginWithSize)
{
    u32 childCount = 0;
    u32 val = 99;
    u32 copy = 0;

    Serializer w = serialWriter();
    serializeBegin(&w);
    ASSERT(serialize(&w, &val));
    serializeEnd(&w);

    Serializer r = serialReader(std::move(w.root));
    serializeBegin(&r, &childCount);
    ASSERT(serialize(&r, &copy));
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
        std::string e;
    };

    auto serializeData = [](Serializer* s, Data* val)
    {
        return serializeObject(s,
            &val->a,
            &val->b,
            &val->c,
            &val->d,
            &val->e);
    };
    Data val{};
    val.a = -42;
    val.b = 99;
    val.c = 2.5f;
    val.d = true;
    val.e = std::string{"composite"};

    Data copy{};
    Serializer w = serialWriter();
    ASSERT(serializeData(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serializeData(&r, &copy));
    ASSERT(copy.a == val.a);
    ASSERT(copy.b == val.b);
    ASSERT(copy.c == val.c);
    ASSERT(copy.d == val.d);
    ASSERT(copy.e == val.e);
}

TEST(testSerializeLifecycle)
{
    Lifecycle::stats.reset();

    Lifecycle val{};
    u64 savedId = val.id;

    Lifecycle copy{};
    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.id == savedId);
    ASSERT(copy.valid == true);
}

TEST(testSerializeCArray)
{
    u32 val[4] = {10, 20, 30, 40};
    u32 copy[4] = {};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy[0] == 10);
    ASSERT(copy[1] == 20);
    ASSERT(copy[2] == 30);
    ASSERT(copy[3] == 40);
}

TEST(testSerializeTrivialArray)
{
    std::array<u32, 4> val{10, 20, 30, 40};
    std::array<u32, 4> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeArrayOfStrings)
{
    std::array<std::string, 2> val{std::string{"alpha"}, std::string{"beta"}};
    std::array<std::string, 2> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy[0] == "alpha");
    ASSERT(copy[1] == "beta");
}

TEST(testSerializeString)
{
    std::string val = std::string{"hello world"};
    std::string copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeMultipleRootValues)
{
    i32 a = -7;
    f64 b = 2.5;
    std::string c = "third";

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &a));
    ASSERT(serialize(&w, &b));
    ASSERT(serialize(&w, &c));

    i32 aCopy = 0;
    f64 bCopy = 0.0;
    std::string cCopy{};

    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &aCopy));
    ASSERT(serialize(&r, &bCopy));
    ASSERT(serialize(&r, &cCopy));
    ASSERT(aCopy == a);
    ASSERT(bCopy == b);
    ASSERT(cCopy == c);
}

TEST(testSerializeTrivialVector)
{
    std::vector<u8> val{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
    std::vector<u8> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.size() == val.size());
    ASSERT(memcmp(copy.data(), val.data(), val.size()) == 0);
}

TEST(testSerializeTrivialVectorInObject)
{
    struct Data {
        std::string name;
        std::vector<u8> bytes;
        i32 id;
    };

    auto serializeData = [](Serializer* s, Data* val)
    {
        return serializeObject(s, &val->name, &val->bytes, &val->id);
    };

    Data val{};
    val.name = "blob";
    val.bytes = {0x01, 0x02, 0x03};
    val.id = -5;

    Data copy{};
    Serializer w = serialWriter();
    ASSERT(serializeData(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serializeData(&r, &copy));
    ASSERT(copy.name == val.name);
    ASSERT(copy.bytes == val.bytes);
    ASSERT(copy.id == val.id);
}

TEST(testSerializeVectorOfStrings)
{
    std::vector<std::string> val{"one", "two", "three"};
    std::vector<std::string> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.size() == val.size());
    ASSERT(copy[0] == "one");
    ASSERT(copy[1] == "two");
    ASSERT(copy[2] == "three");
}

TEST(testSerializeVectorOfVectors)
{
    std::vector<std::vector<u32>> val{{1, 2}, {}, {3}};
    std::vector<std::vector<u32>> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.size() == val.size());
    ASSERT(copy[0].size() == 2);
    ASSERT(copy[0][0] == 1);
    ASSERT(copy[0][1] == 2);
    ASSERT(copy[1].size() == 0);
    ASSERT(copy[2].size() == 1);
    ASSERT(copy[2][0] == 3);
}

TEST(testSerializeUniquePtr)
{
    std::unique_ptr<i32> val = std::make_unique<i32>(42);
    std::unique_ptr<i32> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(*copy == 42);
    ASSERT(copy.get() != val.get());
}

TEST(testSerializeArray)
{
    std::vector<u32> val{};
    val.push_back(1);
    val.push_back(2);
    val.push_back(3);

    std::vector<u32> copy{};
    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.size() == val.size());
    for (u32 i = 0; i < val.size(); ++i)
        ASSERT(copy[i] == val[i]);
}

TEST(testSerializeSet)
{
    HashSet<u32> val{};
    val.add(10);
    val.add(20);
    val.add(30);

    HashSet<u32> copy{};
    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.count == val.count);
    ASSERT(copy.has(10u));
    ASSERT(copy.has(20u));
    ASSERT(copy.has(30u));
}

TEST(testSerializeMap)
{
    HashMap<u32, f32> val{};
    val.add(1, 1.5f);
    val.add(2, 2.5f);
    val.add(3, 3.5f);

    HashMap<u32, f32> copy{};
    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
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
        return serializeObject(s,
            &val->a,
            &val->b,
            &val->c);
    };
    Data val{};
    val.a = -999;
    val.b = 3.14;
    val.c = true;

    Data copy{};
    Serializer w = serialWriter();
    ASSERT(serializeData(&w, &val));
    std::vector<u8> bin = writeSerialBinary(&w);
    std::optional<Serializer> rOpt = readSerialBinary(bin);
    ASSERT(rOpt.has_value());
    Serializer& r = *rOpt;
    ASSERT(serializeData(&r, &copy));
    ASSERT(copy.a == val.a);
    ASSERT(copy.b == val.b);
    ASSERT(copy.c == val.c);
}

TEST(testBinaryRoundTripString)
{
    struct Data {
        i64 a;
        std::string b;
    };

    auto serializeData = [](Serializer* s, Data* val)
    {
        return serializeObject(s,
            &val->a,
            &val->b);
    };
    Data val{};
    val.a = -999;
    val.b = std::string{"binary-test"};

    Data copy{};
    Serializer w = serialWriter();
    ASSERT(serializeData(&w, &val));
    std::vector<u8> bin = writeSerialBinary(&w);
    std::optional<Serializer> rOpt = readSerialBinary(bin);
    ASSERT(rOpt.has_value());
    Serializer& r = *rOpt;
    ASSERT(serializeData(&r, &copy));
    ASSERT(copy.a == val.a);
    ASSERT(copy.b == val.b);
}

TEST(testBinaryRoundTripTrivialVector)
{
    struct Data {
        i64 a;
        std::vector<u8> b;
    };

    auto serializeData = [](Serializer* s, Data* val)
    {
        return serializeObject(s,
            &val->a,
            &val->b);
    };
    Data val{};
    val.a = -123;
    val.b = {0xDE, 0xAD, 0xBE, 0xEF};

    Data copy{};
    Serializer w = serialWriter();
    ASSERT(serializeData(&w, &val));
    std::vector<u8> bin = writeSerialBinary(&w);
    std::optional<Serializer> rOpt = readSerialBinary(bin);
    ASSERT(rOpt.has_value());
    Serializer& r = *rOpt;
    ASSERT(serializeData(&r, &copy));
    ASSERT(copy.a == val.a);
    ASSERT(copy.b.size() == val.b.size());
    ASSERT(memcmp(copy.b.data(), val.b.data(), val.b.size()) == 0);
}

TEST(testBinaryRoundTripVector)
{
    std::vector<u32> val{1, 2, 3, 4};
    std::vector<u32> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    std::vector<u8> bin = writeSerialBinary(&w);
    ASSERT(isSerialData(bin));
    std::optional<Serializer> rOpt = readSerialBinary(bin);
    ASSERT(rOpt.has_value());
    Serializer& r = *rOpt;
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testBinaryRoundTripVectorOfStrings)
{
    std::vector<std::string> val{"alpha", "", "gamma"};
    std::vector<std::string> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    std::vector<u8> bin = writeSerialBinary(&w);
    std::optional<Serializer> rOpt = readSerialBinary(bin);
    ASSERT(rOpt.has_value());
    Serializer& r = *rOpt;
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testBinaryRoundTripMap)
{
    HashMap<std::string, i32> val{};
    val.add("a", 1);
    val.add("b", -2);

    HashMap<std::string, i32> copy{};
    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    std::vector<u8> bin = writeSerialBinary(&w);
    std::optional<Serializer> rOpt = readSerialBinary(bin);
    ASSERT(rOpt.has_value());
    Serializer& r = *rOpt;
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.count == val.count);
    ASSERT(*copy.get("a") == 1);
    ASSERT(*copy.get("b") == -2);
}

TEST(testIsSerialData)
{
    Serializer w = serialWriter();
    i32 val = 5;
    ASSERT(serialize(&w, &val));
    std::vector<u8> bin = writeSerialBinary(&w);
    ASSERT(isSerialData(bin));

    ASSERT(!isSerialData({}));

    std::vector<u8> garbage(32, 0);
    ASSERT(!isSerialData(garbage));
}

TEST(testReadSerialBinaryRejectsGarbage)
{
    setError("");
    ASSERT(!readSerialBinary({}).has_value());
    ASSERT(getError().length() > 0);

    std::vector<u8> garbage(32, 0);
    setError("");
    ASSERT(!readSerialBinary(garbage).has_value());
    ASSERT(getError().length() > 0);
}

TEST(testReadSerialBinaryRejectsWrongVersion)
{
    Serializer w = serialWriter();
    i32 val = 5;
    ASSERT(serialize(&w, &val));
    std::vector<u8> bin = writeSerialBinary(&w);

    // The major version follows the 4 byte tag
    u32 version = 1;
    memcpy(bin.data() + 4, &version, sizeof(version));

    setError("");
    ASSERT(!readSerialBinary(bin).has_value());
    ASSERT(getError().length() > 0);
}

TEST(testReadSerialBinaryRejectsTruncation)
{
    Serializer w = serialWriter();
    std::vector<u32> val{1, 2, 3};
    ASSERT(serialize(&w, &val));
    std::vector<u8> bin = writeSerialBinary(&w);
    ASSERT(isSerialData(bin));

    // Every truncation must fail: the loader validates node and leaf ranges
    for (u64 cut = 1; cut < bin.size(); ++cut)
    {
        std::vector<u8> trunc(bin.begin(), bin.end() - static_cast<i64>(cut));
        setError("");
        ASSERT(!readSerialBinary(trunc).has_value());
        ASSERT(getError().length() > 0);
    }
}

TEST(testProductRoundTripPrimitive)
{
    std::tuple<i64, f64, bool> val{-42, 3.14, true};
    std::tuple<i64, f64, bool> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(std::get<0>(copy) == std::get<0>(val));
    ASSERT(std::get<1>(copy) == std::get<1>(val));
    ASSERT(std::get<2>(copy) == std::get<2>(val));
}

TEST(testProductRoundTripString)
{
    std::tuple<i64, std::string> val{7, std::string{"hello"}};
    std::tuple<i64, std::string> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(std::get<0>(copy) == std::get<0>(val));
    ASSERT(std::get<1>(copy) == std::get<1>(val));
}

TEST(testProductRoundTripEmpty)
{
    std::tuple<> val{};
    std::tuple<> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
}

TEST(testProductRoundTripBinary)
{
    std::tuple<i32, std::string, f32> val{42, std::string{"binary"}, 2.5f};
    std::tuple<i32, std::string, f32> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    std::vector<u8> bin = writeSerialBinary(&w);
    std::optional<Serializer> rOpt = readSerialBinary(bin);
    ASSERT(rOpt.has_value());
    Serializer& r = *rOpt;
    ASSERT(serialize(&r, &copy));
    ASSERT(std::get<0>(copy) == std::get<0>(val));
    ASSERT(std::get<1>(copy) == std::get<1>(val));
    ASSERT(std::get<2>(copy) == std::get<2>(val));
}

TEST(testSumRoundTripFirstType)
{
    std::variant<i64, f64, std::string> val{i64{-42}};
    std::variant<i64, f64, std::string> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(std::holds_alternative<i64>(copy));
    ASSERT(std::get<i64>(copy) == -42);
}

TEST(testSumRoundTripSecondType)
{
    std::variant<i64, f64, std::string> val{f64{3.14}};
    std::variant<i64, f64, std::string> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(std::holds_alternative<f64>(copy));
    ASSERT(std::get<f64>(copy) == 3.14);
}

TEST(testSumRoundTripThirdType)
{
    std::variant<i64, f64, std::string> val{std::string{"world"}};
    std::variant<i64, f64, std::string> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(std::holds_alternative<std::string>(copy));
    ASSERT(std::get<std::string>(copy) == std::string{"world"});
}

TEST(testSumRoundTripEmpty)
{
    std::variant<i64, f64, std::string> val{};
    std::variant<i64, f64, std::string> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(std::holds_alternative<i64>(copy));
    ASSERT(std::get<i64>(copy) == 0);
}

TEST(testSumRoundTripBinary)
{
    std::variant<i64, f64, std::string> val{f64{99.9}};
    std::variant<i64, f64, std::string> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    std::vector<u8> bin = writeSerialBinary(&w);
    std::optional<Serializer> rOpt = readSerialBinary(bin);
    ASSERT(rOpt.has_value());
    Serializer& r = *rOpt;
    ASSERT(serialize(&r, &copy));
    ASSERT(std::holds_alternative<f64>(copy));
    ASSERT(std::get<f64>(copy) == 99.9);
}

TEST(testMaybeRoundTripWithValue)
{
    std::optional<i64> val{42};
    std::optional<i64> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.has_value());
    ASSERT(*copy == 42);
}

TEST(testMaybeRoundTripWithString)
{
    std::optional<std::string> val{std::string{"maybe"}};
    std::optional<std::string> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.has_value());
    ASSERT(*copy == *val);
}

TEST(testMaybeRoundTripEmpty)
{
    std::optional<i64> val{};
    std::optional<i64> copy{std::optional<i64>{99}};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(!copy.has_value());
}

TEST(testMaybeRoundTripBinary)
{
    std::optional<f32> val{2.5f};
    std::optional<f32> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    std::vector<u8> bin = writeSerialBinary(&w);
    std::optional<Serializer> rOpt = readSerialBinary(bin);
    ASSERT(rOpt.has_value());
    Serializer& r = *rOpt;
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.has_value());
    ASSERT(*copy == 2.5f);
}

TEST(testSerializeEmptyString)
{
    std::string val = std::string{""};
    std::string copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeEmptyTrivialVector)
{
    std::vector<u8> val{};
    std::vector<u8> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.size() == val.size());
}

TEST(testSerializeEmptyArray)
{
    std::vector<u32> val{};
    std::vector<u32> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.size() == 0);
}

TEST(testSerializeEmptySet)
{
    HashSet<u32> val{};
    HashSet<u32> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.count == 0);
}

TEST(testSerializeEmptyMap)
{
    HashMap<u32, f32> val{};
    HashMap<u32, f32> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.count == 0);
}

TEST(testSerializeEmptyMaybe)
{
    std::optional<i64> val{};
    std::optional<i64> copy{std::optional<i64>{99}};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(!copy.has_value());
}

TEST(testSerializeNestedProduct)
{
    std::tuple<std::tuple<i64, f64>, bool> val{std::tuple<i64, f64>{-7, 2.5f}, true};
    std::tuple<std::tuple<i64, f64>, bool> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(std::get<0>(std::get<0>(copy)) == -7);
    ASSERT(std::get<1>(std::get<0>(copy)) == 2.5f);
    ASSERT(std::get<1>(copy) == true);
}

TEST(testSerializeNestedMaybe)
{
    std::optional<std::string> val{std::string{"nested"}};
    std::optional<std::string> copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy.has_value());
    ASSERT(*copy == *val);
}

TEST(testSerializeBoolFalse)
{
    bool val = false;
    bool copy = true;

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeLargeString)
{
    std::string builder{};
    for (u32 i = 0; i < 2048; ++i)
        builder += "x";
    std::string val = builder;
    std::string copy{};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));
    Serializer r = serialReader(std::move(w.root));
    ASSERT(serialize(&r, &copy));
    ASSERT(copy == val);
    ASSERT(copy.size() == 2048);
}

TEST(testSerializeReadStringAsIntError)
{
    std::string val = std::string{"hello"};

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));

    i64 copy = 0;
    Serializer r = serialReader(std::move(w.root));
    setError("");
    ASSERT(!serialize(&r, &copy));
    ASSERT(getError().length() > 0);
}

TEST(testSerializeReadMissingChildError)
{
    i64 val = 42;

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));

    struct Data {
        i64 a;
        i64 b;
    };
    Data copy{};

    Serializer r = serialReader(std::move(w.root));
    setError("");
    ASSERT(!serializeObject(&r, &copy.a, &copy.b));
    ASSERT(getError().length() > 0);
}

TEST(testSerializeReadSizeMismatchError)
{
    u32 val = 0xCAFEBABE;

    Serializer w = serialWriter();
    ASSERT(serialize(&w, &val));

    i64 copy = 0;
    Serializer r = serialReader(std::move(w.root));
    setError("");
    ASSERT(!serialize(&r, &copy));
    ASSERT(getError().length() > 0);
}

TEST(testSerializeNestedFailurePropagates)
{
    i32 inner = 42;

    Serializer w = serialWriter();
    serializeBegin(&w);
    ASSERT(serialize(&w, &inner));
    serializeEnd(&w);
    f64 copy = 0.0;
    Serializer r = serialReader(std::move(w.root));
    setError("");
    serializeBegin(&r);
    ASSERT(!serialize(&r, &copy));
    serializeEnd(&r);
    ASSERT(getError().length() > 0);
}


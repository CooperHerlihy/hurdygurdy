#include "tests.hpp"
#include "hg/serialization.hpp"

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

using namespace hg;

TEST(testSerializeBool)
{
    bool val = true;
    bool copy = false;

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeU32)
{
    u32 val = 0xDEADBEEF;
    u32 copy = 0;

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeVec3)
{
    Vec3 val{1.0f, 2.0f, 3.0f};
    Vec3 copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
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

    auto serializeData = [](Serializer& s, Data* val)
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
    Serializer w = Serializer::writer();
    ASSERT(serializeData(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serializeData(r, &copy));
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
    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy.id == savedId);
    ASSERT(copy.valid == true);
}

TEST(testSerializeCArray)
{
    u32 val[4] = {10, 20, 30, 40};
    u32 copy[4] = {};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy[0] == 10);
    ASSERT(copy[1] == 20);
    ASSERT(copy[2] == 30);
    ASSERT(copy[3] == 40);
}

TEST(testSerializeTrivialArray)
{
    std::array<u32, 4> val{10, 20, 30, 40};
    std::array<u32, 4> copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeString)
{
    std::string val = std::string{"hello world"};
    std::string copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeMultipleRootValues)
{
    i32 a = -7;
    f64 b = 2.5;
    std::string c = "third";

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &a));
    ASSERT(serialize(w, &b));
    ASSERT(serialize(w, &c));

    i32 aCopy = 0;
    f64 bCopy = 0.0;
    std::string cCopy{};

    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &aCopy));
    ASSERT(serialize(r, &bCopy));
    ASSERT(serialize(r, &cCopy));
    ASSERT(aCopy == a);
    ASSERT(bCopy == b);
    ASSERT(cCopy == c);
}

TEST(testSerializeTrivialVector)
{
    std::vector<u8> val{0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE, 0xBA, 0xBE};
    std::vector<u8> copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy.size() == val.size());
    ASSERT(memcmp(copy.data(), val.data(), val.size()) == 0);
}

TEST(testSerializeVectorOfStrings)
{
    std::vector<std::string> val{"one", "two", "three"};
    std::vector<std::string> copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeVectorOfVectors)
{
    std::vector<std::vector<u32>> val{{1, 2}, {}, {3}};
    std::vector<std::vector<u32>> copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeVectorReadReplacesContents)
{
    std::vector<u32> val{4, 5};
    std::vector<u32> copy{1, 2, 3, 4, 5, 6};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeUniquePtr)
{
    std::unique_ptr<i32> val = std::make_unique<i32>(42);
    std::unique_ptr<i32> copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy != nullptr);
    ASSERT(*copy == 42);
    ASSERT(copy.get() != val.get());
}

TEST(testSerializeUniquePtrNullRoundTrip)
{
    std::unique_ptr<i32> val{};
    std::unique_ptr<i32> copy = std::make_unique<i32>(99);

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy == nullptr);
}

TEST(testSerializeSet)
{
    HashSet<u32> val{};
    val.add(10);
    val.add(20);
    val.add(30);

    HashSet<u32> copy{};
    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy.size() == val.size());
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
    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy.size() == val.size());
    ASSERT(*copy.get(1u) == 1.5f);
    ASSERT(*copy.get(2u) == 2.5f);
    ASSERT(*copy.get(3u) == 3.5f);
}

TEST(testSerializeMapReadClearsExisting)
{
    HashMap<u32, f32> val{};
    val.add(1, 1.5f);

    HashMap<u32, f32> copy{};
    copy.add(2, 2.5f);
    copy.add(3, 3.5f);

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy.size() == 1);
    ASSERT(*copy.get(1u) == 1.5f);
    ASSERT(copy.get(2u) == nullptr);
}

TEST(testBinaryRoundTripPrimitive)
{
    struct Data {
        i64 a;
        f64 b;
        bool c;
    };

    auto serializeData = [](Serializer& s, Data* val)
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
    Serializer w = Serializer::writer();
    ASSERT(serializeData(w, &val));
    std::vector<byte> bin = w.takeFromWriter();
    ASSERT(isSerialData(bin));
    Serializer r = Serializer::reader(bin);
    ASSERT(serializeData(r, &copy));
    ASSERT(copy.a == val.a);
    ASSERT(copy.b == val.b);
    ASSERT(copy.c == val.c);
}

TEST(testBinaryRoundTripTrivialVector)
{
    struct Data {
        i64 a;
        std::vector<u8> b;
    };

    auto serializeData = [](Serializer& s, Data* val)
    {
        return serializeObject(s,
            &val->a,
            &val->b);
    };
    Data val{};
    val.a = -123;
    val.b = {0xDE, 0xAD, 0xBE, 0xEF};

    Data copy{};
    Serializer w = Serializer::writer();
    ASSERT(serializeData(w, &val));
    std::vector<byte> bin = w.takeFromWriter();
    ASSERT(isSerialData(bin));
    Serializer r = Serializer::reader(bin);
    ASSERT(serializeData(r, &copy));
    ASSERT(copy.a == val.a);
    ASSERT(copy.b.size() == val.b.size());
    ASSERT(memcmp(copy.b.data(), val.b.data(), val.b.size()) == 0);
}

TEST(testSerialDataSignatureConstexpr)
{
    static_assert(sizeof(serialDataSignature) == 2);
    static_assert(serialDataSignature[0] == 'H');
    static_assert(serialDataSignature[1] == 'G');
}

TEST(testIsSerialData)
{
    std::vector<byte> empty{};
    ASSERT(!isSerialData(empty));

    std::vector<byte> one{byte{'H'}};
    ASSERT(!isSerialData(one));

    std::vector<byte> signature{byte{'H'}, byte{'G'}};
    ASSERT(isSerialData(signature));

    std::vector<byte> payload{byte{'H'}, byte{'G'}, byte{0x01}, byte{0x02}};
    ASSERT(isSerialData(payload));

    std::vector<byte> wrong{byte{'X'}, byte{'G'}};
    ASSERT(!isSerialData(wrong));

    std::vector<byte> reversed{byte{'G'}, byte{'H'}};
    ASSERT(!isSerialData(reversed));

    std::vector<byte> extended{byte{'H'}, byte{'G'}, byte{'S'}, byte{'D'}};
    ASSERT(isSerialData(extended));
}

TEST(testWriterStartsWithSignature)
{
    Serializer w = Serializer::writer();
    ASSERT(w.isWriting());
    ASSERT(!w.isReading());

    std::span<const byte> bin = w.get();
    ASSERT(bin.size() == sizeof(serialDataSignature));
    ASSERT(isSerialData(bin));
}

TEST(testWriterGetMatchesTakeFromWriter)
{
    Serializer w = Serializer::writer();
    i32 val = 5;
    ASSERT(serialize(w, &val));

    std::span<const byte> view = w.get();
    std::vector<byte> before{view.begin(), view.end()};

    std::vector<byte> taken = w.takeFromWriter();
    ASSERT(before == taken);
    ASSERT(isSerialData(taken));
}

TEST(testTakeFromWriterResetsWriter)
{
    Serializer w = Serializer::writer();
    i32 val = 5;
    ASSERT(serialize(w, &val));
    std::vector<byte> bin = w.takeFromWriter();
    ASSERT(bin.size() > sizeof(serialDataSignature));

    ASSERT(!w.isWriting());
    ASSERT(w.isReading());
    ASSERT(w.get().empty());
}

TEST(testTakeFromWriterOnReader)
{
    std::vector<byte> bin{byte{'H'}, byte{'G'}, byte{1}};
    Serializer r = Serializer::reader(bin);

    std::vector<byte> taken = r.takeFromWriter();
    ASSERT(taken.empty());
    ASSERT(r.isReading());

    byte out{};
    ASSERT(serialize(r, &out));
    ASSERT(out == byte{1});
}

TEST(testReaderWithoutSignatureStartsAtZero)
{
    u32 expected = 0xDEADBEEF;
    std::array<byte, sizeof(u32)> raw{};
    memcpy(raw.data(), &expected, sizeof(expected));

    Serializer r = Serializer::reader(raw);
    u32 copy = 0;
    ASSERT(serialize(r, &copy));
    ASSERT(copy == expected);
}

TEST(testReaderTrailingDataIgnored)
{
    Serializer w = Serializer::writer();
    i32 val = 5;
    ASSERT(serialize(w, &val));
    std::vector<byte> bin = w.takeFromWriter();
    bin.push_back(byte{0xFF});
    bin.push_back(byte{0xFF});

    Serializer r = Serializer::reader(bin);
    i32 copy = 0;
    ASSERT(serialize(r, &copy));
    ASSERT(copy == val);
}

TEST(testSerializeDataWriterAppends)
{
    std::array<byte, 3> first{byte{1}, byte{2}, byte{3}};
    std::array<byte, 2> second{byte{4}, byte{5}};

    Serializer w = Serializer::writer();
    ASSERT(w.serializeData(first));
    ASSERT(w.get().size() == sizeof(serialDataSignature) + first.size());
    ASSERT(w.serializeData(second));
    ASSERT(w.get().size() == sizeof(serialDataSignature) + first.size() + second.size());
}

TEST(testSerializeDataTruncated)
{
    std::vector<byte> bin{byte{'H'}, byte{'G'}, byte{9}};
    Serializer r = Serializer::reader(bin);

    std::array<byte, 2> out{};
    setError("");
    ASSERT(!r.serializeData(out));
    ASSERT(getError().length() > 0);
    ASSERT(!r.isWriting());
}

TEST(testReadSerialBinaryRejectsGarbage)
{
    ASSERT(!serialLoadFromBinary<i32>({}).has_value());

    std::vector<byte> garbage(32, byte{0});
    ASSERT(!serialLoadFromBinary<i32>(garbage).has_value());
}

TEST(testReadSerialBinaryRejectsTruncation)
{
    Serializer w = Serializer::writer();
    std::vector<u32> val{1, 2, 3};
    ASSERT(serialize(w, &val));
    std::vector<byte> bin = w.takeFromWriter();
    ASSERT(serialLoadFromBinary<std::vector<u32>>(bin).has_value());

    // Every truncation must fail: the reader validates every payload range
    for (u64 cut = 1; cut < bin.size(); ++cut)
    {
        std::vector<byte> trunc(bin.begin(), bin.end() - assertCast<i64>(cut));
        ASSERT(!serialLoadFromBinary<std::vector<u32>>(trunc).has_value());
    }
}

TEST(testProductRoundTripPrimitive)
{
    std::tuple<i64, f64, bool> val{-42, 3.14, true};
    std::tuple<i64, f64, bool> copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(std::get<0>(copy) == std::get<0>(val));
    ASSERT(std::get<1>(copy) == std::get<1>(val));
    ASSERT(std::get<2>(copy) == std::get<2>(val));
}

TEST(testProductRoundTripEmpty)
{
    std::tuple<> val{};
    std::tuple<> copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
}

TEST(testSumRoundTripFirstType)
{
    std::variant<i64, f64, std::string> val{i64{-42}};
    std::variant<i64, f64, std::string> copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(std::holds_alternative<i64>(copy));
    ASSERT(std::get<i64>(copy) == -42);
}

TEST(testSumRoundTripChangesType)
{
    std::variant<i64, f64, std::string> val{std::string{"changed"}};
    std::variant<i64, f64, std::string> copy{i64{99}};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(std::holds_alternative<std::string>(copy));
    ASSERT(std::get<std::string>(copy) == std::string{"changed"});
}

TEST(testSumRoundTripEmpty)
{
    std::variant<i64, f64, std::string> val{};
    std::variant<i64, f64, std::string> copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(std::holds_alternative<i64>(copy));
    ASSERT(std::get<i64>(copy) == 0);
}

TEST(testMaybeRoundTripWithValue)
{
    std::optional<i64> val{42};
    std::optional<i64> copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy.has_value());
    ASSERT(*copy == 42);
}

TEST(testMaybeRoundTripEmpty)
{
    std::optional<i64> val{};
    std::optional<i64> copy{std::optional<i64>{99}};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(!copy.has_value());
}

TEST(testSerializeEmptyString)
{
    std::string val = std::string{""};
    std::string copy = std::string{"not empty"};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy == val);
    ASSERT(copy.empty());
}

TEST(testSerializeEmptyTrivialVector)
{
    std::vector<u8> val{};
    std::vector<u8> copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy.size() == val.size());
}

TEST(testSerializeLargeString)
{
    std::string builder{};
    for (u32 i = 0; i < 2048; ++i)
        builder += "x";
    std::string val = builder;
    std::string copy{};

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &val));
    Serializer r = Serializer::reader(w.get());
    ASSERT(serialize(r, &copy));
    ASSERT(copy == val);
    ASSERT(copy.size() == 2048);
}

TEST(testSerializeDataDirect)
{
    byte data[]{byte{0x01}, byte{0x02}, byte{0x03}, byte{0x04}};

    Serializer w = Serializer::writer();
    ASSERT(w.serializeData(data));
    std::vector<byte> bin = w.takeFromWriter();

    Serializer r = Serializer::reader(bin);
    byte out[4] = {};
    ASSERT(r.serializeData(std::span<byte>{out}));
    ASSERT(memcmp(out, data, 4) == 0);
}

TEST(testSerialLoadStoreToBinary)
{
    std::vector<u32> val{10, 20, 30};

    std::vector<byte> bin = serialStoreToBinary(val);
    ASSERT(isSerialData(bin));
    std::optional<std::vector<u32>> copy = serialLoadFromBinary<std::vector<u32>>(bin);
    ASSERT(copy.has_value());
    ASSERT(*copy == val);

    std::vector<byte> garbage(16, byte{0});
    ASSERT(!serialLoadFromBinary<std::vector<u32>>(garbage).has_value());
}

static constexpr const char* serialTestPath = "/tmp/hg_serialization_test/serial.bin";

struct SerialFileData {
    i64 a;
    std::string b;
    std::vector<f64> c;
};

bool serialize(Serializer& s, SerialFileData* val)
{
    return serializeObject(s, &val->a, &val->b, &val->c);
}

TEST(testSerialStoreLoad)
{
    std::filesystem::create_directories("/tmp/hg_serialization_test");

    SerialFileData val{};
    val.a = -777;
    val.b = "file round trip";
    val.c = {1.5, -2.5, 3.5};

    setError("");
    ASSERT(serialStore(val, serialTestPath));

    std::optional<std::vector<byte>> bin = binaryLoad(serialTestPath);
    ASSERT(bin.has_value());
    ASSERT(isSerialData(*bin));

    setError("");
    std::optional<SerialFileData> loaded = serialLoad<SerialFileData>(serialTestPath);
    ASSERT(loaded.has_value());
    ASSERT(loaded->a == val.a);
    ASSERT(loaded->b == val.b);
    ASSERT(loaded->c == val.c);
}

TEST(testSerialLoadMissingFile)
{
    setError("");
    ASSERT(!serialLoad<i32>("/tmp/hg_serialization_test/nonexistent.bin").has_value());
    ASSERT(getError().length() > 0);
}

TEST(testSerializeNestedFailurePropagates)
{
    i32 inner = 42;

    Serializer w = Serializer::writer();
    ASSERT(serialize(w, &inner));

    f64 copy = 0.0;
    Serializer r = Serializer::reader(w.get());
    setError("");
    ASSERT(!serialize(r, &copy));
    ASSERT(getError().length() > 0);
}

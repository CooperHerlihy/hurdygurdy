#include "tests.hpp"
#include "hg/assets.hpp"

#include <filesystem>
#include <string>
#include <vector>

using namespace hg;

using Binary = std::vector<u8>;

static constexpr const char* testDir = "/tmp/hg_asset_test";

static void ensureTestDir()
{
    std::filesystem::create_directories(testDir);
}

static void writeFile(const char* name, const void* data, u64 size)
{
    std::string path = std::string{testDir} + "/" + name;
    Binary bin{static_cast<const u8*>(data), static_cast<const u8*>(data) + size};
    ASSERT(store(bin, path));
}

TEST(testAssetDefault)
{
    Asset<Binary> a;
    ASSERT(a.data == nullptr);
}

TEST(testAssetNewAsset)
{
    Asset<Binary> a = newAsset<Binary>();
    ASSERT(a.data != nullptr);
    ASSERT(a.data->refCount == 1);
    ASSERT(a->empty());
    ASSERT(a->size() == 0);
}

TEST(testAssetClone)
{
    Asset<Binary> a = newAsset<Binary>();
    ASSERT(a.data->refCount == 1);
    {
        Asset<Binary> b = a.clone();
        ASSERT(b.data == a.data);
        ASSERT(a.data->refCount == 2);
    }
    ASSERT(a.data->refCount == 1);
}

TEST(testAssetMoveConstruct)
{
    Asset<Binary> a = newAsset<Binary>();
    AssetData<Binary>* ptr = a.data;
    Asset<Binary> b = std::move(a);
    ASSERT(a.data == nullptr);
    ASSERT(b.data == ptr);
    ASSERT(b.data->refCount == 1);
}

TEST(testAssetMoveAssign)
{
    Asset<Binary> a = newAsset<Binary>();
    Asset<Binary> b = newAsset<Binary>();
    AssetData<Binary>* ptrA = a.data;
    b = std::move(a);
    ASSERT(a.data == nullptr);
    ASSERT(b.data == ptrA);
}

TEST(testAssetComparison)
{
    Asset<Binary> a;
    Asset<Binary> b;
    ASSERT(a == b);
    ASSERT(a == nullptr);
    ASSERT(!(a != nullptr));

    Asset<Binary> c = newAsset<Binary>();
    ASSERT(c != nullptr);
    ASSERT(!(c == nullptr));
    ASSERT(!(c == a));
    ASSERT(c != a);
}

TEST(testAssetLoad)
{
    ensureTestDir();
    const char data[] = "hello binary world";
    writeFile("load_test", data, sizeof(data));

    std::string path = std::string{testDir} + "/load_test";
    Asset<Binary> a = load<Binary>(path);
    ASSERT(a.data != nullptr);
    ASSERT(a->size() == sizeof(data));
    ASSERT(memcmp(a->data(), data, a->size()) == 0);
}

TEST(testAssetLoadCached)
{
    ensureTestDir();
    writeFile("cached", "cached", 7);

    std::string path = std::string{testDir} + "/cached";
    Asset<Binary> a = load<Binary>(path);
    Asset<Binary> b = load<Binary>(path);
    ASSERT(b.data == a.data);
    ASSERT(a.data->refCount == 2);
}

TEST(testAssetLoadSeparate)
{
    ensureTestDir();
    writeFile("sep_a", "aaaa", 5);
    writeFile("sep_b", "bbbb", 5);

    Asset<Binary> a = load<Binary>(std::string{testDir} + "/sep_a");
    Asset<Binary> b = load<Binary>(std::string{testDir} + "/sep_b");
    ASSERT(a.data != b.data);
}

TEST(testAssetReload)
{
    ensureTestDir();
    const char data1[] = "version one";
    writeFile("reload", data1, sizeof(data1));

    std::string path = std::string{testDir} + "/reload";
    Asset<Binary> a = load<Binary>(path);
    ASSERT(a->size() == sizeof(data1));
    ASSERT(memcmp(a->data(), data1, a->size()) == 0);

    const char data2[] = "version two with more data";
    writeFile("reload", data2, sizeof(data2));

    reload(a);
    ASSERT(a->size() == sizeof(data2));
    ASSERT(memcmp(a->data(), data2, a->size()) == 0);
}

TEST(testAssetReloadNull)
{
    Asset<Binary> a;
    reload(a);
}

TEST(testAssetCacheRemoval)
{
    ensureTestDir();
    writeFile("drop", "drop", 5);
    {
        Asset<Binary> a = load<Binary>(std::string{testDir} + "/drop");
        ASSERT(AssetManager<Binary>::map.has(a.data->path));
    }
    ASSERT(!AssetManager<Binary>::map.has(std::string{testDir} + "/drop"));
}

TEST(testAssetBinaryRoundTrip)
{
    ensureTestDir();
    const char data[] = "round trip data";
    writeFile("roundtrip", data, sizeof(data));

    Asset<Binary> a = load<Binary>(std::string{testDir} + "/roundtrip");
    ASSERT(a->size() == sizeof(data));
    ASSERT(memcmp(a->data(), data, a->size()) == 0);
}

TEST(testAssetRaii)
{
    ensureTestDir();
    writeFile("raii", "raii", 5);

    Asset<Binary> a = load<Binary>(std::string{testDir} + "/raii");
    const u8* heapPtr = a->data();
    ASSERT(heapPtr != nullptr);
    {
        Asset<Binary> b = load<Binary>(std::string{testDir} + "/raii");
        ASSERT(b.data == a.data);
    }
    ASSERT(a->data() == heapPtr);
    a = Asset<Binary>{};
}

TEST(testAssetMultipleLoads)
{
    ensureTestDir();
    const char dataA[] = "file A content";
    const char dataB[] = "file B content";
    writeFile("multi_a", dataA, sizeof(dataA));
    writeFile("multi_b", dataB, sizeof(dataB));

    Asset<Binary> a = load<Binary>(std::string{testDir} + "/multi_a");
    Asset<Binary> b = load<Binary>(std::string{testDir} + "/multi_b");

    ASSERT(a->size() == sizeof(dataA));
    ASSERT(b->size() == sizeof(dataB));
    ASSERT(memcmp(a->data(), dataA, a->size()) == 0);
    ASSERT(memcmp(b->data(), dataB, b->size()) == 0);
    ASSERT(a.data != b.data);
}

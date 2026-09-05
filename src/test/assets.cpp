#include "tests.hpp"
#include "hg/assets.hpp"

#include <cstdio>
#include <sys/stat.h>

using namespace hg;

static void ensureTestDir()
{
    mkdir("/tmp/hg_asset_test", 0755);
}

static void writeFile(const char* name, const void* data, u64 size)
{
    char path[256];
    std::snprintf(path, sizeof(path), "/tmp/hg_asset_test/%s", name);
    BinaryView bv{data, size};
    ASSERT(binaryStore(bv, path));
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
    ASSERT(a->data == nullptr);
    ASSERT(a->size == 0);
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

    Asset<Binary> a = load<Binary>(StringView{"/tmp/hg_asset_test/load_test"});
    ASSERT(a.data != nullptr);
    ASSERT(a->size == sizeof(data));
    ASSERT(memcmp(a->data, data, a->size) == 0);
}

TEST(testAssetLoadCached)
{
    ensureTestDir();
    writeFile("cached", "cached", 7);

    Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/cached");
    Asset<Binary> b = load<Binary>("/tmp/hg_asset_test/cached");
    ASSERT(b.data == a.data);
    ASSERT(a.data->refCount == 2);
}

TEST(testAssetLoadSeparate)
{
    ensureTestDir();
    writeFile("sep_a", "aaaa", 5);
    writeFile("sep_b", "bbbb", 5);

    Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/sep_a");
    Asset<Binary> b = load<Binary>("/tmp/hg_asset_test/sep_b");
    ASSERT(a.data != b.data);
}

TEST(testAssetReload)
{
    ensureTestDir();
    const char data1[] = "version one";
    writeFile("reload", data1, sizeof(data1));

    Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/reload");
    ASSERT(a->size == sizeof(data1));
    ASSERT(memcmp(a->data, data1, a->size) == 0);

    const char data2[] = "version two with more data";
    writeFile("reload", data2, sizeof(data2));

    reload(a);
    ASSERT(a->size == sizeof(data2));
    ASSERT(memcmp(a->data, data2, a->size) == 0);
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
        Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/drop");
        ASSERT(AssetManager<Binary>::map.has(a.data->path));
    }
    ASSERT(!AssetManager<Binary>::map.has("/tmp/hg_asset_test/drop"));
}

TEST(testAssetBinaryRoundTrip)
{
    ensureTestDir();
    const char data[] = "round trip data";
    writeFile("roundtrip", data, sizeof(data));

    Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/roundtrip");
    ASSERT(a->size == sizeof(data));
    ASSERT(memcmp(a->data, data, a->size) == 0);
}

TEST(testAssetRaii)
{
    ensureTestDir();
    writeFile("raii", "raii", 5);
    Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/raii");
    void* heapPtr = a->data;
    ASSERT(heapPtr != nullptr);
    {
        Asset<Binary> b = load<Binary>("/tmp/hg_asset_test/raii");
        ASSERT(b.data == a.data);
    }
    ASSERT(a->data == heapPtr);
    a = Asset<Binary>{};
}

TEST(testAssetMultipleLoads)
{
    ensureTestDir();
    const char dataA[] = "file A content";
    const char dataB[] = "file B content";
    writeFile("multi_a", dataA, sizeof(dataA));
    writeFile("multi_b", dataB, sizeof(dataB));

    Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/multi_a");
    Asset<Binary> b = load<Binary>("/tmp/hg_asset_test/multi_b");

    ASSERT(a->size == sizeof(dataA));
    ASSERT(b->size == sizeof(dataB));
    ASSERT(memcmp(a->data, dataA, a->size) == 0);
    ASSERT(memcmp(b->data, dataB, b->size) == 0);
    ASSERT(a.data != b.data);
}

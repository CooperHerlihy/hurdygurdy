#include "tests.hpp"
#include "hg/assets.hpp"

#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define mkdir(path, mode) _mkdir(path)
#else
#define mkdir(path, mode) mkdir(path, mode)
#endif

using namespace hg;

static char testDir[256];

static void ensureTestDir()
{
    if (testDir[0])
        return;

#ifdef _WIN32
    char tmpDir[256];
    DWORD len = GetTempPathA(sizeof(tmpDir), tmpDir);
    if (len == 0)
        std::snprintf(testDir, sizeof(testDir), "C:/tmp");
    else
        std::snprintf(testDir, sizeof(testDir), "%shg_asset_test", tmpDir);
#else
    std::snprintf(testDir, sizeof(testDir), "/tmp/hg_asset_test");
#endif

    mkdir(testDir, 0755);
}

static char* testPath(char* buf, u64 bufSize, const char* name)
{
    std::snprintf(buf, bufSize, "%s/%s", testDir, name);
    return buf;
}

static void writeFile(const char* name, const void* data, u64 size)
{
    char path[256];
    testPath(path, sizeof(path), name);
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

    char path[256];
    Asset<Binary> a = load<Binary>(testPath(path, sizeof(path), "load_test"));
    ASSERT(a.data != nullptr);
    ASSERT(a->size == sizeof(data));
    ASSERT(memcmp(a->data, data, a->size) == 0);
}

TEST(testAssetLoadCached)
{
    ensureTestDir();
    writeFile("cached", "cached", 7);

    char path[256];
    Asset<Binary> a = load<Binary>(testPath(path, sizeof(path), "cached"));
    Asset<Binary> b = load<Binary>(path);
    ASSERT(b.data == a.data);
    ASSERT(a.data->refCount == 2);
}

TEST(testAssetLoadSeparate)
{
    ensureTestDir();
    writeFile("sep_a", "aaaa", 5);
    writeFile("sep_b", "bbbb", 5);

    char pathA[256], pathB[256];
    Asset<Binary> a = load<Binary>(testPath(pathA, sizeof(pathA), "sep_a"));
    Asset<Binary> b = load<Binary>(testPath(pathB, sizeof(pathB), "sep_b"));
    ASSERT(a.data != b.data);
}

TEST(testAssetReload)
{
    ensureTestDir();
    const char data1[] = "version one";
    writeFile("reload", data1, sizeof(data1));

    char path[256];
    Asset<Binary> a = load<Binary>(testPath(path, sizeof(path), "reload"));
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
        char path[256];
        Asset<Binary> a = load<Binary>(testPath(path, sizeof(path), "drop"));
        ASSERT(AssetManager<Binary>::map.has(a.data->path));
    }
    char path[256];
    ASSERT(!AssetManager<Binary>::map.has(StringView{testPath(path, sizeof(path), "drop")}));
}

TEST(testAssetBinaryRoundTrip)
{
    ensureTestDir();
    const char data[] = "round trip data";
    writeFile("roundtrip", data, sizeof(data));

    char path[256];
    Asset<Binary> a = load<Binary>(testPath(path, sizeof(path), "roundtrip"));
    ASSERT(a->size == sizeof(data));
    ASSERT(memcmp(a->data, data, a->size) == 0);
}

TEST(testAssetRaii)
{
    ensureTestDir();
    writeFile("raii", "raii", 5);
    char path[256];
    Asset<Binary> a = load<Binary>(testPath(path, sizeof(path), "raii"));
    void* heapPtr = a->data;
    ASSERT(heapPtr != nullptr);
    {
        Asset<Binary> b = load<Binary>(path);
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

    char pathA[256], pathB[256];
    Asset<Binary> a = load<Binary>(testPath(pathA, sizeof(pathA), "multi_a"));
    Asset<Binary> b = load<Binary>(testPath(pathB, sizeof(pathB), "multi_b"));

    ASSERT(a->size == sizeof(dataA));
    ASSERT(b->size == sizeof(dataB));
    ASSERT(memcmp(a->data, dataA, a->size) == 0);
    ASSERT(memcmp(b->data, dataB, b->size) == 0);
    ASSERT(a.data != b.data);
}

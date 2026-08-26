#include "tests.hpp"
#include "hg/assets.hpp"

#include <cstdio>
#include <sys/stat.h>

static void ensureTestDir()
{
    mkdir("/tmp/hg_asset_test", 0755);
}

static void writeFile(const char* name, const void* data, u64 size)
{
    char path[256];
    std::snprintf(path, sizeof(path), "/tmp/hg_asset_test/%s", name);
    BinaryView bv{data, size};
    TEST(binaryStore(bv, path));
}

void testAssets()
{
    ensureTestDir();

    // ============================================================================
    // Asset<T>
    // ============================================================================

    // Default-constructed Asset is null
    {
        Asset<Binary> a;
        TEST(a.data == nullptr);
    }

    // newAsset creates an empty unloaded asset
    {
        Asset<Binary> a = newAsset<Binary>();
        TEST(a.data != nullptr);
        TEST(a.data->refCount == 1);
        TEST(a->data == nullptr);
        TEST(a->size == 0);
    }

    // clone increments ref count, both share same data
    {
        Asset<Binary> a = newAsset<Binary>();
        TEST(a.data->refCount == 1);
        {
            Asset<Binary> b = a.clone();
            TEST(b.data == a.data);
            TEST(a.data->refCount == 2);
        }
        TEST(a.data->refCount == 1);
    }

    // Move construct transfers ownership
    {
        Asset<Binary> a = newAsset<Binary>();
        AssetData<Binary>* ptr = a.data;
        Asset<Binary> b = std::move(a);
        TEST(a.data == nullptr);
        TEST(b.data == ptr);
        TEST(b.data->refCount == 1);
    }

    // Move assign transfers ownership and destroys old
    {
        Asset<Binary> a = newAsset<Binary>();
        Asset<Binary> b = newAsset<Binary>();
        AssetData<Binary>* ptrA = a.data;
        b = std::move(a);
        TEST(a.data == nullptr);
        TEST(b.data == ptrA);
    }

    // Comparison operators
    {
        Asset<Binary> a;
        Asset<Binary> b;
        TEST(a == b);
        TEST(a == nullptr);
        TEST(!(a != nullptr));

        Asset<Binary> c = newAsset<Binary>();
        TEST(c != nullptr);
        TEST(!(c == nullptr));
        TEST(!(c == a));
        TEST(c != a);
    }

    // ============================================================================
    // Binary asset loading from files
    // ============================================================================

    // load reads a file from disk
    {
        const char data[] = "hello binary world";
        writeFile("load_test", data, sizeof(data));

        Asset<Binary> a = load<Binary>(StringView{"/tmp/hg_asset_test/load_test"});
        TEST(a.data != nullptr);
        TEST(a->size == sizeof(data));
        TEST(memcmp(a->data, data, a->size) == 0);
    }

    // load returns cached asset for the same path
    {
        writeFile("cached", "cached", 7);

        Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/cached");
        Asset<Binary> b = load<Binary>("/tmp/hg_asset_test/cached");
        TEST(b.data == a.data);
        TEST(a.data->refCount == 2);
    }

    // load creates separate entries for different paths
    {
        writeFile("sep_a", "aaaa", 5);
        writeFile("sep_b", "bbbb", 5);

        Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/sep_a");
        Asset<Binary> b = load<Binary>("/tmp/hg_asset_test/sep_b");
        TEST(a.data != b.data);
    }

    // reload re-reads the file from disk
    {
        const char data1[] = "version one";
        writeFile("reload", data1, sizeof(data1));

        Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/reload");
        TEST(a->size == sizeof(data1));
        TEST(memcmp(a->data, data1, a->size) == 0);

        const char data2[] = "version two with more data";
        writeFile("reload", data2, sizeof(data2));

        reload(a);
        TEST(a->size == sizeof(data2));
        TEST(memcmp(a->data, data2, a->size) == 0);
    }

    // reload on null asset is safe
    {
        Asset<Binary> a;
        reload(a);
    }

    // cache entry is removed when last reference is destroyed
    {
        writeFile("drop", "drop", 5);
        {
            Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/drop");
            TEST(AssetManager<Binary>::map.has(a.data->path));
        }
        TEST(!AssetManager<Binary>::map.has("/tmp/hg_asset_test/drop"));
    }

    // ============================================================================
    // binaryStore round-trip
    // ============================================================================

    // store then load produces identical data
    {
        const char data[] = "round trip data";
        writeFile("roundtrip", data, sizeof(data));

        Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/roundtrip");
        TEST(a->size == sizeof(data));
        TEST(memcmp(a->data, data, a->size) == 0);
    }

    // ============================================================================
    // RAII - assets free underlying heap data on destruction
    // ============================================================================

    // When asset is destroyed, the Binary destructor frees the heap data
    {
        writeFile("raii", "raii", 5);
        Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/raii");
        void* heapPtr = a->data;
        TEST(heapPtr != nullptr);
        {
            Asset<Binary> b = load<Binary>("/tmp/hg_asset_test/raii");
            TEST(b.data == a.data);
        }
        // a still alive, data still valid
        TEST(a->data == heapPtr);
        a = Asset<Binary>{};
        // data freed, but can't easily verify without ASan
    }

    // ============================================================================
    // Multiple independent loads of different files
    // ============================================================================

    {
        const char dataA[] = "file A content";
        const char dataB[] = "file B content";
        writeFile("multi_a", dataA, sizeof(dataA));
        writeFile("multi_b", dataB, sizeof(dataB));

        Asset<Binary> a = load<Binary>("/tmp/hg_asset_test/multi_a");
        Asset<Binary> b = load<Binary>("/tmp/hg_asset_test/multi_b");

        TEST(a->size == sizeof(dataA));
        TEST(b->size == sizeof(dataB));
        TEST(memcmp(a->data, dataA, a->size) == 0);
        TEST(memcmp(b->data, dataB, b->size) == 0);
        TEST(a.data != b.data);
    }
}

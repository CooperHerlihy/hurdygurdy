#include "tests.hpp"
#include "hg/assets.hpp"
#include "hg/error.hpp"

#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

using namespace hg;

static constexpr const char* testDir = "/tmp/hg_filesystem_test";

static void setupTestDir()
{
    std::filesystem::create_directories(testDir);
}

static std::string testPath(const char* name)
{
    return std::string{testDir} + "/" + name;
}

static void cleanupTestDir()
{
    std::filesystem::remove_all(testDir);
}

TEST(testLoadFile)
{
    setupTestDir();
    std::string path = testPath("loadme.bin");
    u32 val = 0xDEADBEEF;
    std::vector<u8> bin{reinterpret_cast<u8*>(&val), reinterpret_cast<u8*>(&val) + sizeof(val)};
    ASSERT(storeFile(bin, path));

    std::optional<std::vector<u8>> loaded = loadFile(path);
    ASSERT(loaded.has_value());
    ASSERT(loaded->size() == sizeof(val));
    u32 result = 0;
    memcpy(&result, loaded->data(), sizeof(result));
    ASSERT(result == 0xDEADBEEF);
    cleanupTestDir();
}

TEST(testLoadFileNonexistent)
{
    setError("");
    std::optional<std::vector<u8>> loaded = loadFile("/tmp/hg_nonexistent_file_12345.bin");
    ASSERT(!loaded.has_value());
    ASSERT(getError().length() > 0);
}

TEST(testStoreFileBadDirectory)
{
    std::string path = std::string{testDir} + "/hg_nonexistent_dir_12345/store.bin";
    std::vector<u8> bin{'h', 'e', 'l', 'l', 'o'};

    setError("");
    ASSERT(!storeFile(bin, path));
    ASSERT(getError().length() > 0);
}

TEST(testStoreFile)
{
    setupTestDir();
    std::string path = testPath("store.bin");
    std::vector<u8> bin{'h', 'e', 'l', 'l', 'o', ' ', 'w', 'o', 'r', 'l', 'd'};
    ASSERT(storeFile(bin, path));

    auto info = std::filesystem::file_size(path);
    ASSERT(info == 11);
    cleanupTestDir();
}

TEST(testStoreFileOverwrite)
{
    setupTestDir();
    std::string path = testPath("overwrite.bin");
    std::vector<u8> first{'f', 'i', 'r', 's', 't'};
    std::vector<u8> second{'s', 'e', 'c', 'o', 'n', 'd'};
    ASSERT(storeFile(first, path));
    ASSERT(storeFile(second, path));

    ASSERT(std::filesystem::file_size(path) == 6);

    std::optional<std::vector<u8>> loaded = loadFile(path);
    ASSERT(loaded.has_value());
    ASSERT(memcmp(loaded->data(), "second", 6) == 0);
    cleanupTestDir();
}

TEST(testStoreFileEmpty)
{
    setupTestDir();
    std::string path = testPath("empty.bin");
    ASSERT(storeFile({}, path));

    ASSERT(std::filesystem::file_size(path) == 0);
    cleanupTestDir();
}

TEST(testLoadAndStoreLargeFile)
{
    setupTestDir();
    std::string path = testPath("large.bin");

    u64 size = 65536;
    std::vector<u8> data(size);
    for (u64 i = 0; i < size; ++i)
        data[i] = static_cast<u8>(i & 0xFF);

    ASSERT(storeFile(data, path));

    std::optional<std::vector<u8>> loaded = loadFile(path);
    ASSERT(loaded.has_value());
    ASSERT(loaded->size() == size);
    for (u64 i = 0; i < size; ++i)
    {
        ASSERT((*loaded)[i] == static_cast<u8>(i & 0xFF));
    }
    cleanupTestDir();
}

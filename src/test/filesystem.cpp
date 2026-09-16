#include "tests.hpp"
#include "hg/filesystem.hpp"
#include "hg/error.hpp"

#include <cstdio>
#include <cstring>

#ifdef HG_PLATFORM_LINUX
#include <unistd.h>
#include <sys/stat.h>
#endif

using namespace hg;

#ifdef HG_PLATFORM_LINUX

static constexpr const char* testDir = "/tmp/hg_filesystem_test";

static void setupTestDir()
{
    mkdir(testDir, 0755);
}

static void cleanupTestDir()
{
    ArenaScope scratch = getScratch();
    Span<DirectoryEntry> entries = getFileInfoInDirectory(scratch, testDir);
    for (u64 i = 0; i < entries.count; ++i)
    {
        FilePath path{scratch};
        path.appendPath(testDir);
        path.appendPath(entries[i].name);
        if (entries[i].info.isDirectory)
            removeDirectoryRecursive(path);
        else
            removeFile(path);
    }
    removeDirectory(testDir);
}

static FilePath testPath(Arena* arena, const char* name)
{
    FilePath fp{arena};
    fp.appendPath(testDir);
    fp.appendPath(name);
    return fp;
}

// FilePath tests

TEST(testFilePathAppendPath)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo"};
    fp.appendPath("bar");
    ASSERT(fp == "/foo/bar");
}

TEST(testFilePathAppendPathTrailingSlash)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/"};
    fp.appendPath("bar");
    ASSERT(fp == "/foo/bar");
}

TEST(testFilePathAppendPathEmpty)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo"};
    fp.appendPath("");
    ASSERT(fp == "/foo/");
}

TEST(testFilePathGetFileName)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar.txt"};
    ASSERT(fp.getFileName() == "bar.txt");
}

TEST(testFilePathGetFileNameNoDir)
{
    Arena arena{4096};
    FilePath fp{&arena, "bar.txt"};
    ASSERT(fp.getFileName() == "bar.txt");
}

TEST(testFilePathGetFileNameDeepPath)
{
    Arena arena{4096};
    FilePath fp{&arena, "/a/b/c/d/file.cpp"};
    ASSERT(fp.getFileName() == "file.cpp");
}

TEST(testFilePathSetFileName)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/old.txt"};
    fp.setFileName("new.txt");
    ASSERT(fp == "/foo/new.txt");
}

TEST(testFilePathGetStem)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar.txt"};
    ASSERT(fp.getStem() == "bar");
}

TEST(testFilePathGetStemNoExt)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar"};
    ASSERT(fp.getStem() == "bar");
}

TEST(testFilePathGetStemMultipleDots)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar.test.txt"};
    ASSERT(fp.getStem() == "bar.test");
}

TEST(testFilePathSetStem)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/old.txt"};
    fp.setStem("new");
    ASSERT(fp == "/foo/new.txt");
}

TEST(testFilePathHasExtension)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar.txt"};
    ASSERT(fp.hasExtension());
}

TEST(testFilePathHasExtensionNoExt)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar"};
    ASSERT(!fp.hasExtension());
}

TEST(testFilePathHasExtensionDotOnly)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar."};
    ASSERT(fp.hasExtension());
}

TEST(testFilePathGetExtension)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar.txt"};
    ASSERT(fp.getExtension() == ".txt");
}

TEST(testFilePathGetExtensionNoExt)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar"};
    ASSERT(fp.getExtension() == "");
}

TEST(testFilePathSetExtension)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar.txt"};
    fp.setExtension(".cpp");
    ASSERT(fp == "/foo/bar.cpp");
}

TEST(testFilePathSetExtensionAdd)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar"};
    fp.setExtension(".txt");
    ASSERT(fp == "/foo/bar.txt");
}

TEST(testFilePathSetExtensionNoDot)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar"};
    fp.setExtension("txt");
    ASSERT(fp == "/foo/bar.txt");
}

TEST(testFilePathGetDirectoryPart)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar.txt"};
    ASSERT(fp.getDirectoryPart() == "/foo/");
}

TEST(testFilePathGetDirectoryPartDeep)
{
    Arena arena{4096};
    FilePath fp{&arena, "/a/b/c/file.txt"};
    ASSERT(fp.getDirectoryPart() == "/a/b/c/");
}

TEST(testFilePathGetDirectoryPartNoDir)
{
    Arena arena{4096};
    FilePath fp{&arena, "file.txt"};
    ASSERT(fp.getDirectoryPart() == "");
}

TEST(testFilePathIsAbsolute)
{
    Arena arena{4096};
    FilePath fp{&arena, "/foo/bar"};
    ASSERT(fp.isAbsolute());
}

TEST(testFilePathIsNotAbsolute)
{
    Arena arena{4096};
    FilePath fp{&arena, "foo/bar"};
    ASSERT(!fp.isAbsolute());
}

TEST(testFilePathEmpty)
{
    Arena arena{4096};
    FilePath fp{&arena};
    ASSERT(fp.length == 0);
    ASSERT(!fp.isAbsolute());
}

// Directory and file operation tests

TEST(testGetCurrentDirectory)
{
    Arena arena{4096};
    FilePath cwd = getCurrentDirectory(&arena);
    ASSERT(cwd.length > 0);
    ASSERT(cwd.isAbsolute());
}

TEST(testGetHomeDirectory)
{
    Arena arena{4096};
    FilePath home = getHomeDirectory(&arena);
    ASSERT(home.length > 0);
    ASSERT(home.isAbsolute());
}

TEST(testGetRootDirectory)
{
    Arena arena{4096};
    FilePath root = getRootDirectory(&arena);
    ASSERT(root == "/");
}

TEST(testMakeDirectory)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "testmkdir");
    ASSERT(makeDirectory(path));
    Maybe<FileInfo> info = getFileInfo(path);
    ASSERT(info.has);
    ASSERT(info->isDirectory);
    cleanupTestDir();
}

TEST(testMakeDirectoryRecursive)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "a/b/c/d");
    ASSERT(makeDirectoryRecursive(path));
    Maybe<FileInfo> info = getFileInfo(path);
    ASSERT(info.has);
    ASSERT(info->isDirectory);
    cleanupTestDir();
}

TEST(testMakeDirectoryRecursiveExisting)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "a/b/c");
    ASSERT(makeDirectoryRecursive(path));
    ASSERT(makeDirectoryRecursive(path));
    cleanupTestDir();
}

TEST(testRemoveDirectory)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "toremove");
    ASSERT(makeDirectory(path));
    ASSERT(removeDirectory(path));
    Maybe<FileInfo> info = getFileInfo(path);
    ASSERT(!info.has);
    cleanupTestDir();
}

TEST(testRemoveDirectoryRecursive)
{
    setupTestDir();
    Arena arena{4096};
    FilePath inner = testPath(&arena, "recursive/a/b");
    ASSERT(makeDirectoryRecursive(inner));
    FilePath file = testPath(&arena, "recursive/a/b/file.txt");
    ASSERT(storeFile(BinaryView{"test", 4}, file));
    ASSERT(removeDirectoryRecursive(testPath(&arena, "recursive")));
    Maybe<FileInfo> info = getFileInfo(testPath(&arena, "recursive"));
    ASSERT(!info.has);
    cleanupTestDir();
}

TEST(testGetFileInfoFile)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "infofile.txt");
    ASSERT(storeFile(BinaryView{"hello", 5}, path));
    Maybe<FileInfo> info = getFileInfo(path);
    ASSERT(info.has);
    ASSERT(!info->isDirectory);
    ASSERT(!info->isSymLink);
    ASSERT(info->size == 5);
    cleanupTestDir();
}

TEST(testGetFileInfoDirectory)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "infodir");
    ASSERT(makeDirectory(path));
    Maybe<FileInfo> info = getFileInfo(path);
    ASSERT(info.has);
    ASSERT(info->isDirectory);
    cleanupTestDir();
}

TEST(testGetFileInfoNonexistent)
{
    Maybe<FileInfo> info = getFileInfo("/tmp/hg_nonexistent_path_12345");
    ASSERT(!info.has);
}

TEST(testGetFileInfoInDirectory)
{
    setupTestDir();
    Arena arena{4096};
    ASSERT(storeFile(BinaryView{"a", 1}, testPath(&arena, "file_a.txt")));
    ASSERT(storeFile(BinaryView{"b", 1}, testPath(&arena, "file_b.txt")));
    ASSERT(makeDirectory(testPath(&arena, "subdir")));

    Arena listArena{16384};
    Span<DirectoryEntry> entries = getFileInfoInDirectory(&listArena, testDir);
    ASSERT(entries.count == 3);

    bool foundA = false, foundB = false, foundDir = false;
    for (u64 i = 0; i < entries.count; ++i)
    {
        if (entries[i].name == "file_a.txt")
        {
            foundA = true;
            ASSERT(!entries[i].info.isDirectory);
            ASSERT(entries[i].info.size == 1);
        }
        else if (entries[i].name == "file_b.txt")
        {
            foundB = true;
            ASSERT(!entries[i].info.isDirectory);
            ASSERT(entries[i].info.size == 1);
        }
        else if (entries[i].name == "subdir")
        {
            foundDir = true;
            ASSERT(entries[i].info.isDirectory);
        }
    }
    ASSERT(foundA);
    ASSERT(foundB);
    ASSERT(foundDir);
    cleanupTestDir();
}

TEST(testGetFileInfoInDirectoryEmpty)
{
    setupTestDir();
    Arena arena{4096};
    Span<DirectoryEntry> entries = getFileInfoInDirectory(&arena, testDir);
    ASSERT(entries.count == 0);
    cleanupTestDir();
}

TEST(testCopyFile)
{
    setupTestDir();
    Arena arena{4096};
    FilePath src = testPath(&arena, "copy_src.txt");
    FilePath dst = testPath(&arena, "copy_dst.txt");
    ASSERT(storeFile(BinaryView{"copy me", 7}, src));
    ASSERT(copyFile(src, dst));

    Maybe<FileInfo> srcInfo = getFileInfo(src);
    Maybe<FileInfo> dstInfo = getFileInfo(dst);
    ASSERT(srcInfo.has);
    ASSERT(dstInfo.has);
    ASSERT(srcInfo->size == dstInfo->size);

    Maybe<Binary> loaded = loadFile(dst);
    ASSERT(loaded.has);
    ASSERT(loaded->size == 7);
    char buf[8]{};
    loaded->read(0, buf, 7);
    ASSERT(memcmp(buf, "copy me", 7) == 0);
    cleanupTestDir();
}

TEST(testMoveFile)
{
    setupTestDir();
    Arena arena{4096};
    FilePath src = testPath(&arena, "move_src.txt");
    FilePath dst = testPath(&arena, "move_dst.txt");
    ASSERT(storeFile(BinaryView{"move me", 7}, src));
    ASSERT(moveFile(src, dst));

    Maybe<FileInfo> srcInfo = getFileInfo(src);
    Maybe<FileInfo> dstInfo = getFileInfo(dst);
    ASSERT(!srcInfo.has);
    ASSERT(dstInfo.has);
    ASSERT(dstInfo->size == 7);
    cleanupTestDir();
}

TEST(testRemoveFile)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "toremove.txt");
    ASSERT(storeFile(BinaryView{"bye", 3}, path));
    ASSERT(removeFile(path));
    Maybe<FileInfo> info = getFileInfo(path);
    ASSERT(!info.has);
    cleanupTestDir();
}

TEST(testLoadFile)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "loadme.bin");
    u32 val = 0xDEADBEEF;
    ASSERT(storeFile(BinaryView{&val, sizeof(val)}, path));

    Maybe<Binary> loaded = loadFile(path);
    ASSERT(loaded.has);
    ASSERT(loaded->size == sizeof(val));
    u32 result = loaded->read<u32>(0);
    ASSERT(result == 0xDEADBEEF);
    cleanupTestDir();
}

TEST(testLoadFileNonexistent)
{
    Maybe<Binary> loaded = loadFile("/tmp/hg_nonexistent_file_12345.bin");
    ASSERT(!loaded.has);
}

TEST(testStoreFile)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "store.bin");
    const char* data = "hello world";
    ASSERT(storeFile(BinaryView{data, 11}, path));

    Maybe<FileInfo> info = getFileInfo(path);
    ASSERT(info.has);
    ASSERT(info->size == 11);
    cleanupTestDir();
}

TEST(testStoreFileOverwrite)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "overwrite.bin");
    ASSERT(storeFile(BinaryView{"first", 5}, path));
    ASSERT(storeFile(BinaryView{"second", 6}, path));

    Maybe<FileInfo> info = getFileInfo(path);
    ASSERT(info.has);
    ASSERT(info->size == 6);

    Maybe<Binary> loaded = loadFile(path);
    ASSERT(loaded.has);
    char buf[7]{};
    loaded->read(0, buf, 6);
    ASSERT(memcmp(buf, "second", 6) == 0);
    cleanupTestDir();
}

TEST(testStoreFileEmpty)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "empty.bin");
    ASSERT(storeFile(BinaryView{}, path));

    Maybe<FileInfo> info = getFileInfo(path);
    ASSERT(info.has);
    ASSERT(info->size == 0);
    cleanupTestDir();
}

TEST(testCopyFileOverwrite)
{
    setupTestDir();
    Arena arena{4096};
    FilePath src = testPath(&arena, "co_src.txt");
    FilePath dst = testPath(&arena, "co_dst.txt");
    ASSERT(storeFile(BinaryView{"old", 3}, dst));
    ASSERT(storeFile(BinaryView{"new data", 8}, src));
    ASSERT(copyFile(src, dst));

    Maybe<Binary> loaded = loadFile(dst);
    ASSERT(loaded.has);
    ASSERT(loaded->size == 8);
    char buf[9]{};
    loaded->read(0, buf, 8);
    ASSERT(memcmp(buf, "new data", 8) == 0);
    cleanupTestDir();
}

TEST(testMoveFileOverwrite)
{
    setupTestDir();
    Arena arena{4096};
    FilePath src = testPath(&arena, "mo_src.txt");
    FilePath dst = testPath(&arena, "mo_dst.txt");
    ASSERT(storeFile(BinaryView{"old", 3}, dst));
    ASSERT(storeFile(BinaryView{"new", 3}, src));
    ASSERT(moveFile(src, dst));

    Maybe<FileInfo> srcInfo = getFileInfo(src);
    ASSERT(!srcInfo.has);
    Maybe<Binary> loaded = loadFile(dst);
    ASSERT(loaded.has);
    ASSERT(loaded->size == 3);
    cleanupTestDir();
}

TEST(testMakeDirectoryFailsOnNonexistentParent)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "nonexistent_parent/child");
    ASSERT(!makeDirectory(path));
    cleanupTestDir();
}

TEST(testRemoveDirectoryFailsOnNonexistent)
{
    ASSERT(!removeDirectory("/tmp/hg_nonexistent_dir_12345"));
}

TEST(testRemoveDirectoryFailsOnNonEmpty)
{
    setupTestDir();
    Arena arena{4096};
    FilePath dir = testPath(&arena, "nonempty");
    ASSERT(makeDirectory(dir));
    FilePath file = testPath(&arena, "nonempty/file.txt");
    ASSERT(storeFile(BinaryView{"x", 1}, file));
    ASSERT(!removeDirectory(dir));
    cleanupTestDir();
}

TEST(testCopyFileNonexistentSource)
{
    setupTestDir();
    Arena arena{4096};
    FilePath dst = testPath(&arena, "copyfail_dst.txt");
    ASSERT(!copyFile(testPath(&arena, "nonexistent_src.txt"), dst));
    cleanupTestDir();
}

TEST(testMoveFileNonexistentSource)
{
    setupTestDir();
    Arena arena{4096};
    FilePath dst = testPath(&arena, "movefail_dst.txt");
    ASSERT(!moveFile(testPath(&arena, "nonexistent_src.txt"), dst));
    cleanupTestDir();
}

TEST(testRemoveFileNonexistent)
{
    ASSERT(!removeFile("/tmp/hg_nonexistent_file_12345.txt"));
}

TEST(testLoadAndStoreLargeFile)
{
    setupTestDir();
    Arena arena{4096};
    FilePath path = testPath(&arena, "large.bin");

    u64 size = 65536;
    u8* data = heapAlloc<u8>(size);
    for (u64 i = 0; i < size; ++i)
        data[i] = static_cast<u8>(i & 0xFF);

    ASSERT(storeFile(BinaryView{data, size}, path));
    heapFree(data, size);

    Maybe<Binary> loaded = loadFile(path);
    ASSERT(loaded.has);
    ASSERT(loaded->size == size);
    for (u64 i = 0; i < size; ++i)
    {
        u8 val = loaded->read<u8>(i);
        ASSERT(val == static_cast<u8>(i & 0xFF));
    }
    cleanupTestDir();
}

TEST(testFilePathRoundTrip)
{
    Arena arena{4096};
    FilePath fp{&arena, "/usr/local/bin"};
    fp.appendPath("program");
    fp.setExtension(".cfg");
    ASSERT(fp == "/usr/local/bin/program.cfg");
    ASSERT(fp.getDirectoryPart() == "/usr/local/bin/");
    ASSERT(fp.getFileName() == "program.cfg");
    ASSERT(fp.getStem() == "program");
    ASSERT(fp.getExtension() == ".cfg");
    ASSERT(fp.hasExtension());
    ASSERT(fp.isAbsolute());
}

#else

TEST(testFilesystemStub)
{
    ASSERT(true);
}

#endif

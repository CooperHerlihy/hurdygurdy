#include "hg/filesystem.hpp"
#include "hg/error.hpp"

#ifdef HG_PLATFORM_LINUX

#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <ftw.h>

namespace hg {

void FilePath::appendPath(StringView other)
{
    if (length > 0 && chars[length - 1] != '/')
        append('/');
    append(other);
}

StringView FilePath::getFileName() const
{
    char* p = chars + length;
    while (p > chars && p[-1] != '/')
        --p;

    return {p, chars + length};
}

void FilePath::setFileName(StringView name)
{
    length = static_cast<u64>(getFileName().chars - chars);
    append(name);
}

StringView FilePath::getStem() const
{
    StringView str = getFileName();
    const char* p = str.chars + str.length;
    while (p > str.chars + 2 && p[-1] != '.')
        --p;

    if (p > str.chars && p[-1] == '.')
        return {str.chars, p - 1};

    return str;
}

void FilePath::setStem(StringView stem)
{
    ArenaScope scratch = getScratch();
    StringView ext = StringBuilder{scratch, getExtension()};
    length = static_cast<u64>(getFileName().chars - chars);
    append(stem);
    append(ext);
}

bool FilePath::hasExtension() const
{
    return getExtension().length > 0;
}

StringView FilePath::getExtension() const
{
    StringView str = getFileName();
    const char* p = str.chars + str.length;
    while (p > str.chars + 2 && p[-1] != '.')
        --p;

    if (p > str.chars && p[-1] == '.')
        return {p - 1, str.chars + str.length};

    return {str.chars + str.length, str.chars + str.length};
}

void FilePath::setExtension(StringView ext)
{
    length = static_cast<u64>(getExtension().chars - chars);
    if (length > 0 && chars[length - 1] != '.' &&
        ext.length > 0 && ext.chars[0] != '.')
        append('.');
    append(ext);
}

StringView FilePath::getDirectoryPart() const
{
    StringView str = getFileName();
    return {chars, str.chars};
}

bool FilePath::isAbsolute() const
{
    return length > 0 && chars[0] == '/';
}

FilePath getCurrentDirectory(Arena* arena)
{
    HG_ASSERT(arena != nullptr);
    char buf[4096];
    if (getcwd(buf, sizeof(buf)) == nullptr)
    {
        setError("Failed to get current directory");
        return {};
    }
    return {arena, buf};
}

FilePath getHomeDirectory(Arena* arena)
{
    HG_ASSERT(arena != nullptr);
    const char* home = getenv("HOME");
    if (home == nullptr)
    {
        setError("HOME environment variable not set");
        return {};
    }
    return {arena, home};
}

FilePath getRootDirectory(Arena* arena)
{
    HG_ASSERT(arena != nullptr);
    return {arena, "/"};
}

Maybe<FileInfo> getFileInfo(StringView path)
{
    ArenaScope scratch = getScratch();
    char* cPath = cString(scratch, path);

    struct stat st;
    if (lstat(cPath, &st) != 0)
        return {};

    FileInfo info{};
    info.isDirectory = S_ISDIR(st.st_mode);
    info.isSymLink = S_ISLNK(st.st_mode);
    info.size = static_cast<u64>(st.st_size);
    info.lastModified = static_cast<u64>(st.st_mtime);
    return some<FileInfo>(info);
}

Span<DirectoryEntry> getFileInfoInDirectory(Arena* arena, StringView path)
{
    HG_ASSERT(arena != nullptr);
    ArenaScope scratch = getScratch(&arena, 1);

    char* cPath = cString(scratch, path);

    DIR* dir = opendir(cPath);
    if (dir == nullptr)
    {
        setError("Failed to open directory: %s", cPath);
        return {};
    }

    u64 capacity = 64;
    u64 count = 0;
    DirectoryEntry* entries = arena->alloc<DirectoryEntry>(capacity);

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (count >= capacity)
        {
            u64 newCapacity = capacity * 2;
            DirectoryEntry* newEntries = arena->alloc<DirectoryEntry>(newCapacity);
            memcpy(newEntries, entries, count * sizeof(DirectoryEntry));
            entries = newEntries;
            capacity = newCapacity;
        }

        StringView name{entry->d_name};
        char* nameCopy = arena->alloc<char>(name.length);
        memcpy(nameCopy, name.chars, name.length);

        FilePath fullPath{scratch};
        fullPath.appendPath(path);
        fullPath.appendPath(name);

        struct stat st;
        if (lstat(cString(scratch, fullPath), &st) == 0)
        {
            entries[count].name = {nameCopy, name.length};
            entries[count].info.isDirectory = S_ISDIR(st.st_mode);
            entries[count].info.isSymLink = S_ISLNK(st.st_mode);
            entries[count].info.size = static_cast<u64>(st.st_size);
            entries[count].info.lastModified = static_cast<u64>(st.st_mtime);
        }
        else
        {
            entries[count].name = {nameCopy, name.length};
            entries[count].info = {};
        }

        ++count;
    }

    closedir(dir);
    return {entries, count};
}

bool makeDirectory(StringView path)
{
    ArenaScope scratch = getScratch();
    char* cPath = cString(scratch, path);

    if (mkdir(cPath, 0755) != 0)
    {
        setError("Failed to create directory: %s", cPath);
        return false;
    }
    return true;
}

bool makeDirectoryRecursive(StringView path)
{
    ArenaScope scratch = getScratch();
    char* cPath = cString(scratch, path);

    for (char* p = cPath + 1; *p; ++p)
    {
        if (*p == '/')
        {
            *p = '\0';
            if (mkdir(cPath, 0755) != 0 && errno != EEXIST)
            {
                setError("Failed to create directory: %s", cPath);
                return false;
            }
            *p = '/';
        }
    }

    if (mkdir(cPath, 0755) != 0 && errno != EEXIST)
    {
        setError("Failed to create directory: %s", cPath);
        return false;
    }
    return true;
}

bool removeDirectory(StringView path)
{
    ArenaScope scratch = getScratch();
    char* cPath = cString(scratch, path);

    if (rmdir(cPath) != 0)
    {
        setError("Failed to remove directory: %s", cPath);
        return false;
    }
    return true;
}

static int removeDirectoryRecursiveCallback(const char* fpath, const struct stat* st, int typeflag, struct FTW* ftwbuf)
{
    (void)st;
    (void)typeflag;
    (void)ftwbuf;
    return remove(fpath);
}

bool removeDirectoryRecursive(StringView path)
{
    ArenaScope scratch = getScratch();
    char* cPath = cString(scratch, path);

    if (nftw(cPath, removeDirectoryRecursiveCallback, 64, FTW_DEPTH | FTW_PHYS) != 0)
    {
        setError("Failed to remove directory recursively: %s", cPath);
        return false;
    }
    return true;
}

bool copyFile(StringView oldPath, StringView newPath)
{
    ArenaScope scratch = getScratch();

    int inFd = open(cString(scratch, oldPath), O_RDONLY);
    if (inFd < 0)
    {
        setError("Failed to open source file for copy");
        return false;
    }

    struct stat st;
    if (fstat(inFd, &st) != 0)
    {
        close(inFd);
        setError("Failed to stat source file for copy");
        return false;
    }

    int outFd = open(cString(scratch, newPath), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (outFd < 0)
    {
        close(inFd);
        setError("Failed to open destination file for copy");
        return false;
    }

    u64 remaining = static_cast<u64>(st.st_size);
    char buf[8192];

    while (remaining > 0)
    {
        u64 toRead = remaining > sizeof(buf) ? sizeof(buf) : remaining;
        ssize_t bytesRead = read(inFd, buf, toRead);
        if (bytesRead <= 0)
        {
            close(inFd);
            close(outFd);
            setError("Failed to read source file for copy");
            return false;
        }

        u64 toWrite = static_cast<u64>(bytesRead);
        const char* ptr = buf;
        while (toWrite > 0)
        {
            ssize_t bytesWritten = write(outFd, ptr, toWrite);
            if (bytesWritten <= 0)
            {
                close(inFd);
                close(outFd);
                setError("Failed to write destination file for copy");
                return false;
            }
            toWrite -= static_cast<u64>(bytesWritten);
            ptr += bytesWritten;
        }

        remaining -= static_cast<u64>(bytesRead);
    }

    close(inFd);
    close(outFd);
    return true;
}

bool moveFile(StringView oldPath, StringView newPath)
{
    ArenaScope scratch = getScratch();
    char* cOld = cString(scratch, oldPath);
    char* cNew = cString(scratch, newPath);

    if (rename(cOld, cNew) != 0)
    {
        setError("Failed to move file");
        return false;
    }
    return true;
}

bool removeFile(StringView path)
{
    ArenaScope scratch = getScratch();
    char* cPath = cString(scratch, path);

    if (unlink(cPath) != 0)
    {
        setError("Failed to remove file: %s", cPath);
        return false;
    }
    return true;
}

Maybe<Binary> loadFile(StringView path)
{
    ArenaScope scratch = getScratch();

    char* cPath = cString(scratch, path);

    struct stat st;
    if (stat(cPath, &st) != 0)
    {
        setError("Failed to stat file: %s", cPath);
        return {};
    }

    u64 fileSize = static_cast<u64>(st.st_size);

    int fd = open(cPath, O_RDONLY);
    if (fd < 0)
    {
        setError("Failed to open file: %s", cPath);
        return {};
    }

    void* data = heapAlloc(fileSize, 1);

    u64 remaining = fileSize;
    u8* ptr = static_cast<u8*>(data);
    while (remaining > 0)
    {
        ssize_t bytesRead = read(fd, ptr, remaining);
        if (bytesRead <= 0)
        {
            heapFree(data, fileSize);
            close(fd);
            setError("Failed to read file: %s", cPath);
            return {};
        }
        remaining -= static_cast<u64>(bytesRead);
        ptr += bytesRead;
    }

    close(fd);

    Binary bin;
    bin.data = data;
    bin.size = fileSize;
    return some<Binary>(std::move(bin));
}

bool storeFile(BinaryView bin, StringView path)
{
    ArenaScope scratch = getScratch();
    char* cPath = cString(scratch, path);

    int fd = open(cPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        setError("Failed to open file for writing: %s", cPath);
        return false;
    }

    u64 remaining = bin.size;
    const u8* ptr = static_cast<const u8*>(bin.data);
    while (remaining > 0)
    {
        ssize_t bytesWritten = write(fd, ptr, remaining);
        if (bytesWritten <= 0)
        {
            close(fd);
            setError("Failed to write file: %s", cPath);
            return false;
        }
        remaining -= static_cast<u64>(bytesWritten);
        ptr += bytesWritten;
    }

    close(fd);
    return true;
}

} // namespace hg

#else

// Stub implementations for non-Linux platforms
namespace hg {

void FilePath::appendPath(StringView other) { (void)other; }
StringView FilePath::getFileName() const { return {}; }
void FilePath::setFileName(StringView name) { (void)name; }
StringView FilePath::getStem() const { return {}; }
void FilePath::setStem(StringView stem) { (void)stem; }
bool FilePath::hasExtension() const { return false; }
StringView FilePath::getExtension() const { return {}; }
void FilePath::setExtension(StringView ext) { (void)ext; }
StringView FilePath::getDirectoryPart() const { return {}; }
bool FilePath::isAbsolute() const { return false; }

FilePath getCurrentDirectory(Arena* arena) { (void)arena; return {}; }
FilePath getHomeDirectory(Arena* arena) { (void)arena; return {}; }
FilePath getRootDirectory(Arena* arena) { (void)arena; return {}; }
Maybe<FileInfo> getFileInfo(StringView path) { (void)path; return {}; }
Span<DirectoryEntry> getFileInfoInDirectory(Arena* arena, StringView path) { (void)arena; (void)path; return {}; }
bool makeDirectory(StringView path) { (void)path; return false; }
bool makeDirectoryRecursive(StringView path) { (void)path; return false; }
bool removeDirectory(StringView path) { (void)path; return false; }
bool removeDirectoryRecursive(StringView path) { (void)path; return false; }
bool copyFile(StringView oldPath, StringView newPath) { (void)oldPath; (void)newPath; return false; }
bool moveFile(StringView oldPath, StringView newPath) { (void)oldPath; (void)newPath; return false; }
bool removeFile(StringView path) { (void)path; return false; }
Maybe<Binary> loadFile(StringView path) { (void)path; return {}; }
bool storeFile(BinaryView bin, StringView path) { (void)bin; (void)path; return false; }

} // namespace hg

#endif

#include "hg/assets.hpp"

#include "hg/error.hpp"
#include "hg/memory.hpp"

#ifdef HG_PLATFORM_LINUX

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace hg {

std::optional<std::vector<u8>> loadFile(std::string_view path)
{
    ArenaScope scratch = getScratch();
    StringTemp p{path, scratch};
    const char* cPath = p.c_str();

    struct stat st;
    if (stat(cPath, &st) != 0)
    {
        setError("Failed to stat file: {}", cPath);
        return {};
    }

    u64 fileSize = assertCast<u64>(st.st_size);

    int fd = open(cPath, O_RDONLY);
    if (fd < 0)
    {
        setError("Failed to open file: {}", cPath);
        return {};
    }
    HG_DEFER(close(fd));

    std::optional<std::vector<u8>> bin = std::vector<u8>(fileSize);

    u64 remaining = fileSize;
    u8* ptr = bin->data();
    while (remaining > 0)
    {
        ssize_t bytesRead = read(fd, ptr, remaining);
        if (bytesRead <= 0)
        {
            setError("Failed to read file: {}", cPath);
            return {};
        }
        remaining -= assertCast<u64>(bytesRead);
        ptr += bytesRead;
    }

    return bin;
}

bool storeFile(std::span<u8> bin, std::string_view path)
{
    ArenaScope scratch = getScratch();
    StringTemp p{path, scratch};
    const char* cPath = p.c_str();

    int fd = open(cPath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
    {
        setError("Failed to open file for writing: {}", cPath);
        return false;
    }
    HG_DEFER(close(fd));

    u64 remaining = bin.size();
    const u8* ptr = static_cast<const u8*>(bin.data());
    while (remaining > 0)
    {
        ssize_t bytesWritten = write(fd, ptr, remaining);
        if (bytesWritten <= 0)
        {
            setError("Failed to write file: {}", cPath);
            return false;
        }
        remaining -= assertCast<u64>(bytesWritten);
        ptr += bytesWritten;
    }

    return true;
}

} // namespace hg

#endif

#if defined(HG_PLATFORM_WINDOWS)

#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>

namespace hg {

static bool win32PathToWide(std::string_view path, wchar_t* wPath, u64 capacity)
{
    int wlen = MultiByteToWideChar(
        CP_UTF8,
        0,
        path.data(),
        assertCast<int>(path.length()),
        wPath,
        assertCast<int>(capacity) - 1);
    if (wlen <= 0)
        return false;
    wPath[wlen] = L'\0';
    return true;
}

std::optional<std::vector<u8>> loadFile(std::string_view path)
{
    ArenaScope scratch = getScratch();
    string_temp p{path, scratch};

    wchar_t wPath[1024]{};
    if (!win32PathToWide(p.c_str(), wPath, sizeof(wPath) / sizeof(wchar_t)))
    {
        setError("Invalid file path: {}", p.c_str());
        return {};
    }

    HANDLE file = CreateFileW(wPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        setError("Failed to open file: {}", p.c_str());
        return {};
    }
    HG_DEFER(CloseHandle(file));

    LARGE_INTEGER size{};
    if (!GetFileSizeEx(file, &size))
    {
        setError("Failed to stat file: {}", p.c_str());
        return {};
    }

    u64 fileSize = assertCast<u64>(size.QuadPart);

    std::optional<std::vector<u8>> bin = std::vector<u8>(fileSize);

    u64 remaining = fileSize;
    u8* ptr = bin->data();
    while (remaining > 0)
    {
        DWORD bytesToRead = remaining > 0xFFFFFFFF ? 0xFFFFFFFF : assertCast<DWORD>(remaining);
        DWORD bytesRead = 0;
        if (!ReadFile(file, ptr, bytesToRead, &bytesRead, nullptr) || bytesRead == 0)
        {
            setError("Failed to read file: {}", p.c_str());
            return {};
        }
        remaining -= assertCast<u64>(bytesRead);
        ptr += bytesRead;
    }

    return bin;
}

bool storeFile(std::span<u8> bin, std::string_view path)
{
    ArenaScope scratch = getScratch();
    string_temp p{path, scratch};

    wchar_t wPath[1024]{};
    if (!win32PathToWide(p.c_str(), wPath, sizeof(wPath) / sizeof(wchar_t)))
    {
        setError("Invalid file path: {}", p.c_str());
        return false;
    }

    HANDLE file = CreateFileW(wPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        setError("Failed to open file for writing: {}", p.c_str());
        return false;
    }
    HG_DEFER(CloseHandle(file));

    u64 remaining = bin.size();
    const u8* ptr = bin.data();
    while (remaining > 0)
    {
        DWORD bytesToWrite = remaining > 0xFFFFFFFF ? 0xFFFFFFFF : assertCast<DWORD>(remaining);
        DWORD bytesWritten = 0;
        if (!WriteFile(file, ptr, bytesToWrite, &bytesWritten, nullptr) || bytesWritten == 0)
        {
            setError("Failed to write file: {}", p.c_str());
            return false;
        }
        remaining -= assertCast<u64>(bytesWritten);
        ptr += bytesWritten;
    }

    return true;
}

} // namespace hg

#endif

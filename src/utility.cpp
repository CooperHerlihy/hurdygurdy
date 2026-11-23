#include "hg/utility.hpp"
#include "hg/format.hpp"

#include "hg/memory.hpp"

#include <cstring>
#include <string>
#include <charconv>

#ifdef HG_PLATFORM_LINUX
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#ifdef HG_PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#endif

namespace hg {

thread_local static char errorData[4096];
thread_local static u32 errorLength = 0;

void setError(std::string_view error)
{
    u32 newLength = assertCast<u32>(std::min(error.length(), (sizeof(errorData) - 1)));
    std::memcpy(errorData, error.data(), newLength);
    errorData[newLength] = '\0';
    errorLength = newLength;
}

std::string_view getError()
{
    return {errorData, errorLength};
}

void printError()
{
    std::fprintf(stderr, "HurdyGurdy Error: %.*s\n", errorLength, errorData);
}

void assertionFailed(const char* file, i32 line, const char* func, const char* cond)
{
#ifdef HG_LOGGING
    std::fprintf(stderr, "HurdyGurdy Panic: Assertion failed in %s:%d %s() \"%s\"\n", file, line, func, cond);
    printError();
#else
    (void)file;
    (void)line;
    (void)func;
    (void)cond;
#endif
    std::abort();
}

bool isWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool isNumeral(char c)
{
    return c >= '0' && c <= '9';
}

bool isInteger(std::string_view str)
{
    if (str.length() == 0)
        return false;

    u64 head = 0;

    if (str[head] == '+' || str[head] == '-')
        ++head;

    if (head == str.length())
        return false;

    while (head < str.length() && isNumeral(str[head]))
    {
        ++head;
    }
    return head == str.length();
}

bool isFloat(std::string_view str)
{
    bool hasNumber = false;

    u64 head = 0;
    if (head == str.length())
        return false;

    if (str[head] == '+' || str[head] == '-')
        ++head;

    if (head == str.length())
        return false;

    while (head < str.length() && isNumeral(str[head]))
    {
        hasNumber = true;
        ++head;
    }

    if (head == str.length())
        return false;

    if (str[head] != '.' && str[head] != 'e')
        return false;

    if (str[head] == '.')
    {
        ++head;
        while (head < str.length() && isNumeral(str[head]))
        {
            hasNumber = true;
            ++head;
        }
    }

    if (!hasNumber)
        return false;

    if (head == str.length())
        return true;

    if (str[head] == 'e')
    {
        ++head;
        if (head == str.length())
            return false;

        if (str[head] == '+' || str[head] == '-')
            ++head;

        bool hasExponent = false;
        while (head < str.length() && isNumeral(str[head]))
        {
            hasExponent = true;
            ++head;
        }
        if (!hasExponent)
            return false;
    }

    if (head == str.length())
        return true;

    if (str[head] == 'f')
        ++head;

    if (head == str.length())
        return true;

    return false;
}

i64 stringToInteger(std::string_view str)
{
    HG_ASSERT(isInteger(str));

    if (str.starts_with('+'))
        str.remove_prefix(1);

    i64 ret = 0;
    [[maybe_unused]]
    std::from_chars_result res = std::from_chars(str.data(), str.data() + str.length(), ret);
    HG_ASSERT(res.ec == std::errc{});
    HG_ASSERT(res.ptr == str.data() + str.length());
    return ret;
}

f64 stringToFloat(std::string_view str)
{
    HG_ASSERT(isFloat(str));

    f64 ret = 0.0;
    [[maybe_unused]]
    std::from_chars_result res = std::from_chars(str.data(), str.data() + str.length(), ret);
    HG_ASSERT(res.ec == std::errc{});
    HG_ASSERT(res.ptr == str.data() + str.length());
    return ret;
}

std::string integerToString(i64 num)
{
    char buffer[24];
    std::to_chars_result res = std::to_chars(buffer, buffer + sizeof(buffer), num);
    HG_ASSERT(res.ec == std::errc{});
    return std::string{buffer, res.ptr};
}

std::string floatToString(f64 num, u32 precision)
{
    char buffer[64];
    std::to_chars_result res = std::to_chars(
        buffer,
        buffer + sizeof(buffer),
        num,
        std::chars_format::general,
        assertCast<i32>(precision));
    *res.ptr = '\0';

    HG_ASSERT(res.ec == std::errc{});
    return buffer;
}

#if defined(HG_PLATFORM_LINUX)

std::optional<std::vector<byte>> binaryLoad(std::string_view path)
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

    std::optional bin = std::make_optional<std::vector<byte>>(fileSize);

    u64 remaining = fileSize;
    byte* ptr = bin->data();
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

bool binaryStore(std::span<const byte> bin, std::string_view path)
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
    const byte* ptr = bin.data();
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

#elif defined(HG_PLATFORM_WINDOWS)

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

std::optional<std::vector<byte>> loadFile(std::string_view path)
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

    std::optional bin = std::make_optional<std::vector<byte>>(fileSize);

    u64 remaining = fileSize;
    byte* ptr = bin->data();
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

bool storeFile(std::span<const byte> bin, std::string_view path)
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
    const byte* ptr = bin.data();
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

#endif

} // namespace hg

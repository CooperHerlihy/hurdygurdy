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
    string_temp p{path, scratch};
    const char* cPath = p.c_str();

    struct stat st;
    if (stat(cPath, &st) != 0)
    {
        setError("Failed to stat file: {}", cPath);
        return {};
    }

    u64 fileSize = static_cast<u64>(st.st_size);

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
        remaining -= static_cast<u64>(bytesRead);
        ptr += bytesRead;
    }

    return bin;
}

bool storeFile(std::span<u8> bin, std::string_view path)
{
    ArenaScope scratch = getScratch();
    string_temp p{path, scratch};
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
        remaining -= static_cast<u64>(bytesWritten);
        ptr += bytesWritten;
    }

    return true;
}

} // namespace hg

#endif

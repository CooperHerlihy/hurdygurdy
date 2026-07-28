#include "hg/binary.hpp"

namespace hg {

void BinaryBuilder::resize(u64 newSize)
{
    HG_ASSERT(arena != nullptr);
    if (!arena->extend(data, size, newSize))
    {
        char* newData = arena->alloc<char>(newSize);
        memcpy(newData, data, size);
        data = newData;
    }
    size = newSize;
}

void BinaryBuilder::overwrite(u64 idx, const void* src, u64 len)
{
    HG_ASSERT(idx + len <= size);
    memcpy(static_cast<u8*>(data) + idx, src, len);
}

void BinaryBuilder::append(const void* src, u64 len)
{
    HG_ASSERT(arena != nullptr);
    if (!arena->extend(data, size, size + len))
    {
        char* newData = arena->alloc<char>(size + len);
        memcpy(newData, data, size);
        data = newData;
    }
    memcpy(static_cast<u8*>(data) + size, src, len);
    size += len;
}

void Binary::read(u64 idx, void* dst, u64 len)
{
    HG_ASSERT(idx + len <= size);
    memcpy(dst, static_cast<const u8*>(data) + idx, len);
}

Binary Binary::create(BinaryView data)
{
    Binary bin;
    bin.size = data.size;
    bin.data = heapAlloc(bin.size, 1);
    if (bin.size > 0)
        memcpy(bin.data, data.data, bin.size);
    return bin;
}

Binary::~Binary() noexcept
{
    if (data != nullptr)
        heapFree(data, size);
}

} // namespace hg

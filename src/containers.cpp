#include "hurdygurdy.hpp"
#include "internal.hpp"

namespace hg {

void BinaryBuilder::read(u64 idx, void* dst, u64 len)
{
    HG_ASSERT(idx + len <= size);
    memcpy(dst, static_cast<const u8*>(data) + idx, len);
}

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

HandlePool handlePoolCreate()
{
    HandlePool handles{};
    handles.handles = Array<Handle>{0, 1024};
    handles.freed = Array<Handle>{0, 1024};

    handlePoolAlloc(&handles);

    return handles;
}

void handlePoolDestroy(HandlePool* pool)
{
    HG_ASSERT(pool != nullptr);

    pool->handles = {};
    pool->freed = {};
}

void handlePoolReset(HandlePool* pool)
{
    HG_ASSERT(pool != nullptr);

    pool->handles.count = 0;
    pool->freed.count = 0;

    handlePoolAlloc(pool);
}

Handle handlePoolAlloc(HandlePool* pool)
{
    HG_ASSERT(pool != nullptr);

    if (pool->freed.count > 0)
    {
        Handle handle = pool->freed.pop();
        pool->handles[handleIdx(handle)] = handle;
        return handle;
    }
    else
    {
        Handle handle = {static_cast<u32>(pool->handles.count)};
        pool->handles.push(handle);
        return handle;
    }
}

bool handlePoolAlive(HandlePool* pool, Handle handle)
{
    HG_ASSERT(pool != nullptr);

    u32 idx = handleIdx(handle);
    return handle != handleNull && idx < pool->handles.count && pool->handles[idx] == handle;
}

void handlePoolFree(HandlePool* pool, Handle handle)
{
    HG_ASSERT(pool != nullptr);
    HG_ASSERT(handlePoolAlive(pool, handle));
    pool->handles[handleIdx(handle)] = handleNull;
    pool->freed.push(handleNextGeneration(handle));
}

} // namespace hg

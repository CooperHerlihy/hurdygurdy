#include "hg/pool.hpp"

namespace hg {

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

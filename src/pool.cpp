#include "hg/pool.hpp"

namespace hg {

HandlePool HandlePool::create()
{
    HandlePool handles{};
    handles.handles = Array<Handle>{0, 1024};
    handles.freed = Array<Handle>{0, 1024};
    handles.alloc();
    return handles;
}

void HandlePool::reset()
{
    handles.count = 0;
    freed.count = 0;
    alloc();
}

Handle HandlePool::alloc()
{
    if (freed.count > 0)
    {
        Handle handle = freed.pop();
        handles[handle.idx()] = handle;
        return handle;
    }
    else
    {
        Handle handle = {static_cast<u32>(handles.count)};
        handles.push(handle);
        return handle;
    }
}

bool HandlePool::alive(Handle handle)
{
    u32 idx = handle.idx();
    return handle != nullHandle && idx < handles.count && handles[idx] == handle;
}

void HandlePool::free(Handle handle)
{
    HG_ASSERT(alive(handle));
    handles[handle.idx()] = nullHandle;
    freed.push(handle.nextGeneration());
}

} // namespace hg

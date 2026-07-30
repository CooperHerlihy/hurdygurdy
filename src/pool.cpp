#include "hg/pool.hpp"

namespace hg {

void HandlePool::reset()
{
    handles.count = 0;
    freed.count = 0;
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

bool HandlePool::alive(Handle handle) const
{
    u32 idx = handle.idx();
    return idx < handles.count && handles[idx] == handle;
}

void HandlePool::free(Handle handle)
{
    HG_ASSERT(alive(handle));
    handles[handle.idx()] = nullHandle;
    freed.push(handle.nextGeneration());
}

} // namespace hg

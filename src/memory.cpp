#include "hg/memory.hpp"
#include "hg/error.hpp"
#include "hg/utility.hpp"
#include "hg/containers.hpp"

namespace hg {

void* heapAlloc(u64 size, u64 align)
{
    void* alloc = align <= 16 ? malloc(size) : aligned_alloc(align, size);
    if (alloc == nullptr)
        HG_PANIC("malloc out of memory");
    return alloc;
}

void heapFree(void* allocation, u64 size)
{
    static_cast<void>(size);
    free(allocation);
}

Arena::Arena(u64 capacityVal)
    : memory{heapAlloc(capacityVal, alignof(std::max_align_t))}
    , capacity{capacityVal}
    , head{0}
{}

Arena::~Arena() noexcept
{
    if (memory != nullptr)
        heapFree(memory, capacity);
}

void* Arena::alloc(u64 size, u64 alignment)
{
    u64 newHead = alignUp(static_cast<u64>(head), alignment) + size;
    if (newHead > capacity)
    {
        setError("Arena out of memory");
        return nullptr;
    }

    head = newHead;
    return reinterpret_cast<void*>(reinterpret_cast<uptr>(memory) + head - size);
}

bool Arena::extend(void* allocation, u64 oldSize, u64 newSize)
{
    if (reinterpret_cast<uptr>(allocation) + oldSize - reinterpret_cast<uptr>(memory) != static_cast<uptr>(head))
        return false;

    u64 newHead = reinterpret_cast<uptr>(allocation) + newSize - reinterpret_cast<uptr>(memory);
    if (newHead > capacity)
    {
        return false;
    }

    head = newHead;
    return true;
}

static thread_local Array<Arena> scratchArenas{};

ArenaScope getScratch(Arena const* const* conflicts, u32 count)
{
    if (count > 0)
        HG_ASSERT(conflicts != nullptr);

    for (u32 i = 0; i < scratchArenas.count; ++i)
    {
        for (u32 j = 0; j < count; ++j)
        {
            if (&scratchArenas[i] == conflicts[j])
                goto next;
        }
        return &scratchArenas[i];
next:
        continue;
    }

    return scratchArenas.push(Arena{((u64)1 << 28) - 1});
}

} // namespace hg

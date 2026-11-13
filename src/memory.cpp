#include "hg/memory.hpp"
#include "hg/error.hpp"
#include "hg/utility.hpp"

#ifdef HG_PLATFORM_WINDOWS
#include <malloc.h>
#endif

#include <vector>
#include <memory>

namespace hg {

Arena::Arena(u64 capacityVal)
    : memory{std::malloc(capacityVal)}
    , capacity{capacityVal}
    , head{0}
{}

Arena::~Arena() noexcept
{
    std::free(memory);
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

static thread_local std::vector<std::unique_ptr<Arena>> scratchArenas{};

ArenaScope getScratch()
{
    return getScratch({});
}

ArenaScope getScratch(const Arena* conflict)
{
    return getScratch({&conflict, 1});
}

ArenaScope getScratch(std::span<const Arena*> conflicts)
{
    for (std::unique_ptr<Arena>& arena : scratchArenas)
    {
        for (const Arena* conflict : conflicts)
        {
            if (arena.get() == conflict)
                goto next;
        }
        return arena.get();
next:
        continue;
    }

    return scratchArenas.emplace_back(std::make_unique<Arena>((static_cast<u64>(1) << 28) - 1)).get();
}

} // namespace hg

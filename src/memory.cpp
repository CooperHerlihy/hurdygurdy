#include "hg/memory.hpp"

#include "hg/utility.hpp"

#ifdef HG_PLATFORM_WINDOWS
#include <malloc.h>
#endif

#include <vector>
#include <memory>

namespace hg {

Arena::Arena(usize capacityVal)
    : memory{std::make_unique_for_overwrite<byte[]>(capacityVal)}
    , capacity{capacityVal}
    , head{0}
{}

void* Arena::alloc(usize size, usize alignment)
{
    u64 newHead = alignUp(head, alignment) + size;
    if (newHead > capacity)
    {
        warn("Arena out of memory\n");
        setError("Arena out of memory");
        return nullptr;
    }

    head = newHead;
    return memory.get() + head - size;
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

    return scratchArenas.emplace_back(std::make_unique<Arena>((u64{1} << 28) - 1)).get();
}

} // namespace hg

#include "hg/memory.hpp"

#include "hg/utility.hpp"

#ifdef HG_PLATFORM_WINDOWS
#include <malloc.h>
#endif

#include <algorithm>
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
    auto it = std::ranges::find_if(scratchArenas, [&](std::unique_ptr<Arena>& arena)
    {
        return std::ranges::none_of(conflicts, [&](const Arena* conflict)
        {
            return conflict == arena.get();
        });
    });
    if (it == scratchArenas.end())
        return scratchArenas.emplace_back(std::make_unique<Arena>((u64{1} << 28) - 1)).get();
    return it->get();
}

} // namespace hg

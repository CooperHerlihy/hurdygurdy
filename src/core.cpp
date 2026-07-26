#include "hurdygurdy.hpp"
#include "internal.hpp"

namespace hg {

thread_local static char errorData[4096];
thread_local static u64 errorLength = 0;

StringView getError()
{
    return {errorData, errorLength};
}

void setError(StringView error)
{
    u64 newLength = std::min(error.length, sizeof(errorData));
    memcpy(errorData, error.chars, newLength);
    errorLength = newLength;
}

void logError()
{
    std::fprintf(stderr, "HurdyGurdy Error: %.*s\n", (int)errorLength, errorData);
}
static u32 initialized = 0;

Maybe<HurdyGurdy> init()
{
    if (initialized > 0)
        return some<HurdyGurdy>();

    if (!internal::initPlatform())
        return {};

    if (!internal::initGpu())
    {
        internal::deinitPlatform();
        return {};
    }

    if (!internal::initAudio())
    {
        internal::deinitGpu();
        internal::deinitPlatform();
        return {};
    }

    return some<HurdyGurdy>();
}

HurdyGurdy::HurdyGurdy() noexcept
{
    ++initialized;
}

HurdyGurdy::HurdyGurdy(const HurdyGurdy&)
{
    ++initialized;
}

HurdyGurdy& HurdyGurdy::operator=(const HurdyGurdy&)
{
    ++initialized;
    return *this;
}

HurdyGurdy::HurdyGurdy(HurdyGurdy&&) noexcept
{
    ++initialized;
}

HurdyGurdy& HurdyGurdy::operator=(HurdyGurdy&&) noexcept
{
    ++initialized;
    return *this;
}

HurdyGurdy::~HurdyGurdy() noexcept
{
    if (--initialized == 0)
    {
        internal::deinitAudio();
        internal::deinitGpu();
        internal::deinitPlatform();
    }
}
void BinaryView::read(u64 idx, void* dst, u64 len) const
{
    HG_ASSERT(idx + len <= size);
    memcpy(dst, static_cast<const u8*>(data) + idx, len);
}


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
    u64 newHead = align(static_cast<u64>(head), alignment) + size;
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

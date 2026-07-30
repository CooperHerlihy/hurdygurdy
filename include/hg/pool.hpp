#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"
#include "hg/memory.hpp"
#include "hg/array.hpp"
#include "hg/hash.hpp"

namespace hg {

/**
 * A pool of objects
 */
template<typename T>
struct Pool {
    /**
     * The size of each preallocated block
     */
    static constexpr u32 blockCount = 1024;
    /**
     * The uninitialized preallocated memory
     */
    Array<T*> prealloc = {};
    /**
     * The freed objects
     */
    Array<T*> inactive = {};
#ifdef HG_DEBUG_MODE
    /**
     * The currently allocated objects
     */
    Array<T*> active = {};
#endif

    /**
     * Construct empty
     */
    Pool() noexcept = default;

    /**
     * Destroy the pool
     */
    ~Pool() noexcept
    {
#ifdef HG_DEBUG_MODE
        if (active.count > 0)
            HG_WARN("Memory leak, pool destroyed with active objects\n");
        for (u32 i = 0; i < active.count; ++i)
        {
            active[i]->~T();
        }
#endif

        for (u32 i = 0; i < prealloc.count; ++i)
        {
            heapFree(prealloc[i], blockCount);
        }
    }

    /**
     * Allocate an object from the pool
     */
    template<typename... Args>
    T* alloc(Args&&... args)
    {
        if (inactive.count == 0)
        {
            T* block = heapAlloc<T>(blockCount);
            for (u32 i = 0; i < blockCount; ++i)
            {
                inactive.push(block + i);
            }
            prealloc.push(block);
        }

        T* object = new (inactive.pop()) T{std::forward<Args>(args)...};
#ifdef HG_DEBUG_MODE
        active.push(object);
#endif
        return object;
    }

    /**
     * Free an object to the pool
     */
    void free(T* object)
    {
        if (object == nullptr)
            return;

        object->~T();
#ifdef HG_DEBUG_MODE
        for (u32 i = 0; i < active.count; ++i)
        {
            if (active[i] == object)
            {
                active.removeSwap(i);
                goto found;
            }
        }
        HG_WARN("Invalid attempt to free to pool, object not in pool, possible double free\n");
        return;
found:
#endif
        inactive.push(object);
    }

    /**
     * Move construct
     */
    Pool(Pool&& other) noexcept
        : prealloc{std::exchange(other.prealloc, {})}
        , inactive{std::exchange(other.inactive, {})}
#ifdef HG_DEBUG_MODE
        , active{std::exchange(other.active, {})}
#endif
    {}

    /**
     * Move assign
     */
    Pool& operator=(Pool&& other) noexcept
    {
        if (this != &other)
        {
            this->~Pool();
            new (this) Pool{std::move(other)};
        }
        return *this;
    }

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;
};

/**
 * A generation counted handle
 */
struct Handle {
    /**
     * The handle id
     */
    u32 id = (u32)-1;

    /**
     * The number of bits in a handle used for the index
     */
    static constexpr u32 idxBits = 24;

    /**
     * Get the index from a handle
     */
    constexpr u32 idx()
    {
        return id & ((1 << idxBits) - 1);
    }

    /**
     * Get the generation from a handle
     */
    constexpr u32 generation()
    {
        return id & ~(((u32)1 << idxBits) - (u32)1);
    }

    /**
     * Returns a new handle at the same index
     */
    constexpr Handle nextGeneration()
    {
        return {id + (1 << idxBits)};
    }
};

/**
 * The null handle
 */
static constexpr Handle nullHandle = Handle{(u32)-1};

/**
 * Compare handles
 */
constexpr bool operator==(Handle lhs, Handle rhs)
{
    return lhs.id == rhs.id;
}

/**
 * Compare handles
 */
constexpr bool operator!=(Handle lhs, Handle rhs)
{
    return lhs.id != rhs.id;
}

/**
 * Hash map hashing for Handle
 */
template<>
constexpr u64 hash(Handle val)
{
    return hash(val.id);
}

/**
 * A handle pool
 */
struct HandlePool {
    /**
     * The currently active handles, or null in vacant slots
     */
    Array<Handle> handles = {};
    /**
     * The freed handles
     */
    Array<Handle> freed = {};

    /**
     * Reset a handle pool
     */
    void reset();

    /**
     * Allocate an index from the pool
     */
    Handle alloc();

    /**
     * Returns whether a handle is alive in the pool
     */
    bool alive(Handle handle) const;

    /**
     * Free an index back into a pool
     *
     * Note, the object handle must be valid and alive
     */
    void free(Handle handle);
};

} // namespace hg


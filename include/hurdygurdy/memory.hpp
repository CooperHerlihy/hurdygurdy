#pragma once

/**
 * Allocates memory from a general purpose allocator
 *
 * Parameters
 * - size The size in bytes to allocate
 * - alignment The required alignment of the allocation in bytes
 *
 * Returns
 * - The allocation, never nullptr
 */
void* heapAlloc(u64 size, u64 alignment);

/**
 * A convenience to allocate an array of a type
 *
 * Note, objects are not initialized
 *
 * Parameters
 * - count The number of T to allocate
 *
 * Returns
 * - The allocated array, never nullptr
 */
template<typename T>
T* heapAlloc(u64 count)
{
    return static_cast<T*>(heapAlloc(count * sizeof(T), alignof(T)));
}

/**
 * Free an allocation from a general purpose allocator
 *
 * Parameters
 * - allocation The allocation to free
 * - size The size of the allocation in bytes
 * - alignment The alignment of the allocation in bytes
 */
void heapFree(void* allocation, u64 size);

/**
 * A convenience to free an allocation from a general purpose allocator
 *
 * Parameters
 * - allocation The allocation to free
 * - count The number of T allocated
 */
template<typename T>
void heapFree(T* allocation, u64 count)
{
    heapFree(static_cast<void*>(allocation), count * sizeof(T));
}

/**
 * An arena allocator
 */
struct Arena {
    /**
     * A pointer to the memory being allocated
     */
    void* memory = nullptr;
    /**
     * The capacity of the memory being allocated
     */
    u64 capacity = 0;
    /**
     * The next allocation to be given out
     */
    u64 head = 0;

    /**
     * Construct empty
     */
    Arena() noexcept = default;

    /**
     * Construct with capacity
     */
    Arena(u64 capacityVal);

    /**
     * Free the arena
     */
    ~Arena() noexcept;

    /**
     * Allocates memory
     *
     * Parameters
     * - size The size in bytes to allocate
     * - alignment The required alignment of the allocation in bytes
     *
     * Returns
     * - The allocation, or nullptr if out of memory
     */
    void* alloc(u64 size, u64 alignment);

    /**
     * A convenience to allocate an array of a type
     *
     * Note, objects are not initialized
     *
     * Parameters
     * - count The number of T to allocate
     *
     * Returns
     * - The allocated array, or nullptr if out of memory
     */
    template<typename T>
    T* alloc(u64 count)
    {
        return static_cast<T*>(alloc(count * sizeof(T), alignof(T)));
    }

    /**
     * Extends the allocation from oldSize to newSize if possible
     *
     * Returns
     * - Whether it could be extended
     */
    bool extend(void* allocation, u64 oldSize, u64 newSize);

    /**
     * Extends the allocation to newSize, canExtend must return true
     *
     * Returns
     * - Whether it could be extended
     */
    template<typename T>
    bool extend(T* allocation, u64 oldSize, u64 newSize)
    {
        return extend(static_cast<void*>(allocation), oldSize * sizeof(T), newSize * sizeof(T));
    }

    /**
     * Move construct
     */
    Arena(Arena&& other) noexcept
        : memory{std::exchange(other.memory, nullptr)}
        , capacity{std::exchange(other.capacity, 0)}
        , head{std::exchange(other.head, 0)}
    {}

    /**
     * Move assign
     */
    Arena& operator=(Arena&& other) noexcept
    {
        if (this != &other)
        {
            this->~Arena();
            new (this) Arena{std::move(other)};
        }
        return *this;
    }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
};

/**
 * An arena allocator
 */
struct ArenaScope {
    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;
    /**
     * Where to restore head at end of scope
     */
    u64 head = 0;

    /**
     * Construct empty
     */
    ArenaScope() noexcept = default;

    /**
     * Create a scope for an arena
     */
    ArenaScope(Arena* arenaVal)
        : arena{arenaVal}, head{arenaVal->head}
    {}

    /**
     * Return the arena's head
     */
    ~ArenaScope() noexcept
    {
        if (arena != nullptr)
            arena->head = head;
    }

    /**
     * Implicit convert to underlying arena
     */
    operator Arena*()
    {
        return arena;
    }

    /**
     * Allocates memory
     *
     * Parameters
     * - size The size in bytes to allocate
     * - alignment The required alignment of the allocation in bytes
     *
     * Returns
     * - The allocation, or nullptr if out of memory
     */
    void* alloc(u64 size, u64 alignment)
    {
        return arena->alloc(size, alignment);
    }

    /**
     * A convenience to allocate an array of a type
     *
     * Note, objects are not initialized
     *
     * Parameters
     * - count The number of T to allocate
     *
     * Returns
     * - The allocated array, or nullptr if out of memory
     */
    template<typename T>
    T* alloc(u64 count)
    {
        return arena->alloc<T>(count);
    }

    /**
     * Extends the allocation from oldSize to newSize if possible
     *
     * Returns
     * - Whether it could be extended
     */
    bool extend(void* allocation, u64 oldSize, u64 newSize)
    {
        return arena->extend(allocation, oldSize, newSize);
    }

    /**
     * Extends the allocation to newSize, canExtend must return true
     *
     * Returns
     * - Whether it could be extended
     */
    template<typename T>
    bool extend(T* allocation, u64 oldSize, u64 newSize)
    {
        return arena->extend(allocation, oldSize, newSize);
    }

    /**
     * Move construct
     */
    ArenaScope(ArenaScope&& other) noexcept
        : arena{std::exchange(other.arena, nullptr)}
        , head{std::exchange(other.head, 0)}
    {}

    /**
     * Move assign
     */
    ArenaScope& operator=(ArenaScope&& other) noexcept
    {
        if (this != &other)
        {
            this->~ArenaScope();
            new (this) ArenaScope{std::move(other)};
        }
        return *this;
    }

    ArenaScope(const ArenaScope&) = delete;
    ArenaScope& operator=(const ArenaScope&) = delete;
};

/**
 * Get a scratch arena for temporary allocations, accounting for conflicts
 *
 * Parameters
 * - conflict If arenas are being used, the returned arena will be different
 * - count The number of conflicts
 *
 * Returns
 * - A scratch arena, never nullptr
 */
ArenaScope getScratch(Arena const* const* conflicts = nullptr, u32 count = 0);

#pragma once

#include "hg/inttypes.hpp"
#include "hg/macros.hpp"

#include <utility>
#include <span>
#include <new>

#include <string>
#include <vector>

namespace hg {

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
     * Extends the allocation from oldSize to newSize if possible
     *
     * Returns
     * - Whether it could be extended
     */
    bool extend(void* allocation, u64 oldSize, u64 newSize);

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
 * An arena allocator for std containers
 */
template<typename T>
struct ArenaAllocator {
    /**
     * The type to be allocated
     */
    using value_type = T;

    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;

    /**
     * Construct empty
     */
    ArenaAllocator() noexcept = default;

    /**
     * Construct with an arena
     */
    ArenaAllocator(Arena* arenaVal)
        : arena{arenaVal}
    {}

    /**
     * Construct with an arena scope
     */
    ArenaAllocator(const ArenaScope& scope)
        : arena{scope.arena}
    {}

    /**
     * Allocate from the arena
     */
    T* allocate(std::size_t size)
    {
        HG_ASSERT(arena != nullptr);
        return new (static_cast<T*>(arena->alloc(size * sizeof(T), alignof(T)))) T;
    }

    /**
     * Deallocate from the arena
     */
    void deallocate(T*, std::size_t)
    {
    }
};

/**
 * Compare arena allocators
 */
template <typename T, typename U>
bool operator==(const ArenaAllocator<T>& lhs, const ArenaAllocator<U>& rhs) noexcept {
    return lhs.arena == rhs.arena;
}

/**
 * A string using an arena allocator
 */
using string_temp = std::basic_string<char, std::char_traits<char>, ArenaAllocator<char>>;

/**
 * A vector using an arena allocator
 */
template<typename T>
using vector_temp = std::vector<T, ArenaAllocator<T>>;

/**
 * Get a scratch arena for temporary allocations
 *
 * Returns
 * - A scratch arena, never nullptr
 */
ArenaScope getScratch();

/**
 * Get a scratch arena for temporary allocations, avoiding a single conflict
 *
 * Parameters
 * - conflict The arena to avoid
 *
 * Returns
 * - A scratch arena, never nullptr
 */
ArenaScope getScratch(const Arena* conflict);

/**
 * Get a scratch arena for temporary allocations, accounting for conflicts
 *
 * Parameters
 * - conflicts Array of arenas to avoid
 * - count The number of conflicts
 *
 * Returns
 * - A scratch arena, never nullptr
 */
ArenaScope getScratch(std::span<const Arena*> conflicts);

} // namespace hg

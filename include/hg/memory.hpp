#pragma once

#include "hg/inttypes.hpp"
#include "hg/macros.hpp"

#include <utility>
#include <span>
#include <new>
#include <memory>
#include <string>
#include <vector>

namespace hg {

/**
 * An arena allocator
 */
class Arena {
private:
    /**
     * A pointer to the memory being allocated
     */
    std::unique_ptr<u8[]> memory = nullptr;
    /**
     * The capacity of the memory being allocated
     */
    usize capacity = 0;
    /**
     * The next allocation to be given out
     */
    usize head = 0;

public:
    /**
     * An arena scope guard to dealloc
     */
    friend class ArenaScope;

    /**
     * Construct empty
     */
    Arena() noexcept = default;

    /**
     * Construct with capacity
     */
    Arena(usize capacityVal);

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
    void* alloc(usize size, usize alignment);

    /**
     * Move construct
     */
    Arena(Arena&& other) noexcept
        : memory{std::move(other.memory)}
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
};

/**
 * An arena allocator
 */
class ArenaScope {
private:
    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;
    /**
     * Where to restore head at end of scope
     */
    usize head = 0;

public:
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
    operator Arena*() const
    {
        return arena;
    }

    /**
     * Allocates memory using the underlying arena
     *
     * Parameters
     * - size The size in bytes to allocate
     * - alignment The required alignment of the allocation in bytes
     *
     * Returns
     * - The allocation, or nullptr if out of memory
     */
    void* alloc(usize size, usize alignment)
    {
        return arena->alloc(size, alignment);
    }

    ArenaScope(ArenaScope&&) = delete;
    ArenaScope& operator=(ArenaScope&&) = delete;
    ArenaScope(const ArenaScope&) = delete;
    ArenaScope& operator=(const ArenaScope&) = delete;
};

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

/**
 * An arena allocator for std containers
 */
template<typename T>
class ArenaAllocator {
private:
    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;

public:
    /**
     * The type to be allocated
     */
    using value_type = T;

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
    ArenaAllocator(const ArenaScope& arenaScope)
        : arena{arenaScope}
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

    /**
     * Compare arena allocators
     */
    template <typename U, typename V>
    friend bool operator==(const ArenaAllocator<U>& lhs, const ArenaAllocator<V>& rhs) noexcept;
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

} // namespace hg

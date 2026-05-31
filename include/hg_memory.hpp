/*
 * =============================================================================
 *
 * Copyright (c) 2025 Cooper Herlihy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIdED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 */

#ifndef HG_MEMORY_HPP
#define HG_MEMORY_HPP

#include "hg_core.hpp"

/**
 * An arena allocator
 */
struct HgArena {
    /**
     * A pointer to the memory being allocated
     */
    void* memory;
    /**
     * The capacity of the memory being allocated
     */
    u64 capacity;
    /**
     * The next allocation to be given out
     */
    u64 head;
};

/**
 * Create a guard which restores an arena's head at the end of the scope
 */
#define hgArenaScope(arena) \
    u64 hgArenaHead_##arena = arena->head; \
    hgDefer(arena->head = hgArenaHead_##arena);

/**
 * Allocates memory from an arena
 *
 * Parameters
 * - arena The arena to allocate from
 * - size The size in bytes to allocate
 * - alignment The required alignment of the allocation in bytes
 *
 * Returns
 * - The allocation, never nullptr
 */
void* hgAlloc(HgArena* arena, u64 size, u64 alignment);

/**
 * A convenience to allocate an array of a type
 *
 * Note, objects are not initialized
 *
 * Parameters
 * - arena The arena to allocate from
 * - count The number of T to allocate
 *
 * Returns
 * - The allocated array, never nullptr
 */
template<typename T>
T* hgAlloc(HgArena* arena, u64 count)
{
    return (T*)hgAlloc(arena, count * sizeof(T), alignof(T));
}

/**
 * Reallocates memory from a arena
 *
 * Simply increases the size if allocation is the most recent allocation
 *
 * Parameters
 * - arena The arena to allocate from
 * - allocation The allocation to grow
 * - oldSize The old size in bytes allocation
 * - newSize The new size in bytes to allocate
 * - alignment The required alignment of the allocation in bytes
 *
 * Returns
 * - The allocation, never nullptr
 */
void* hgRealloc(HgArena* arena, void* allocation, u64 oldSize, u64 newSize, u64 alignment);

/**
 * A convenience to reallocate an array of a type
 *
 * Note, objects are default constructed if possible, otherwise they are
 * left uninitialized
 *
 * Parameters
 * - arena The arena to allocate from
 * - allocation The allocation to reallocate
 * - oldCount The old number of T allocated
 * - newCount The new number of T to allocate
 *
 * Returns
 * - The reallocated array, never nullptr
 */
template<typename T>
T* hgRealloc(HgArena* arena, T* allocation, u64 oldCount, u64 newCount)
{
    static_assert(std::is_trivially_copyable_v<T>);
    return (T*)hgRealloc(arena, allocation, oldCount * sizeof(T), newCount * sizeof(T), alignof(T));
}

/**
 * Initializes scratch arenas on this thread
 *
 * Parameters
 * - size The size of each arena in bytes
 */
void hgScratchInit(u64 size);

/**
 * Deinitializes scratch arenas
 */
void hgScratchDeinit();

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
HgArena* hgScratch(HgArena const* const* conflicts = nullptr, u32 count = 0);

#endif // HG_MEMORY_HPP

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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
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
 * Clear a section of memory to 0
 */
void hgMemClear(void* dst, u64 size);

/**
 * Copy memory from src to dst, may not overlap
 */
void hgMemCopy(void* __restrict dst, const void* __restrict src, u64 size);

/**
 * Copy memory from src to dst, may overlap
 */
void hgMemMove(void* dst, const void* src, u64 size);

/**
 * Check if two regions of memory are identical
 */
bool hgMemEqual(const void* dst, const void* src, u64 size);

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
void* hgGpaAlloc(u64 size, u64 alignment);

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
T* hgGpaAlloc(u64 count)
{
    return (T*)hgGpaAlloc(count * sizeof(T), alignof(T));
}

/**
 * Reallocates memory from a general purpose allocator
 *
 * Parameters
 * - allocation The allocation to grow
 * - oldSize The old size in bytes allocation
 * - newSize The new size in bytes to allocate
 * - alignment The required alignment of the allocation in bytes
 *
 * Returns
 * - The allocation, never nullptr
 */
void* hgGpaRealloc(void* allocation, u64 oldSize, u64 newSize, u64 alignment);

/**
 * A convenience to reallocate an array of a type
 *
 * Note, objects are not initialized
 *
 * Parameters
 * - allocation The allocation to reallocate
 * - oldCount The old number of T allocated
 * - newCount The new number of T to allocate
 *
 * Returns
 * - The reallocated array, never nullptr
 */
template<typename T>
T* hgGpaRealloc(T* allocation, u64 oldCount, u64 newCount)
{
    return (T*)hgGpaRealloc(allocation, oldCount * sizeof(T), newCount * sizeof(T), alignof(T));
}

/**
 * A convenienve to free an allocation from a general purpose allocator
 *
 * Parameters
 * - allocation The allocation to free
 * - count The number of T allocated
 */
template<typename T>
void hgGpaFree(T* allocation, u64 count)
{
    hgGpaFree((void*)allocation, count * sizeof(T));
}

/**
 * Free an allocation from a general purpose allocator
 *
 * Parameters
 * - allocation The allocation to free
 * - size The size of the allocation in bytes
 * - alignment The alignment of the allocation in bytes
 */
template<>
void hgGpaFree(void* allocation, u64 size);

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
    u64 hgArenaScopeHead_##arena = arena->head; \
    hgDefer(arena->head = hgArenaScopeHead_##arena);

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
void* hgArenaAlloc(HgArena* arena, u64 size, u64 alignment);

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
T* hgArenaAlloc(HgArena* arena, u64 count)
{
    return (T*)hgArenaAlloc(arena, count * sizeof(T), alignof(T));
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
void* hgArenaRealloc(HgArena* arena, void* allocation, u64 oldSize, u64 newSize, u64 alignment);

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
T* hgArenaRealloc(HgArena* arena, T* allocation, u64 oldCount, u64 newCount)
{
    return (T*)hgArenaRealloc(arena, allocation, oldCount * sizeof(T), newCount * sizeof(T), alignof(T));
}

/**
 * Initializes scratch arenas on this thread
 *
 * Parameters
 * - count The number of arenas to allocate
 * - size The size of each arena in bytes
 */
void hgScratchInit(u32 count, u64 size);

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

/**
 * A block of binary data
 */
struct HgBinary {
    /**
     * The data in the file
     */
    void* data;
    /**
     * The size of the file in bytes
     */
    u64 size;
};

/**
 * Resize the file
 *
 * Parameters
 * - arena The arena to allocate from
 * - newSize The new size of the file in bytes
 */
void hgBinaryResize(HgArena* arena, HgBinary* bin, u64 newSize);

/**
 * Read data at index into a buffer
 *
 * Parameters
 * - idx The index into the file in bytes to read from
 * - dst A pointer to store the read data
 * - size The size in bytes to read
 */
void hgBinaryRead(const HgBinary* bin, u64 idx, void* dst, u64 len);

/**
 * Read data of arbitrary type from the file
 *
 * Parameters
 * - idx The index into the file in bytes to read from
 */
template<typename T>
T hgBinaryRead(const HgBinary* bin, u64 idx)
{
    T ret;
    hgBinaryRead(bin, idx, &ret, sizeof(T));
    return ret;
}

/**
 * Overwrite data at the index
 *
 * Parameters
 * - idx The index into the file to overwrite
 * - src The data to write
 * - size The size of the data in bytes
 */
void hgBinaryOverwrite(HgBinary* bin, u64 idx, const void* src, u64 len);

/**
 * Overwrite data of arbitrary type at the index
 *
 * Parameters
 * - idx The index into the file to overwrite
 * - src The data to write
 */
template<typename T>
void hgBinaryOverwrite(HgBinary* bin, u64 idx, const T& src)
{
    hgBinaryOverwrite(bin, idx, &src, sizeof(T));
}

#endif // HG_MEMORY_HPP

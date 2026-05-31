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

#ifndef HG_TEMPLATES_HPP
#define HG_TEMPLATES_HPP

#include "hg_core.hpp"
#include "hg_memory.hpp"

/**
 * The hash template
 */
template<typename T>
constexpr u64 hgHash(T)
{
    static_assert(false, "hgHash must be implemented for each type");
    return 0;
}

/**
 * A hash set
 */
template<typename V>
struct HgSet {
    static_assert(std::is_trivially_copyable_v<V> && std::is_trivially_destructible_v<V>);

    /**
     * Whether each index has a value
     */
    bool* hasVal;
    /**
     * Where the values are stored;
     */
    V* vals;
    /**
     * The max number of vals
     */
    u32 capacity;
    /**
     * The current number of values that are stored
     */
    u32 count;
};

/**
 * Creates a new hash set
 *
 * Parameters
 * - arena The arena to allocate from
 * - slotCount The max number of slots to store values in
 */
template<typename V>
HgSet<V> hgSetCreate(HgArena* arena, u32 slotCount)
{
    hgAssert(slotCount > 0);

    HgSet<V> set;
    set.hasVal = hgAlloc<bool>(arena, slotCount);
    set.vals = hgAlloc<V>(arena, slotCount);
    set.capacity = slotCount;
    hgSetReset(&set);
    return set;
}

/**
 * Empties all slots
 */
template<typename V>
void hgSetReset(HgSet<V>* set)
{
    for (u32 i = 0; i < set->capacity; ++i)
    {
        set->hasVal[i] = false;
    }
    set->count = 0;
}

/**
 * Add a value to the set
 */
template<typename V, typename T = V>
void hgSetAdd(HgSet<V>* set, const T& val)
{
    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    hgAssert(set->count < set->capacity - 1);

    u32 idx = hgHash(v) % set->capacity;
    for (u32 dist = 0; set->hasVal[idx] && set->vals[idx] != v; ++dist)
    {
        u32 otherDist = hgHash(set->vals[idx]) % set->capacity - idx;
        if (otherDist > set->capacity)
            otherDist += set->capacity;

        if (otherDist < dist)
        {
            V valTmp = set->vals[idx];
            set->vals[idx] = v;
            v = valTmp;
            dist = otherDist;
        }

        idx = (idx + 1) % set->capacity;
    }

    set->hasVal[idx] = true;
    set->vals[idx] = v;
    ++set->count;
}

/**
 * Remove a value from the set
 */
template<typename V, typename T = V>
void hgSetRemove(HgSet<V>* set, const T& val)
{
    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    u32 idx = hgHash(v) % set->capacity;
    while (set->hasVal[idx])
    {
        if (set->vals[idx] == v)
            break;
        idx = (idx + 1) % set->capacity;
    }
    if (!set->hasVal[idx])
        return;

    u32 next = (idx + 1) % set->capacity;
    while (set->hasVal[next])
    {
        if (hgHash(set->vals[next]) % set->capacity != next)
        {
            set->vals[idx] = set->vals[next];
            idx = next;
        }
        next = (next + 1) % set->capacity;
    }
    set->hasVal[idx] = false;
    --set->count;
}

/**
 * Checks whether a value is contained in the set
 */
template<typename V, typename T = V>
bool hgSetHas(const HgSet<V>* set, const T& val)
{
    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    for (u32 idx = hgHash(v) % set->capacity; set->hasVal[idx]; idx = (idx + 1) % set->capacity)
    {
        if (set->vals[idx] == v)
            return true;
    }
    return false;
}

/**
 * A key-value hash map
 */
template<typename K, typename V>
struct HgMap {
    static_assert(std::is_trivially_copyable_v<K>
               && std::is_trivially_copyable_v<V>
               && std::is_trivially_destructible_v<K>
               && std::is_trivially_destructible_v<V>);

    /**
     * Whether each index has a value
     */
    bool* hasVal;
    /**
     * Where the keys are stored;
     */
    K* keys;
    /**
     * Where the values are stored
     */
    V* vals;
    /**
     * The max number of key value pairs
     */
    u32 capacity;
    /**
     * The current number of values that are stored
     */
    u32 count;
};

/**
 * Creates a new hash map
 *
 * Parameters
 * - arena The arena to allocate from
 * - slotCount The max number of slots to store values in
 */
template<typename K, typename V>
HgMap<K, V> hgMapCreate(HgArena* arena, u32 slotCount)
{
    hgAssert(slotCount > 0);

    HgMap<K, V> map;
    map.hasVal = hgAlloc<bool>(arena, slotCount);
    map.keys = hgAlloc<K>(arena, slotCount);
    map.vals = hgAlloc<V>(arena, slotCount);
    map.capacity = slotCount;
    hgMapReset(&map);
    return map;
}

/**
 * Empties all slots
 */
template<typename K, typename V>
void hgMapReset(HgMap<K, V>* map)
{
    for (u32 i = 0; i < map->capacity; ++i)
    {
        map->hasVal[i] = false;
    }
    map->count = 0;
}

/**
 * Add a key-value pair to the hash map
 *
 * Parameters
 * - key The key to add
 *
 * Returns
 * - A reference to the added value
 */
template<typename K, typename V, typename T = K, typename U = V>
V* hgMapAdd(HgMap<K, V>* map, const T& key, const U& val)
{
    static_assert(std::is_convertible_v<T, K> && std::is_convertible_v<U, V>);
    K k = (K)key;
    V v = (V)val;

    hgAssert(map->count < map->capacity - 1);

    u32 idx = hgHash(k) % map->capacity;
    for (u32 dist = 0; map->hasVal[idx] && map->keys[idx] != k; ++dist)
    {
        u32 otherDist = hgHash(map->keys[idx]) % map->capacity - idx;
        if (otherDist > map->capacity)
            otherDist += map->capacity;

        if (otherDist < dist)
        {
            K keyTmp = map->keys[idx];
            V valTmp = map->vals[idx];
            map->keys[idx] = k;
            map->vals[idx] = v;
            k = keyTmp;
            v = valTmp;
            dist = otherDist;
        }

        idx = (idx + 1) % map->capacity;
    }

    map->hasVal[idx] = true;
    map->keys[idx] = k;
    map->vals[idx] = v;
    ++map->count;

    return map->vals + idx;
}

/**
 * Remove a key-value pair from the hash map, and stores it
 *
 * Parameters
 * - key The key to remove
 * - val A pointer to store the value, if found
 *
 * Returns
 * - Whether a value was found and stored in value
 */
template<typename K, typename V, typename T = K>
bool hgMapRemove(HgMap<K, V>* map, const T& key, V* val = nullptr)
{
    static_assert(std::is_convertible_v<T, K>);
    K k = (K)key;

    u32 idx = hgHash(k) % map->capacity;
    while (map->hasVal[idx])
    {
        if (map->keys[idx] == k)
            break;
        idx = (idx + 1) % map->capacity;
    }
    if (!map->hasVal[idx])
        return false;

    if (val != nullptr)
        *val = map->vals[idx];

    u32 next = (idx + 1) % map->capacity;
    while (map->hasVal[next])
    {
        if (hgHash(map->keys[next]) % map->capacity != next)
        {
            map->keys[idx] = map->keys[next];
            map->vals[idx] = map->vals[next];
            idx = next;
        }
        next = (next + 1)  % map->capacity;
    }
    map->hasVal[idx] = false;
    --map->count;

    return true;
}

/**
 * Gets the value stored at a key
 *
 * Returns
 * - A pointer to the value, or nullptr if it does not exist
 */
template<typename K, typename V, typename T = K>
V* hgMapGet(const HgMap<K, V>* map, const T& key)
{
    static_assert(std::is_convertible_v<T, K>);
    K k = (K)key;

    for (u32 idx = hgHash(key) % map->capacity; map->hasVal[idx]; idx = (idx + 1) % map->capacity)
    {
        if (map->keys[idx] == k)
            return map->vals + idx;
    }
    return nullptr;
}

/**
 * Hash map hashing for u8
 */
template<>
constexpr u64 hgHash(u8 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u16
 */
template<>
constexpr u64 hgHash(u16 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u32
 */
template<>
constexpr u64 hgHash(u32 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u64
 */
template<>
constexpr u64 hgHash(u64 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i8
 */
template<>
constexpr u64 hgHash(i8 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i16
 */
template<>
constexpr u64 hgHash(i16 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i32
 */
template<>
constexpr u64 hgHash(i32 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i64
 */
template<>
constexpr u64 hgHash(i64 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for f32
 */
template<>
constexpr u64 hgHash(f32 val)
{
    union {
        f32 asFloat;
        u64 asHash;
    } u{};
    u.asFloat = val;
    return u.asHash;
}

/**
 * Hash map hashing for f64
 */
template<>
constexpr u64 hgHash(f64 val)
{
    union {
        f64 asFloat;
        u64 asHash;
    } u{};
    u.asFloat = val;
    return u.asHash;
}

/**
 * Hash map hashing for arbitrary pointer types
 */
template<typename T>
constexpr u64 hgPtrHash(T* val)
{
    union {
        T* asPtr;
        uptr asUptr;
    } u{};
    u.asPtr = val;
    return (u64)u.asUptr;
};

/**
 * Hash map hashing for void*
 */
template<>
constexpr u64 hgHash(void* val)
{
    return hgPtrHash<void>(val);
}

/**
 * A generation counted handle
 */
struct HgHandle {
    /**
     * The handle id, defaults to null
     */
    u32 id = (u32)-1;
};

/**
 * The number of bits in a handle used for the index
 */
static constexpr u32 hgHandleIdxBits = 24;

/**
 * Get the index from a handle
 */
constexpr u32 hgHandleIdx(HgHandle handle)
{
    return handle.id & ((1 << hgHandleIdxBits) - 1);
}

/**
 * Get the generation from a handle
 */
constexpr u32 hgHandleGeneration(HgHandle handle)
{
    return handle.id & ~((1 << hgHandleIdxBits) - 1);
}

/**
 * Returns a new handle at the same index
 */
constexpr HgHandle hgHandleNextGeneration(HgHandle handle)
{
    return {handle.id + (1 << hgHandleIdxBits)};
}

/**
 * Returns whether the handle is the null handle
 */
constexpr bool hgNullHandle(HgHandle handle)
{
    return handle.id == HgHandle{}.id;
}

/**
 * An object pool
 */
template<typename T>
struct HgPool {
    static_assert(std::is_trivially_destructible_v<T>);

    /**
     * The values stored in the pool
     */
    T* vals;
    /**
     * The handle free list
     */
    HgHandle* freeList;
    /**
     * The next handle in the free list
     */
    HgHandle next;
    /**
     * The capacity of the pool
     */
    u32 capacity;
};

/**
 * Reset an object pool
 */
template<typename T>
constexpr void hgPoolReset(HgPool<T>* pool)
{
    hgAssert(pool != nullptr);

    for (u32 i = 0; i < pool->capacity; ++i)
    {
        pool->freeList[i] = {i + 1};
    }
    pool->next = {0};
}

/**
 * Create a new object pool
 */
template<typename T>
HgPool<T> hgPoolCreate(HgArena* arena, u32 capacity)
{
    hgAssert(arena != nullptr);
    hgAssert(capacity > 0);

    HgPool<T> pool{};
    pool.vals = hgAlloc<T>(arena, capacity);
    pool.freeList = hgAlloc<HgHandle>(arena, capacity);
    pool.capacity = capacity;
    hgPoolReset(&pool);

    return pool;
}

/**
 * Allocate an object from a pool
 *
 * Note, does not initialize the object
 */
template<typename T>
constexpr HgHandle hgPoolAlloc(HgPool<T>* pool)
{
    hgAssert(pool != nullptr);
    hgAssert(hgHandleIdx(pool->next) < pool->capacity);

    HgHandle handle = pool->next;
    pool->next = pool->freeList[hgHandleIdx(handle)];
    pool->freeList[hgHandleIdx(handle)] = handle;
    return handle;
}

/**
 * Returns whether a handle is alive in the pool
 */
template<typename T>
constexpr bool hgPoolAlive(HgPool<T>* pool, HgHandle handle)
{
    return hgHandleIdx(handle) < pool->capacity && pool->freeList[hgHandleIdx(handle)].id == handle.id;
}

/**
 * Free an object back into a pool
 *
 * Note, the object handle must be valid and alive
 */
template<typename T>
constexpr void hgPoolFree(HgPool<T>* pool, HgHandle handle)
{
    hgAssert(hgPoolAlive(pool, handle));
    pool->freeList[hgHandleIdx(handle)] = pool->next;
    pool->next = hgHandleNextGeneration(handle);
}

/**
 * Get an object from a pool
 */
template<typename T>
constexpr T* hgPoolGet(HgPool<T>* pool, HgHandle handle)
{
    hgAssert(hgPoolAlive(pool, handle));
    return &pool->vals[hgHandleIdx(handle)];
}

/**
 * A pool of indices
 */
typedef HgPool<void> HgIndexPool;

/**
 * A pool of indices
 */
template<>
struct HgPool<void> {
    /**
     * The handle free list
     */
    HgHandle* freeList;
    /**
     * The next handle in the free list
     */
    HgHandle next;
    /**
     * The capacity of the pool
     */
    u32 capacity;
};

/**
 * Create a new object pool
 */
template<>
inline HgPool<void> hgPoolCreate(HgArena* arena, u32 capacity)
{
    hgAssert(arena != nullptr);
    hgAssert(capacity > 0);

    HgPool<void> pool{};
    pool.freeList = hgAlloc<HgHandle>(arena, capacity);
    pool.capacity = capacity;
    hgPoolReset(&pool);

    return pool;
}

#endif // HG_TEMPLATES_HPP

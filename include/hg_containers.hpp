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

#ifndef HG_CONTAINERS_HPP
#define HG_CONTAINERS_HPP

#include "hg_core.hpp"
#include "hg_math.hpp"
#include "hg_memory.hpp"
#include "hg_strings.hpp"
#include "hg_serialization.hpp"

/**
 * A dynamic array
 */
template<typename T>
struct HgArray {
    /**
     * The values stored
     */
    T* vals;
    /**
     * The number of vals
     */
    u32 count;
    /**
     * The current max number of vals
     */
    u32 capacity;

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr T& operator[](u64 idx) const
    {
        hgAssert(vals != nullptr);
        hgAssert(idx < count);
        return vals[idx];
    }
};

/**
 * Create an array
 */
template<typename T>
HgArray<T> hgArrayCreate(u32 count = 0, u32 capacity = 1024)
{
    if (capacity < count)
        capacity = count;

    HgArray<T> arr{};
    arr.vals = hgGpaAlloc<T>(capacity);
    arr.count = count;
    arr.capacity = capacity;

    return arr;
}

/**
 * Destroy an array
 */
template<typename T>
void hgArrayDestroy(HgArray<T>* arr)
{
    hgAssert(arr != nullptr);

    hgGpaFree(arr->vals, arr->capacity);
}

/**
 * Resize an array
 */
template<typename T>
void hgArrayResize(HgArray<T>* arr, u32 newCount)
{
    if (newCount > arr->capacity)
    {
        arr->vals = hgGpaRealloc(arr->vals, arr->capacity, newCount * 2);
        arr->capacity = newCount * 2;
    }
    arr->count = newCount;
}

/**
 * Push a value to the end of the array
 */
template<typename T>
T* hgArrayPush(HgArray<T>* arr)
{
    if (arr->count == arr->capacity)
    {
        u32 newCapacity = arr->capacity == 0 ? 16 : arr->capacity * 2;
        hgGpaRealloc(arr->vals, arr->capacity, newCapacity);
        arr->capacity = newCapacity;
    }
    return &arr->vals[arr->count++];
}

/**
 * Create a temporary array which need not be destroyed, but cannot be resized
 */
template<typename T>
HgArray<T> hgArrayTemp(HgArena* arena, u32 count = 0, u32 capacity = 1024)
{
    hgAssert(arena != nullptr);
    hgAssert(count <= capacity);

    HgArray<T> arr{};
    arr.vals = hgArenaAlloc<T>(arena, capacity);
    arr.count = count;
    arr.capacity = capacity;

    return arr;
}

/**
 * Push a value to the end of the array using an arena
 */
template<typename T>
T* hgArrayPushTemp(HgArena* arena, HgArray<T>* arr)
{
    if (arr->count == arr->capacity)
        hgArenaRealloc(arena, arr->vals, arr->capacity, arr->capacity == 0 ? 16 : arr->capacity * 2);
    return &arr->vals[arr->count++];
}

/**
 * Pop a value from the end of the array
 */
template<typename T>
T hgArrayPop(HgArray<T>* arr)
{
    hgAssert(arr->count > 0);
    return arr->vals[--arr->count];
}

/**
 * An array of values of unknown type
 */
struct HgArrayAny {
    /**
     * The values stored
     */
    void* vals;
    /**
     * The number of vals
     */
    u32 count;
    /**
     * The current max number of vals
     */
    u32 capacity;
    /**
     * The width of each val in bytes
     */
    u32 width;
    /**
     * The alignment of each val in bytes
     */
    u32 align;

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr void* operator[](u64 idx) const
    {
        hgAssert(vals != nullptr);
        hgAssert(idx < count);
        return (u8*)vals + idx * width;
    }
};

/**
 * Create an array
 */
HgArrayAny hgArrayAnyCreate(u32 width, u32 align, u32 count = 0, u32 capacity = 1024);

/**
 * Destroy an array
 */
void hgArrayAnyDestroy(HgArrayAny* arr);

/**
 * Resize an array
 */
void hgArrayAnyResize(HgArrayAny* arr, u32 newCount);

/**
 * Push a value to the end of the array
 */
void* hgArrayAnyPush(HgArrayAny* arr);

/**
 * Create a temporary array which need not be destroyed, but cannot be resized
 */
HgArrayAny hgArrayAnyTemp(HgArena* arena, u32 width, u32 align, u32 capacity, u32 count);

/**
 * Push a value to the end of the array using an arena
 */
void hgArrayAnyPushTemp(HgArena* arena, HgArrayAny* arr, void* val = nullptr);

/**
 * Pop a value from the end of the array
 */
void hgArrayAnyPop(HgArrayAny* arr, void* dst);

// queue : TODO

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
 * - slotCount The max number of slots to store values in
 */
template<typename V>
HgSet<V> hgSetCreate(u32 slotCount)
{
    hgAssert(slotCount > 0);

    HgSet<V> set;
    set.hasVal = hgGpaAlloc<bool>(slotCount);
    set.vals = hgGpaAlloc<V>(slotCount);
    set.capacity = slotCount;
    hgSetReset(&set);
    return set;
}

/**
 * Destroy a hash set
 */
template<typename V>
void hgSetDestroy(HgSet<V>* set)
{
    hgAssert(set != nullptr);

    hgGpaFree(set->hasVal);
    hgGpaFree(set->vals);
}

/**
 * Create a new hash set which need not be destroyed, but cannot be resized
 *
 * Parameters
 * - arena The arena to allocate from
 * - slotCount The max number of slots to store values in
 */
template<typename V>
HgSet<V> hgSetTemp(HgArena* arena, u32 slotCount)
{
    hgAssert(slotCount > 0);

    HgSet<V> set;
    set.hasVal = hgArenaAlloc<bool>(arena, slotCount);
    set.vals = hgArenaAlloc<V>(arena, slotCount);
    set.capacity = slotCount;
    hgSetReset(&set);
    return set;
}

/**
 * Resize the set
 */
template<typename V>
void hgSetResize(HgSet<V>* set, u32 newSize)
{
    hgAssert(set != nullptr);

    if (newSize > set->capacity)
    {
        HgSet<V> newSet = hgSetCreate<V>(newSize);

        for (u32 i = 0; i < set->capacity; ++i)
        {
            if (set->hasVal[i])
                hgSetAdd(&newSet, set->vals[i]);
        }

        hgSetDestroy(set);
        *set = newSet;
    }
}

/**
 * Empty all slots
 */
template<typename V>
void hgSetReset(HgSet<V>* set)
{
    hgAssert(set != nullptr);

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
    hgAssert(set != nullptr);
    hgAssert(set->count < set->capacity - 1);

    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    u32 idx = hgHash(v) % set->capacity;
    for (u32 dist = 0; set->hasVal[idx] && !(set->vals[idx] == v); ++dist)
    {
        u32 otherDist = hgHash(set->vals[idx]) % set->capacity - idx;
        if (otherDist > set->capacity)
            otherDist += set->capacity;

        if (otherDist < dist)
        {
            hgSwap(&v, &set->vals[idx]);
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
    hgAssert(set != nullptr);

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
 * Check whether a value is contained in the set
 */
template<typename V, typename T = V>
bool hgSetHas(const HgSet<V>* set, const T& val)
{
    hgAssert(set != nullptr);

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
 * Calls a function for each value in the hash map
 */
template<typename V, typename F>
void hgSetForEach(HgSet<V>* set, F fn)
{
    hgAssert(set != nullptr);
    static_assert(std::is_invocable_r_v<void, F, V*>);

    for (u32 i = 0; i < set->capacity; ++i)
    {
        if (set->hasVal[i])
            fn(&set->vals[i]);
    }
}

/**
 * A key-value hash map
 */
template<typename K, typename V>
struct HgMap {
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
 * Create a new hash map
 *
 * Parameters
 * - slotCount The max number of slots to store values in
 */
template<typename K, typename V>
HgMap<K, V> hgMapCreate(u32 slotCount = 1024)
{
    hgAssert(slotCount > 0);

    HgMap<K, V> map;
    map.hasVal = hgGpaAlloc<bool>(slotCount);
    map.keys = hgGpaAlloc<K>(slotCount);
    map.vals = hgGpaAlloc<V>(slotCount);
    map.capacity = slotCount;
    hgMapReset(&map);

    return map;
}

/**
 * Destroy a hash map
 */
template<typename K, typename V>
void hgMapDestroy(HgMap<K, V>* map)
{
    hgAssert(map != nullptr);

    hgGpaFree(map->hasVal, map->capacity);
    hgGpaFree(map->keys, map->capacity);
    hgGpaFree(map->vals, map->capacity);
}

/**
 * Create a new hash map which need not be destroyed, but cannot be resized
 *
 * Parameters
 * - arena The arena to allocate from
 * - slotCount The max number of slots to store values in
 */
template<typename K, typename V>
HgMap<K, V> hgMapTemp(HgArena* arena, u32 slotCount)
{
    hgAssert(arena != nullptr);
    hgAssert(slotCount > 0);

    HgMap<K, V> map;
    map.hasVal = hgArenaAlloc<bool>(arena, slotCount);
    map.keys = hgArenaAlloc<K>(arena, slotCount);
    map.vals = hgArenaAlloc<V>(arena, slotCount);
    map.capacity = slotCount;
    hgMapReset(&map);

    return map;
}

/**
 * Resize a hash map
 */
template<typename K, typename V>
void hgMapResize(HgMap<K, V>* map, u32 newSize)
{
    hgAssert(map != nullptr);

    if (newSize > map->capacity)
    {
        HgMap<K, V> newMap = hgMapCreate<K, V>(newSize);

        for (u32 i = 0; i < map->capacity; ++i)
        {
            if (map->hasVal[i])
                hgMapAdd(&newMap, map->keys[i], map->vals[i]);
        }

        hgMapDestroy(map);
        *map = newMap;
    }
}

/**
 * Empties all slots
 */
template<typename K, typename V>
void hgMapReset(HgMap<K, V>* map)
{
    hgAssert(map != nullptr);

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
    hgAssert(map != nullptr);
    hgAssert(map->count < map->capacity - 1);

    static_assert(std::is_convertible_v<T, K> && std::is_convertible_v<U, V>);
    K k = (K)key;
    V v = (V)val;

    u32 idx = hgHash(k) % map->capacity;
    for (u32 dist = 0; map->hasVal[idx] && !(map->keys[idx] == k); ++dist)
    {
        u32 otherDist = hgHash(map->keys[idx]) % map->capacity - idx;
        if (otherDist > map->capacity)
            otherDist += map->capacity;

        if (otherDist < dist)
        {
            hgSwap(&k, &map->keys[idx]);
            hgSwap(&v, &map->vals[idx]);
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
    hgAssert(map != nullptr);

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
    hgAssert(map != nullptr);

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
 * Calls a function for each value in the hash map
 */
template<typename K, typename V, typename F>
void hgMapForEach(HgMap<K, V>* map, F fn)
{
    hgAssert(map != nullptr);
    static_assert(std::is_invocable_r_v<void, F, K*, V*>);

    for (u32 i = 0; i < map->capacity; ++i)
    {
        if (map->hasVal[i])
            fn(&map->keys[i], &map->vals[i]);
    }
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
constexpr u64 hgHashPtr(T* val)
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
    return hgHashPtr<void>(val);
}

/**
 * Hash map hashing for strings
 */
template<>
constexpr u64 hgHash(HgStringView str)
{
    u64 ret = 0;
    u64 mult = 1;
    for (u32 i = 0; i < str.length; ++i)
    {
        ret += (u64)str[i] * mult;
        mult *= 257;
    }
    return ret;
}

/**
 * Hash map hashing for C string
 */
template<>
constexpr u64 hgHash(const char* str)
{
    return hgHash(HgStringView{str});
}

/**
 * Hash map hashing for HgStringOwner
 */
template<>
constexpr u64 hgHash(HgStringOwner str)
{
    return hgHash(HgStringView{str});
}

/**
 * Hash map hashing for HgStringBuilder
 */
template<>
constexpr u64 hgHash(HgStringBuilder str)
{
    return hgHash(HgStringView{str});
}

/**
 * Hash map hashing for HgHandle
 */
template<>
constexpr u64 hgHash(HgHandle val)
{
    return hgHash(val.id);
}

/**
 * A handle pool
 */
template<typename T>
struct HgHandlePool {
    /**
     * The values the handles point to
     */
    HgArray<T> vals;
    /**
     * The currently active handles, or null in vacant slots
     */
    HgArray<HgHandle> handles;
    /**
     * The freed handles
     */
    HgArray<HgHandle> freed;
};

/**
 * Create a new object pool
 */
template<typename T>
HgHandlePool<T> hgPoolCreate()
{
    HgHandlePool<T> pool{};
    pool.vals = hgArrayCreate<T>();
    pool.handles = hgArrayCreate<HgHandle>();
    pool.freed = hgArrayCreate<HgHandle>();

    hgPoolAlloc(&pool);

    return pool;
}

/**
 * Destroy an object pool
 */
template<typename T>
void hgPoolDestroy(HgHandlePool<T>* pool)
{
    hgAssert(pool != nullptr);

    hgArrayDestroy(&pool->vals);
    hgArrayDestroy(&pool->handles);
    hgArrayDestroy(&pool->freed);
}

/**
 * Reset an object pool
 */
template<typename T>
void hgPoolReset(HgHandlePool<T>* pool)
{
    hgAssert(pool != nullptr);

    pool->vals.count = 0;
    pool->handles.count = 0;
    pool->freed.count = 0;
}

/**
 * Allocate an index from the pool
 */
template<typename T>
HgHandle hgPoolAlloc(HgHandlePool<T>* pool)
{
    hgAssert(pool != nullptr);

    if (pool->freed.count > 0)
    {
        HgHandle handle = hgArrayPop(&pool->freed);
        pool->handles[hgHandleIdx(handle)] = handle;
        return handle;
    }
    else
    {
        HgHandle handle = {pool->handles.count};
        *hgArrayPush(&pool->handles) = handle;
        hgArrayPush(&pool->vals);
        return handle;
    }
}

/**
 * Returns whether a handle is alive in the pool
 */
template<typename T>
bool hgPoolAlive(HgHandlePool<T>* pool, HgHandle handle)
{
    hgAssert(pool != nullptr);
    hgAssert(hgHandleIdx(handle) < pool->handles.count);

    return handle != hgHandleNull && pool->handles[hgHandleIdx(handle)] == handle;
}


/**
 * Free an index back into a pool
 *
 * Note, the object handle must be valid and alive
 */
template<typename T>
void hgPoolFree(HgHandlePool<T>* pool, HgHandle handle)
{
    hgAssert(pool != nullptr);
    hgAssert(hgPoolAlive(pool, handle));
    pool->handles[hgHandleIdx(handle)] = hgHandleNull;
    *hgArrayPush(&pool->freed) = hgHandleNextGeneration(handle);
}

/**
 * Get an item from the pool
 */
template<typename T>
T* hgPoolGet(HgHandlePool<T>* pool, HgHandle handle)
{
    hgAssert(pool != nullptr);
    hgAssert(hgPoolAlive(pool, handle));
    return &pool->vals[hgHandleIdx(handle)];
}

/**
 * A handle pool with no data
 */
template<>
struct HgHandlePool<void> {
    /**
     * The currently active handles, or null in vacant slots
     */
    HgArray<HgHandle> handles;
    /**
     * The freed handles
     */
    HgArray<HgHandle> freed;
};

/**
 * Create a new object pool
 */
template<>
HgHandlePool<void> hgPoolCreate();

/**
 * Destroy an object pool
 */
template<>
void hgPoolDestroy(HgHandlePool<void>* pool);

/**
 * Reset an object pool
 */
template<>
void hgPoolReset(HgHandlePool<void>* pool);

/**
 * Allocate an index from the pool
 */
template<>
HgHandle hgPoolAlloc(HgHandlePool<void>* pool);

/**
 * Returns whether a handle is alive in the pool
 */
template<>
bool hgPoolAlive(HgHandlePool<void>* pool, HgHandle handle);

/**
 * Free an index back into a pool
 *
 * Note, the object handle must be valid and alive
 */
template<>
void hgPoolFree(HgHandlePool<void>* pool, HgHandle handle);

/**
 * An entity in the ecs
 */
struct HgEntity {
    /**
     * The entity handle
     */
    HgHandle handle;
};

/**
 */
static constexpr HgEntity hgEntityNull = HgEntity{};

/**
 * Compare entities
 */
constexpr bool operator==(HgEntity lhs, HgEntity rhs)
{
    return lhs.handle.id == rhs.handle.id;
}

/**
 * Compare entities
 */
constexpr bool operator!=(HgEntity lhs, HgEntity rhs)
{
    return lhs.handle.id != rhs.handle.id;
}

/**
 * Hashing for entities
 */
template<>
constexpr u64 hgHash(HgEntity e)
{
    return hgHash(e.handle.id);
}

/**
 * The unique component id for a type
 */
template<typename T>
inline u64 hgComponentId = (u64)-1;

/**
 * The function called on removing the component, may be overridden
 */
template<typename T>
void hgEcsDtor(T* component)
{
    (void)component;
}

/**
 * The serializer for an ecs
 */
union HgEntitySerializer {
    /**
     * The indices of entities
     */
    HgMap<HgEntity, u32> entityToIdx;
    /**
     * The entities by index
     */
    HgEntity* idxToEntity;
};

/**
 * The default serialization for a component, should be overridden
 */
template<typename T>
void hgEcsSerialize(HgArena* arena, HgSerializer* s, HgStringView name, T* val, HgEntitySerializer* entities)
{
    hgSerialize(arena, s, name, val);
    (void)entities;
}

/**
 * A system of components
 */
struct HgComponent {
    /**
     * The name of the component type
     */
    HgStringOwner name;
    /**
     * The component lookup from entity index
     */
    HgArray<u32> indices;
    /**
     * The entity lookup from component index
     */
    HgArray<HgEntity> entities;
    /**
     * The component data
     */
    HgArrayAny components;
    /**
     * The function called on removing the component
     */
    void (*dtor)(void* component);
    /**
     * The function called on serializing the component
     *
     * Parameters
     * - arena The arena to allocate from
     * - s The serializer
     * - name The name of the field
     * - The value to serialize
     * - ecs The ecs serializer data, if needed
     */
    void (*serialize)(HgArena* arena, HgSerializer* s, HgStringView name, void* val, HgEntitySerializer* ecs);
};

/**
 * An entity component system
 */
struct HgEcs {
    /**
     * The entity pool
     */
    HgHandlePool<void> entities;
    /**
     * The component systems
     */
    HgMap<u64, HgComponent> components;
};

/**
 * Create a new entity component system
 */
HgEcs hgEcsCreate();

/**
 * Destroy an entity component system
 */
void hgEcsDestroy(HgEcs* ecs);

/**
 * Destroy all entities, leave components registered
 */
void hgEcsReset(HgEcs* ecs);

/**
 * HgEcs serialization
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgEcs* ecs);

/**
 * HgEntity ecs serialization
 */
void hgEntitySerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgEntity* val, HgEntitySerializer* ecs);

/**
 * The config to register a component
 */
struct HgEcsRegisterComponent {
    /**
     * The name of the component to create
     *
     * Note, the componentId is derived from this name
     */
    HgStringView name;
    /**
     * The width of the component data in bytes
     */
    u32 width;
    /**
     * The alignment of the component data in bytes
     */
    u32 align;
    /**
     * The function called on removing the component
     */
    void (*dtor)(void* component);
    /**
     * The function called on serializing the component
     */
    void (*serialize)(HgArena* arena, HgSerializer* s, HgStringView name, void* val, HgEntitySerializer* ecs);
};

/**
 * Register a new component type
 */
void hgEcsRegisterComponent(HgEcs* ecs, HgEcsRegisterComponent* config);

/**
 * Register a new component type, creating the name from the type
 */
#define hgEcsRegisterType(ecs, T) \
    do { \
        hgComponentId<T> = hgHash(#T); \
        HgEcsRegisterComponent registerComponent_##T{}; \
        registerComponent_##T.name = #T; \
        registerComponent_##T.width = sizeof(T); \
        registerComponent_##T.align = alignof(T); \
        registerComponent_##T.dtor = [](void* component) \
        { \
            hgEcsDtor<T>((T*)component); \
        }; \
        registerComponent_##T.serialize = []( \
            HgArena* arena, \
            HgSerializer* s, \
            HgStringView name, \
            void* val, \
            HgEntitySerializer* entities) \
        { \
            hgEcsSerialize<T>(arena, s, name, (T*)val, entities); \
        }; \
        hgEcsRegisterComponent(ecs, &registerComponent_##T); \
    } while (0)

/**
 * Unregister a component
 */
void hgEcsUnregisterComponent(HgEcs* ecs, u64 componentId);

/**
 * Unregister a component
 */
template<typename T>
void hgEcsUnregisterComponent(HgEcs* ecs)
{
    hgEcsUnregisterComponent(ecs, hgComponentId<T>);
}

/**
 * Returns the name of the component type
 */
HgStringView hgEcsComponentName(HgEcs* ecs, u64 componentId);

/**
 * Return a new entity
 */
HgEntity hgEcsSpawn(HgEcs* ecs);

/**
 * Destroy an entity
 *
 * Note, this function will invalidate iterators
 */
void hgEcsDespawn(HgEcs* ecs, HgEntity e);

/**
 * Return whether an entity is alive
 */
bool hgEcsAlive(HgEcs* ecs, HgEntity e);

/**
 * Add a component to an entity
 *
 * Note, the entity must not have a component of this type already
 *
 * Returns
 * - A pointer to the created component
 */
void* hgEcsAdd(HgEcs* ecs, HgEntity e, u64 componentId);

/**
 * Add a component to an entity
 *
 * Note, the entity must not have a component of this type already
 *
 * Returns
 * - A pointer to the created component
 */
template<typename T>
T* hgEcsAdd(HgEcs* ecs, HgEntity e)
{
    return (T*)hgEcsAdd(ecs, e, hgComponentId<T>);
}

/**
 * Remove a component from an entity
 *
 * Note, this function will invalidate iterators
 */
void hgEcsRemove(HgEcs* ecs, HgEntity e, u64 componentId);

/**
 * Remove a component from an entity
 *
 * Note, this function will invalidate iterators
 */
template<typename T>
void hgEcsRemove(HgEcs* ecs, HgEntity e)
{
    hgEcsRemove(ecs, e, hgComponentId<T>);
}

/**
 * Check whether an entity has a component or not
 */
bool hgEcsHas(HgEcs* ecs, HgEntity e, u64 componentId);

/**
 * Check whether an entity has a component or not
 */
template<typename T>
bool hgEcsHas(HgEcs* ecs, HgEntity e)
{
    return hgEcsHas(ecs, e, hgComponentId<T>);
}

/**
 * Return whether the entity has all given components
 */
template<typename... Ts>
bool hgEcsHasAll(HgEcs* ecs, HgEntity e)
{
    return (hgEcsHas<Ts>(ecs, e) && ...);
}

/**
 * Return whether the entity has any of the given components
 */
template<typename... Ts>
bool hgEcsHasAny(HgEcs* ecs, HgEntity e)
{
    return (hgEcsHas<Ts>(ecs, e) || ...);
}

/**
 * Get a pointer to the entity's component
 *
 * Note, the entity must have the component
 *
 * Returns
 * - A pointer to the entity's component, will never be nullptr
 */
void* hgEcsGet(HgEcs* ecs, HgEntity e, u64 componentId);

/**
 * Get a reference to the entity's component
 *
 * Note, the entity must have the component
 *
 * Returns
 * - A reference to the entity's component
 */
template<typename T>
T* hgEcsGet(HgEcs* ecs, HgEntity e)
{
    return (T*)hgEcsGet(ecs, e, hgComponentId<T>);
}

/**
 * Get the entity from it's component
 *
 * Parameters
 * - c The component to lookup, must be a valid component
 * - componentId The id of the component
 */
HgEntity hgEcsGetEntity(HgEcs* ecs, const void* c, u64 componentId);

/**
 * Get the entity from it's component
 *
 * Parameters
 * - c The component to lookup, must be a valid component
 */
template<typename T>
HgEntity hgEcsGetEntity(HgEcs* ecs, const T* c)
{
    return hgEcsGetEntity(ecs, (void*)c, hgComponentId<T>);
}

/**
 * Return a pointer to all entities of type
 */
HgEntity* hgEcsEntities(HgEcs* ecs, u64 componentId);

/**
 * Return a pointer to all entities of type
 */
template<typename T>
HgEntity* hgEcsEntities(HgEcs* ecs)
{
    return hgEcsEntities(ecs, hgComponentId<T>);
}

/**
 * Return a pointer to all components of type
 */
void* hgEcsComponents(HgEcs* ecs, u64 componentId);

/**
 * Return a pointer to all components of type
 */
template<typename T>
T* hgEcsComponents(HgEcs* ecs)
{
    return (T*)hgEcsComponents(ecs, hgComponentId<T>);
}

/**
 * Return the number of active components of a type
 */
u32 hgEcsCount(HgEcs* ecs, u64 componentId);

/**
 * Return the number of active components of a type
 */
template<typename T>
u32 hgEcsCount(HgEcs* ecs)
{
    return hgEcsCount(ecs, hgComponentId<T>);
}

/**
 * Find the id of the system with the fewest elements
 */
u64 hgEcsFindSmallest(HgEcs* ecs, u64* ids, u32 idCount);

/**
 * Find the id of the system with the fewest elements
 */
template<typename... Ts>
u64 hgEcsFindSmallest(HgEcs* ecs)
{
    u32 index = 0;
    u64 ids[sizeof...(Ts)];
    ((ids[index++] = hgComponentId<Ts>), ...);
    return hgEcsFindSmallest(ecs, ids, sizeof...(Ts));
}

/**
 * Iterate over all entities with the single given component
 *
 * Note, specifying only one component allows deterministic ordering (such
 * as in the case of sorting), as well as extra optimization
 *
 * The function receives as parameters:
 * - The entity id
 * - A reference to the component
 *
 * Parameters
 * - function The function to call
 */
template<typename T, typename Fn>
void hgEcsForEachSingle(HgEcs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, HgEntity, T*>);

    HgEntity* e = hgEcsEntities<T>(ecs);
    HgEntity* end = e + hgEcsCount<T>(ecs);
    T* c = hgEcsComponents<T>(ecs);
    for (; e != end; ++e, ++c)
    {
        fn(*e, c);
    }
}

/**
 * Iterate over all entities with the given components
 *
 * The function receives as parameters:
 * - The entity id
 * - A reference to each component...
 *
 * Parameters
 * - function The function to call
 */
template<typename... Ts, typename Fn>
void hgEcsForEachMulti(HgEcs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, HgEntity, Ts*...>);

    u64 id = hgEcsFindSmallest<Ts...>(ecs);
    HgComponent* system = hgMapGet(&ecs->components, id);
    hgAssert(system != nullptr);

    HgEntity* e = system->entities.vals + 1;
    HgEntity* end = e + system->entities.count - 1;
    for (; e != end; ++e)
    {
        if (hgEcsHasAll<Ts...>(ecs, *e))
            fn(*e, hgEcsGet<Ts>(ecs, *e)...);
    }
}

/**
 * Iterate over all entities with the given components
 *
 * Note, calls the single or multi version from the number of components
 *
 * Parameters
 * - function The function to call
 */
template<typename... Ts, typename Fn>
void hgEcsForEach(HgEcs* ecs, Fn fn)
{
    static_assert(sizeof...(Ts) != 0);

    if constexpr (sizeof...(Ts) == 1)
    {
        hgEcsForEachSingle<Ts...>(ecs, fn);
    } else {
        hgEcsForEachMulti<Ts...>(ecs, fn);
    }
}

/**
 * Iterate over all entities with the single given component
 *
 * Note, specifying only one component allows deterministic ordering (such
 * as in the case of sorting), as well as extra optimization
 *
 * The function receives as parameters:
 * - The entity id
 * - A reference to the component
 *
 * Parameters
 * - fn The function to call
 */
template<typename T, typename Fn>
void hgEcsForParSingle(HgEcs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, HgEntity, T*>);

    struct Capture {
        HgEcs* ecs;
        Fn* fn;
    };
    Capture capture{ecs, &fn};

    hgThreadsFor(0, hgEcsCount<T>(ecs), &capture, [](void* pcapture, u64 idx)
    {
        Capture* capture = (Capture*)pcapture;
        (*capture->fn)(
            hgEcsEntities<T>(capture->ecs)[idx],
            &hgEcsComponents<T>(capture->ecs)[idx]);
    });
}

/**
 * Iterate over all entities with the given components
 *
 * The function receives as parameters:
 * - The entity id
 * - A reference to each component...
 *
 * Parameters
 * - fn The function to call
 */
template<typename... Ts, typename Fn>
void hgEcsForParMulti(HgEcs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, HgEntity, Ts*...>);

    HgComponent* system = hgMapGet(&ecs->components, hgEcsFindSmallest<Ts...>(ecs));
    hgAssert(system != nullptr);

    struct Capture {
        HgEcs* ecs;
        HgComponent* system;
        Fn* fn;
    };
    Capture capture{ecs, system, &fn};

    hgThreadsFor(1, system->entities.count, &capture, [](void* pcapture, u64 idx)
    {
        Capture* capture = (Capture*)pcapture;
        HgEntity e = capture->system->entities[idx];
        if (hgEcsHasAll<Ts...>(capture->ecs, e))
            (*capture->fn)(e, hgEcsGet<Ts>(capture->ecs, e)...);
    });
}

/**
 * Iterate over all entities with the given components
 *
 * Note, calls the single of multi version from the number of components
 *
 * The function receives as parameters:
 * - The entity id
 * - A reference to each component...
 *
 * Parameters
 * - chunkSize The number of executions per group
 * - fn The function to call
 */
template<typename... Ts, typename Fn>
void hgEcsForPar(HgEcs* ecs, Fn fn)
{
    static_assert(sizeof...(Ts) != 0);

    if constexpr (sizeof...(Ts) == 1)
    {
        hgEcsForParSingle<Ts...>(ecs, fn);
    } else {
        hgEcsForParMulti<Ts...>(ecs, fn);
    }
}

/**
 * Sort components
 *
 * Parameters
 * - componentId The component system to sort
 * - data The data passed to compare
 * - compare The comparison function
 */
void hgEcsSort(HgEcs* ecs, u64 componentId, void* data, bool (*compare)(void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs));

/**
 * Sort components
 *
 * Parameters
 * - data The data passed to compare
 * - compare The comparison function
 */
template<typename T>
void hgEcsSort(HgEcs* ecs, void* data, bool (*compare)(void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs))
{
    hgEcsSort(ecs, hgComponentId<T>, data, compare);
}

/**
 * A node component for entities in a hierarchy
 */
struct HgNode {
    /**
     * The entity's parent, if any
     */
    HgEntity parent{};
    /**
     * The next child of this entity's parent
     */
    HgEntity nextSibling{};
    /**
     * The previous child of this entity's parent
     */
    HgEntity prevSibling{};
    /**
     * The first of this entity's children, forming a linked list
     */
    HgEntity firstChild{};
};

/**
 * HgNode serialization implementation
 */
template<>
void hgEcsSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgNode* node, HgEntitySerializer* ecs);

/**
 * Add an empty node to an entity
 */
HgNode* hgNodeAdd(HgEcs* ecs, HgEntity e);

/**
 * Remove the entity from its hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to detach, must be alive
 */
void hgNodeDetach(HgEcs* ecs, HgEntity e);

/**
 * Destroy the entity and all its children in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to destroy to, must be alive
 */
void hgNodeDestroy(HgEcs* ecs, HgEntity e);

/**
 * Add a new child to an entity in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - parent The parent to add to, must be alive
 * - child The child to add, must be alive
 */
void hgNodeAddChild(HgEcs* ecs, HgEntity parent, HgEntity child);

/**
 * The transform component for entities in absolute space
 */
struct HgTransform {
    /**
     * The entity's transform model matrix in world space
     */
    HgMat4 mat{1.0f};
    /**
     * The entity's position relative to its parent
     * - x: -left, +right
     * - y: -up, +down
     * - z: -backward, +forward
     */
    HgVec3 position{0.0f, 0.0f, 0.0f};
    /**
     * The entity's scaling relative to its parent
     * - x: horizonatal
     * - y: vertical
     * - z: depth
     */
    HgVec3 scale{1.0f, 1.0f, 1.0f};
    /**
     * The entity's rotation relative to its parent
     */
    HgQuat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

/**
 * HgTransform serialization impl
 */
template<>
void hgSerialize(HgArena* arena, HgSerializer* s, HgStringView name, HgTransform* node);

/**
 * Add an identity transform to an entity
 */
HgTransform* hgTransformAdd(
    HgEcs* ecs,
    HgEntity e,
    HgVec3 position = HgVec3{0.0f},
    HgVec3 scale = HgVec3{1.0f},
    HgQuat rotation = HgQuat{1.0f});

/**
 * Get the position from HgTransform mat
 */
constexpr HgVec3 hgTransformWorldPos(HgTransform& tf)
{
    return HgVec3{tf.mat.w};
}

/**
 * Update HgTransform for the entity and its children to the HgTransformLocal
 */
void hgTransformUpdate(HgEcs* ecs, HgEntity e);

#endif // HG_CONTAINERS_HPP

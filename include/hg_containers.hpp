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
#include "hg_memory.hpp"
#include "hg_serialization.hpp"
#include "hg_strings.hpp"

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
 * HgArray serialization
 */
template<typename T>
void hgSerialize(HgSerializer* s, HgArray<T>* arr);

/**
 * Create an array
 */
template<typename T>
HgArray<T> hgArrayCreate(u32 count = 0, u32 capacity = 1024);

/**
 * Destroy an array
 */
template<typename T>
void hgArrayDestroy(HgArray<T>* arr);

/**
 * Create a temporary array which need not be destroyed, but cannot be resized
 */
template<typename T>
HgArray<T> hgArrayTemp(HgArena* arena, u32 count = 0, u32 capacity = 1024);

/**
 * Resize an array
 */
template<typename T>
void hgArrayResize(HgArray<T>* arr, u32 newCount);

/**
 * Push a value to the end of the array
 */
template<typename T>
T* hgArrayPush(HgArray<T>* arr);

/**
 * Push a value to the end of the array using an arena
 */
template<typename T>
T* hgArrayPushTemp(HgArena* arena, HgArray<T>* arr);

/**
 * Pop a value from the end of the array
 */
template<typename T>
T hgArrayPop(HgArray<T>* arr);

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
 * HgArrayAny serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgArrayAny* arr);

/**
 * Create an array of unknown type
 */
HgArrayAny hgArrayAnyCreate(u32 width, u32 align, u32 count = 0, u32 capacity = 1024);

/**
 * Destroy an array of unknown type
 */
void hgArrayAnyDestroy(HgArrayAny* arr);

/**
 * Create a temporary array which need not be destroyed, but cannot be resized
 */
HgArrayAny hgArrayAnyTemp(HgArena* arena, u32 width, u32 align, u32 count = 0, u32 capacity = 1024);

/**
 * Resize an array of unknown type
 */
void hgArrayAnyResize(HgArrayAny* arr, u32 newCount);

/**
 * Push a value to the end of the array
 */
void* hgArrayAnyPush(HgArrayAny* arr);

/**
 * Push a value to the end of the array using an arena
 */
void* hgArrayAnyPushTemp(HgArena* arena, HgArrayAny* arr);

/**
 * Pop a value from the end of the array
 */
void hgArrayAnyPop(HgArrayAny* arr, void* dst);

/**
 * A double ended ring buffer queue
 */
template<typename T>
struct HgQueue {
    T* vals;
    u32 front;
    u32 back;
    u32 count;
    u32 capacity;
};

/**
 * Create a new empty queue
 */
template<typename T>
HgQueue<T> hgQueueCreate(u32 capacity = 1024);

/**
 * Destroy a queue
 */
template<typename T>
void hgQueueDestroy(HgQueue<T>* queue);

/**
 * Push a value to the front of the queue
 */
template<typename T, typename U = T>
void hgQueuePushFront(HgQueue<T>* queue, U val);

/**
 * Push a value to the back of the queue
 */
template<typename T, typename U = T>
void hgQueuePushBack(HgQueue<T>* queue, U val);

/**
 * Pop a value from the front of the queue
 */
template<typename T>
T hgQueuePopFront(HgQueue<T>* queue);

/**
 * Pop a value from the back of the queue
 */
template<typename T>
T hgQueuePopBack(HgQueue<T>* queue);

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
 * HgSet serialization
 */
template<typename V>
void hgSerialize(HgSerializer* s, HgSet<V>* set);

/**
 * Creates a new hash set
 *
 * Parameters
 * - slotCount The max number of slots to store values in
 */
template<typename V>
HgSet<V> hgSetCreate(u32 slotCount);

/**
 * Destroy a hash set
 */
template<typename V>
void hgSetDestroy(HgSet<V>* set);

/**
 * Create a new hash set which need not be destroyed, but cannot be resized
 *
 * Parameters
 * - arena The arena to allocate from
 * - slotCount The max number of slots to store values in
 */
template<typename V>
HgSet<V> hgSetTemp(HgArena* arena, u32 slotCount);

/**
 * Resize the set
 */
template<typename V>
void hgSetResize(HgSet<V>* set, u32 newSize);

/**
 * Empty all slots
 */
template<typename V>
void hgSetReset(HgSet<V>* set);

/**
 * Add a value to the set
 */
template<typename V, typename T = V>
void hgSetAdd(HgSet<V>* set, const T& val);

/**
 * Remove a value from the set
 */
template<typename V, typename T = V>
void hgSetRemove(HgSet<V>* set, const T& val);

/**
 * Check whether a value is contained in the set
 */
template<typename V, typename T = V>
bool hgSetHas(const HgSet<V>* set, const T& val);

/**
 * Calls a function for each value in the hash map
 */
template<typename V, typename F>
void hgSetForEach(HgSet<V>* set, F fn);

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
 * HgMap serialization
 */
template<typename K, typename V>
void hgSerialize(HgSerializer* s, HgMap<K, V>* set);

/**
 * Create a new hash map
 *
 * Parameters
 * - slotCount The max number of slots to store values in
 */
template<typename K, typename V>
HgMap<K, V> hgMapCreate(u32 slotCount = 1024);

/**
 * Destroy a hash map
 */
template<typename K, typename V>
void hgMapDestroy(HgMap<K, V>* map);

/**
 * Create a new hash map which need not be destroyed, but cannot be resized
 *
 * Parameters
 * - arena The arena to allocate from
 * - slotCount The max number of slots to store values in
 */
template<typename K, typename V>
HgMap<K, V> hgMapTemp(HgArena* arena, u32 slotCount);

/**
 * Resize a hash map
 */
template<typename K, typename V>
void hgMapResize(HgMap<K, V>* map, u32 newSize);

/**
 * Empties all slots
 */
template<typename K, typename V>
void hgMapReset(HgMap<K, V>* map);

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
V* hgMapAdd(HgMap<K, V>* map, const T& key, const U& val);

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
bool hgMapRemove(HgMap<K, V>* map, const T& key, V* val = nullptr);

/**
 * Gets the value stored at a key
 *
 * Returns
 * - A pointer to the value, or nullptr if it does not exist
 */
template<typename K, typename V, typename T = K>
V* hgMapGet(const HgMap<K, V>* map, const T& key);

/**
 * Calls a function for each value in the hash map
 */
template<typename K, typename V, typename F>
void hgMapForEach(HgMap<K, V>* map, F fn);

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
 * A pool of objects
 */
template<typename T>
struct HgPool {
    /**
     * The free list
     */
    HgQueue<T*> freeList;
    /**
     * The items in the pool
     */
    HgArray<T*> itemStores;
};

/**
 * Create an object pool
 */
template<typename T>
HgPool<T> hgPoolCreate();

/**
 * Destroy an object pool
 */
template<typename T>
void hgPoolDestroy(HgPool<T>* pool);

/**
 * Allocate an object from the pool
 */
template<typename T>
T* hgPoolAlloc(HgPool<T>* pool);

/**
 * Free an object from the pool
 */
template<typename T>
void hgPoolFree(HgPool<T>* pool, T* item);

/**
 * A generation counted handle
 */
struct HgHandle {
    /**
     * The handle id
     */
    u32 id;
};

/**
 * The null handle
 */
static constexpr HgHandle hgHandleNull = HgHandle{0};

/**
 * Compare handles
 */
constexpr bool operator==(HgHandle lhs, HgHandle rhs)
{
    return lhs.id == rhs.id;
}

/**
 * Compare handles
 */
constexpr bool operator!=(HgHandle lhs, HgHandle rhs)
{
    return lhs.id != rhs.id;
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
    return handle.id & ~(((u32)1 << hgHandleIdxBits) - (u32)1);
}

/**
 * Returns a new handle at the same index
 */
constexpr HgHandle hgHandleNextGeneration(HgHandle handle)
{
    return {handle.id + (1 << hgHandleIdxBits)};
}

/**
 * A handle pool
 */
struct HgHandlePool {
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
HgHandlePool hgHandlePoolCreate();

/**
 * Destroy a handle pool
 */
void hgHandlePoolDestroy(HgHandlePool* pool);

/**
 * Reset a handle pool
 */
void hgHandlePoolReset(HgHandlePool* pool);

/**
 * Allocate an index from the pool
 */
HgHandle hgHandlePoolAlloc(HgHandlePool* pool);

/**
 * Returns whether a handle is alive in the pool
 */
bool hgHandlePoolAlive(HgHandlePool* pool, HgHandle handle);

/**
 * Free an index back into a pool
 *
 * Note, the object handle must be valid and alive
 */
void hgHandlePoolFree(HgHandlePool* pool, HgHandle handle);

#endif // HG_CONTAINERS_HPP

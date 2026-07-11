/*
 * =============================================================================
 *
 * Copyright (c) 2025-2026 Cooper Herlihy
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

namespace hg {

/**
 * A dynamic array
 */
template<typename T>
struct Array {
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
        HG_ASSERT(vals != nullptr);
        HG_ASSERT(idx < count);
        return vals[idx];
    }
};

/**
 * Array serialization
 */
template<typename T>
void serialize(Serializer* s, Array<T>* arr);

/**
 * Create an array
 */
template<typename T>
Array<T> arrayCreate(u32 count = 0, u32 capacity = 1024);

/**
 * Destroy an array
 */
template<typename T>
void arrayDestroy(Array<T>* arr);

/**
 * Create a temporary array which need not be destroyed, but cannot be resized
 */
template<typename T>
Array<T> arrayTemp(Arena* arena, u32 count = 0, u32 capacity = 1024);

/**
 * Resize an array
 */
template<typename T>
void arrayResize(Array<T>* arr, u32 newCount);

/**
 * Resize an array using an arena
 */
template<typename T>
void arrayResizeTemp(Arena* arena, Array<T>* arr, u32 newCount);

/**
 * Push a value to the end of the array
 */
template<typename T>
T* arrayPush(Array<T>* arr);

/**
 * Push a value to the end of the array using an arena
 */
template<typename T>
T* arrayPushTemp(Arena* arena, Array<T>* arr);

/**
 * Remove the value at idx in the array, with stanle order
 */
template<typename T>
T arrayRemove(Array<T>* arr, u32 idx);

/**
 * Remove the value at idx in the array, without stable order
 */
template<typename T>
T arrayRemoveSwap(Array<T>* arr, u32 idx);

/**
 * Pop a value from the end of the array
 */
template<typename T>
T arrayPop(Array<T>* arr);

/**
 * An array of values of unknown type
 */
struct ArrayAny {
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
        HG_ASSERT(vals != nullptr);
        HG_ASSERT(idx < count);
        return (u8*)vals + idx * width;
    }
};

/**
 * ArrayAny serialization
 */
template<>
void serialize(Serializer* s, ArrayAny* arr);

/**
 * Create an array of unknown type
 */
ArrayAny arrayAnyCreate(u32 width, u32 align, u32 count = 0, u32 capacity = 1024);

/**
 * Destroy an array of unknown type
 */
void arrayAnyDestroy(ArrayAny* arr);

/**
 * Create a temporary array which need not be destroyed, but cannot be resized
 */
ArrayAny arrayAnyTemp(Arena* arena, u32 width, u32 align, u32 count = 0, u32 capacity = 1024);

/**
 * Resize an array of unknown type
 */
void arrayAnyResize(ArrayAny* arr, u32 newCount);

/**
 * Resize an array of unkown type using an arena
 */
void arrayAnyResizeTemp(Arena* arean, ArrayAny* arr, u32 newCount);

/**
 * Push a value to the end of the array
 */
void* arrayAnyPush(ArrayAny* arr);

/**
 * Push a value to the end of the array using an arena
 */
void* arrayAnyPushTemp(Arena* arena, ArrayAny* arr);

/**
 * Remove a value from the array, with stable order
 */
void arrayAnyRemove(ArrayAny* arr, u32 idx, void* dst);

/**
 * Remove a value from the array, without stable order
 */
void arrayAnyRemoveSwap(ArrayAny* arr, u32 idx, void* dst);

/**
 * Pop a value from the end of the array
 */
void arrayAnyPop(ArrayAny* arr, void* dst);

/**
 * A double ended ring buffer queue
 */
template<typename T>
struct Queue {
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
Queue<T> queueCreate(u32 capacity = 1024);

/**
 * Destroy a queue
 */
template<typename T>
void queueDestroy(Queue<T>* queue);

/**
 * Push a value to the front of the queue
 */
template<typename T, typename U = T>
void queuePushFront(Queue<T>* queue, U val);

/**
 * Push a value to the back of the queue
 */
template<typename T, typename U = T>
void queuePushBack(Queue<T>* queue, U val);

/**
 * Pop a value from the front of the queue
 */
template<typename T>
T queuePopFront(Queue<T>* queue);

/**
 * Pop a value from the back of the queue
 */
template<typename T>
T queuePopBack(Queue<T>* queue);

/**
 * The hash template
 */
template<typename T>
constexpr u64 hash(T)
{
    static_assert(false, "hash must be implemented for each type");
    return 0;
}

/**
 * A hash set
 */
template<typename V>
struct Set {
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
 * Set serialization
 */
template<typename V>
void serialize(Serializer* s, Set<V>* set);

/**
 * Creates a new hash set
 *
 * Parameters
 * - slotCount The max number of slots to store values in
 */
template<typename V>
Set<V> setCreate(u32 slotCount);

/**
 * Destroy a hash set
 */
template<typename V>
void setDestroy(Set<V>* set);

/**
 * Create a new hash set which need not be destroyed, but cannot be resized
 *
 * Parameters
 * - arena The arena to allocate from
 * - slotCount The max number of slots to store values in
 */
template<typename V>
Set<V> setTemp(Arena* arena, u32 slotCount);

/**
 * Resize the set
 */
template<typename V>
void setResize(Set<V>* set, u32 newSize);

/**
 * Empty all slots
 */
template<typename V>
void setReset(Set<V>* set);

/**
 * Add a value to the set
 */
template<typename V, typename T = V>
void setAdd(Set<V>* set, const T& val);

/**
 * Remove a value from the set
 */
template<typename V, typename T = V>
void setRemove(Set<V>* set, const T& val);

/**
 * Check whether a value is contained in the set
 */
template<typename V, typename T = V>
bool setHas(const Set<V>* set, const T& val);

/**
 * Calls a function for each value in the hash map
 */
template<typename V, typename F>
void setForEach(Set<V>* set, F fn);

/**
 * A key-value hash map
 */
template<typename K, typename V>
struct Map {
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
 * Map serialization
 */
template<typename K, typename V>
void serialize(Serializer* s, Map<K, V>* set);

/**
 * Create a new hash map
 *
 * Parameters
 * - slotCount The max number of slots to store values in
 */
template<typename K, typename V>
Map<K, V> mapCreate(u32 slotCount = 1024);

/**
 * Destroy a hash map
 */
template<typename K, typename V>
void mapDestroy(Map<K, V>* map);

/**
 * Create a new hash map which need not be destroyed, but cannot be resized
 *
 * Parameters
 * - arena The arena to allocate from
 * - slotCount The max number of slots to store values in
 */
template<typename K, typename V>
Map<K, V> mapTemp(Arena* arena, u32 slotCount);

/**
 * Resize a hash map
 */
template<typename K, typename V>
void mapResize(Map<K, V>* map, u32 newSize);

/**
 * Empties all slots
 */
template<typename K, typename V>
void mapReset(Map<K, V>* map);

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
V* mapAdd(Map<K, V>* map, const T& key, const U& val);

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
bool mapRemove(Map<K, V>* map, const T& key, V* val = nullptr);

/**
 * Gets the value stored at a key
 *
 * Returns
 * - A pointer to the value, or nullptr if it does not exist
 */
template<typename K, typename V, typename T = K>
V* mapGet(const Map<K, V>* map, const T& key);

/**
 * Calls a function for each value in the hash map
 */
template<typename K, typename V, typename F>
void mapForEach(Map<K, V>* map, F fn);

/**
 * Hash map hashing for u8
 */
template<>
constexpr u64 hash(u8 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u16
 */
template<>
constexpr u64 hash(u16 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u32
 */
template<>
constexpr u64 hash(u32 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u64
 */
template<>
constexpr u64 hash(u64 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i8
 */
template<>
constexpr u64 hash(i8 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i16
 */
template<>
constexpr u64 hash(i16 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i32
 */
template<>
constexpr u64 hash(i32 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i64
 */
template<>
constexpr u64 hash(i64 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for f32
 */
template<>
constexpr u64 hash(f32 val)
{
    union {
        f32 asFloat;
        u32 asHash;
    } u{};
    u.asFloat = val;
    return (u64)u.asHash;
}

/**
 * Hash map hashing for f64
 */
template<>
constexpr u64 hash(f64 val)
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
constexpr u64 hashPtr(T* val)
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
constexpr u64 hash(void* val)
{
    return hashPtr<void>(val);
}

/**
 * Hash map hashing for strings
 */
template<>
constexpr u64 hash(String str)
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
constexpr u64 hash(const char* str)
{
    return hash(String{str});
}

/**
 * Hash map hashing for StringBuilder
 */
template<>
constexpr u64 hash(StringBuilder str)
{
    return hash(String{str});
}

/**
 * A pool of objects
 */
struct Pool {
    /**
     * The free list
     */
    Queue<void*> freeList;
    /**
     * The items in the pool
     */
    Array<void*> itemStores;
    /**
     * The size of each item in bytes
     */
    u32 width;
    /**
     * The alignment of each item in bytes
     */
    u32 align;
};

/**
 * Create an object pool
 */
Pool poolCreate(u32 width, u32 align);

/**
 * Create an object pool
 */
template<typename T>
Pool poolCreate()
{
    return poolCreate(sizeof(T), alignof(T));
}

/**
 * Destroy an object pool
 */
void poolDestroy(Pool* pool);

/**
 * Allocate an object from the pool
 */
void* poolAlloc(Pool* pool);

/**
 * Free an object from the pool
 */
void poolFree(Pool* pool, void* item);

/**
 * A generation counted handle
 */
struct Handle {
    /**
     * The handle id
     */
    u32 id;
};

/**
 * The null handle
 */
static constexpr Handle handleNull = Handle{0};

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
 * The number of bits in a handle used for the index
 */
static constexpr u32 handleIdxBits = 24;

/**
 * Get the index from a handle
 */
constexpr u32 handleIdx(Handle handle)
{
    return handle.id & ((1 << handleIdxBits) - 1);
}

/**
 * Get the generation from a handle
 */
constexpr u32 handleGeneration(Handle handle)
{
    return handle.id & ~(((u32)1 << handleIdxBits) - (u32)1);
}

/**
 * Returns a new handle at the same index
 */
constexpr Handle handleNextGeneration(Handle handle)
{
    return {handle.id + (1 << handleIdxBits)};
}

/**
 * A handle pool
 */
struct HandlePool {
    /**
     * The currently active handles, or null in vacant slots
     */
    Array<Handle> handles;
    /**
     * The freed handles
     */
    Array<Handle> freed;
};

/**
 * Create a new object pool
 */
HandlePool handlePoolCreate();

/**
 * Destroy a handle pool
 */
void handlePoolDestroy(HandlePool* pool);

/**
 * Reset a handle pool
 */
void handlePoolReset(HandlePool* pool);

/**
 * Allocate an index from the pool
 */
Handle handlePoolAlloc(HandlePool* pool);

/**
 * Returns whether a handle is alive in the pool
 */
bool handlePoolAlive(HandlePool* pool, Handle handle);

/**
 * Free an index back into a pool
 *
 * Note, the object handle must be valid and alive
 */
void handlePoolFree(HandlePool* pool, Handle handle);

} // namespace hg

#endif // CONTAINERS_HPP

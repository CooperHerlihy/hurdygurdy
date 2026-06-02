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

/**
 * The hash template
 */
template<typename T>
constexpr u64 hgHashImpl(T)
{
    static_assert(false, "hgHashImpl must be implemented for each type");
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

    u32 idx = hgHashImpl(v) % set->capacity;
    for (u32 dist = 0; set->hasVal[idx] && !(set->vals[idx] == v); ++dist)
    {
        u32 otherDist = hgHashImpl(set->vals[idx]) % set->capacity - idx;
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
    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    u32 idx = hgHashImpl(v) % set->capacity;
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
        if (hgHashImpl(set->vals[next]) % set->capacity != next)
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

    for (u32 idx = hgHashImpl(v) % set->capacity; set->hasVal[idx]; idx = (idx + 1) % set->capacity)
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

    u32 idx = hgHashImpl(k) % map->capacity;
    for (u32 dist = 0; map->hasVal[idx] && !(map->keys[idx] == k); ++dist)
    {
        u32 otherDist = hgHashImpl(map->keys[idx]) % map->capacity - idx;
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
    static_assert(std::is_convertible_v<T, K>);
    K k = (K)key;

    u32 idx = hgHashImpl(k) % map->capacity;
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
        if (hgHashImpl(map->keys[next]) % map->capacity != next)
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

    for (u32 idx = hgHashImpl(key) % map->capacity; map->hasVal[idx]; idx = (idx + 1) % map->capacity)
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
constexpr u64 hgHashImpl(u8 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u16
 */
template<>
constexpr u64 hgHashImpl(u16 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u32
 */
template<>
constexpr u64 hgHashImpl(u32 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u64
 */
template<>
constexpr u64 hgHashImpl(u64 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i8
 */
template<>
constexpr u64 hgHashImpl(i8 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i16
 */
template<>
constexpr u64 hgHashImpl(i16 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i32
 */
template<>
constexpr u64 hgHashImpl(i32 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i64
 */
template<>
constexpr u64 hgHashImpl(i64 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for f32
 */
template<>
constexpr u64 hgHashImpl(f32 val)
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
constexpr u64 hgHashImpl(f64 val)
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
constexpr u64 hgHashImpl(void* val)
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
 * The null handle
 */
static constexpr HgHandle hgNullHandle = HgHandle{};

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
 * Hash map hashing for void*
 */
template<>
constexpr u64 hgHashImpl(HgHandle val)
{
    return hgHashImpl(val.id);
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
 * An index pool
 */
struct HgPool {
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
HgPool hgPoolCreate(HgArena* arena, u32 capacity);

/**
 * Reset an object pool
 */
void hgPoolReset(HgPool* pool);

/**
 * Allocate an index from the pool
 */
HgHandle hgPoolAlloc(HgPool* pool);

/**
 * Returns whether a handle is alive in the pool
 */
bool hgPoolAlive(HgPool* pool, HgHandle handle);

/**
 * Free an index back into a pool
 *
 * Note, the object handle must be valid and alive
 */
void hgPoolFree(HgPool* pool, HgHandle handle);

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
HgBinary hgBinaryResize(HgArena* arena, const HgBinary* bin, u64 newSize);

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

/**
 * A span view into a string
 */
struct HgStringView {
    /**
     * The characters
     */
    const char* chars;
    /**
     * The number of characters;
     */
    u64 length;

    /**
     * Construct uninitialized
     */
    HgStringView() = default;

    /**
     * Create a string view from a pointer and length
     */
    constexpr HgStringView(const char* charsVal, u64 lengthVal)
        : chars{charsVal}, length{lengthVal} {}

    /**
     * Create a string view from begin and end pointers
     */
    constexpr HgStringView(const char* charsBegin, const char* charsEnd)
        : chars{charsBegin}, length{(uptr)(charsEnd - charsBegin)}
    {
        hgAssert(charsBegin <= charsEnd);
    }

    /**
     * Implicit constexpr conversion from c string
     *
     * Potentially dangerous, c string should be at most 4096 chars
     */
    constexpr HgStringView(const char* cStr) : chars{cStr}, length{0}
    {
        if (cStr == nullptr)
            return;

        while (cStr[length] != '\0')
        {
            ++length;
            hgAssert(length <= 4096);
        }
    }

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr const char& operator[](u64 index) const
    {
        hgAssert(chars != nullptr);
        hgAssert(index < length);
        return chars[index];
    }
};

/**
 * Hash map hashing for strings
 */
template<>
constexpr u64 hgHashImpl(HgStringView str)
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
 * Compare string views
 */
constexpr bool operator==(HgStringView lhs, HgStringView rhs)
{
    return lhs.length == rhs.length && (lhs.length == 0 || memcmp(lhs.chars, rhs.chars, lhs.length) == 0);
}

/**
 * Compare string views
 */
constexpr bool operator!=(HgStringView lhs, HgStringView rhs)
{
    return !(lhs == rhs);
}

/**
 * Create a null terminated string for C interop
 *
 * Parameters
 * - arena The arena to allocate from
 * - str The string to create from
 */
char* hgCString(HgArena* arena, HgStringView str);

/**
 * Hash map hashing for C string
 */
template<>
constexpr u64 hgHashImpl(const char* str)
{
    return hgHashImpl(HgStringView{str});
}

/**
 * A dynamically allocated owning string
 */
struct HgStringOwner {
    /**
     * The string characters
     */
    const char* chars;
    /**
     * The length of the string
     */
    u64 length;

    /**
     * Access using the index operator
     */
    constexpr const char& operator[](u64 index) const
    {
        hgAssert(index < length);
        return chars[index];
    }

    /**
     * Implicit converts to a string view
     */
    constexpr operator HgStringView() const
    {
        return {chars, length};
    }
};

/**
 * Hash map hashing for HgStringOwner
 */
template<>
constexpr u64 hgHashImpl(HgStringOwner str)
{
    return hgHashImpl(HgStringView{str});
}

/**
 * Compare string owners
 */
inline bool operator==(const HgStringOwner& lhs, const HgStringOwner& rhs)
{
    return HgStringView{lhs} == HgStringView{rhs};
}

/**
 * Compare string owners
 */
inline bool operator!=(const HgStringOwner& lhs, const HgStringOwner& rhs)
{
    return !(lhs == rhs);
}

/**
 * A string builder using arenas
 */
struct HgStringBuilder {
    /**
     * The string data
     */
    char* chars;
    /**
     * The number of characters currently in the string
     */
    u64 length;

    /**
     * Access using the index operator
     */
    constexpr char& operator[](u64 index) const
    {
        hgAssert(index < length);
        return chars[index];
    }

    /**
     * Implicit converts to a string view
     */
    constexpr operator HgStringView() const
    {
        return {chars, length};
    }
};

/**
 * Hash map hashing for HgStringBuilder
 */
template<>
constexpr u64 hgHashImpl(HgStringBuilder str)
{
    return hgHashImpl(HgStringView{str});
}

/**
 * Compare string builders
 */
inline bool operator==(const HgStringBuilder& lhs, const HgStringBuilder& rhs)
{
    return HgStringView{lhs} == HgStringView{rhs};
}

/**
 * Compare string builders
 */
inline bool operator!=(const HgStringBuilder& lhs, const HgStringBuilder& rhs)
{
    return !(lhs == rhs);
}

/**
 * Creates a new string copied from an existing string
 *
 * Parameters
 * - arena The arena to allocate from
 * - init The initial string to copy from
 */
HgStringBuilder hgStringCopy(HgArena* arena, HgStringView str);

/**
 * Create a formatted string : TODO
 *
 * Format specifiers
 * - int (i64): "{i}"
 * - unsigned int (u64): "{u}"
 * - hexadecimal (i64): "{x}"
 * - float with 6 decimals (f64): "{f}"
 * - float with N decimals (f64): "{fN}"
 * - char (char): "{c}"
 * - string (HgStringView): "{s}"
 * - c string (char*): "{cstr}"
 *
 * Use {{ and }} to escape the format specifier
 *
 * Parameters
 * - arena The arena to allocate from
 * - fmt The format string
 * - ... The format parameters
 */
HgStringBuilder hgStringFormat(HgArena* arena, HgStringView fmt, ...);

/**
 * Copies another string into the string at index
 *
 * Parameters
 * - arena The arena to allocate from
 * - dst The string to insert into
 * - idx The index into dst
 * - src The string to copy from
 */
void hgStringInsert(HgArena* arena, HgStringBuilder* dst, u64 idx, HgStringView src);

/**
 * Copies another string to the end of the string
 */
inline void hgStringAppend(HgArena* arena, HgStringBuilder* dst, HgStringView src)
{
    hgStringInsert(arena, dst, dst->length, src);
}

/**
 * Copies another string to the beginning of the string
 */
inline void hgStringPrepend(HgArena* arena, HgStringBuilder* dst, HgStringView src)
{
    hgStringInsert(arena, dst, 0, src);
}

/**
 * Copies a character into the string at index
 *
 * Parameters
 * - arena The arena to allocate from
 * - dst The string to insert into
 * - idx The index into dst
 * - c The character to insert
 */
inline void hgStringInsertc(HgArena* arena, HgStringBuilder* dst, u64 idx, char c)
{
    hgStringInsert(arena, dst, idx, {&c, 1});
}

/**
 * Copies another string to the end of the string
 */
inline void hgStringAppendc(HgArena* arena, HgStringBuilder* dst, char c)
{
    hgStringInsertc(arena, dst, dst->length, c);
}

/**
 * Copies another string to the beginning of the string
 */
inline void hgStringPrependc(HgArena* arena, HgStringBuilder* dst, char c)
{
    hgStringInsertc(arena, dst, 0, c);
}

/**
 * Allocate a new HgStringOwner
 */
HgStringOwner hgStringAlloc(HgStringView data);

/**
 * Free an HgStringOwner
 */
void hgStringFree(HgStringOwner* str);

/**
 * Check whether a character is whitespace (space, tab, or newline)
 */
bool hgIsWhitespace(char c);

/**
 * Check whether a character is a base 10 numeral (0-9)
 */
bool hgIsNumeral(char c);

/**
 * Check whether a string is a base 10 integer
 */
bool hgIsInteger(HgStringView str);

/**
 * Check whether a string is a base 10 floating point number
 */
bool hgIsFloat(HgStringView str);

/**
 * Create an integer from a base 10 string
 */
i64 hgStringToInteger(HgStringView str);

/**
 * Create a float from a base 10 string
 */
f64 hgStringToFloat(HgStringView str);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 */
HgStringBuilder hgIntegerToString(HgArena* arena, i64 num);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 * - decimalCount The number of trailing decimal digits
 */
HgStringBuilder hgFloatToString(HgArena* arena, f64 num, u32 decimalCount);

// base 2 and 16 string-int conversions : TODO
// arbitrary base string-int conversions : TODO?

#endif // HG_CONTAINERS_HPP

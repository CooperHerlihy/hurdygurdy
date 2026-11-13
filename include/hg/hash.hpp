#pragma once

#include "hg/inttypes.hpp"

#include <cstring>
#include <memory>
#include <utility>
#include <string_view>
#include <string>

namespace hg {

/**
 * The hash template
 */
template<typename T>
constexpr u64 hash(const T&)
{
    static_assert(false, "Type cannot be hashed without template specialization");
    return 0;
}

/**
 * u8 hash function
 */
template<>
constexpr u64 hash(const u8& val)
{
    return static_cast<u64>(val);
}

/**
 * u16 hash function
 */
template<>
constexpr u64 hash(const u16& val)
{
    return static_cast<u64>(val);
}

/**
 * u32 hash function
 */
template<>
constexpr u64 hash(const u32& val)
{
    return static_cast<u64>(val);
}

/**
 * u64 hash function
 */
template<>
constexpr u64 hash(const u64& val)
{
    return static_cast<u64>(val);
}

/**
 * i8 hash function
 */
template<>
constexpr u64 hash(const i8& val)
{
    return static_cast<u64>(val);
}

/**
 * i16 hash function
 */
template<>
constexpr u64 hash(const i16& val)
{
    return static_cast<u64>(val);
}

/**
 * i32 hash function
 */
template<>
constexpr u64 hash(const i32& val)
{
    return static_cast<u64>(val);
}

/**
 * i64 hash function
 */
template<>
constexpr u64 hash(const i64& val)
{
    return static_cast<u64>(val);
}

/**
 * f32 hash function
 */
template<>
constexpr u64 hash(const f32& val)
{
    return static_cast<u64>(std::bit_cast<u32>(val));
}

/**
 * f64 hash function
 */
template<>
constexpr u64 hash(const f64& val)
{
    return std::bit_cast<u64>(val);
}

/**
 * pointer hash function
 */
template<typename T>
constexpr u64 hash(T* const& val)
{
    return std::bit_cast<u64>(val);
};

/**
 * unique pointer hash function
 */
template<typename T>
constexpr u64 hash(const std::unique_ptr<T>& val)
{
    return hash(val.get());
};

/**
 * strings hash function
 */
template<>
constexpr u64 hash(const std::string_view& str)
{
    u64 ret = 0;
    u64 mult = 1;
    for (u32 i = 0; i < str.length(); ++i)
    {
        ret += static_cast<u64>(str[i]) * mult;
        mult *= 257;
    }
    return ret;
}

/**
 * std::string hash function
 */
template<>
constexpr u64 hash(const std::string& str)
{
    return hash(std::string_view{str});
}

/**
 * C string hash function
 */
template<>
constexpr u64 hash(const char* const& str)
{
    return hash(std::string_view{str});
}

/**
 * C array string hash function
 */
template<u64 N>
constexpr u64 hash(const char (&str)[N])
{
    return hash(std::string_view{str, N - 1});
}

/**
 * A hash set
 */
template<typename V>
struct HashSet {
    /**
     * Whether each index has a value
     */
    bool* hasVal = nullptr;
    /**
     * Where the values are stored;
     */
    V* vals = nullptr;
    /**
     * The max number of vals
     */
    u64 capacity = 0;
    /**
     * The current number of values that are stored
     */
    u64 count = 0;

    /**
     * Construct empty
     */
    HashSet() noexcept = default;

    /**
     * Destroy the set
     */
    ~HashSet() noexcept
    {
        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                vals[i].~V();
        }
        std::free(hasVal);
        std::free(vals);
    }

    /**
     * Construct with capacity
     */
    explicit HashSet(u64 initCapacity)
        : hasVal{static_cast<bool*>(std::malloc(initCapacity))}
        , vals{static_cast<V*>(std::malloc(initCapacity * sizeof(*vals)))}
        , capacity{initCapacity}
        , count{0}
    {
        std::memset(hasVal, 0, capacity);
    }

    /**
     * Remove all elements
     */
    void clear()
    {
        V* v = vals;
        bool* has = hasVal;
        bool* end = hasVal + capacity;
        for (; has != end; ++has, ++v)
        {
            if (*has)
            {
                v->~V();
                *has = false;
            }
        }
        count = 0;
    }

    /**
     * Change the capacity
     */
    void reserve(u64 newCapacity)
    {
        if (newCapacity <= capacity)
            return;

        HashSet<V> newSet{newCapacity};

        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                newSet.add(std::move(vals[i]));
        }

        *this = std::move(newSet);
    }

    /**
     * Add a value to the set
     */
    void add(const V& val)
    {
        V v = val;
        if (count >= capacity / 2)
            reserve(capacity == 0 ? 128 : capacity * 2);

        u64 idx = static_cast<u64>(hash(v) % capacity);
        for (u64 dist = 0; hasVal[idx] && !(vals[idx] == v); ++dist)
        {
            u64 otherDist = static_cast<u64>(hash(vals[idx]) % capacity) - idx;
            if (otherDist > capacity)
                otherDist += capacity;

            if (otherDist < dist)
            {
                std::swap(v, vals[idx]);
                dist = otherDist;
            }

            idx = (idx + 1) % capacity;
        }

        if (!hasVal[idx])
        {
            new (vals + idx) V{std::move(v)};
            hasVal[idx] = true;
            ++count;
        }
    }

    /**
     * Add a value by rvalue reference to the set
     */
    void add(V&& val)
    {
        if (count >= capacity / 2)
            reserve(capacity == 0 ? 128 : capacity * 2);

        u64 idx = static_cast<u64>(hash(val) % capacity);
        for (u64 dist = 0; hasVal[idx] && !(vals[idx] == val); ++dist)
        {
            u64 otherDist = static_cast<u64>(hash(vals[idx]) % capacity) - idx;
            if (otherDist > capacity)
                otherDist += capacity;

            if (otherDist < dist)
            {
                std::swap(val, vals[idx]);
                dist = otherDist;
            }

            idx = (idx + 1) % capacity;
        }

        if (!hasVal[idx])
        {
            new (vals + idx) V{std::move(val)};
            hasVal[idx] = true;
            ++count;
        }
    }

    /**
     * Remove a value from the set
     */
    template<typename T>
    void remove(const T& val)
    {
        u64 idx = static_cast<u64>(hash(val) % capacity);
        while (hasVal[idx])
        {
            if (vals[idx] == val)
                break;
            idx = (idx + 1) % capacity;
        }
        if (!hasVal[idx])
            return;

        u64 next = (idx + 1) % capacity;
        while (hasVal[next])
        {
            if (hash(vals[next]) % capacity != next)
            {
                vals[idx] = std::move(vals[next]);
                idx = next;
            }
            next = (next + 1) % capacity;
        }

        vals[idx].~V();
        hasVal[idx] = false;
        --count;
    }

    /**
     * Returns whether a value is contained in the set
     */
    template<typename T>
    bool has(const T& val) const
    {
        for (u64 idx = static_cast<u64>(hash(val) % capacity); hasVal[idx]; idx = (idx + 1) % capacity)
        {
            if (vals[idx] == val)
                return true;
        }
        return false;
    }

    /**
     * Calls a function for each value in the hash set
     */
    template<typename F> requires std::is_invocable_r_v<void, F, const V&>
    void forEach(F fn)
    {
        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                fn(vals[i]);
        }
    }

    /**
     * Calls a function for each value in the hash set (const)
     */
    template<typename F> requires std::is_invocable_r_v<void, F, const V&>
    void forEach(F fn) const
    {
        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                fn(vals[i]);
        }
    }

    /**
     * Move construct
     */
    HashSet(HashSet&& other) noexcept
        : hasVal{std::exchange(other.hasVal, nullptr)}
        , vals{std::exchange(other.vals, nullptr)}
        , capacity{std::exchange(other.capacity, 0)}
        , count{std::exchange(other.count, 0)}
    {}

    /**
     * Move assign
     */
    HashSet& operator=(HashSet&& other) noexcept
    {
        if (this != &other)
        {
            this->~HashSet();
            new (this) HashSet{std::move(other)};
        }
        return *this;
    }

    HashSet(const HashSet&) = delete;
    HashSet& operator=(const HashSet&) = delete;
};

/**
 * A key-value hash map
 */
template<typename K, typename V>
struct HashMap {
    /**
     * Whether each index has a value
     */
    bool* hasVal = nullptr;
    /**
     * Where the keys are stored;
     */
    K* keys = nullptr;
    /**
     * Where the values are stored
     */
    V* vals = nullptr;
    /**
     * The max number of key value pairs
     */
    u64 capacity = 0;
    /**
     * The current number of values that are stored
     */
    u64 count = 0;

    /**
     * Construct empty
     */
    HashMap() noexcept = default;

    /**
     * Destroy the Map
     */
    ~HashMap() noexcept
    {
        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
            {
                keys[i].~K();
                vals[i].~V();
            }
        }
        std::free(hasVal);
        std::free(keys);
        std::free(vals);
    }

    /**
     * Construct with capacity
     */
    explicit HashMap(u64 initCapacity)
        : hasVal{static_cast<bool*>(std::malloc(initCapacity))}
        , keys{static_cast<K*>(std::malloc(initCapacity * sizeof(K)))}
        , vals{static_cast<V*>(std::malloc(initCapacity * sizeof(V)))}
        , capacity{initCapacity}
        , count{0}
    {
        std::memset(hasVal, 0, capacity);
    }

    /**
     * Remove all elements
     */
    void clear()
    {
        K* k = keys;
        V* v = vals;
        bool* has = hasVal;
        bool* end = hasVal + capacity;
        for (; has != end; ++has, ++k, ++v)
        {
            if (*has)
            {
                k->~K();
                v->~V();
                *has = false;
            }
        }
        count = 0;
    }

    /**
     * Change the capacity
     */
    void reserve(u64 newCapacity)
    {
        if (newCapacity <= capacity)
            return;

        HashMap<K, V> newMap{newCapacity};

        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                newMap.add(std::move(keys[i]), std::move(vals[i]));
        }

        *this = std::move(newMap);
    }

    /**
     * Add a key-value pair
     */
    V* add(const K& key, const V& val)
    {
        K k = key;
        V v = val;
        if (count >= capacity / 2)
            reserve(capacity == 0 ? 128 : capacity * 2);

        u64 idx = static_cast<u64>(hash(k) % capacity);
        for (u64 dist = 0; hasVal[idx] && !(keys[idx] == k); ++dist)
        {
            u64 otherDist = static_cast<u64>(hash(keys[idx]) % capacity) - idx;
            if (otherDist > capacity)
                otherDist += capacity;

            if (otherDist < dist)
            {
                std::swap(k, keys[idx]);
                std::swap(v, vals[idx]);
                dist = otherDist;
            }

            idx = (idx + 1) % capacity;
        }

        if (!hasVal[idx])
        {
            hasVal[idx] = true;
            new (keys + idx) K{std::move(k)};
            new (vals + idx) V{std::move(v)};
            ++count;
        }
        else
        {
            vals[idx] = std::move(v);
        }

        return vals + idx;
    }

    /**
     * Add a key-value pair by rvalue reference
     */
    V* add(K&& key, V&& val)
    {
        if (count >= capacity / 2)
            reserve(capacity == 0 ? 128 : capacity * 2);

        u64 idx = static_cast<u64>(hash(key) % capacity);
        for (u64 dist = 0; hasVal[idx] && !(keys[idx] == key); ++dist)
        {
            u64 otherDist = static_cast<u64>(hash(keys[idx]) % capacity) - idx;
            if (otherDist > capacity)
                otherDist += capacity;

            if (otherDist < dist)
            {
                std::swap(key, keys[idx]);
                std::swap(val, vals[idx]);
                dist = otherDist;
            }

            idx = (idx + 1) % capacity;
        }

        if (!hasVal[idx])
        {
            hasVal[idx] = true;
            new (keys + idx) K{std::move(key)};
            new (vals + idx) V{std::move(val)};
            ++count;
        }
        else
        {
            vals[idx] = std::move(val);
        }

        return vals + idx;
    }

    /**
     * Remove a key-value pair
     *
     * Parameters
     * - key The key of the pair to remove
     * - val A pointer to store the value, if found
     *
     * Returns
     * - Whether a key-value pair was found
     */
    template<typename T>
    bool remove(const T& key, V* val = nullptr)
    {
        if (capacity == 0)
            return false;

        u64 idx = static_cast<u64>(hash(key) % capacity);
        while (hasVal[idx])
        {
            if (keys[idx] == key)
                break;
            idx = (idx + 1) % capacity;
        }
        if (!hasVal[idx])
            return false;

        if (val != nullptr)
            *val = std::move(vals[idx]);

        u64 next = (idx + 1) % capacity;
        while (hasVal[next])
        {
            if (hash(keys[next]) % capacity != next)
            {
                keys[idx] = std::move(keys[next]);
                vals[idx] = std::move(vals[next]);
                idx = next;
            }
            next = (next + 1) % capacity;
        }

        keys[idx].~K();
        vals[idx].~V();
        hasVal[idx] = false;
        --count;

        return true;
    }

    /**
     * Returns whether the key is contained in the map
     */
    template<typename T>
    bool has(const T& key) const
    {
        if (capacity == 0)
            return false;

        for (u64 idx = static_cast<u64>(hash(key) % capacity); hasVal[idx]; idx = (idx + 1) % capacity)
        {
            if (keys[idx] == key)
                return true;
        }
        return false;
    }

    /**
     * Returns a pointer to the value at key, or nullptr if it does not exist
     */
    template<typename T>
    V* get(const T& key)
    {
        if (capacity == 0)
            return nullptr;

        for (u64 idx = static_cast<u64>(hash(key) % capacity); hasVal[idx]; idx = (idx + 1) % capacity)
        {
            if (keys[idx] == key)
                return vals + idx;
        }
        return nullptr;
    }

    /**
     * Calls a function for each value in the hash map
     */
    template<typename F> requires std::is_invocable_r_v<void, F, const K&, V&>
    void forEach(F fn)
    {
        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                fn(keys[i], vals[i]);
        }
    }

    /**
     * Calls a function for each value in the hash map (const)
     */
    template<typename F> requires std::is_invocable_r_v<void, F, const K&, const V&>
    void forEach(F fn) const
    {
        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                fn(keys[i], vals[i]);
        }
    }

    /**
     * Move construct
     */
    HashMap(HashMap&& other) noexcept
        : hasVal{std::exchange(other.hasVal, nullptr)}
        , keys{std::exchange(other.keys, nullptr)}
        , vals{std::exchange(other.vals, nullptr)}
        , capacity{std::exchange(other.capacity, 0)}
        , count{std::exchange(other.count, 0)}
    {}

    /**
     * Move assign
     */
    HashMap& operator=(HashMap&& other) noexcept
    {
        if (this != &other)
        {
            this->~HashMap();
            new (this) HashMap{std::move(other)};
        }
        return *this;
    }

    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;
};

} // namespace hg

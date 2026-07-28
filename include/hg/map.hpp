#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"
#include "hg/memory.hpp"
#include "hg/hash.hpp"

#include <cstring>

namespace hg {

/**
 * A key-value hash map
 */
template<typename K, typename V>
struct Map {
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
    Map() noexcept = default;

    /**
     * Destroy the Map
     */
    ~Map() noexcept
    {
        forEach([&](K* key, V* val)
        {
            key->~K();
            val->~V();
        });
        heapFree(hasVal, capacity);
        heapFree(keys, capacity);
        heapFree(vals, capacity);
    }

    /**
     * Construct with capacity
     */
    Map(u64 initCapacity)
        : hasVal{heapAlloc<bool>(initCapacity)}
        , keys{heapAlloc<K>(initCapacity)}
        , vals{heapAlloc<V>(initCapacity)}
        , capacity{initCapacity}
        , count{0}
    {
        memset(hasVal, 0, capacity);
    }

    /**
     * Remove all elements
     */
    void reset()
    {
        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
            {
                keys[i].~K();
                vals[i].~V();
                hasVal[i] = false;
            }
        }
        count = 0;
    }

    /**
     * Change the capacity
     */
    void resize(u64 newCapacity)
    {
        HG_ASSERT(newCapacity > count);
        if (newCapacity == capacity)
            return;

        Map<K, V> newMap{newCapacity};

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
            resize(capacity == 0 ? 128 : capacity * 2);

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
            resize(capacity == 0 ? 128 : capacity * 2);

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
    bool remove(const K& key, V* val = nullptr)
    {
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
    bool has(const K& key)
    {
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
    V* get(const K& key)
    {
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
    template<typename F> requires std::is_invocable_r_v<void, F, K*, V*>
    void forEach(F fn)
    {
        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                fn(&keys[i], &vals[i]);
        }
    }

    /**
     * Move construct
     */
    Map(Map&& other) noexcept
        : hasVal{std::exchange(other.hasVal, nullptr)}
        , keys{std::exchange(other.keys, nullptr)}
        , vals{std::exchange(other.vals, nullptr)}
        , capacity{std::exchange(other.capacity, 0)}
        , count{std::exchange(other.count, 0)}
    {}

    /**
     * Move assign
     */
    Map& operator=(Map&& other) noexcept
    {
        if (this != &other)
        {
            this->~Map();
            new (this) Map{std::move(other)};
        }
        return *this;
    }

    Map(const Map&) = delete;
    Map& operator=(const Map&) = delete;
};

/**
 * A key-value hash map using an arena
 */
template<typename K, typename V>
struct MapTemp {
    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;
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
    MapTemp() noexcept = default;

    /**
     * Destroy the map
     */
    ~MapTemp() noexcept
    {
        forEach([&](K* key, V* val)
        {
            key->~K();
            val->~V();
        });
    }

    /**
     * Construct with capacity
     */
    MapTemp(Arena* arenaVal, u64 initCapacity)
        : arena{arenaVal}
        , hasVal{arenaVal->alloc<bool>(initCapacity)}
        , keys{arenaVal->alloc<K>(initCapacity)}
        , vals{arenaVal->alloc<V>(initCapacity)}
        , capacity{initCapacity}
        , count{0}
    {
        memset(hasVal, 0, capacity);
    }

    /**
     * Remove all elements
     */
    void reset()
    {
        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
            {
                keys[i].~K();
                vals[i].~V();
                hasVal[i] = false;
            }
        }
        count = 0;
    }

    /**
     * Change the capacity
     */
    void resize(u64 newCapacity)
    {
        HG_ASSERT(arena != nullptr);
        HG_ASSERT(newCapacity > count);
        if (newCapacity == capacity)
            return;

        MapTemp<K, V> newMapTemp{arena, newCapacity};

        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                newMapTemp.add(std::move(keys[i]), std::move(vals[i]));
        }

        *this = std::move(newMapTemp);
    }

    /**
     * Add a key-value pair
     */
    V* add(const K& key, const V& val)
    {
        K k = key;
        V v = val;
        if (count >= capacity / 2)
            resize(capacity == 0 ? 128 : capacity * 2);

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
            resize(capacity == 0 ? 128 : capacity * 2);

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
    bool remove(const K& key, V* val = nullptr)
    {
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
    bool has(const K& key)
    {
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
    V* get(const K& key)
    {
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
    template<typename F> requires std::is_invocable_r_v<void, F, K*, V*>
    void forEach(F fn)
    {
        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                fn(&keys[i], &vals[i]);
        }
    }

    /**
     * Move construct
     */
    MapTemp(MapTemp&& other) noexcept
        : arena{std::exchange(other.arena, nullptr)}
        , hasVal{std::exchange(other.hasVal, nullptr)}
        , keys{std::exchange(other.keys, nullptr)}
        , vals{std::exchange(other.vals, nullptr)}
        , capacity{std::exchange(other.capacity, 0)}
        , count{std::exchange(other.count, 0)}
    {}

    /**
     * Move assign
     */
    MapTemp& operator=(MapTemp&& other) noexcept
    {
        if (this != &other)
        {
            this->~MapTemp();
            new (this) MapTemp{std::move(other)};
        }
        return *this;
    }

    MapTemp(const MapTemp&) = delete;
    MapTemp& operator=(const MapTemp&) = delete;
};

} // namespace hg

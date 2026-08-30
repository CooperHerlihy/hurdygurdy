#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"
#include "hg/memory.hpp"
#include "hg/hash.hpp"

#include <cstring>

namespace hg {

/**
 * A hash set
 */
template<typename V>
struct Set {
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
    Set() noexcept = default;

    /**
     * Destroy the set
     */
    ~Set() noexcept
    {
        for (u32 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                vals[i].~V();
        }
        heapFree(hasVal, capacity);
        heapFree(vals, capacity);
    }

    /**
     * Construct with capacity
     */
    Set(u64 initCapacity)
        : hasVal{heapAlloc<bool>(initCapacity)}
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
                vals[i].~V();
                hasVal[i] = false;
            }
        }
        count = 0;
    }

    /**
     * Change the capacity, must be greater than count
     */
    void resize(u64 newCapacity)
    {
        HG_ASSERT(newCapacity > count);
        if (newCapacity == capacity)
            return;

        Set<V> newSet{newCapacity};

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
            resize(capacity == 0 ? 128 : capacity * 2);

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
            resize(capacity == 0 ? 128 : capacity * 2);

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
    bool has(const T& val)
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
     * Move construct
     */
    Set(Set&& other) noexcept
        : hasVal{std::exchange(other.hasVal, nullptr)}
        , vals{std::exchange(other.vals, nullptr)}
        , capacity{std::exchange(other.capacity, 0)}
        , count{std::exchange(other.count, 0)}
    {}

    /**
     * Move assign
     */
    Set& operator=(Set&& other) noexcept
    {
        if (this != &other)
        {
            this->~Set();
            new (this) Set{std::move(other)};
        }
        return *this;
    }

    Set(const Set&) = delete;
    Set& operator=(const Set&) = delete;
};

/**
 * A hash set using an arena
 */
template<typename V>
struct SetTemp {
    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;
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
    SetTemp() noexcept = default;

    /**
     * Destroy the set
     */
    ~SetTemp() noexcept
    {
        for (u32 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                vals[i].~V();
        }
    }

    /**
     * Construct with capacity
     */
    SetTemp(Arena* arenaVal, u64 initCapacity)
        : arena{arenaVal}
        , hasVal{arenaVal->alloc<bool>(initCapacity)}
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
                vals[i].~V();
                hasVal[i] = false;
            }
        }
        count = 0;
    }

    /**
     * Change the capacity, must be greater than or equal to count
     */
    void resize(u64 newCapacity)
    {
        HG_ASSERT(arena != nullptr);
        HG_ASSERT(newCapacity > count);
        if (newCapacity == capacity)
            return;

        SetTemp<V> newSet{arena, newCapacity};

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
            resize(capacity == 0 ? 128 : capacity * 2);

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
            resize(capacity == 0 ? 128 : capacity * 2);

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
    bool has(const T& val)
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
     * Move construct
     */
    SetTemp(SetTemp&& other) noexcept
        : arena{std::exchange(other.arena, nullptr)}
        , hasVal{std::exchange(other.hasVal, nullptr)}
        , vals{std::exchange(other.vals, nullptr)}
        , capacity{std::exchange(other.capacity, 0)}
        , count{std::exchange(other.count, 0)}
    {}

    /**
     * Move assign
     */
    SetTemp& operator=(SetTemp&& other) noexcept
    {
        if (this != &other)
        {
            this->~SetTemp();
            new (this) SetTemp{std::move(other)};
        }
        return *this;
    }

    SetTemp(const SetTemp&) = delete;
    SetTemp& operator=(const SetTemp&) = delete;
};

} // namespace hg

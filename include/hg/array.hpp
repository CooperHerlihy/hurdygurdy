#pragma once

#include "macros.hpp"
#include "inttypes.hpp"
#include "span.hpp"
#include "memory.hpp"

#include <utility>

namespace hg {

/**
 * A dynamic array
 */
template<typename T>
struct Array {
    /**
     * The values stored
     */
    T* vals = nullptr;
    /**
     * The number of vals
     */
    u64 count = 0;
    /**
     * The current max number of vals
     */
    u64 capacity = 0;

    /**
     * Construct empty
     */
    Array() noexcept = default;

    /**
     * Construct with init size
     */
    Array(u64 countVal, u64 capacityVal)
        : vals{heapAlloc<T>(capacityVal)}
        , count{countVal}
        , capacity{capacityVal}
    {
        HG_ASSERT(capacity >= count);
        for (u64 i = 0; i < count; ++i)
        {
            new (vals + i) T{};
        }
    }

    /**
     * Free the array
     */
    ~Array() noexcept
    {
        for (u64 i = 0; i < count; ++i)
        {
            vals[i].~T();
        }
        heapFree(vals, capacity);
    }

    /**
     * Implicit convert to span
     */
    constexpr operator Span<T>()
    {
        return {vals, count};
    }

    /**
     * Implicit convert to const span
     */
    constexpr operator Span<const T>() const
    {
        return {vals, count};
    }

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr T& operator[](u64 idx)
    {
        HG_ASSERT(vals != nullptr);
        HG_ASSERT(idx < count);
        return vals[idx];
    }

    /**
     * Convenience to index into the array with debug bounds checking (const)
     */
    constexpr const T& operator[](u64 idx) const
    {
        HG_ASSERT(vals != nullptr);
        HG_ASSERT(idx < count);
        return vals[idx];
    }

    /**
     * Remove all elements from the array
     */
    void reset()
    {
        for (u64 i = 0; i < count; ++i)
        {
            vals[i].~T();
        }
        count = 0;
    }

    /**
     * Increase the size of the array, must be greater or equal to count
     */
    void resize(u64 newCount)
    {
        if (newCount < count)
        {
            for (u64 i = newCount; i < count; ++i)
                vals[i].~T();
            count = newCount;
        }

        if (newCount > count)
        {
            if (newCount > capacity)
                reserve(newCount * 2);

            for (u64 i = count; i < newCount; ++i)
                new (vals + i) T{};
            count = newCount;
        }
    }

    /**
     * Increase the capacity of the array to at least newCapacity
     */
    void reserve(u64 newCapacity)
    {
        if (newCapacity > capacity)
        {
            T* newVals = heapAlloc<T>(newCapacity);
            for (u64 i = 0; i < count; ++i)
            {
                new (newVals + i) T{std::move(vals[i])};
                vals[i].~T();
            }
            heapFree(vals, capacity);
            vals = newVals;
            capacity = newCapacity;
        }
    }

    /**
     * Default-construct a value at the end of the array
     */
    T& push()
    {
        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        new (vals + count) T{};
        return vals[count++];
    }

    /**
     * Push a value to the end of the array
     */
    T& push(const T& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        new (vals + count) T{val};
        return vals[count++];
    }

    /**
     * Push a value by rvalue reference
     */
    T& push(T&& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        new (vals + count) T{std::move(val)};
        return vals[count++];
    }

    /**
     * Pop a value from the end of the array
     */
    T pop()
    {
        HG_ASSERT(count > 0);

        --count;
        T ret = std::move(vals[count]);
        vals[count].~T();
        return ret;
    }

    /**
     * Insert a value at idx, shifting values over
     */
    T& insertShift(u64 idx, const T& val)
    {
        HG_ASSERT(idx <= count);

        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        if (idx < count)
        {
            new (vals + count) T{std::move(vals[count - 1])};
            for (u64 i = count - 1; i >= idx + 1; --i)
            {
                vals[i] = std::move(vals[i - 1]);
            }
            vals[idx] = val;
        }
        else
        {
            new (vals + count) T{val};
        }
        return vals[count++];
    }

    /**
     * Insert a value by rvalue reference at idx, shifting values over
     */
    T& insertShift(u64 idx, T&& val)
    {
        HG_ASSERT(idx <= count);

        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        if (idx < count)
        {
            new (vals + count) T{std::move(vals[count - 1])};
            for (u64 i = count - 1; i >= idx + 1; --i)
            {
                vals[i] = std::move(vals[i - 1]);
            }
            vals[idx] = std::move(val);
        }
        else
        {
            new (vals + count) T{std::move(val)};
        }
        return vals[count++];
    }

    /**
     * Remove the value from idx, shifting values over
     */
    T removeShift(u64 idx)
    {
        HG_ASSERT(idx < count);

        --count;
        T ret = std::move(vals[idx]);
        for (u64 i = idx; i < count; ++i)
        {
            vals[i] = std::move(vals[i + 1]);
        }
        vals[count].~T();
        return ret;
    }

    /**
     * Insert a value at idx, moving the previous value to the end
     */
    T& insertSwap(u64 idx, const T& val)
    {
        HG_ASSERT(idx <= count);

        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        if (idx < count)
        {
            new (vals + count) T{std::move(vals[idx])};
            vals[idx] = val;
        }
        else
        {
            new (vals + count) T{val};
        }
        return vals[count++];
    }

    /**
     * Insert a value by rvalue reference at idx, moving the previous value to the end
     */
    T& insertSwap(u64 idx, T&& val)
    {
        HG_ASSERT(idx <= count);

        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        if (idx < count)
        {
            new (vals + count) T{std::move(vals[idx])};
            vals[idx] = std::move(val);
        }
        else
        {
            new (vals + count) T{std::move(val)};
        }
        return vals[count++];
    }

    /**
     * Remove the value from idx, swapping with the last value
     */
    T removeSwap(u64 idx)
    {
        HG_ASSERT(idx < count);

        --count;
        T ret = std::move(vals[idx]);
        if (idx < count)
        {
            vals[idx] = std::move(vals[count]);
        }
        vals[count].~T();
        return ret;
    }

    /**
     * Use range for
     */
    constexpr T* begin()
    {
        return vals;
    }

    /**
     * Use range for
     */
    constexpr T* end()
    {
        return vals + count;
    }

    /**
     * Use range for
     */
    constexpr const T* begin() const
    {
        return vals;
    }

    /**
     * Use range for
     */
    constexpr const T* end() const
    {
        return vals + count;
    }

    /**
     * Move construct
     */
    Array(Array&& other) noexcept
        : vals{std::exchange(other.vals, nullptr)}
        , count{std::exchange(other.count, 0)}
        , capacity{std::exchange(other.capacity, 0)}
    {}

    /**
     * Move assign
     */
    Array& operator=(Array&& other) noexcept
    {
        if (this != &other)
        {
            this->~Array();
            new (this) Array{std::move(other)};
        }
        return *this;
    }

    Array(const Array&) = delete;
    Array& operator=(const Array&) = delete;
};

/**
 * A dynamic array using arenas
 */
template<typename T>
struct ArrayTemp {
    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;
    /**
     * The values stored
     */
    T* vals = nullptr;
    /**
     * The number of vals
     */
    u64 count = 0;
    /**
     * The current max number of vals
     */
    u64 capacity = 0;

    /**
     * Construct empty
     */
    ArrayTemp() noexcept = default;

    /**
     * Construct with init size
     */
    ArrayTemp(Arena* arenaVal, u64 countVal, u64 capacityVal)
        : arena{arenaVal}
        , vals{arenaVal->alloc<T>(capacityVal)}
        , count{countVal}
        , capacity{capacityVal}
    {
        HG_ASSERT(capacity >= count);
        for (u64 i = 0; i < count; ++i)
        {
            new (vals + i) T{};
        }
    }

    /**
     * Free the array
     */
    ~ArrayTemp() noexcept
    {
        for (u64 i = 0; i < count; ++i)
        {
            vals[i].~T();
        }
    }

    /**
     * Implicit convert to span
     */
    constexpr operator Span<T>()
    {
        return {vals, count};
    }

    /**
     * Implicit convert to const span
     */
    constexpr operator Span<const T>() const
    {
        return {vals, count};
    }

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr T& operator[](u64 idx)
    {
        HG_ASSERT(vals != nullptr);
        HG_ASSERT(idx < count);
        return vals[idx];
    }

    /**
     * Convenience to index into the array with debug bounds checking (const)
     */
    constexpr const T& operator[](u64 idx) const
    {
        HG_ASSERT(vals != nullptr);
        HG_ASSERT(idx < count);
        return vals[idx];
    }

    /**
     * Remove all elements from the array
     */
    void reset()
    {
        for (u64 i = 0; i < count; ++i)
        {
            vals[i].~T();
        }
        count = 0;
    }

    /**
     * Increase the size of the array, must be greater or equal to count
     */
    void resize(u64 newCount)
    {
        if (newCount < count)
        {
            for (u64 i = newCount; i < count; ++i)
                vals[i].~T();
            count = newCount;
        }

        if (newCount > count)
        {
            if (newCount > capacity)
                reserve(newCount * 2);

            for (u64 i = count; i < newCount; ++i)
                new (vals + i) T{};
            count = newCount;
        }
    }

    /**
     * Increase the capacity of the array to at least newCapacity
     */
    void reserve(u64 newCapacity)
    {
        if (newCapacity > capacity)
        {
            if (!arena->extend(vals, capacity, newCapacity))
            {
                T* newVals = arena->alloc<T>(newCapacity);
                for (u64 i = 0; i < count; ++i)
                {
                    new (newVals + i) T{std::move(vals[i])};
                    vals[i].~T();
                }
                vals = newVals;
            }
            capacity = newCapacity;
        }
    }

    /**
     * Default-construct a value at the end of the array
     */
    T& push()
    {
        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        new (vals + count) T{};
        return vals[count++];
    }

    /**
     * Push a value to the end of the array
     */
    T& push(const T& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        new (vals + count) T{val};
        return vals[count++];
    }

    /**
     * Push a value by rvalue reference
     */
    T& push(T&& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        new (vals + count) T{std::move(val)};
        return vals[count++];
    }

    /**
     * Pop a value from the end of the array
     */
    T pop()
    {
        HG_ASSERT(count > 0);

        --count;
        T ret = std::move(vals[count]);
        vals[count].~T();
        return ret;
    }

    /**
     * Insert a value at idx, shifting values over
     */
    T& insertShift(u64 idx, const T& val)
    {
        HG_ASSERT(idx <= count);

        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        if (idx < count)
        {
            new (vals + count) T{std::move(vals[count - 1])};
            for (u64 i = count - 1; i >= idx + 1; --i)
            {
                vals[i] = std::move(vals[i - 1]);
            }
            vals[idx] = val;
        }
        else
        {
            new (vals + count) T{val};
        }
        return vals[count++];
    }

    /**
     * Insert a value by rvalue reference at idx, shifting values over
     */
    T& insertShift(u64 idx, T&& val)
    {
        HG_ASSERT(idx <= count);

        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        if (idx < count)
        {
            new (vals + count) T{std::move(vals[count - 1])};
            for (u64 i = count - 1; i >= idx + 1; --i)
            {
                vals[i] = std::move(vals[i - 1]);
            }
            vals[idx] = std::move(val);
        }
        else
        {
            new (vals + count) T{std::move(val)};
        }
        return vals[count++];
    }

    /**
     * Remove the value from idx, shifting values over
     */
    T removeShift(u64 idx)
    {
        HG_ASSERT(idx < count);

        --count;
        T ret = std::move(vals[idx]);
        for (u64 i = idx; i < count; ++i)
        {
            vals[i] = std::move(vals[i + 1]);
        }
        vals[count].~T();
        return ret;
    }

    /**
     * Insert a value at idx, moving the previous value to the end
     */
    T& insertSwap(u64 idx, const T& val)
    {
        HG_ASSERT(idx <= count);

        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        if (idx < count)
        {
            new (vals + count) T{std::move(vals[idx])};
            vals[idx] = val;
        }
        else
        {
            new (vals + count) T{val};
        }
        return vals[count++];
    }

    /**
     * Insert a value by rvalue reference at idx, moving the previous value to the end
     */
    T& insertSwap(u64 idx, T&& val)
    {
        HG_ASSERT(idx <= count);

        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        if (idx < count)
        {
            new (vals + count) T{std::move(vals[idx])};
            vals[idx] = std::move(val);
        }
        else
        {
            new (vals + count) T{std::move(val)};
        }
        return vals[count++];
    }

    /**
     * Remove the value from idx, swapping with the last value
     */
    T removeSwap(u64 idx)
    {
        HG_ASSERT(idx < count);

        --count;
        T ret = std::move(vals[idx]);
        if (idx < count)
        {
            vals[idx] = std::move(vals[count]);
        }
        vals[count].~T();
        return ret;
    }

    /**
     * Use range for
     */
    constexpr T* begin()
    {
        return vals;
    }

    /**
     * Use range for
     */
    constexpr T* end()
    {
        return vals + count;
    }

    /**
     * Use range for
     */
    constexpr const T* begin() const
    {
        return vals;
    }

    /**
     * Use range for
     */
    constexpr const T* end() const
    {
        return vals + count;
    }

    /**
     * Move construct
     */
    ArrayTemp(ArrayTemp&& other) noexcept
        : arena{std::exchange(other.arena, nullptr)}
        , vals{std::exchange(other.vals, nullptr)}
        , count{std::exchange(other.count, 0)}
        , capacity{std::exchange(other.capacity, 0)}
    {}

    /**
     * Move assign
     */
    ArrayTemp& operator=(ArrayTemp&& other) noexcept
    {
        if (this != &other)
        {
            this->~ArrayTemp();
            new (this) ArrayTemp{std::move(other)};
        }
        return *this;
    }

    ArrayTemp(const ArrayTemp&) = delete;
    ArrayTemp& operator=(const ArrayTemp&) = delete;
};

} // namespace hg

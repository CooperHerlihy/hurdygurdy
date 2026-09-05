#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"
#include "hg/memory.hpp"

#include <utility>

namespace hg {

/**
 * A double ended ring buffer queue
 */
template<typename T>
struct Queue {
    /**
     * The values in the queue
     */
    T* vals = nullptr;
    /**
     * The index of the front
     */
    u64 front = 0;
    /**
     * The index of the back
     */
    u64 back = 0;
    /**
     * The number of vals in the queue
     */
    u64 count = 0;
    /**
     * The max number of vals
     */
    u64 capacity = 0;

    /**
     * Construct empty
     */
    Queue() noexcept = default;

    /**
     * Construct with init size
     */
    Queue(u64 capacityVal)
        : vals{heapAlloc<T>(capacityVal)}
        , front{0}
        , back{0}
        , count{0}
        , capacity{capacityVal}
    {}

    /**
     * Free the queue
     */
    ~Queue() noexcept
    {
        if (back != front)
            HG_WARN("Non-empty queue destroyed\n");

        for (u64 n = 0; n < count; ++n)
        {
            vals[(front + n) % capacity].~T();
        }
        heapFree(vals, capacity);
    }

    /**
     * Increase the capacity of the queue to at least newCapacity
     */
    void reserve(u64 newCapacity)
    {
        if (newCapacity > capacity)
        {
            T* newVals = heapAlloc<T>(newCapacity);

            T* nextVal = newVals;
            for (u64 n = 0; n < count; ++n)
            {
                u64 i = (front + n) % capacity;
                new (nextVal++) T{std::move(vals[i])};
                vals[i].~T();
            }

            heapFree(vals, capacity);
            vals = newVals;
            capacity = newCapacity;
            front = 0;
            back = count;
        }
    }

    /**
     * Default-construct a value in place at the front of the queue
     */
    T* pushFront()
    {
        if (count == capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        front = (front == 0 ? capacity : front) - 1;
        new (vals + front) T{};
        ++count;
        return vals + front;
    }

    /**
     * Push a value to the front of the queue
     */
    void pushFront(const T& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        front = (front == 0 ? capacity : front) - 1;
        new (vals + front) T{val};
        ++count;
    }

    /**
     * Push a value by rvalue reference to the front of the queue
     */
    void pushFront(T&& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        front = (front == 0 ? capacity : front) - 1;
        new (vals + front) T{std::move(val)};
        ++count;
    }

    /**
     * Default-construct a value in place at the back of the queue
     */
    T* pushBack()
    {
        if (count == capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        new (vals + back) T{};
        T* ret = vals + back;
        back = (back + 1) % capacity;
        ++count;
        return ret;
    }

    /**
     * Push a value to the back of the queue
     */
    void pushBack(const T& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        new (vals + back) T{val};
        back = (back + 1) % capacity;
        ++count;
    }

    /**
     * Push a value by rvalue reference to the back of the queue
     */
    void pushBack(T&& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        new (vals + back) T{std::move(val)};
        back = (back + 1) % capacity;
        ++count;
    }

    /**
     * Pop a value from the front of the queue
     */
    T popFront()
    {
        HG_ASSERT(count > 0);
        --count;

        T ret = std::move(vals[front]);
        vals[front].~T();
        front = (front + 1) % capacity;
        return ret;
    }

    /**
     * Pop a value from the back of the queue
     */
    T popBack()
    {
        HG_ASSERT(count > 0);
        --count;

        back = (back == 0 ? capacity : back) - 1;
        T ret = std::move(vals[back]);
        vals[back].~T();
        return ret;
    }

    /**
     * Move construct
     */
    Queue(Queue&& other) noexcept
        : vals{std::exchange(other.vals, nullptr)}
        , front{std::exchange(other.front, 0)}
        , back{std::exchange(other.back, 0)}
        , count{std::exchange(other.count, 0)}
        , capacity{std::exchange(other.capacity, 0)}
    {}

    /**
     * Move assign
     */
    Queue& operator=(Queue&& other) noexcept
    {
        if (this != &other)
        {
            this->~Queue();
            new (this) Queue{std::move(other)};
        }
        return *this;
    }

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;
};

/**
 * A double ended ring buffer queue using an arena
 */
template<typename T>
struct QueueTemp {
    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;
    /**
     * The values in the queue
     */
    T* vals = nullptr;
    /**
     * The index of the front
     */
    u64 front = 0;
    /**
     * The index of the back
     */
    u64 back = 0;
    /**
     * The number of vals in the queue
     */
    u64 count = 0;
    /**
     * The max number of vals
     */
    u64 capacity = 0;

    /**
     * Construct empty
     */
    QueueTemp() noexcept = default;

    /**
     * Construct with init size
     */
    QueueTemp(Arena* arenaVal, u64 capacityVal)
        : arena{arenaVal}
        , vals{arenaVal->alloc<T>(capacityVal)}
        , front{0}
        , back{0}
        , count{0}
        , capacity{capacityVal}
    {}

    /**
     * Free the queue
     */
    ~QueueTemp() noexcept
    {
        if (back != front)
            HG_WARN("Non-empty queue destroyed\n");

        for (u64 n = 0; n < count; ++n)
        {
            vals[(front + n) % capacity].~T();
        }
    }

    /**
     * Increase the capacity of the queue to at least newCapacity
     */
    void reserve(u64 newCapacity)
    {
        HG_ASSERT(arena != nullptr);

        if (newCapacity > capacity)
        {
            if (arena->extend(vals, capacity, newCapacity))
            {
                if (front > back)
                {
                    for (u64 i = 0; i < front - back; ++i)
                    {
                        new (vals + capacity + i) T{std::move(vals[back + i])};
                        vals[back + i].~T();
                    }
                    back += capacity;
                }
            }
            else
            {
                T* newVals = arena->alloc<T>(newCapacity);

                T* nextVal = newVals;
                for (u64 n = 0; n < count; ++n)
                {
                    u64 i = (front + n) % capacity;
                    new (nextVal++) T{std::move(vals[i])};
                    vals[i].~T();
                }

                vals = newVals;
                front = 0;
                back = count;
            }
            capacity = newCapacity;
        }
    }

    /**
     * Default-construct a value in place at the front of the queue
     */
    T* pushFront()
    {
        if (count == capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        front = (front == 0 ? capacity : front) - 1;
        new (vals + front) T{};
        ++count;
        return vals + front;
    }

    /**
     * Push a value to the front of the queue
     */
    void pushFront(const T& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        front = (front == 0 ? capacity : front) - 1;
        new (vals + front) T{val};
        ++count;
    }

    /**
     * Push a value by rvalue reference to the front of the queue
     */
    void pushFront(T&& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        front = (front == 0 ? capacity : front) - 1;
        new (vals + front) T{std::move(val)};
        ++count;
    }

    /**
     * Default-construct a value in place at the back of the queue
     */
    T* pushBack()
    {
        if (count == capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        new (vals + back) T{};
        T* ret = vals + back;
        back = (back + 1) % capacity;
        ++count;
        return ret;
    }

    /**
     * Push a value to the back of the queue
     */
    void pushBack(const T& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        new (vals + back) T{val};
        back = (back + 1) % capacity;
        ++count;
    }

    /**
     * Push a value by rvalue reference to the back of the queue
     */
    void pushBack(T&& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        new (vals + back) T{std::move(val)};
        back = (back + 1) % capacity;
        ++count;
    }

    /**
     * Pop a value from the front of the queue
     */
    T popFront()
    {
        HG_ASSERT(count > 0);
        --count;

        T ret = std::move(vals[front]);
        vals[front].~T();
        front = (front + 1) % capacity;
        return ret;
    }

    /**
     * Pop a value from the back of the queue
     */
    T popBack()
    {
        HG_ASSERT(count > 0);

        --count;
        back = (back == 0 ? capacity : back) - 1;
        T ret = std::move(vals[back]);
        vals[back].~T();
        return ret;
    }

    /**
     * Move construct
     */
    QueueTemp(QueueTemp&& other) noexcept
        : arena{std::exchange(other.arena, nullptr)}
        , vals{std::exchange(other.vals, nullptr)}
        , front{std::exchange(other.front, 0)}
        , back{std::exchange(other.back, 0)}
        , count{std::exchange(other.count, 0)}
        , capacity{std::exchange(other.capacity, 0)}
    {}

    /**
     * Move assign
     */
    QueueTemp& operator=(QueueTemp&& other) noexcept
    {
        if (this != &other)
        {
            this->~QueueTemp();
            new (this) QueueTemp{std::move(other)};
        }
        return *this;
    }

    QueueTemp(const QueueTemp&) = delete;
    QueueTemp& operator=(const QueueTemp&) = delete;
};

} // namespace hg

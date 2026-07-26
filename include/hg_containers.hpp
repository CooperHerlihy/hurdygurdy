#pragma once

#include <bit>
#include <type_traits>

#include "hg_types.hpp"
#include "hg_memory.hpp"
#include "hg_strings.hpp"

namespace hg {

/**
 * A binary builder
 */
struct BinaryBuilder {
    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;
    /**
     * The data
     */
    void* data = nullptr;
    /**
     * The size of the data in bytes
     */
    u64 size = 0;

    /**
     * Construct empty
     */
    BinaryBuilder() noexcept = default;

    /**
     * Construct a new builder
     */
    BinaryBuilder(Arena* arenaVal, u64 sizeVal = 0)
        : arena{arenaVal}, size{sizeVal}
    {
        data = arena->alloc(sizeVal, 1);
    }

    /**
     * Implicitly convert to Binary
     */
    constexpr operator BinaryView()
    {
        return {data, size};
    }

    /**
     * Read data at index into a buffer
     *
     * Parameters
     * - idx The index into the file in bytes to read from
     * - dst A pointer to store the read data
     * - size The size in bytes to read
     */
    void read(u64 idx, void* dst, u64 len);

    /**
     * Read data of arbitrary type from the file
     *
     * Parameters
     * - idx The index into the file in bytes to read from
     */
    template<typename T>
    T read(u64 idx)
    {
        T ret;
        read(idx, &ret, sizeof(T));
        return ret;
    }

    /**
     * Resize the binary
     *
     * Parameters
     * - arena The arena to allocate from
     * - newSize The new size of the file in bytes
     */
    void resize(u64 newSize);

    /**
     * Overwrite data at the index
     *
     * Parameters
     * - idx The index into the file to overwrite
     * - src The data to write
     * - size The size of the data in bytes
     */
    void overwrite(u64 idx, const void* src, u64 len);

    /**
     * Overwrite data of arbitrary type at the index
     *
     * Parameters
     * - idx The index into the file to overwrite
     * - src The data to write
     */
    template<typename T>
    void overwrite(u64 idx, const T& src)
    {
        overwrite(idx, &src, sizeof(T));
    }

    /**
     * Append data to the end, increasing size
     */
    void append(const void* src, u64 len);

    /**
     * Append data of arbitrary type to the end, increasing size
     */
    template<typename T>
    void append(const T& src)
    {
        append(&src, sizeof(T));
    }
};

/**
 * An owner of binary data
 */
struct Binary {
    /**
     * The data
     */
    void* data = nullptr;
    /**
     * The size of the data in bytes
     */
    u64 size = 0;

    /**
     * Construct empty
     */
    Binary() noexcept = default;

    /**
     * Create a new binary block from data
     */
    static Binary create(BinaryView data);

    /**
     * Free the binary
     */
    ~Binary() noexcept;

    /**
     * Read data at index into a buffer
     *
     * Parameters
     * - idx The index into the file in bytes to read from
     * - dst A pointer to store the read data
     * - size The size in bytes to read
     */
    void read(u64 idx, void* dst, u64 len);

    /**
     * Read data of arbitrary type from the file
     *
     * Parameters
     * - idx The index into the file in bytes to read from
     */
    template<typename T>
    T read(u64 idx)
    {
        T ret;
        read(idx, &ret, sizeof(T));
        return ret;
    }

    /**
     * Implicitly convert to Binary
     */
    constexpr operator BinaryView()
    {
        return {data, size};
    }

    /**
     * Move construct
     */
    Binary(Binary&& other) noexcept
        : data{std::exchange(other.data, nullptr)}
        , size{std::exchange(other.size, 0)}
    {}

    /**
     * Move assign
     */
    Binary& operator=(Binary&& other) noexcept
    {
        if (this != &other)
        {
            this->~Binary();
            new (this) Binary{std::move(other)};
        }
        return *this;
    }

    Binary(const Binary&) = delete;
    Binary& operator=(const Binary&) = delete;
};

/**
 * A smart pointer with unique ownership
 */
template<typename T>
struct UniquePtr {
    /**
     * The pointer
     */
    T* ptr = nullptr;

    /**
     * Construct empty
     */
    UniquePtr() noexcept = default;

    /**
     * Construct empty explicitly
     */
    UniquePtr(std::nullptr_t) {}

    /**
     * Free the pointer
     */
    ~UniquePtr() noexcept
    {
        if (ptr != nullptr)
        {
            ptr->~T();
            heapFree(ptr, 1);
        }
    }

    /**
     * Implicit convert to underlying
     */
    operator T*() const
    {
        return ptr;
    }

    /**
     * Dereference underlying
     */
    T& operator*() const
    {
        HG_ASSERT(ptr != nullptr);
        return *ptr;
    }

    /**
     * Dereference underlying
     */
    T* operator->() const
    {
        HG_ASSERT(ptr != nullptr);
        return ptr;
    }

    /**
     * Move construct
     */
    UniquePtr(UniquePtr&& other) noexcept
        : ptr{std::exchange(other.ptr, nullptr)}
    {}

    /**
     * Move assign
     */
    UniquePtr& operator=(UniquePtr&& other) noexcept
    {
        if (this != &other)
        {
            this->~UniquePtr();
            new (this) UniquePtr{std::move(other)};
        }
        return *this;
    }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;
};

/**
 * Allocate a unique pointer on the heap
 */
template<typename T, typename... Args>
UniquePtr<T> makeUnique(Args&&... args)
{
    UniquePtr<T> ptr{};
    ptr.ptr = new (heapAlloc(sizeof(T), alignof(T))) T{std::forward<Args>(args)...};
    return ptr;
}

/**
 * A reference counted smart pointer with shared ownership
 *
 * Note, not thread safe
 */
template<typename T>
struct SharedPtr {
    /**
     * The smart pointer data
     */
    struct Block {
        /**
         * The reference count
         */
        u64 refCount;
        /**
         * The value pointed to
         */
        T val;
    };

    /**
     * The data block
     */
    Block* ptr = nullptr;

    /**
     * Construct empty
     */
    SharedPtr() noexcept = default;

    /**
     * Construct empty explicitly
     */
    SharedPtr(std::nullptr_t) {}

    /**
     * Free the pointer or decrement the ref count
     */
    ~SharedPtr() noexcept
    {
        if (ptr != nullptr && --ptr->refCount == 0)
        {
            ptr->val.~T();
            heapFree(ptr, sizeof(Block));
        }
    }

    /**
     * Create a new reference
     */
    SharedPtr clone() const
    {
        SharedPtr other{};
        other.ptr = ptr;
        ++ptr->refCount;
        return other;
    }

    /**
     * Implicit convert to underlying
     */
    operator T*() const
    {
        return ptr ? &ptr->val : nullptr;
    }

    /**
     * Dereference underlying
     */
    T& operator*() const
    {
        HG_ASSERT(ptr != nullptr);
        return ptr->val;
    }

    /**
     * Dereference underlying
     */
    T* operator->() const
    {
        HG_ASSERT(ptr != nullptr);
        return &ptr->val;
    }

    /**
     * Move construct
     */
    SharedPtr(SharedPtr&& other) noexcept
        : ptr{std::exchange(other.ptr, nullptr)}
    {}

    /**
     * Move assign
     */
    SharedPtr& operator=(SharedPtr&& other) noexcept
    {
        if (this != &other)
        {
            this->~SharedPtr();
            new (this) SharedPtr{std::move(other)};
        }
        return *this;
    }

    SharedPtr(const SharedPtr&) = delete;
    SharedPtr& operator=(const SharedPtr&) = delete;
};

/**
 * Allocate a shared pointer on the heap
 */
template<typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args)
{
    using Block = SharedPtr<T>::Block;

    SharedPtr<T> ptr{};
    ptr.ptr = new (heapAlloc(sizeof(Block), alignof(Block)))
        Block{1, std::forward<Args>(args)...};
    return ptr;
}

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
    constexpr operator Span<T>() const
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
    constexpr T& operator[](u64 idx) const
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
        if (newCount > capacity)
            reserve(newCount * 2);

        for (u64 i = count; i < newCount; ++i)
        {
            new (vals + i) T{};
        }
        count = newCount;
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
    T* push()
    {
        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        new (vals + count) T{};
        return vals + count++;
    }

    /**
     * Push a value to the end of the array
     */
    T* push(const T& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        new (vals + count) T{val};
        return vals + count++;
    }

    /**
     * Push a value by rvalue reference
     */
    T* push(T&& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        new (vals + count) T{std::move(val)};
        return vals + count++;
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
    T* insertShift(u64 idx, const T& val)
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
        return vals + count++;
    }

    /**
     * Insert a value by rvalue reference at idx, shifting values over
     */
    T* insertShift(u64 idx, T&& val)
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
        return vals + count++;
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
    T* insertSwap(u64 idx, const T& val)
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
        return vals + count++;
    }

    /**
     * Insert a value by rvalue reference at idx, moving the previous value to the end
     */
    T* insertSwap(u64 idx, T&& val)
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
        return vals + count++;
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
    constexpr operator Span<T>() const
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
    constexpr T& operator[](u64 idx) const
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
        if (newCount > capacity)
            reserve(newCount * 2);

        for (u64 i = count; i < newCount; ++i)
        {
            new (vals + i) T{};
        }
        count = newCount;
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
    T* push()
    {
        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        new (vals + count) T{};
        return vals + count++;
    }

    /**
     * Push a value to the end of the array
     */
    T* push(const T& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        new (vals + count) T{val};
        return vals + count++;
    }

    /**
     * Push a value by rvalue reference
     */
    T* push(T&& val)
    {
        if (count == capacity)
            reserve(capacity == 0 ? 64 : capacity * 2);

        new (vals + count) T{std::move(val)};
        return vals + count++;
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
    T* insertShift(u64 idx, const T& val)
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
        return vals + count++;
    }

    /**
     * Insert a value by rvalue reference at idx, shifting values over
     */
    T* insertShift(u64 idx, T&& val)
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
        return vals + count++;
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
    T* insertSwap(u64 idx, const T& val)
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
        return vals + count++;
    }

    /**
     * Insert a value by rvalue reference at idx, moving the previous value to the end
     */
    T* insertSwap(u64 idx, T&& val)
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
        return vals + count++;
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

        for (u64 i = front; i != back; i = (i + 1) % capacity)
        {
            vals[i].~T();
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
            for (u64 i = front; i != back; i = (i + 1) % capacity)
            {
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
     * Push a value to the front of the queue
     */
    void pushFront(const T& val)
    {
        if (++count >= capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        front = (front == 0 ? capacity : front) - 1;
        new (vals + front) T{val};
    }

    /**
     * Push a value by rvalue reference to the front of the queue
     */
    void pushFront(T&& val)
    {
        if (++count >= capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        front = (front == 0 ? capacity : front) - 1;
        new (vals + front) T{std::move(val)};
    }

    /**
     * Push a value to the back of the queue
     */
    void pushBack(const T& val)
    {
        if (++count >= capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        new (vals + back) T{val};
        back = (back + 1) % capacity;
    }

    /**
     * Push a value by rvalue reference to the back of the queue
     */
    void pushBack(T&& val)
    {
        if (++count >= capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        new (vals + back) T{std::move(val)};
        back = (back + 1) % capacity;
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

        for (u64 i = front; i != back; i = (i + 1) % capacity)
        {
            vals[i].~T();
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
                        new (vals + capacity + i) T{std::move(vals + back + i)};
                        vals[back + i].~T();
                    }
                }
                back += capacity;
            }
            else
            {
                T* newVals = arena->alloc<T>(newCapacity);

                T* nextVal = newVals;
                for (u64 i = front; i != back; i = (i + 1) % capacity)
                {
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
     * Push a value to the front of the queue
     */
    void pushFront(const T& val)
    {
        if (++count >= capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        front = (front == 0 ? capacity : front) - 1;
        new (vals + front) T{val};
    }

    /**
     * Push a value by rvalue reference to the front of the queue
     */
    void pushFront(T&& val)
    {
        if (++count >= capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        front = (front == 0 ? capacity : front) - 1;
        new (vals + front) T{std::move(val)};
    }

    /**
     * Push a value to the back of the queue
     */
    void pushBack(const T& val)
    {
        if (++count >= capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        new (vals + back) T{val};
        back = (back + 1) % capacity;
    }

    /**
     * Push a value by rvalue reference to the back of the queue
     */
    void pushBack(T&& val)
    {
        if (++count >= capacity)
            reserve(capacity == 0 ? 128 : capacity * 2);

        new (vals + back) T{std::move(val)};
        back = (back + 1) % capacity;
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

/**
 * The hash template
 */
template<typename T>
constexpr u64 hash(T)
{
    static_assert(false, "Type cannot be hashed without template specialization");
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
        forEach([&](V* val)
        {
            val->~V();
        });
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

        new (vals + idx) V{std::move(v)};
        hasVal[idx] = true;
        ++count;
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

        new (vals + idx) V{std::move(val)};
        hasVal[idx] = true;
        ++count;
    }

    /**
     * Remove a value from the set
     */
    void remove(const V& val)
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
    bool has(const V& val)
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
    template<typename F> requires std::is_invocable_r_v<void, F, V*>
    void forEach(F fn)
    {
        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                fn(vals + i);
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
        forEach([&](V* val)
        {
            val->~V();
        });
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
        reset();
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
        HG_ASSERT(newCapacity > count);
        if (newCapacity == capacity)
            return;

        SetTemp<V> newSet{newCapacity};

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

        new (vals + idx) V{std::move(v)};
        hasVal[idx] = true;
        ++count;
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

        new (vals + idx) V{std::move(val)};
        hasVal[idx] = true;
        ++count;
    }

    /**
     * Remove a value from the set
     */
    void remove(const V& val)
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
    bool has(const V& val)
    {
        for (u64 idx = static_cast<u64>(hash(val) % capacity); hasVal[idx]; idx = (idx + 1) % capacity)
        {
            if (vals[idx] == val)
                return true;
        }
        return false;
    }

    /**
     * Calls a function for each value in the hash map
     */
    template<typename F> requires std::is_invocable_r_v<void, F, V*>
    void forEach(F fn)
    {
        for (u64 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                fn(vals + i);
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

        hasVal[idx] = true;
        new (keys + idx) K{std::move(k)};
        new (vals + idx) V{std::move(v)};
        ++count;

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

        hasVal[idx] = true;
        new (keys + idx) K{std::move(key)};
        new (vals + idx) V{std::move(val)};
        ++count;

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
        HG_ASSERT(newCapacity > count);
        if (newCapacity == capacity)
            return;

        MapTemp<K, V> newMapTemp{newCapacity};

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

        hasVal[idx] = true;
        new (keys + idx) K{std::move(k)};
        new (vals + idx) V{std::move(v)};
        ++count;

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

        hasVal[idx] = true;
        new (keys + idx) K{std::move(key)};
        new (vals + idx) V{std::move(val)};
        ++count;

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

/**
 * Hash map hashing for u8
 */
template<>
constexpr u64 hash(u8 val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for u16
 */
template<>
constexpr u64 hash(u16 val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for u32
 */
template<>
constexpr u64 hash(u32 val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for u64
 */
template<>
constexpr u64 hash(u64 val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for i8
 */
template<>
constexpr u64 hash(i8 val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for i16
 */
template<>
constexpr u64 hash(i16 val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for i32
 */
template<>
constexpr u64 hash(i32 val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for i64
 */
template<>
constexpr u64 hash(i64 val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for f32
 */
template<>
constexpr u64 hash(f32 val)
{
    return static_cast<u64>(std::bit_cast<u32>(val));
}

/**
 * Hash map hashing for f64
 */
template<>
constexpr u64 hash(f64 val)
{
    return std::bit_cast<u64>(val);
}

/**
 * Hash map hashing for arbitrary pointer types
 */
template<typename T>
constexpr u64 hashPtr(T* val)
{
    return std::bit_cast<u64>(val);
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
constexpr u64 hash(StringView str)
{
    u64 ret = 0;
    u64 mult = 1;
    for (u32 i = 0; i < str.length; ++i)
    {
        ret += static_cast<u64>(str[i]) * mult;
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
    return hash(StringView{str});
}

/**
 * Hash map hashing for StringBuilder
 */
template<>
constexpr u64 hash(StringBuilder str)
{
    return hash(StringView{str});
}

/**
 * A pool of objects
 */
template<typename T>
struct Pool {
    /**
     * The size of each preallocated block
     */
    static constexpr u32 blockCount = 1024;
    /**
     * The uninitialized preallocated memory
     */
    Array<T*> prealloc = {};
    /**
     * The freed objects
     */
    Array<T*> inactive = {};
#ifdef HG_DEBUG_MODE
    /**
     * The currently allocated objects
     */
    Array<T*> active = {};
#endif

    /**
     * Construct empty
     */
    Pool() noexcept = default;

    /**
     * Destroy the pool
     */
    ~Pool() noexcept
    {
#ifdef HG_DEBUG_MODE
        if (active.count > 0)
            HG_WARN("Memory leak, pool destroyed with active objects\n");
        for (u32 i = 0; i < active.count; ++i)
        {
            active[i]->~T();
        }
#endif

        for (u32 i = 0; i < prealloc.count; ++i)
        {
            heapFree(prealloc[i], blockCount);
        }
    }

    /**
     * Allocate an object from the pool
     */
    template<typename... Args>
    T* alloc(Args&&... args)
    {
        if (inactive.count == 0)
        {
            T* block = heapAlloc<T>(blockCount);
            for (u32 i = 0; i < blockCount; ++i)
            {
                inactive.push(block + i);
            }
            prealloc.push(block);
        }

        T* object = new (inactive.pop()) T{std::forward<Args>(args)...};
#ifdef HG_DEBUG_MODE
        active.push(object);
#endif
        return object;
    }

    /**
     * Free an object to the pool
     */
    void free(T* object)
    {
        if (object == nullptr)
            return;

        object->~T();
#ifdef HG_DEBUG_MODE
        for (u32 i = 0; i < active.count; ++i)
        {
            if (active[i] == object)
            {
                active.removeSwap(i);
                goto found;
            }
        }
        HG_WARN("Invalid attempt to free to pool, object not in pool, possible double free\n");
        return;
found:
#endif
        inactive.push(object);
    }

    /**
     * Move construct
     */
    Pool(Pool&& other) noexcept
        : prealloc{std::exchange(other.prealloc, {})}
        , inactive{std::exchange(other.inactive, {})}
#ifdef HG_DEBUG_MODE
        , active{std::exchange(other.active, {})}
#endif
    {}

    /**
     * Move assign
     */
    Pool& operator=(Pool&& other) noexcept
    {
        if (this != &other)
        {
            this->~Pool();
            new (this) Pool{std::move(other)};
        }
        return *this;
    }

    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;
};

/**
 * A generation counted handle
 */
struct Handle {
    /**
     * The handle id
     */
    u32 id = 0;
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
    Array<Handle> handles = {};
    /**
     * The freed handles
     */
    Array<Handle> freed = {};
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


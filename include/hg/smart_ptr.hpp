#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"
#include "hg/memory.hpp"

#include <utility>

namespace hg {

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
        Block{1, T{std::forward<Args>(args)...}};
    return ptr;
}

} // namespace hg

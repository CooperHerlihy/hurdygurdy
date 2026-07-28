#pragma once

#include "hg/macros.hpp"
#include "hg/strings.hpp"

#include <utility>

namespace hg {

/**
 * An object which may or may not exist
 */
template<typename T>
struct Maybe
{
    /**
     * Whether val exists
     */
    bool has = false;
    union {
        /**
         * The value, if it exists
         */
        T val;
    };

    /**
     * Construct empty
     */
    constexpr Maybe() noexcept
        : has{false}
    {};

    /**
     * Construct with lvalue
     */
    constexpr Maybe(const T& val)
        : has{true}, val{val}
    {}

    /**
     * Construct with rvalue
     */
    constexpr Maybe(T&& val)
        : has{true}, val{std::move(val)}
    {}

    /**
     * Destroy if it exists
     */
    constexpr ~Maybe() noexcept
    {
        if (has)
            val.~T();
    }

    /**
     * Access by dereference
     */
    constexpr T& operator*()
    {
        HG_ASSERT(has);
        return val;
    }

    /**
     * Access by dereference
     */
    constexpr T* operator->()
    {
        HG_ASSERT(has);
        return &val;
    }

    /**
     * Takes the value, or returns a default
     */
    T orElse(const T& defaultVal)
    {
        if (has)
        {
            T tmp = std::move(val);
            val.~T();
            has = false;
            return tmp;
        }
        else
        {
            return defaultVal;
        }
    }

    /**
     * Takes the value, or returns a default
     */
    T orElse(T&& defaultVal)
    {
        if (has)
        {
            T tmp = std::move(val);
            val.~T();
            has = false;
            return tmp;
        }
        else
        {
            return std::move(defaultVal);
        }
    }

    /**
     * Takes the value, or panics
     */
    T expect(StringView errMsg)
    {
        if (has)
        {
            T tmp = std::move(val);
            val.~T();
            has = false;
            return tmp;
        }
        else
        {
            (void)errMsg;
            HG_PANIC("%.*s", (int)errMsg.length, errMsg.chars);
        }
    }

    /**
     * Copy construct
     */
    Maybe(const Maybe& other)
    {
        if (other.has)
            new (&val) T{other.val};
        has = other.has;
    }

    /**
     * Copy assign
     */
    Maybe& operator=(const Maybe& other)
    {
        if (this != &other)
        {
            if (has)
                val.~T();

            if (other.has)
                new (&val) T{other.val};
            has = other.has;
        }
        return *this;
    }

    /**
     * Move construct
     */
    Maybe(Maybe&& other) noexcept
    {
        if (other.has)
            new (&val) T{std::move(other.val)};
        has = other.has;

        if (other.has)
            other.val.~T();
        other.has = false;
    }

    /**
     * Move assign
     */
    Maybe& operator=(Maybe&& other) noexcept
    {
        if (this != &other)
        {
            this->~Maybe();
            new (this) Maybe{std::move(other)};
        }
        return *this;
    }
};

template<typename T, typename... Args>
Maybe<T> some(Args&&... args)
{
    Maybe<T> maybe;
    new (&maybe.val) T{std::forward<Args>(args)...};
    maybe.has = true;
    return maybe;
}

} // namespace hg

#pragma once

#include "hg/inttypes.hpp"

#include <utility>

namespace hg {

/**
 * A product type, or tuple
 */
template<typename... Ts>
struct Product {};

/**
 * A product type, or tuple
 */
template<typename T, typename... Ts>
struct Product<T, Ts...> {
    /**
     * The number of elements
     */
    static constexpr u64 count = 1 + sizeof...(Ts);
    /**
     * The first element
     */
    T first{};
    /**
     * The rest of the elements, expanded recursively
     */
    Product<Ts...> rest{};

    /**
     * Construct empty
     */
    Product() noexcept = default;

    /**
     * Construct from a list with lvalue
     */
    template<typename... Rest>
    constexpr Product(const T& x, Rest&&... xs)
        : first{x}, rest{std::forward<Rest>(xs)...}
    {}

    /**
     * Construct from a list with rvalue
     */
    template<typename... Rest>
    constexpr Product(T&& x, Rest&&... xs)
        : first{std::move(x)}, rest{std::forward<Rest>(xs)...}
    {}

    /**
     * Get an element by index
     */
    template<u64 N> requires (N < count)
    constexpr auto& get()
    {
        if constexpr (N == 0)
            return first;
        else
            return rest.template get<N - 1>();
    }

    /**
     * Get an element by index
     */
    template<u64 N, typename U> requires (N < count)
    constexpr auto& set(U&& val)
    {
        if constexpr (N == 0)
            return first = std::forward<U>(val);
        else
            return rest.template set<N - 1>(std::forward<U>(val));
    }
};

} // namespace hg

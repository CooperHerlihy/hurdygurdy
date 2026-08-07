#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"

#include <utility>
#include <memory>

namespace hg {

/**
 * A recursive union
 */
template<typename... Ts>
union SumUntagged {};

/**
 * A recursive union
 */
template<typename T, typename... Ts>
union SumUntagged<T, Ts...> {
    /**
     * The number of elements
     */
    static constexpr u32 count = 1 + sizeof...(Ts);
    /**
     * The first element
     */
    T first;
    /**
     * The rest of the elements, expanded recursively
     */
    SumUntagged<Ts...> rest;

    /**
     * Construct empty
     */
    constexpr SumUntagged() noexcept {}

    /**
     * Destructor does nothing
     */
    constexpr ~SumUntagged() noexcept {}

    /**
     * Construct by lvalue, base case
     */
    constexpr SumUntagged(const T& val)
        : first{val}
    {}

    /**
     * Construct by rvalue, base case
     */
    constexpr SumUntagged(T&& val)
        : first{std::move(val)}
    {}

    /**
     * Construct recursive case
     */
    template<typename U> requires (std::same_as<std::remove_cvref_t<U>, Ts> || ...)
    constexpr SumUntagged(U&& val)
        : rest{std::forward<U>(val)}
    {}

    /**
     * Get by type
     */
    template<typename U> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr auto& get()
    {
        if constexpr (std::same_as<U, T>)
            return first;
        else
            return rest.template get<U>();
    }

    /**
     * Get by type as const
     */
    template<typename U> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr const auto& get() const
    {
        if constexpr (std::same_as<U, T>)
            return first;
        else
            return rest.template get<U>();
    }

    /**
     * Get by index
     */
    template<u64 N> requires (N < count)
    constexpr auto& getN()
    {
        if constexpr (N == 0)
            return first;
        else
            return rest.template getN<N - 1>();
    }

    /**
     * Get by index as const
     */
    template<u64 N> requires (N < count)
    constexpr const auto& getN() const
    {
        if constexpr (N == 0)
            return first;
        else
            return rest.template getN<N - 1>();
    }

    /**
     * Construct in place by type
     */
    template<typename U, typename... Args> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    auto& emplace(Args&&... args)
    {
        if constexpr (std::same_as<U, T>)
            return *(new (&first) T{std::forward<Args>(args)...});
        else
            return rest.template emplace<U>(std::forward<Args>(args)...);
    }

    /**
     * Constuct in place by index
     */
    template<u64 N, typename... Args> requires (N < count)
    auto& emplaceN(Args&&... args)
    {
        if constexpr (N == 0)
            return *(new (&first) T{std::forward<Args>(args)...});
        else
            return rest.template emplaceN<N - 1>(std::forward<Args>(args)...);
    }
};

/**
 * Overload helper for Sum match
 */
template<typename... Fs>
struct Overload : Fs... {
    using Fs::operator()...;
};

/**
 * CTADS for Overload
 */
template<typename... Fs>
Overload(Fs...) -> Overload<Fs...>;

/**
 * A sum type, or tagged union
 *
 * Note, may also contain nothing
 */
template<typename... Ts>
struct Sum {};

/**
 * A sum type, or tagged union
 *
 * Note, may also contain nothing
 */
template<typename T, typename... Ts>
struct Sum<T, Ts...> {
    /**
     * The number of elements
     */
    static constexpr u64 count = 1 + sizeof...(Ts);
    /**
     * The currently active element, if any
     */
    u32 tag = count;
    /**
     * The unioned data
     */
    SumUntagged<T, Ts...> data;

    /**
     * Construct empty
     */
    Sum() noexcept = default;

    /**
     * Construct by type
     */
    template<typename U> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr Sum(U&& val)
        : tag{typeIdx<std::remove_cvref_t<U>>}, data{std::forward<U>(val)}
    {}

    /**
     * Call a function on the currently active element
     */
    template<typename F>
    auto call(F&& f)
    {
        using R = decltype(f(data.template getN<0>()));
        using RawF = std::remove_reference_t<F>;
        using Func = R (*)(Sum*, RawF*);

        return [&]<u64... Is>(std::index_sequence<Is...>)
        {
            static constexpr Func table[count + 1] = {
                +[](Sum* self, RawF* fn) -> R
                {
                    return std::forward<F>(*fn)(self->data.template getN<Is>());
                }...,
                +[](Sum*, RawF*) -> R
                {
                    if constexpr (!std::is_void_v<R>)
                        HG_PANIC("call on empty Sum");
                }
            };
            return table[tag < count ? tag : count](this, &f);
        }(std::make_index_sequence<count>{});
    }

    /**
     * Match a list of functions to the currently active element
     */
    template<typename... Fs>
    auto match(Fs&&... fs)
    {
        return call(Overload{std::forward<Fs>(fs)...});
    }

    /**
     * Destroy the currently active element
     */
    ~Sum() noexcept
    {
        call([](auto& val) { std::destroy_at(&val); });
    }

    /**
     * Get the index of a type
     */
    template<typename U>
    static constexpr u32 typeIdx = []() constexpr
    {
        if constexpr (std::same_as<U, T>)
            return 0;
        else
            return 1 + Sum<Ts...>::template typeIdx<U>;
    }();

    /**
     * Returns whether all types are inactive
     */
    constexpr bool isEmpty() const
    {
        return tag >= count;
    };

    /**
     * Returns whether the type is active
     */
    template<typename U> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr bool is() const
    {
        return tag == typeIdx<U>;
    }

    /**
     * Returns whether the index is active
     */
    template<u64 N> requires (N < count)
    constexpr bool isN() const
    {
        return tag == N;
    }

    /**
     * Get by type, asserting that it is active
     */
    template<typename U> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr auto& get()
    {
        HG_ASSERT(tag == typeIdx<U>);
        return data.template get<U>();
    }

    /**
     * Get by type as const, asserting that it is active
     */
    template<typename U> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr auto& get() const
    {
        HG_ASSERT(tag == typeIdx<U>);
        return data.template get<U>();
    }

    /**
     * Get by index, asserting that it is active
     */
    template<u64 N> requires (N < count)
    constexpr auto& getN()
    {
        HG_ASSERT(tag == N);
        return data.template getN<N>();
    }

    /**
     * Get by index as const, asserting that it is active
     */
    template<u64 N> requires (N < count)
    constexpr auto& getN() const
    {
        HG_ASSERT(tag == N);
        return data.template getN<N>();
    }

    /**
     * Construct in place by type
     */
    template<typename U, typename... Args> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    auto& emplace(Args&&... args)
    {
        this->~Sum();
        tag = typeIdx<U>;
        return data.template emplace<U>(std::forward<Args>(args)...);
    }

    /**
     * Construct in place by index
     */
    template<u64 N, typename... Args> requires (N < count)
    auto& emplaceN(Args&&... args)
    {
        this->~Sum();
        tag = N;
        return data.template emplaceN<N>(std::forward<Args>(args)...);
    }

    /**
     * Copy construct
     */
    Sum(const Sum& other)
        : tag{other.tag}
    {
        call([&](auto& val)
        {
            using U = std::remove_cvref_t<decltype(val)>;
            data.template emplace<U>(other.get<U>());
        });
    }

    /**
     * Copy assign
     */
    Sum& operator=(const Sum& other)
    {
        if (this != &other)
        {
            this->~Sum();
            new (this) Sum{other};
        }
        return *this;
    }

    /**
     * Move construct
     */
    Sum(Sum&& other) noexcept
        : tag{other.tag}
    {
        call([&](auto& val)
        {
            using U = std::remove_cvref_t<decltype(val)>;
            data.template emplace<U>(std::move(other.get<U>()));
        });
        other.tag = other.count;
    }

    /**
     * Move assign
     */
    Sum& operator=(Sum&& other) noexcept
    {
        if (this != &other)
        {
            this->~Sum();
            new (this) Sum{std::move(other)};
        }
        return *this;
    }
};

} // namespace hg

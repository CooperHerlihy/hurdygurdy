#pragma once

#include <cstdint>
#include <utility>
#include <memory>

#include "hg_macros.hpp"

namespace hg {

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using uptr = uintptr_t;
using iptr = intptr_t;

using f32 = float;
using f64 = double;

/**
 * A non-owning view into a string
 */
struct StringView {
    /**
     * The character data
     */
    const char* chars = nullptr;
    /**
     * The view length in bytes
     */
    u64 length = 0;

    /**
     * Construct empty
     */
    StringView() = default;

    /**
     * Construct from pointer and length
     */
    constexpr StringView(const char* charsVal, u64 lengthVal)
        : chars{charsVal}, length{lengthVal}
    {}

    /**
     * Construct from begin and end
     */
    constexpr StringView(const char* begin, const char* end)
        : chars{begin}, length{static_cast<u64>(end - begin)}
    {
        HG_ASSERT(begin <= end);
    }

    /**
     * Construct from null terminated c string
     */
    constexpr StringView(const char* cStr)
        : chars{cStr}, length{0}
    {
        if (cStr != nullptr)
        {
            while (cStr[length] != '\0')
                ++length;
        }
    }

    /**
     * Access by index
     */
    constexpr const char& operator[](u64 idx) const
    {
        HG_ASSERT(chars != nullptr);
        HG_ASSERT(idx < length);
        return chars[idx];
    }

    /**
     * C++ style for loop
     */
    constexpr const char* begin() const
    {
        return chars;
    }

    /**
     * C++ style for loop
     */
    constexpr const char* end() const
    {
        return chars + length;
    }
};

/**
 * A non-owning view into binary data
 */
struct BinaryView {
    /**
     * The viewed data
     */
    const void* data = nullptr;
    /**
     * The size of the data in bytes
     */
    u64 size = 0;

    /**
     * Read a section of data
     */
    void read(u64 idx, void* dst, u64 len) const;

    /**
     * Read a section of data
     */
    template<typename T>
    T read(u64 idx) const
    {
        T ret;
        read(idx, &ret, sizeof(T));
        return ret;
    }
};

/**
 * A non-owning view into data
 */
template<typename T>
struct Span {
    /**
     * The viewed data
     */
    T* data = nullptr;
    /**
     * The number of elements in data
     */
    u64 count = 0;

    /**
     * Construct empty
     */
    constexpr Span() noexcept = default;

    /**
     * Construct from pointer and count
     */
    constexpr Span(T* dataVal, u64 countVal)
        : data{dataVal}, count{countVal}
    {}

    /**
     * Construct from begin and end
     */
    constexpr Span(T* begin, T* end)
        : data{begin}, count{static_cast<u64>(end - begin)}
    {
        HG_ASSERT(begin <= end);
    }

    /**
     * Construct from c array
     */
    template<u64 N>
    constexpr Span(T (&arr)[N])
        : data{arr}, count{N}
    {}

    /**
     * Access by index
     */
    constexpr T& operator[](u64 idx) const
    {
        HG_ASSERT(data != nullptr);
        HG_ASSERT(idx < count);
        return data[idx];
    }

    /**
     * C++ style for loop
     */
    constexpr T* begin() const
    {
        return data;
    }

    /**
     * C++ style for loop
     */
    constexpr T* end() const
    {
        return data + count;
    }
};

/**
 * A non-owning view into data
 */
template<>
struct Span<void> {
    /**
     * The viewed data
     */
    void* data = nullptr;
    /**
     * The number of elements in data
     */
    u64 size = 0;

    /**
     * Construct empty
     */
    constexpr Span() noexcept = default;

    /**
     * Construct from pointer and count
     */
    constexpr Span(void* dataVal, u64 sizeVal)
        : data{dataVal}, size{sizeVal}
    {}

    /**
     * Construct from begin and end
     */
    constexpr Span(void* begin, void* end)
        : data{begin}, size{static_cast<u64>(static_cast<u8*>(end) - static_cast<u8*>(begin))}
    {
        HG_ASSERT(begin <= end);
    }

    /**
     * Access by index
     */
    constexpr void* operator[](u64 idx) const
    {
        HG_ASSERT(data != nullptr);
        HG_ASSERT(idx < size);
        return static_cast<u8*>(data) + idx;
    }
};

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

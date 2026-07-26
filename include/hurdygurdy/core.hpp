#pragma once

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

struct StringView;
struct BinaryView;
template<typename T> struct Span;
template<typename... Ts> struct Product;
template<typename... Ts> struct Sum;
template<typename T> struct Maybe;

struct StringView {
    const char* chars = nullptr;
    u64 length = 0;

    StringView() = default;
    constexpr StringView(const char* charsVal, u64 lengthVal)
        : chars{charsVal}, length{lengthVal} {}
    constexpr StringView(const char* charsBegin, const char* charsEnd)
        : chars{charsBegin}, length{static_cast<u64>(charsEnd - charsBegin)}
    {
        HG_ASSERT(charsBegin <= charsEnd);
    }
    constexpr StringView(const char* cStr)
        : chars{cStr}, length{0}
    {
        if (cStr != nullptr)
        {
            while (cStr[length] != '\0')
                ++length;
        }
    }
    constexpr const char& operator[](u64 idx) const
    {
        HG_ASSERT(chars != nullptr);
        HG_ASSERT(idx < length);
        return chars[idx];
    }
    constexpr const char* begin() const
    {
        return chars;
    }
    constexpr const char* end() const
    {
        return chars + length;
    }
};

struct BinaryView {
    const void* data = nullptr;
    u64 size = 0;

    void read(u64 idx, void* dst, u64 len) const;

    template<typename T>
    T read(u64 idx) const
    {
        T ret;
        read(idx, &ret, sizeof(T));
        return ret;
    }
};

template<typename T>
struct Span {
    T* data = nullptr;
    u64 count = 0;

    constexpr Span() noexcept = default;
    constexpr Span(T* valuesVal, u64 countVal)
        : data{valuesVal}, count{countVal}
    {}
    constexpr Span(T* begin, T* end)
        : data{begin}, count{static_cast<u64>(end - begin)}
    {}
    template<u64 N>
    constexpr Span(T (&arr)[N])
        : data{arr}, count{N}
    {}
    constexpr T& operator[](u64 idx) const
    {
        HG_ASSERT(data != nullptr);
        HG_ASSERT(idx < count);
        return data[idx];
    }
    constexpr T* begin() const
    {
        return data;
    }
    constexpr T* end() const
    {
        return data + count;
    }
};

template<>
struct Span<void> {
    void* data = nullptr;
    u64 size = 0;

    constexpr Span() noexcept = default;
    constexpr Span(void* valsVal, u64 countVal)
        : data{valsVal}, size{countVal}
    {}
    constexpr Span(void* begin, void* end)
        : data{begin}, size{static_cast<uptr>(static_cast<u8*>(end) - static_cast<u8*>(begin))}
    {}
    constexpr void* operator[](u64 idx) const
    {
        HG_ASSERT(idx < size);
        return static_cast<u8*>(data) + idx;
    }
};

template<typename... Ts> struct Product;

template<>
struct Product<> {};

template<typename T, typename... Ts>
struct Product<T, Ts...> {
    T first{};
    Product<Ts...> rest{};
    static constexpr u64 count = 1 + sizeof...(Ts);

    Product() noexcept = default;
    template<typename... Rest>
    constexpr Product(const T& x, Rest&&... xs)
        : first{x}, rest{std::forward<Rest>(xs)...}
    {}
    template<typename... Rest>
    constexpr Product(T&& x, Rest&&... xs)
        : first{std::move(x)}, rest{std::forward<Rest>(xs)...}
    {}
    template<u64 N> requires (N < count)
    constexpr auto& get()
    {
        if constexpr (N == 0)
            return first;
        else
            return rest.template get<N - 1>();
    }
    template<u64 N, typename U> requires (N < count)
    constexpr auto& set(U&& val)
    {
        if constexpr (N == 0)
            return first = std::forward<U>(val);
        else
            return rest.template set<N - 1>(std::forward<U>(val));
    }
};

template<typename... Ts> union SumUntagged;

template<>
union SumUntagged<> {};

template<typename T, typename... Ts>
union SumUntagged<T, Ts...> {
    T first;
    SumUntagged<Ts...> rest;
    static constexpr u32 count = 1 + sizeof...(Ts);

    constexpr SumUntagged() noexcept {}
    constexpr ~SumUntagged() noexcept {}
    constexpr SumUntagged(const T& val) : first{val} {}
    constexpr SumUntagged(T&& val) : first{std::move(val)} {}
    template<typename U> requires (std::same_as<std::remove_cvref_t<U>, Ts> || ...)
    constexpr SumUntagged(U&& val) : rest{std::forward<U>(val)} {}

    template<typename U> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr auto& get()
    {
        if constexpr (std::same_as<U, T>)
            return first;
        else
            return rest.template get<U>();
    }
    template<typename U> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr const auto& get() const
    {
        if constexpr (std::same_as<U, T>)
            return first;
        else
            return rest.template get<U>();
    }
    template<u64 N> requires (N < count)
    constexpr auto& getN()
    {
        if constexpr (N == 0)
            return first;
        else
            return rest.template getN<N - 1>();
    }
    template<u64 N> requires (N < count)
    constexpr const auto& getN() const
    {
        if constexpr (N == 0)
            return first;
        else
            return rest.template getN<N - 1>();
    }
    template<typename U, typename... Args> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    auto& emplace(Args&&... args)
    {
        if constexpr (std::same_as<U, T>)
            return *(new (&first) T{std::forward<Args>(args)...});
        else
            return rest.template emplace<U>(std::forward<Args>(args)...);
    }
    template<u64 N, typename... Args> requires (N < count)
    auto& emplaceN(Args&&... args)
    {
        if constexpr (N == 0)
            return *(new (&first) T{std::forward<Args>(args)...});
        else
            return rest.template emplaceN<N - 1>(std::forward<Args>(args)...);
    }
};

template<typename... Fs>
struct Overload : Fs... {
    using Fs::operator()...;
};

template<typename... Fs>
Overload(Fs...) -> Overload<Fs...>;

template<typename... Ts> struct Sum;

template<>
struct Sum<> {};

template<typename T, typename... Ts>
struct Sum<T, Ts...> {
    u32 tag = count;
    SumUntagged<T, Ts...> data;
    static constexpr u64 count = 1 + sizeof...(Ts);

    Sum() noexcept = default;
    template<typename U> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr Sum(U&& val)
        : tag{typeIdx<std::remove_cvref_t<U>>}, data{std::forward<U>(val)}
    {}

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

    template<typename... Fs>
    auto match(Fs&&... fs)
    {
        return call(Overload{std::forward<Fs>(fs)...});
    }

    ~Sum() noexcept
    {
        call([](auto& val) { std::destroy_at(&val); });
    }

    template<typename U>
    static constexpr u32 typeIdx = []() constexpr
    {
        if constexpr (std::same_as<U, T>)
            return 0;
        else
            return 1 + Sum<Ts...>::template typeIdx<U>;
    }();

    template<typename U> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr bool is() const
    {
        return tag == typeIdx<U>;
    }
    template<u64 N> requires (N < count)
    constexpr bool isN() const
    {
        return tag == N;
    }
    template<typename U> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr auto& get()
    {
        HG_ASSERT(tag == typeIdx<U>);
        return data.template get<U>();
    }
    template<typename U> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    constexpr auto& get() const
    {
        HG_ASSERT(tag == typeIdx<U>);
        return data.template get<U>();
    }
    template<u64 N> requires (N < count)
    constexpr auto& getN()
    {
        HG_ASSERT(tag == N);
        return data.template getN<N>();
    }
    template<u64 N> requires (N < count)
    constexpr auto& getN() const
    {
        HG_ASSERT(tag == N);
        return data.template getN<N>();
    }
    template<typename U, typename... Args> requires (std::same_as<U, T> || (std::same_as<U, Ts> || ...))
    auto& emplace(Args&&... args)
    {
        this->~Sum();
        tag = typeIdx<U>;
        return data.template emplace<U>(std::forward<Args>(args)...);
    }
    template<u64 N, typename... Args> requires (N < count)
    auto& emplaceN(Args&&... args)
    {
        this->~Sum();
        tag = N;
        return data.template emplaceN<N>(std::forward<Args>(args)...);
    }

    Sum(const Sum& other) : tag{other.tag}
    {
        call([&](auto& val)
        {
            using U = std::remove_cvref_t<decltype(val)>;
            data.template emplace<U>(other.get<U>());
        });
    }
    Sum& operator=(const Sum& other)
    {
        if (this != &other)
        {
            this->~Sum();
            new (this) Sum{other};
        }
        return *this;
    }
    Sum(Sum&& other) noexcept : tag{other.tag}
    {
        call([&](auto& val)
        {
            using U = std::remove_cvref_t<decltype(val)>;
            data.template emplace<U>(std::move(other.get<U>()));
        });
        other.tag = other.count;
    }
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

template<typename T>
struct Maybe
{
    bool has = false;
    union { T val; };

    constexpr Maybe() noexcept : has{false} {};
    constexpr Maybe(const T& val) : has{true}, val{val} {}
    constexpr Maybe(T&& val) : has{true}, val{std::move(val)} {}
    constexpr ~Maybe() noexcept
    {
        if (has)
            val.~T();
    }
    constexpr T& operator*()
    {
        HG_ASSERT(has);
        return val;
    }
    constexpr T* operator->()
    {
        HG_ASSERT(has);
        return &val;
    }
    T orElse(const T& defaultVal);
    T orElse(T&& defaultVal);
    T expect(StringView errMsg);
    Maybe(const Maybe& other);
    Maybe& operator=(const Maybe& other);
    Maybe(Maybe&& other) noexcept;
    Maybe& operator=(Maybe&& other) noexcept;
};

template<typename T, typename... Args>
Maybe<T> some(Args&&... args)
{
    Maybe<T> maybe;
    new (&maybe.val) T{std::forward<Args>(args)...};
    maybe.has = true;
    return maybe;
}
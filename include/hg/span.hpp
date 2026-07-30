#pragma once

#include "hg/macros.hpp"
#include "hg/inttypes.hpp"

namespace hg {

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
     * Implicit conversion to const span
     */
    constexpr operator Span<const T>() const
    {
        return {data, count};
    }

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

} // namespace hg

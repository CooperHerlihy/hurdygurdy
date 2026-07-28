#pragma once

#include "hg/inttypes.hpp"
#include "hg/strings.hpp"

#include <bit>

namespace hg {

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

} // namespace hg

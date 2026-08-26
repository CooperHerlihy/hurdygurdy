#pragma once

#include "hg/inttypes.hpp"
#include "hg/strings.hpp"

#include <bit>

namespace hg {

/**
 * The hash template
 */
template<typename T>
constexpr u64 hash(const T&)
{
    static_assert(false, "Type cannot be hashed without template specialization");
    return 0;
}

/**
 * Hash map hashing for u8
 */
template<>
constexpr u64 hash(const u8& val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for u16
 */
template<>
constexpr u64 hash(const u16& val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for u32
 */
template<>
constexpr u64 hash(const u32& val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for u64
 */
template<>
constexpr u64 hash(const u64& val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for i8
 */
template<>
constexpr u64 hash(const i8& val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for i16
 */
template<>
constexpr u64 hash(const i16& val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for i32
 */
template<>
constexpr u64 hash(const i32& val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for i64
 */
template<>
constexpr u64 hash(const i64& val)
{
    return static_cast<u64>(val);
}

/**
 * Hash map hashing for f32
 */
template<>
constexpr u64 hash(const f32& val)
{
    return static_cast<u64>(std::bit_cast<u32>(val));
}

/**
 * Hash map hashing for f64
 */
template<>
constexpr u64 hash(const f64& val)
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
constexpr u64 hash(void* const& val)
{
    return hashPtr<void>(val);
}

/**
 * Hash map hashing for strings
 */
template<>
constexpr u64 hash(const StringView& str)
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
constexpr u64 hash(const char* const& str)
{
    return hash(StringView{str});
}

/**
 * Hash map hashing for C array
 */
template<u64 N>
constexpr u64 hash(const char (&str)[N])
{
    return hash(StringView{str, N});
}

/**
 * Hash map hashing for StringBuilder
 */
template<>
constexpr u64 hash(const StringBuilder& str)
{
    return hash(StringView{str});
}

/**
 * Hash map hashing for String
 */
template<>
constexpr u64 hash(const String& str)
{
    return hash(StringView{str});
}

} // namespace hg

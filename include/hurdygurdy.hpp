/*
 * =============================================================================
 *
 * Copyright (c) 2025 Cooper Herlihy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 */

#ifndef HURDYGURDY_HPP
#define HURDYGURDY_HPP

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <atomic>
#include <chrono>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

#ifdef __GNUC__
#define HG_COMPILER_GCC 1
#endif

#ifdef __clang__
#define HG_COMPILER_CLANG 1
#endif

#ifdef _MSC_VER
#define HG_COMPILER_MSVC 1
#endif

#ifdef __linux__
#define HG_PLATFORM_LINUX 1
#endif

#ifdef _WIN32
#define HG_PLATFORM_WINDOWS 1
#endif

#if !defined(HG_PLATFORM_LINUX) && !defined(HG_PLATFORM_WINDOWS)
#error "Unsupported platfom"
#endif

#ifdef NDEBUG
#define HG_RELEASE_MODE 1
#else
#define HG_DEBUG_MODE 1
#endif

#ifdef HG_DEBUG_MODE

#ifndef HG_NO_LOGGING
#define HG_LOGGING 1
#endif

#ifndef HG_NO_ASSERTIONS
#define HG_ASSERTIONS 1
#endif

#ifndef HG_NO_VK_DEBUG_MESSENGER
#define HG_VK_DEBUG_MESSENGER 1
#endif

#endif

#ifdef HG_RELEASE_MODE

#ifndef HG_NO_LOGGING
#define HG_LOGGING 1
#endif

#ifndef HG_ASSERTIONS
#define HG_NO_ASSERTIONS 1
#endif

#ifndef HG_VK_DEBUG_MESSENGER
#define HG_NO_VK_DEBUG_MESSENGER 1
#endif

#endif

#define hgMacroConcatInternal(x, y) x##y

/**
 * Concatenate two macros
 */
#define hgMacroConcat(x, y) hgMacroConcatInternal(x, y)

/**
 * A template to defer code execution until end of scope
 */
template<typename F>
struct HgDefer
{
    /**
     * The function to execute
     */
    F fn;

    /**
     * Prepare the function to defer
     */
    HgDefer(F fnVal) : fn(fnVal) {}

    /**
     * Execute the function
     */
    ~HgDefer()
    {
        fn();
    }
};

/**
 * Defers a piece of code until the end of the scope
 *
 * Parameters
 * - code The code to run, may be placed inside braces or not
 */
#define hgDefer(code) [[maybe_unused]] HgDefer hgMacroConcat(hgDefer_, __COUNTER__){[&]{code;}};

#ifdef HG_LOGGING

/**
 * Formats and logs a debug message to stderr for debugging purposes
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hgDebug(...) do { { (void)fprintf(stderr, "HurdyGurdy Debug: " __VA_ARGS__); } } while(0)

/**
 * Formats and logs a warning message to stderr
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hgWarn(...) do { (void)fprintf(stderr, "HurdyGurdy Warning: " __VA_ARGS__); } while(0)

/**
 * Formats and logs an error message to stderr and aborts the program
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hgError(...) do { (void)fprintf(stderr, "HurdyGurdy Error: " __VA_ARGS__); abort(); } while(0)

#else

/**
 * Formats and logs a debug message to stderr for debugging puposes
 */
#define hgDebug(...) do {} while(0)

/**
 * Formats and logs a warning message to stderr
 */
#define hgWarn(...) do {} while(0)

/**
 * Formats and logs an error message to stderr and aborts the program
 */
#define hgError(...) do { abort(); } while(0)

#endif

#ifdef HG_ASSERTIONS

/**
 * Asserts condition, or aborts the program
 */
#define hgAssert(cond) do { \
    if (!(cond)) \
    { \
        hgError("Assertion failed in " __FILE__ ":%d %s() " #cond "\n", __LINE__, __func__); \
    } \
} while(0)

#else

/**
 * Asserts condition, or aborts the program
 */
#define hgAssert(cond) do {} while(0)

#endif

/**
 * An 8 bit, 1 byte unsigned integer
 */
typedef uint8_t u8;
/**
 * A 16 bit, 2 byte unsigned integer
 */
typedef uint16_t u16;
/**
 * A 32 bit, 4 byte unsigned integer
 */
typedef uint32_t u32;
/**
 * A 64 bit, 8 byte unsigned integer
 */
typedef uint64_t u64;

/**
 * An 8 bit, 1 byte signed integer
 */
typedef int8_t i8;
/**
 * A 16 bit, 2 byte signed integer
 */
typedef int16_t i16;
/**
 * A 32 bit, 4 byte signed integer
 */
typedef int32_t i32;
/**
 * A 64 bit, 8 byte signed integer
 */
typedef int64_t i64;

/**
 * An unsigned integer representing a pointer
 */
typedef uintptr_t uptr;
/**
 * A signed integer representing a pointer
 */
typedef intptr_t iptr;

/**
 * A 32 bit, 4 byte floating point value
 */
typedef float_t f32;
/**
 * A 64 bit, 8 byte floating point value
 */
typedef double_t f64;

/**
 * Initializes the HurdyGurdy library
 *
 * Subsystems initialized:
 * - Arena allocation
 * - Thread pool
 * - IO thread
 * - Resource managers
 * - OS windowing
 * - Hardware graphics
 */
void hgInit();

/**
 * Shuts down the HurdyGurdy library
 */
void hgExit();

/**
 * Run Hurdy Gurdy tests, asserting success
 */
void hgTest();

/**
 * The value of Pi
 */
#define hgPi 3.1415926535897932
/**
 * The value of Euler's number
 */
#define hgEuler 2.7182818284590452
/**
 * The value of square root 2
 */
#define hgRoot2 1.4142135623730951
/**
 * The value of square root 3
 */
#define hgRoot3 1.7320508075688772

/**
 * A 2D vector
 */
struct HgVec2
{
    /**
     * The vector components
     */
    f32 x, y;

    /**
     * Construct uninitialized
     */
    HgVec2() = default;
    /**
     * Construct from a single scalar
     */
    explicit constexpr HgVec2(f32 scalar) : x{scalar}, y{scalar} {}
    /**
     * Construct from a list of values
     */
    explicit constexpr HgVec2(f32 xVal, f32 yVal) : x{xVal}, y{yVal} {}

    /**
     * Access with index
     */
    constexpr f32& operator[](u32 index)
    {
        hgAssert(index < 2);
        return *(&x + index);
    }

    /**
     * Access with index
     */
    constexpr const f32& operator[](u32 index) const {
        hgAssert(index < 2);
        return *(&x + index);
    }

    /**
     * Add another vector in place
     */
    const HgVec2& operator+=(HgVec2 other);
    /**
     * Subtract another vector in place
     */
    const HgVec2& operator-=(HgVec2 other);
    /**
     * Multiply another vector in place
     */
    const HgVec2& operator*=(HgVec2 other);
    /**
     * Divide another vector in place
     */
    const HgVec2& operator/=(HgVec2 other);
};

/**
 * A 3D vector
 */
struct HgVec3
{
    /**
     * The vector components
     */
    f32 x, y, z;

    /**
     * Construct uninitialized
     */
    HgVec3() = default;
    /**
     * Construct from a single scalar
     */
    explicit constexpr HgVec3(f32 scalar) : x{scalar}, y{scalar}, z{scalar} {}
    /**
     * Construct from a list of values
     */
    explicit constexpr HgVec3(f32 xVal, f32 yVal, f32 zVal) : x{xVal}, y{yVal}, z{zVal} {}
    /**
     * Construct from a Vec2 and scalar
     */
    explicit constexpr HgVec3(HgVec2 other, f32 zVal) : x{other.x}, y{other.y}, z{zVal} {}

    /**
     * Downsize to Vec2
     */
    explicit operator HgVec2()
    {
        return HgVec2{x, y};
    }

    /**
     * Access with index
     */
    constexpr f32& operator[](u32 index)
    {
        hgAssert(index < 3);
        return *(&x + index);
    }

    /**
     * Access with index
     */
    constexpr const f32& operator[](u32 index) const {
        hgAssert(index < 3);
        return *(&x + index);
    }

    /**
     * Add another vector in place
     */
    const HgVec3& operator+=(HgVec3 other);
    /**
     * Subtract another vector in place
     */
    const HgVec3& operator-=(HgVec3 other);
    /**
     * Multiply another vector in place
     */
    const HgVec3& operator*=(HgVec3 other);
    /**
     * Divide another vector in place
     */
    const HgVec3& operator/=(HgVec3 other);
};

/**
 * A 4D vector
 */
struct HgVec4
{
    /**
     * The vector components
     */
    f32 x, y, z, w;

    /**
     * Construct uninitialized
     */
    HgVec4() = default;
    /**
     * Construct from a single scalar
     */
    explicit constexpr HgVec4(f32 scalar) : x{scalar}, y{scalar}, z{scalar}, w{scalar} {}
    /**
     * Construct from a list of values
     */
    explicit constexpr HgVec4(f32 xVal, f32 yVal, f32 zVal, f32 wVal) : x{xVal}, y{yVal}, z{zVal}, w{wVal} {}
    /**
     * Construct from a Vec2 and scalars
     */
    explicit constexpr HgVec4(HgVec2 other, f32 zVal, f32 wVal) : x{other.x}, y{other.y}, z{zVal}, w{wVal} {}
    /**
     * Construct from a Vec3 and scalar
     */
    explicit constexpr HgVec4(HgVec3 other, f32 wVal) : x{other.x}, y{other.y}, z{other.z}, w{wVal} {}

    /**
     * Downsize to Vec2
     */
    explicit operator HgVec2()
    {
        return HgVec2{x, y};
    }
    /**
     * Downsize to Vec3
     */
    explicit operator HgVec3()
    {
        return HgVec3{x, y, z};
    }

    /**
     * Access with index
     */
    constexpr f32& operator[](u32 index)
    {
        hgAssert(index < 4);
        return *(&x + index);
    }

    /**
     * Access with index
     */
    constexpr const f32& operator[](u32 index) const {
        hgAssert(index < 4);
        return *(&x + index);
    }

    /**
     * Add another vector in place
     */
    const HgVec4& operator+=(HgVec4 other);
    /**
     * Subtract another vector in place
     */
    const HgVec4& operator-=(HgVec4 other);
    /**
     * Multiply another vector in place
     */
    const HgVec4& operator*=(HgVec4 other);
    /**
     * Divide another vector in place
     */
    const HgVec4& operator/=(HgVec4 other);
};

/**
 * A 2x2 matrix
 */
struct HgMat2
{
    /**
     * The matrix components
     */
    HgVec2 x, y;

    /**
     * Construct uninitialized
     */
    HgMat2() = default;
    /**
     * Construct from a single scalar
     */
    explicit constexpr HgMat2(f32 scalar) : x{scalar, 0}, y{0, scalar} {}
    /**
     * Construct from a list of values
     */
    explicit constexpr HgMat2(f32 xx, f32 xy, f32 yx, f32 yy) : x{xx, xy}, y{yx, yy} {}
    /**
     * Construct from a list of vectors
     */
    explicit constexpr HgMat2(HgVec2 xVal, HgVec2 yVal) : x{xVal}, y{yVal} {}

    /**
     * Access with index
     */
    constexpr HgVec2& operator[](u32 index)
    {
        hgAssert(index < 2);
        return *(&x + index);
    }

    /**
     * Access with index
     */
    constexpr const HgVec2& operator[](u32 index) const {
        hgAssert(index < 2);
        return *(&x + index);
    }

    /**
     * Add another matrix in place
     */
    const HgMat2& operator+=(const HgMat2& other);
    /**
     * Subtract another matrix in place
     */
    const HgMat2& operator-=(const HgMat2& other);
};

/**
 * A 3x3 matrix
 */
struct HgMat3
{
    /**
     * The matrix components
     */
    HgVec3 x, y, z;

    /**
     * Construct uninitialized
     */
    HgMat3() = default;
    /**
     * Construct from a single scalar
     */
    explicit constexpr HgMat3(f32 scalar)
        : x{scalar, 0, 0}, y{0, scalar, 0}, z{0, 0, scalar} {}
    /**
     * Construct from a list of vectors
     */
    explicit constexpr HgMat3(HgVec3 xVal, HgVec3 yVal, HgVec3 zVal)
        : x{xVal}, y{yVal}, z{zVal} {}
    /**
     * Construct from a Mat2
     */
    explicit constexpr HgMat3(const HgMat2& other)
        : x{other.x, 0}, y{other.y, 0}, z{0, 0, 1} {}

    /**
     * Downsize to Mat2
     */
    explicit constexpr operator HgMat2()
    {
        return HgMat2{HgVec2{x}, HgVec2{y}};
    }

    /**
     * Access with index
     */
    constexpr HgVec3& operator[](u32 index)
    {
        hgAssert(index < 3);
        return *(&x + index);
    }

    /**
     * Access with index
     */
    constexpr const HgVec3& operator[](u32 index) const {
        hgAssert(index < 3);
        return *(&x + index);
    }

    /**
     * Add another matrix in place
     */
    const HgMat3& operator+=(const HgMat3& other);
    /**
     * Subtract another matrix in place
     */
    const HgMat3& operator-=(const HgMat3& other);
};

/**
 * A 4x4 matrix
 */
struct HgMat4
{
    /**
     * The matrix components
     */
    HgVec4 x, y, z, w;

    /**
     * Construct uninitialized
     */
    HgMat4() = default;
    /**
     * Construct from a single scalar
     */
    explicit constexpr HgMat4(f32 scalar)
        : x{scalar, 0, 0, 0}, y{0, scalar, 0, 0}, z{0, 0, scalar, 0}, w{0, 0, 0, scalar} {}
    /**
     * Construct from a list of vectors
     */
    explicit constexpr HgMat4(HgVec4 xVal, HgVec4 yVal, HgVec4 zVal, HgVec4 wVal)
        : x{xVal}, y{yVal}, z{zVal}, w{wVal} {}
    /**
     * Construct from a Mat2
     */
    explicit constexpr HgMat4(const HgMat2& other)
        : x{other.x, 0, 0}, y{other.y, 0, 0}, z{0, 0, 1, 0}, w{0, 0, 0, 1} {}
    /**
     * Construct from a Mat3
     */
    explicit constexpr HgMat4(const HgMat3& other)
        : x{other.x, 0}, y{other.y, 0}, z{other.z, 0}, w{0, 0, 0, 1} {}

    /**
     * Downsize to Mat2
     */
    explicit constexpr operator HgMat2()
    {
        return HgMat2{HgVec2{x}, HgVec2{y}};
    }
    /**
     * Downsize to Mat3
     */
    explicit constexpr operator HgMat3()
    {
        return HgMat3{HgVec3{x}, HgVec3{y}, HgVec3{z}};
    }

    /**
     * Access with index
     */
    constexpr HgVec4& operator[](u32 index)
    {
        hgAssert(index < 4);
        return *(&x + index);
    }

    /**
     * Access with index
     */
    constexpr const HgVec4& operator[](u32 index) const {
        hgAssert(index < 4);
        return *(&x + index);
    }

    /**
     * Add another matrix in place
     */
    const HgMat4& operator+=(const HgMat4& other);
    /**
     * Subtract another matrix in place
     */
    const HgMat4& operator-=(const HgMat4& other);
};

/**
 * A complex number
 */
struct HgComplex
{
    /**
     * The real part
     */
    f32 r;
    /**
     * The imaginary part
     */
    f32 i;

    /**
     * Construct uninitialized
     */
    HgComplex() = default;
    /**
     * Construct from just a real value
     */
    explicit constexpr HgComplex(f32 rVal) : r{rVal}, i{0} {}
    /**
     * Construct from a list of values
     */
    explicit constexpr HgComplex(f32 rVal, f32 iVal) : r{rVal}, i{iVal} {}

    /**
     * Access with index
     */
    constexpr f32& operator[](u32 index)
    {
        hgAssert(index < 2);
        return *(&r + index);
    }

    /**
     * Access with index
     */
    constexpr const f32& operator[](u32 index) const {
        hgAssert(index < 2);
        return *(&r + index);
    }

    /**
     * Add another complex number in place
     */
    const HgComplex& operator+=(HgComplex other);
    /**
     * Subtract another complex number in place
     */
    const HgComplex& operator-=(HgComplex other);
};

/**
 * A quaternion
 */
struct HgQuat
{
    /**
     * The real part
     */
    f32 r;
    /**
     * The imaginary parts
     */
    f32 i, j, k;

    /**
     * Construct uninitialized
     */
    HgQuat() = default;
    /**
     * Construct from just a real value
     */
    explicit constexpr HgQuat(f32 rVal) : r{rVal}, i{0}, j{0}, k{0} {}
    /**
     * Construct from a list of values
     */
    explicit constexpr HgQuat(f32 rVal, f32 iVal, f32 jVal, f32 kVal) : r{rVal}, i{iVal}, j{jVal}, k{kVal} {}

    /**
     * Access with index
     */
    constexpr f32& operator[](u32 index)
    {
        hgAssert(index < 4);
        return *(&r + index);
    }

    /**
     * Access with index
     */
    constexpr const f32& operator[](u32 index) const {
        hgAssert(index < 4);
        return *(&r + index);
    }

    /**
     * Add another quaternion in place
     */
    const HgQuat& operator+=(HgQuat other);
    /**
     * Subtract another quaternion in place
     */
    const HgQuat& operator-=(HgQuat other);
};

/**
 * Compare vectors
 */
constexpr bool operator==(HgVec2 lhs, HgVec2 rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

/**
 * Compare vectors
 */
constexpr bool operator!=(HgVec2 lhs, HgVec2 rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

/**
 * Compare vectors
 */
constexpr bool operator==(HgVec3 lhs, HgVec3 rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

/**
 * Compare vectors
 */
constexpr bool operator!=(HgVec3 lhs, HgVec3 rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}

/**
 * Compare vectors
 */
constexpr bool operator==(HgVec4 lhs, HgVec4 rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

/**
 * Compare vectors
 */
constexpr bool operator!=(HgVec4 lhs, HgVec4 rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
}

/**
 * Compare matrices
 */
constexpr bool operator==(const HgMat2& lhs, const HgMat2& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

/**
 * Compare matrices
 */
constexpr bool operator!=(const HgMat2& lhs, const HgMat2& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

/**
 * Compare matrices
 */
constexpr bool operator==(const HgMat3& lhs, const HgMat3& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

/**
 * Compare matrices
 */
constexpr bool operator!=(const HgMat3& lhs, const HgMat3& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}

/**
 * Compare matrices
 */
constexpr bool operator==(const HgMat4& lhs, const HgMat4& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

/**
 * Compare matrices
 */
constexpr bool operator!=(const HgMat4& lhs, const HgMat4& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
}

/**
 * Compare complex numbers
 */
constexpr bool operator==(HgComplex lhs, HgComplex rhs)
{
    return lhs.r == rhs.r && lhs.i == rhs.i;
}

/**
 * Compare complex numbers
 */
constexpr bool operator!=(HgComplex lhs, HgComplex rhs)
{
    return lhs.r != rhs.r || lhs.i != rhs.i;
}

/**
 * Compare quaternions
 */
constexpr bool operator==(HgQuat lhs, HgQuat rhs)
{
    return lhs.r == rhs.r && lhs.i == rhs.i && lhs.j == rhs.j && lhs.k == rhs.k;
}

/**
 * Compare quaternions
 */
constexpr bool operator!=(HgQuat lhs, HgQuat rhs)
{
    return lhs.r != rhs.r || lhs.i != rhs.i || lhs.j != rhs.j || lhs.k != rhs.k;
}

/**
 * Add arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hgAddVec(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Add 2D vectors
 */
constexpr HgVec2 operator+(HgVec2 lhs, HgVec2 rhs)
{
    return HgVec2{lhs.x + rhs.x, lhs.y + rhs.y};
}

/**
 * Add 3D vectors
 */
constexpr HgVec3 operator+(HgVec3 lhs, HgVec3 rhs)
{
    return HgVec3{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

/**
 * Add 4D vectors
 */
constexpr HgVec4 operator+(HgVec4 lhs, HgVec4 rhs)
{
    return HgVec4{lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

/**
 * Subtract arbitrary size vectors
 *
 * parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hgSubVec(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Subtract 2D vectors
 */
constexpr HgVec2 operator-(HgVec2 lhs, HgVec2 rhs)
{
    return HgVec2{lhs.x - rhs.x, lhs.y - rhs.y};
}

/**
 * Subtract 3D vectors
 */
constexpr HgVec3 operator-(HgVec3 lhs, HgVec3 rhs)
{
    return HgVec3{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

/**
 * Subtract 4D vectors
 */
constexpr HgVec4 operator-(HgVec4 lhs, HgVec4 rhs)
{
    return HgVec4{lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

/**
 * Multiply pairwise arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hgPairwiseMulVec(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Multiply pairwise 2D vectors
 */
constexpr HgVec2 operator*(HgVec2 lhs, HgVec2 rhs)
{
    return HgVec2{lhs.x * rhs.x, lhs.y * rhs.y};
}

/**
 * Multiply pairwise 3D vectors
 */
constexpr HgVec3 operator*(HgVec3 lhs, HgVec3 rhs)
{
    return HgVec3{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

/**
 * Multiply pairwise 4D vectors
 */
constexpr HgVec4 operator*(HgVec4 lhs, HgVec4 rhs)
{
    return HgVec4{lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w};
}

/**
 * Multiply a scalar and a vector
 */
void hgMulVecScalar(u32 size, f32* dst, f32 scalar, const f32* vec);

/**
 * Multiply a scalar and a 2D vector
 */
constexpr HgVec2 operator*(f32 scalar, HgVec2 vec)
{
    return HgVec2{scalar * vec.x, scalar * vec.y};
}

/**
 * Multiply a scalar and a 2D vector
 */
constexpr HgVec2 operator*(HgVec2 vec, f32 scalar)
{
    return HgVec2{scalar * vec.x, scalar * vec.y};
}

/**
 * Multiply a scalar and a 3D vector
 */
constexpr HgVec3 operator*(f32 scalar, HgVec3 vec)
{
    return HgVec3{scalar * vec.x, scalar * vec.y, scalar * vec.z};
}

/**
 * Multiply a scalar and a 3D vector
 */
constexpr HgVec3 operator*(HgVec3 vec, f32 scalar)
{
    return HgVec3{scalar * vec.x, scalar * vec.y, scalar * vec.z};
}

/**
 * Multiply a scalar and a 4D vector
 */
constexpr HgVec4 operator*(f32 scalar, HgVec4 vec)
{
    return HgVec4{scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
}

/**
 * Multiply a scalar and a 4D vector
 */
constexpr HgVec4 operator*(HgVec4 vec, f32 scalar)
{
    return HgVec4{scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
}

/**
 * Divide pairwise arbitrary size vectors
 *
 * Note, cannot divide by 0
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hgPairwiseDivVec(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Divide pairwise 2D vectors
 *
 * Note, cannot divide by 0
 */
constexpr HgVec2 operator/(HgVec2 lhs, HgVec2 rhs)
{
    hgAssert(rhs.x != 0 && rhs.y != 0);
    return HgVec2{lhs.x / rhs.x, lhs.y / rhs.y};
}

/**
 * Divide pairwise 3D vectors
 *
 * Note, cannot divide by 0
 */
constexpr HgVec3 operator/(HgVec3 lhs, HgVec3 rhs)
{
    hgAssert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
    return HgVec3{lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
}

/**
 * Divide pairwise 4D vectors
 *
 * Note, cannot divide by 0
 */
constexpr HgVec4 operator/(HgVec4 lhs, HgVec4 rhs)
{
    hgAssert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0 && rhs.w != 0);
    return HgVec4{lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w};
}

/**
 * Divide a vector by a scalar
 *
 * Note, cannot divide by 0
 */
void hgDivVecScalar(u32 size, f32* dst, const f32* vec, f32 scalar);

/**
 * Divide a 2D vector by a scalar
 *
 * Note, cannot divide by 0
 */
constexpr HgVec2 operator/(HgVec2 vec, f32 scalar)
{
    hgAssert(scalar != 0);
    return HgVec2{vec.x / scalar, vec.y / scalar};
}

/**
 * Divide a 3D vector by a scalar
 *
 * Note, cannot divide by 0
 */
constexpr HgVec3 operator/(HgVec3 vec, f32 scalar)
{
    hgAssert(scalar != 0);
    return HgVec3{vec.x / scalar, vec.y / scalar, vec.z / scalar};
}

/**
 * Divide a 4D vector by a scalar
 *
 * Note, cannot divide by 0
 */
constexpr HgVec4 operator/(HgVec4 vec, f32 scalar)
{
    hgAssert(scalar != 0);
    return HgVec4{vec.x / scalar, vec.y / scalar, vec.z / scalar, vec.w / scalar};
}

/**
 * Compute the dot product of arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hgDot(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Compute the dot product of 2D vectors
 */
constexpr f32 hgDot(HgVec2 lhs, HgVec2 rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

/**
 * Compute the dot product of 3D vectors
 */
constexpr f32 hgDot(HgVec3 lhs, HgVec3 rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

/**
 * Compute the dot product of 4D vectors
 */
constexpr f32 hgDot(HgVec4 lhs, HgVec4 rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

/**
 * Compute the length of a vector
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be nullptr
 * - vec The vector to compute the length of, must not be nullptr
 */
void hgLen(u32 size, f32* dst, const f32* vec);

/**
 * Compute the length of a 2D vector
 */
f32 hgLen(HgVec2 vec);

/**
 * Compute the length of a 3D vector
 */
f32 hgLen(HgVec3 vec);

/**
 * Compute the length of a 4D vector
 */
f32 hgLen(HgVec4 vec);

/**
 * Normalize a vector
 *
 * Note, cannot normalize 0
 */
void hgNorm(u32 size, f32* dst, const f32* vec);

/**
 * Normalize a 2D vector
 *
 * Note, cannot normalize 0
 */
HgVec2 hgNorm(HgVec2 vec);

/**
 * Normalize a 3D vector
 *
 * Note, cannot normalize 0
 */
HgVec3 hgNorm(HgVec3 vec);

/**
 * Normalize a 4D vector
 *
 * Note, cannot normalize 0
 */
HgVec4 hgNorm(HgVec4 vec);

/**
 * Compute the cross product of 3D vectors
 *
 * Parameters
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hgCross(f32* dst, const f32* lhs, const f32* rhs);

/**
 * Compute the cross product of 3D vectors
 */
HgVec3 hgCross(HgVec3 lhs, HgVec3 rhs);

/**
 * Add arbitrary size matrices
 *
 * Parameters
 * - width The width of the matrices
 * - height The height of the matrices
 * - dst The destination matrix, must not be nullptr
 * - lhs The left-hand side matrix, must not be nullptr
 * - rhs The right-hand side matrix, must not be nullptr
 */
void hgAddMat(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Add 2x2 matrices
 */
HgMat2 operator+(const HgMat2& lhs, const HgMat2& rhs);

/**
 * Add 3x3 matrices
 */
HgMat3 operator+(const HgMat3& lhs, const HgMat3& rhs);

/**
 * Add 4x4 matrices
 */
HgMat4 operator+(const HgMat4& lhs, const HgMat4& rhs);

/**
 * Subtract arbitrary size matrices
 *
 * Parameters
 * - width The width of the matrices
 * - height The height of the matrices
 * - dst The destination matrix, must not be nullptr
 * - lhs The left-hand side matrix, must not be nullptr
 * - rhs The right-hand side matrix, must not be nullptr
 */
void hgSubMat(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Subtract 2x2 matrices
 */
HgMat2 operator-(const HgMat2& lhs, const HgMat2& rhs);

/**
 * Subtract 3x3 matrices
 */
HgMat3 operator-(const HgMat3& lhs, const HgMat3& rhs);

/**
 * Subtract 4x4 matrices
 */
HgMat4 operator-(const HgMat4& lhs, const HgMat4& rhs);

/**
 * Multiply arbitrary size matrices
 *
 * Parameters
 * - dst The destination matrix, must not be nullptr
 * - wl The width of the left-hand side matrix
 * - hl The height of the left-hand side matrix
 * - lhs The left-hand side matrix, must not be nullptr
 * - wr The width of the right-hand side matrix
 * - hr The height of the right-hand side matrix
 * - rhs The right-hand side matrix, must not be nullptr
 */
void hgMulMat(f32* dst, u32 wl, u32 hl, const f32* lhs, u32 wr, u32 hr, const f32* rhs);

/**
 * Multiply 2x2 matrices
 */
HgMat2 operator*(const HgMat2& lhs, const HgMat2& rhs);

/**
 * Multiply 3x3 matrices
 */
HgMat3 operator*(const HgMat3& lhs, const HgMat3& rhs);

/**
 * Multiply 4x4 matrices
 */
HgMat4 operator*(const HgMat4& lhs, const HgMat4& rhs);

/**
 * Multiply a matrix and a vector
 *
 * Parameters
 * - width The width of the matrix
 * - height The height of the matrix
 * - dst The destination vector, must not be nullptr
 * - mat The matrix to multiply with, must not be nullptr
 * - vec The vector to multiply with, must not be nullptr
 */
void hgMulMatVec(u32 width, u32 height, f32* dst, const f32* mat, const f32* vec);

/**
 * Multiply a 2x2 matrix and a 2D vector
 */
HgVec2 operator*(const HgMat2& lhs, HgVec2 rhs);

/**
 * Multiply a 3x3 matrix and a 3D vector
 */
HgVec3 operator*(const HgMat3& lhs, HgVec3 rhs);

/**
 * Multiply a 4x4 matrix and a 4D vector
 */
HgVec4 operator*(const HgMat4& lhs, HgVec4 rhs);

/**
 * Add complex numbers
 */
constexpr HgComplex operator+(HgComplex lhs, HgComplex rhs)
{
    return HgComplex{lhs.r + rhs.r, lhs.i + rhs.i};
}

/**
 * Subtract complex numbers
 */
constexpr HgComplex operator-(HgComplex lhs, HgComplex rhs)
{
    return HgComplex{lhs.r - rhs.r, lhs.i - rhs.i};
}

/**
 * Multiply complex numbers
 */
constexpr HgComplex operator*(HgComplex lhs, HgComplex rhs)
{
    return HgComplex{lhs.r * rhs.r - lhs.i * rhs.i, lhs.r * rhs.i + lhs.i * rhs.r};
}

/**
 * Add quaternions
 */
constexpr HgQuat operator+(HgQuat lhs, HgQuat rhs)
{
    return HgQuat{lhs.r + rhs.r, lhs.i + rhs.i, lhs.j + rhs.j, lhs.k + rhs.k};
}

/**
 * Subtract quaternions
 */
constexpr HgQuat operator-(HgQuat lhs, HgQuat rhs)
{
    return HgQuat{lhs.r - rhs.r, lhs.i - rhs.i, lhs.j - rhs.j, lhs.k - rhs.k};
}

/**
 * Multiply quaternions
 */
HgQuat operator*(HgQuat lhs, HgQuat rhs);

/**
 * Compute the conjugate of a quaternion
 */
constexpr HgQuat hgConj(HgQuat quat)
{
    return HgQuat{quat.r, -quat.i, -quat.j, -quat.k};
}

/**
 * Create a rotation quaternion from an axis and angle
 */
HgQuat hgAxisAngle(HgVec3 axis, f32 angle);

/**
 * Rotate a 3D vector using a quaternion
 */
HgVec3 hgRotate(HgQuat lhs, HgVec3 rhs);

/**
 * Rotate a 3x3 matrix using a quaternion
 */
HgMat3 hgRotate(HgQuat lhs, HgMat3 rhs);

/**
 * Creates a model matrix for 2D graphics
 *
 * Parameters
 * - position The position of the model
 * - scale The scale of the model
 * - rotation The rotation of the model
 */
HgMat4 hgModelMatrix2D(HgVec3 position, HgVec2 scale, f32 rotation);

/**
 * Creates a model matrix for 3D graphics
 *
 * Parameters
 * - position The position of the model
 * - scale The scale of the model
 * - rotation The rotation of the model
 */
HgMat4 hgModelMatrix3D(const HgVec3& position, const HgVec3& scale, const HgQuat& rotation);

/**
 * Creates a view matrix
 *
 * Parameters
 * - position The position of the camera
 * - zoom The zoom of the camera
 * - rotation The rotation of the camera
 */
HgMat4 hgViewMatrix(const HgVec3& position, const HgVec3& zoom, const HgQuat& rotation);

/**
 * Creates an orthographic projection matrix
 *
 * Parameters
 * - left The left-hand side of the view frustum
 * - right The right-hand side of the view frustum
 * - top The top of the view frustum
 * - bottom The bottom of the view frustum
 * - near The near plane of the view frustum
 * - far The far plane of the view frustum
 */
HgMat4 hgOrthographicProjection(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far);

/**
 * Creates a perspective projection matrix
 *
 * Parameters
 * - fov The field of view of the projection in radians
 * - aspect The aspect ratio of the projection
 * - near The near plane of the projection, must be greater than 0.0f
 * - far The far plane of the projection, must be greater than near
 */
HgMat4 hgPerspectiveProjection(f32 fov, f32 aspect, f32 near, f32 far);

// random number generators : TODO

// sort and search algorithms : TODO

/**
 * Calculates the maximum number of mipmap levels that an image can have
 *
 * Parameters
 * - width The width of the image
 * - height The height of the image
 * - depth The depth of the image
 */
u32 hgMaxMipmaps(u32 width, u32 height, u32 depth);

/**
 * Aligns a pointer to an alignment
 *
 * Parameters
 * - value The value to align
 * - alignment The alignment, must be a power of two
 *
 * Returns
 * - The aligned value
 */
constexpr uptr hgAlign(uptr value, uptr alignment)
{
    hgAssert(alignment > 0 && (alignment & (alignment - 1)) == 0);
    return (value + alignment - 1) & ~(alignment - 1);
}

/**
 * An arena allocator
 */
struct HgArena
{
    /**
     * A pointer to the memory being allocated
     */
    void* memory;
    /**
     * The capacity of the memory being allocated
     */
    u64 capacity;
    /**
     * The next allocation to be given out
     */
    u64 head;

    /**
     * Construct uninitialized
     */
    HgArena() = default;

    /**
     * Create an arena from a block of memory
     */
    HgArena(void* memoryVal, u64 capacityVal) : memory{memoryVal}, capacity{capacityVal}, head{0} {}
};

/**
 * Allocates memory from an arena
 *
 * Parameters
 * - arena The arena to allocate from
 * - size The size in bytes to allocate
 * - alignment The required alignment of the allocation in bytes
 *
 * Returns
 * - The allocation, never nullptr
 */
void* hgAlloc(HgArena* arena, u64 size, u64 alignment);

/**
 * A convenience to allocate an array of a type
 *
 * Note, objects are not initialized
 *
 * Parameters
 * - arena The arena to allocate from
 * - count The number of T to allocate
 *
 * Returns
 * - The allocated array, never nullptr
 */
template<typename T>
T* hgAlloc(HgArena* arena, u64 count)
{
    return (T*)hgAlloc(arena, count * sizeof(T), alignof(T));
}

/**
 * Reallocates memory from a arena
 *
 * Simply increases the size if allocation is the most recent allocation
 *
 * Parameters
 * - arena The arena to allocate from
 * - allocation The allocation to grow
 * - oldSize The old size in bytes allocation
 * - newSize The new size in bytes to allocate
 * - alignment The required alignment of the allocation in bytes
 *
 * Returns
 * - The allocation, never nullptr
 */
void* hgRealloc(HgArena* arena, void* allocation, u64 oldSize, u64 newSize, u64 alignment);

/**
 * A convenience to reallocate an array of a type
 *
 * Note, objects are default constructed if possible, otherwise they are
 * left uninitialized
 *
 * Parameters
 * - arena The arena to allocate from
 * - allocation The allocation to reallocate
 * - oldCount The old number of T allocated
 * - newCount The new number of T to allocate
 *
 * Returns
 * - The reallocated array, never nullptr
 */
template<typename T>
T* hgRealloc(HgArena* arena, T* allocation, u64 oldCount, u64 newCount) 
{
    static_assert(std::is_trivially_copyable_v<T>);
    return (T*)hgRealloc(arena, allocation, oldCount * sizeof(T), newCount * sizeof(T), alignof(T));
}

/**
 * Create a guard which restores an arena's head at the end of the scope
 */
struct HgArenaScope
{
    /**
     * The arena to restore
     */
    HgArena* arena;
    /**
     * The arena's original head
     */
    u64 head;

    /**
     * Constructs the scope guard
     */
    HgArenaScope(HgArena* arenaVal) : arena{arenaVal}, head{arenaVal->head} {}

    /**
     * Restores the arena's head
     */
    ~HgArenaScope()
    {
        arena->head = head;
    }
};

/**
 * Initializes scratch arenas
 */
void hgInitScratchMemory();

/**
 * Deinitializes scratch arenas
 */
void hgDeinitScratchMemory();

/**
 * Get a scratch arena for temporary allocations, accounting for conflicts
 *
 * Parameters
 * - conflict If arenas are being used, the returned arena will be different
 * - count The number of conflicts
 *
 * Returns
 * - A scratch arena, never nullptr
 */
HgArena* hgGetScratch(HgArena const* const* conflicts = nullptr, u32 count = 0);

/**
 * A span view into a string
 */
struct HgStringView
{
    /**
     * The characters
     */
    const char* chars;
    /**
     * The number of characters;
     */
    u64 length;

    /**
     * Construct uninitialized
     */
    HgStringView() = default;

    /**
     * Create a string view from a pointer and length
     */
    constexpr HgStringView(const char* charsVal, u64 lengthVal)
        : chars{charsVal}, length{lengthVal} {}

    /**
     * Create a string view from begin and end pointers
     */
    constexpr HgStringView(const char* charsBegin, const char* charsEnd)
        : chars{charsBegin}, length{(uptr)(charsEnd - charsBegin)}
    {
        hgAssert(charsBegin <= charsEnd);
    }

    /**
     * Implicit constexpr conversion from c string
     *
     * Potentially dangerous, c string should be at most 4096 chars
     */
    constexpr HgStringView(const char* cStr) : chars{cStr}, length{0}
    {
        while (cStr[length] != '\0')
        {
            ++length;
            hgAssert(length <= 4096);
        }
    }

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr const char& operator[](u64 index) const
    {
        hgAssert(chars != nullptr);
        hgAssert(index < length);
        return chars[index];
    }

    /**
     * For c++ ranged based for
     */
    constexpr const char* begin() const
    {
        return chars;
    }

    /**
     * For c++ ranged based for
     */
    constexpr const char* end() const
    {
        return chars + length;
    }
};

/**
 * Create a null terminated string for C interop
 *
 * Parameters
 * - arena The arena to allocate from
 * - str The string to create from
 */
char* hgCString(HgArena* arena, HgStringView str);

/**
 * A dynamic string container
 */
struct HgString
{
    /**
     * The character buffer
     */
    char* chars;
    /**
     * The max number of characters in the buffer
     */
    u64 capacity;
    /**
     * The number of characters currently in the string
     */
    u64 length;

    /**
     * Creates a new string with empty capacity
     *
     * Parameters
     * - arena The arena to allocate from
     * - capacity The capacity to begin with
     */
    static HgString create(HgArena* arena, u64 capacity);

    /**
     * Creates a new string copied from an existing string
     *
     * Parameters
     * - arena The arena to allocate from
     * - init The initial string to copy from
     */
    static HgString copy(HgArena* arena, HgStringView str);

    /**
     * Removes all characters
     */
    constexpr void reset()
    {
        length = 0;
    }

    /**
     * Changes the capacity
     *
     * Parameters
     * - arena The arena to allocate from
     * - newCapacity The new minimum capacity
     */
    void reserve(HgArena* arena, u64 newCapacity);

    /**
     * Increases the capacity of the string, or inits to 1
     *
     * Parameters
     * - arena The arena to allocate from
     * - factor The growth factor to increase by
     */
    void grow(HgArena* arena, f32 factor = 2.0f);

    /**
     * Access using the index operator
     */
    constexpr char& operator[](u64 index)
    {
        hgAssert(index < length);
        return chars[index];
    }

    /**
     * Access using the index operator in a const context
     */
    constexpr const char& operator[](u64 index) const
    {
        hgAssert(index < length);
        return chars[index];
    }

    /**
     * For c++ ranged based for loop
     */
    constexpr char* begin() const
    {
        return chars;
    }

    /**
     * For c++ ranged based for loop
     */
    constexpr char* end() const
    {
        return chars + length;
    }

    /**
     * Inserts a char into this string at index
     *
     * Parameters
     * - arena The arena to allocate from
     * - c The char to insert
     */
    HgString& insert(HgArena* arena, u64 index, char c);

    /**
     * Appends a char to the end of this string
     *
     * Parameters
     * - arena The arena to allocate from
     * - c The char to append
     */
    HgString& append(HgArena* arena, char c)
    {
        return insert(arena, length, c);
    }

    /**
     * Prepends a char to the beginning of this string
     *
     * Parameters
     * - arena The arena to allocate from
     * - c The char to prepend
     */
    HgString& prepend(HgArena* arena, char c)
    {
        return insert(arena, 0, c);
    }

    /**
     * Copies another string into this string at index
     *
     * Parameters
     * - arena The arena to allocate from
     * - str The string to copy from
     */
    HgString& insert(HgArena* arena, u64 index, HgStringView str);

    /**
     * Copies another string to the end of this string
     *
     * Parameters
     * - arena The arena to allocate from
     * - str The string to copy from
     */
    HgString& append(HgArena* arena, HgStringView str)
    {
        return insert(arena, length, str);
    }

    /**
     * Copies another string to the beginning of this string
     *
     * Parameters
     * - arena The arena to allocate from
     * - str The string to copy from
     */
    HgString& prepend(HgArena* arena, HgStringView str)
    {
        return insert(arena, 0, str);
    }

    /**
     * Implicit converts to a string view
     */
    constexpr operator HgStringView() const
    {
        return {chars, length};
    }
};

/**
 * Compare string views
 */
constexpr bool operator==(HgStringView lhs, HgStringView rhs)
{
    return lhs.length == rhs.length && memcmp(lhs.chars, rhs.chars, lhs.length) == 0;
}

/**
 * Compare string views
 */
constexpr bool operator!=(HgStringView lhs, HgStringView rhs)
{
    return !(lhs == rhs);
}

/**
 * Compare strings
 */
inline bool operator==(const HgString& lhs, const HgString& rhs)
{
    return lhs.length == rhs.length && memcmp(lhs.chars, rhs.chars, lhs.length) == 0;
}

/**
 * Compare strings
 */
inline bool operator!=(const HgString& lhs, const HgString& rhs)
{
    return !(lhs == rhs);
}

/**
 * Check whether a character is whitespace (space, tab, or newline)
 */
bool hgIsWhitespace(char c);

/**
 * Check whether a character is a base 10 numeral (0-9)
 */
bool hgIsNumeral(char c);

/**
 * Check whether a string is a base 10 integer
 */
bool hgIsIntenger(HgStringView str);

/**
 * Check whether a string is a base 10 floating point number
 */
bool hgIsFloat(HgStringView str);

/**
 * Create an integer from a base 10 string
 */
i64 hgStrToInt(HgStringView str);

/**
 * Create a float from a base 10 string
 */
f64 hgStrToFloat(HgStringView str);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 */
HgString hgIntToStr(HgArena* arena, i64 num);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 * - decimalCount The number of trailing decimal digits
 */
HgString hgFloatToStr(HgArena* arena, f64 num, u32 decimalCount);

// base 2 and 16 string-int conversions : TODO
// arbitrary base string-int conversions : TODO?

/**
 * Produce a formatted string : TODO
 *
 * Format specifiers
 * - int (i64): "{i}"
 * - unsigned int (u64): "{u}"
 * - hexadecimal (i64): "{x}"
 * - float with 6 decimals (f64): "{f}"
 * - float with N decimals (f64): "{fN}"
 * - char (char): "{c}"
 * - string (HgStringView): "{s}"
 * - c string (char*): "{cstr}"
 *
 * Use {{ and }} to escape the format specifier
 *
 * Parameters
 * - arena The arena to allocate from
 * - fmt The format string
 * - ... The format parameters
 */
HgString hgFormatString(HgArena* arena, HgStringView fmt, ...);

/**
 * An error contained in the json
 */
struct HgJsonError
{
    /**
     * The next error
     */
    HgJsonError* next;
    /**
     * The error message
     */
    HgString msg;
};

/**
 * A node in the json file
 */
struct HgJsonNode;

/**
 * The types contained in nodes
 */
enum HgJsonType : u32
{
    HgJsonType_none = 0,
    HgJsonType_struct,
    HgJsonType_field,
    HgJsonType_array,
    HgJsonType_string,
    HgJsonType_float,
    HgJsonType_integer,
    HgJsonType_bool,
};

/**
 * A field in a struct
 */
struct HgJsonField
{
    /**
     * The next field
     */
    HgJsonField* next;
    /**
     * The name of the field
     */
    HgString name;
    /**
     * The value stored in the field
     */
    HgJsonNode* value;
};

/**
 * A struct contained in the json
 */
struct HgJsonStruct
{
    /**
     * The first field
     */
    HgJsonField* fields;
};

/**
 * An element in an array
 */
struct HgJsonElem
{
    /**
     * The next element
     */
    HgJsonElem* next;
    /**
     * The value stored in the element
     */
    HgJsonNode* value;
};

/**
 * An array contained in the json
 */
struct HgJsonArray
{
    /**
     * The first element
     */
    HgJsonElem* elems;
};

/**
 * A node in the json file
 */
struct HgJsonNode
{
    /**
     * The node's type
     */
    HgJsonType type;
    /**
     * The value in the node
     */
    union
    {
        HgJsonStruct jstruct;
        HgJsonField field;
        HgJsonArray array;
        HgString string;
        f64 floating;
        i64 integer;
        bool boolean;
    };
};

/**
 * A parsed Json file
 */
struct HgJson
{
    /**
     * The successfully parsed nodes
     */
    HgJsonNode* file;
    /**
     * The errors found
     */
    HgJsonError* errors;
};

/**
 * Parses json text into a tree
 *
 * Parameters
 * - arena The arena to allocate from
 * - text The json text to parse
 *
 * Returns
 * - The parsed json, errors contained inside
 */
HgJson hgParseJson(HgArena* arena, HgStringView text);

/**
 * Hash map hashing for u8
 */
constexpr u64 hgHash(u8 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u16
 */
constexpr u64 hgHash(u16 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u32
 */
constexpr u64 hgHash(u32 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u64
 */
constexpr u64 hgHash(u64 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i8
 */
constexpr u64 hgHash(i8 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i16
 */
constexpr u64 hgHash(i16 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i32
 */
constexpr u64 hgHash(i32 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i64
 */
constexpr u64 hgHash(i64 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for f32
 */
constexpr u64 hgHash(f32 val)
{
    union
    {
        f32 asFloat;
        u64 asHash;
    } u{};
    u.asFloat = val;
    return u.asHash;
}

/**
 * Hash map hashing for f64
 */
constexpr u64 hgHash(f64 val)
{
    union
    {
        f64 asFloat;
        u64 asHash;
    } u{};
    u.asFloat = val;
    return u.asHash;
}

/**
 * Hash map hashing for void*
 */
constexpr u64 hgHash(void* val)
{
    union
    {
        void* asPtr;
        uptr asUptr;
    } u{};
    u.asPtr = val;
    return (u64)u.asUptr;
}

/**
 * Hash map hashing for strings
 */
constexpr u64 hgHash(HgStringView str)
{
    u64 ret = 0;
    u64 mult = 1;
    for (char c : str)
    {
        ret += (u64)c * mult;
        mult *= 257;
    }
    return ret;
}

/**
 * Hash map hashing for HgString
 */
constexpr u64 hgHash(HgString str)
{
    return hgHash(HgStringView{str});
}

/**
 * Hash map hashing for C string
 */
constexpr u64 hgHash(const char* str)
{
    return hgHash(HgStringView{str});
}

/**
 * A hash set
 */
template<typename Value, u64 (*hashFn)(Value) = hgHash>
struct HgHashSet
{
    static_assert(std::is_trivially_copyable_v<Value> && std::is_trivially_destructible_v<Value>);

    /**
     * Whether each index has a value
     */
    bool* hasVal;
    /**
     * Where the values are stored;
     */
    Value* vals;
    /**
     * The max number of vals
     */
    u32 capacity;
    /**
     * The current number of values that are stored
     */
    u32 count;

    /**
     * Creates a new hash set
     *
     * Parameters
     * - arena The arena to allocate from
     * - slotCount The max number of slots to store values in
     */
    static HgHashSet create(HgArena* arena, u32 slotCount)
    {
        hgAssert(slotCount > 0);

        HgHashSet set;
        set.hasVal = hgAlloc<bool>(arena, slotCount);
        set.vals = hgAlloc<Value>(arena, slotCount);
        set.capacity = slotCount;
        set.reset();
        return set;
    }

    /**
     * Resizes the slots and rehashes all entries
     *
     * Parameters
     * - arena The arena to allocate from
     * - newSize The new number of slots
     */
    void resize(HgArena* arena, u32 newSize)
    {
        HgHashSet old = *this;
        *this = create(arena, newSize);

        for (u32 i = 0; i < old.capacity; ++i)
        {
            if (old.hasVal[i])
                add(old.vals[i]);
        }
    }

    /**
     * Empties all slots
     */
    void reset()
    {
        for (u32 i = 0; i < capacity; ++i)
        {
            hasVal[i] = false;
        }
        count = 0;
    }

    /**
     * Add a value to the set
     */
    void add(Value val)
    {
        hgAssert(count < capacity - 1);

        u32 idx = hashFn(val) % capacity;
        u32 dist = 0;
        while (hasVal[idx] && vals[idx] != val)
        {
            u32 otherDist = hashFn(vals[idx]) % capacity;
            if (otherDist < idx)
                otherDist += capacity;
            otherDist -= idx;

            if (otherDist < dist)
            {
                Value valTmp = vals[idx];
                vals[idx] = val;
                val = valTmp;

                dist = otherDist;
            }

            idx = (idx + 1) % capacity;
            ++dist;
        }

        hasVal[idx] = true;
        vals[idx] = val;
        ++count;
    }

    /**
     * Remove a value from the set
     */
    void remove(const Value& val)
    {
        u32 idx = hashFn(val) % capacity;
        while (hasVal[idx])
        {
            if (vals[idx] == val)
                break;
            idx = (idx + 1) % capacity;
        }
        if (!hasVal[idx])
            return;

        u32 next = (idx + 1) % capacity;
        while (hasVal[next])
        {
            if (hashFn(vals[next]) != idx)
            {
                vals[idx] = vals[next];
                idx = next;
            }
            next = (next + 1)  % capacity;
        }
        hasVal[idx] = false;
        --count;
    }

    /**
     * Checks whether a value is contained in the set
     */
    bool has(const Value& val) const
    {
        for (u32 idx = hashFn(val) % capacity; hasVal[idx]; idx = (idx + 1) % capacity)
        {
            if (vals[idx] == val)
                return true;
        }
        return false;
    }

    /**
     * Execute a function for every value
     *
     * Parameters
     * - fn The function to execute
     */
    template<typename Fn>
    void forEach(Fn fn)
    {
        static_assert(std::is_invocable_r_v<void, Fn, const Value&>);
        for (u32 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                fn(vals[i]);
        }
    }
};

/**
 * A key-value hash map
 */
template<typename Key, typename Value, u64 (*hashFn)(Key) = hgHash>
struct HgHashMap
{
    static_assert(std::is_trivially_copyable_v<Key>
               && std::is_trivially_copyable_v<Value>
               && std::is_trivially_destructible_v<Key>
               && std::is_trivially_destructible_v<Value>);

    /**
     * Whether each index has a value
     */
    bool* hasVal;
    /**
     * Where the keys are stored;
     */
    Key* keys;
    /**
     * Where the values are stored
     */
    Value* vals;
    /**
     * The max number of key value pairs
     */
    u32 capacity;
    /**
     * The current number of values that are stored
     */
    u32 count;

    /**
     * Creates a new hash map
     *
     * Parameters
     * - arena The arena to allocate from
     * - slotCount The max number of slots to store values in
     */
    static HgHashMap create(HgArena* arena, u32 slotCount)
    {
        hgAssert(slotCount > 0);

        HgHashMap map;
        map.hasVal = hgAlloc<bool>(arena, slotCount);
        map.keys = hgAlloc<Key>(arena, slotCount);
        map.vals = hgAlloc<Value>(arena, slotCount);
        map.capacity = slotCount;
        map.reset();
        return map;
    }

    /**
     * Resizes the slots and rehashes all entries
     *
     * Parameters
     * - arena The arena to allocate from
     * - newSize The new number of slots
     */
    void resize(HgArena* arena, u32 newSize)
    {
        HgHashMap old = *this;
        *this = create(arena, newSize);

        for (u32 i = 0; i < old.capacity; ++i)
        {
            if (old.hasVal[i])
                add(old.keys[i], old.vals[i]);
        }
    }

    /**
     * Empties all slots
     */
    void reset()
    {
        for (u32 i = 0; i < capacity; ++i)
        {
            hasVal[i] = false;
        }
        count = 0;
    }

    /**
     * Add a key-value pair to the hash map
     *
     * Parameters
     * - key The key to add
     *
     * Returns
     * - A reference to the added value
     */
    Value& add(Key key)
    {
        hgAssert(count < capacity - 1);

        Value val{};

        u32 idx = hashFn(key) % capacity;
        u32 dist = 0;
        while (hasVal[idx] && keys[idx] != key)
        {
            u32 otherDist = hashFn(keys[idx]) % capacity;
            if (otherDist < idx)
                otherDist += capacity;
            otherDist -= idx;

            if (otherDist < dist)
            {
                Key keyTmp = keys[idx];
                Value valTmp = vals[idx];
                keys[idx] = key;
                vals[idx] = val;
                key = keyTmp;
                val = valTmp;

                dist = otherDist;
            }

            idx = (idx + 1) % capacity;
            ++dist;
        }

        hasVal[idx] = true;
        keys[idx] = key;
        vals[idx] = val;
        ++count;

        return vals[idx];
    }

    /**
     * Remove a key-value pair from the hash map, and stores it
     *
     * Parameters
     * - key The key to remove 
     * - value A pointer to store the value, if found
     *
     * Returns
     * - Whether a value was found and stored in value
     */
    bool remove(const Key& key, Value* value = nullptr)
    {
        u32 idx = hashFn(key) % capacity;
        while (hasVal[idx])
        {
            if (keys[idx] == key)
                break;
            idx = (idx + 1) % capacity;
        }
        if (!hasVal[idx])
            return false;

        if (value != nullptr)
            *value = vals[idx];

        u32 next = (idx + 1) % capacity;
        while (hasVal[next])
        {
            if (hashFn(keys[next]) != idx)
            {
                keys[idx] = keys[next];
                vals[idx] = vals[next];
                idx = next;
            }
            next = (next + 1)  % capacity;
        }

        hasVal[idx] = false;
        --count;

        return true;
    }

    /**
     * Checks whether a key-value pair exists
     */
    bool has(const Key& key) const
    {
        for (u32 idx = hashFn(key) % capacity; hasVal[idx]; idx = (idx + 1) % capacity)
        {
            if (keys[idx] == key)
                return true;
        }
        return false;
    }

    /**
     * Get a reference to the value stored at a key
     *
     * Parameters
     * - The key to lookup, must be in the hash map
     */
    Value& operator[](const Key& key)
    {
        for (u32 idx = hashFn(key) % capacity; hasVal[idx]; idx = (idx + 1) % capacity)
        {
            if (keys[idx] == key)
                return vals[idx];
        }
        hgAssert(false);
    }

    /**
     * Get a reference to the value stored at a key, in a const context
     *
     * Parameters
     * - The key to lookup, must be in the hash map
     */
    const Value& operator[](const Key& key) const
    {
        for (u32 idx = hashFn(key) % capacity; hasVal[idx]; idx = (idx + 1) % capacity)
        {
            if (keys[idx] == key)
                return vals[idx];
        }
        hgAssert(false);
    }

    /**
     * Execute a function for every key-value pair
     *
     * Parameters
     * - fn The function to execute
     */
    template<typename Fn>
    void forEach(Fn fn)
    {
        static_assert(std::is_invocable_r_v<void, Fn, const Key&, Value&>);
        for (u32 i = 0; i < capacity; ++i)
        {
            if (hasVal[i])
                fn(keys[i], vals[i]);
        }
    }
};

/**
 * A high precision clock for timers and game deltas
 */
struct HgClock
{
    /**
     * The begin time
     */
    std::chrono::time_point<std::chrono::high_resolution_clock> time = std::chrono::high_resolution_clock::now();

    /**
     * Resets the clock
     *
     * Returns
     * - The time in seconds since the last tick
     */
    f64 tick();
};

/**
 * Returns the number of concurrent threads available in hardware
 */
u32 hgHardwareThreadCount();

/**
 * A spinlock fence for basic thread synchronization
 */
struct HgFence
{
    /**
     * The number of events the fence is waiting on
     */
    std::atomic<u32> counter{0};

    /**
     * Add more events for the fence to wait on
     *
     * Parameters
     * - count The number of added events
     */
    void add(u32 count);

    /**
     * Signal that events have completed
     *
     * Parameters
     * - count The number of signaled events
     */
    void signal(u32 count);

    /**
     * Returns whether all work has been completed
     */
    bool isComplete();

    /**
     * Spin waits for all work submissions to be completed
     */
    void waitIndefinite();

    /**
     * Spin waits for all work submissions to be completed
     *
     * Parameters
     * - timeout The time in seconds to wait before timing out
     */
    bool wait(f64 timeout);
};

/**
 * Initialize the thread pool
 *
 * Note, the recommended thread is the hardware thread count minus dedicated
 * threads such as main thread, IO thread, etc.
 *
 * Parameters
 * - arena The arena to allocate from
 * - queueSize The max capacity of the thread work queue
 * - threadCount The number of threads to spawn in the pool
 */
void hgInitThreadPool(HgArena* arena, u32 queueSize, u32 threadCount);

/**
 * Deinitialize the thread pool
 */
void hgDeinitThreadPool();

/**
 * Wait on a fence, and help complete work in the meantime
 *
 * Parameters
 * - fence The fence to wait on
 * - timeout The max time in seconds to spend working
 *
 * Returns
 * - true if the fence was completed
 * - false if the timeout was reached
 */
bool hgHelpThreadPool(HgFence& fence, f64 timeout);

/**
 * Pushes work to the thread pool queue to be executed
 *
 * Parameters
 * - fences The fences to signal upon completion
 * - fenceCount The number of fences
 * - data The data passed to the function
 * - work The function to be executed
 */
void hgCallPar(HgFence* fences, u32 fenceCount, void* data, void (*fn)(void*));

/**
 * Iterates in parallel over a function n times using the thread pool
 *
 * Note, uses a fence internally to wait for all work to complete
 *
 * Parameters
 * - n The number of elements to iterate [0, count)
 * - chunkSize The number of elements to iterate per chunk
 * - fn The function to use to iterate, takes begin and end indicces
 */
template<typename Fn>
void hgForPar(u64 n, u64 chunkSize, Fn fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, u64, u64>);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    HgFence fence;
    for (u64 i = 0; i < n; i += chunkSize)
    {
        struct Capture
        {
            u64 begin;
            u64 end;
            Fn* fn;
        };

        Capture* data = hgAlloc<Capture>(scratch, 1);
        data->begin = i;
        data->end = std::min(i + chunkSize, n);
        data->fn = &fn;

        hgCallPar(&fence, 1, data, [](void* pdata)
        {
            Capture* data = (Capture*)pdata;
            (*data->fn)(data->begin, data->end);
        });
    }
    hgHelpThreadPool(fence, INFINITY);
}

/**
 * Initializes the IO thread
 *
 * Parameters
 * - arena The arena to allocate from
 * - queueSize The max concurrent request capacity
 */
void hgInitIOThread(HgArena* arena, u32 queueSize);

/**
 * Deinitializes the IO thread
 */
void hgDeinitIOThread();

/**
 * Make an asynchronous IO request on the IO thread
 *
 * Parameters
 * - fences The fences to signal on completion
 * - fenceCount The number of fences
 * - resource The resource pointer passed to fn
 * - path The path string passed to fn
 * - fn The function to execute
 */
void hgRequestIO(
    HgFence* fences,
    u32 fenceCount,
    void* resource,
    HgStringView path,
    void (*fn)(void* resource, HgStringView path));

/**
 * The uuid derived from the resource's name/path
 */
using HgResource = u64;

/**
 * Get the resource uuid from the name/path
 */
constexpr HgResource hgResourceID(HgStringView name)
{
    return hgHash(name);
}

/**
 * A resource manager
 */
struct HgResourceManager
{
    /**
     * The reference count for each resource
     */
    u32* refCounts;
    /**
     * Where the resource ids are stored;
     */
    HgResource* ids;
    /**
     * Where the resources are stored;
     */
    void* resources;
    /**
     * The size of each resource in bytes
     */
    u32 width;
    /**
     * The current number of resources that are stored
     */
    u32 count;
    /**
     * The max number of resources
     */
    u32 capacity;

    /**
     * Creates a new resource manager
     *
     * Parameters
     * - resourceWidth The size in bytes of each resource
     * - capacity The initial number of slots
     */
    static HgResourceManager create(u32 resourceWidth, u32 capacity = 128);

    /**
     * Free the resource manager's memory
     */
    void destroy();

    /**
     * Resizes the slots and rehashes all entries
     *
     * Parameters
     * - arena The arena to allocate from
     * - newSize The new number of slots
     */
    void resize(u32 newSize);

    /**
     * Empties all slots
     */
    void reset();

    /**
     * Add a resource to the resource manager
     *
     * Returns
     * - The new index of the resource
     */
    u32 add(HgResource id);

    /**
     * Remove a resource from the resource manager
     */
    void remove(HgResource id);

    /**
     * Increment the resource's reference count
     *
     * Note, automatically adds the resource if needed
     *
     * Returns
     * - Whether the reference count was 0, and needs to be loaded
     */
    bool load(HgResource id);

    /**
     * Decrement the resource's reference count
     *
     * Parameters
     * - id The resource to unload
     * - resource A pointer to store the resource if it needs to be unloaded
     *
     * Returns
     * - Whether the reference count dropped to 0, and needs to be unloaded
     */
    bool unload(HgResource id, void* resource);

    /**
     * Get a pointer to a resource
     *
     * Returns
     * - A pointer to the resource, or nullptr if it is not loaded
     */
    void* get(HgResource id);

    /**
     * Execute a function for every key-value pair
     *
     * Parameters
     * - fn The function to execute
     */
    template<typename Fn>
    void forEach(Fn fn)
    {
        static_assert(std::is_invocable_r_v<void, Fn, HgResource, void*>);
        for (u32 i = 0; i < capacity; ++i)
        {
            if (refCounts[i] != (u32)-1 && refCounts[i] > 0)
                fn(ids[i], (u8*)resources + i * width);
        }
    }
};

/**
 * A loaded binary file
 */
struct HgBinary
{
    /**
     * The data in the file
     */
    void* data;
    /**
     * The size of the file in bytes
     */
    u64 size;

    /**
     * Resize the file
     *
     * Parameters
     * - arena The arena to allocate from
     * - newSize The new size of the file in bytes
     */
    void resize(HgArena* arena, u64 newSize)
    {
        data = hgRealloc(arena, data, size, newSize, 1);
        size = newSize;
    }

    /**
     * Read data at index into a buffer
     *
     * Parameters
     * - idx The index into the file in bytes to read from
     * - dst A pointer to store the read data
     * - size The size in bytes to read
     */
    void read(u64 idx, void* dst, u64 len) const
    {
        hgAssert(idx + len <= size);
        memcpy(dst, (u8*)data + idx, len);
    }

    /**
     * Read data of arbitrary type from the file
     *
     * Parameters
     * - idx The index into the file in bytes to read from
     */
    template<typename T>
    T read(u64 idx) const
    {
        T ret;
        read(idx, &ret, sizeof(T));
        return ret;
    }

    /**
     * Overwrite data at the index
     *
     * Parameters
     * - idx The index into the file to overwrite
     * - src The data to write
     * - size The size of the data in bytes
     */
    void overwrite(u64 idx, const void* src, u64 len)
    {
        hgAssert(idx + len <= size);
        memcpy((u8*)data + idx, src, len);
    }

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
};

/**
 * Load a binary file from disc
 *
 * Note, this is blocking
 *
 * Parameters
 * - arena The arena to allocate from
 * - path The file path to the image
 *
 * Returns
 * - The loaded binary
 */
HgBinary hgLoadBinary(HgArena* arena, HgStringView path);

/**
 * Store a binary file to disc
 *
 * Note, this is blocking
 *
 * Parameters
 * - bin The binary to store
 * - path The file path to store at
 */
void hgStoreBinary(HgBinary bin, HgStringView path);

/**
 * Initialize the resource manager
 */
void hgInitResources();

/**
 * Deinitialize the resource manager
 */
void hgDeinitResource();

/**
 * Loads an empty resource (or just increments the reference count)
 *
 * Parameters
 * - id The resource to load
 */
void hgLoadEmptyResource(HgResource id);

/**
 * Loads a resource (or just increments the reference count)
 *
 * Parameters
 * - fence The fences to signal on completion
 * - fenceCount The number of fences
 * - id The resource to load into
 * - path The filepath to load from
 */
void hgLoadResource(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path);

/**
 * Unloads a resource (or just decrements the reference count)
 *
 * Parameters
 * - fence The fences to signal on completion
 * - fenceCount The number of fences
 * - id The resource to load into
 */
void hgUnloadResource(HgFence* fences, u32 fenceCount, HgResource id);

/**
 * Stores a resource to disc
 *
 * Parameters
 * - fence The fences to signal on completion
 * - fenceCount The number of fences
 * - id The resource to load into
 * - path The filepath to store to
 */
void hgStoreResource(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path);

/**
 * Get a resource from the global store
 *
 * Parameters
 * - id The resource to get
 *
 * Returns
 * - A pointer to the resource binary
 * - nullptr, if the resource is not loaded
 */
HgBinary* hgGetResource(HgResource id);

/**
 * An image resource
 */
struct HgImageData
{
    /**
     * The identifier prepended to the info
     */
    static constexpr char imageIdentifier[] = "HGIMAGE";

    /**
     * The info prepended to an image resource
     */
    struct Info
    {
        /**
         * The identifier to ensure the file is a Hurdy Gurdy image
         */
        char identifier[sizeof(imageIdentifier)];
        /**
         * The format of each pixel
         */
        VkFormat format;
        /**
         * The width in pixels
         */
        u32 width;
        /**
         * The height in pixels
         */
        u32 height;
        /**
         * The depth in pixels
         */
        u32 depth;
        /**
         * The index of the pixels
         */
        u64 pixelsBegin;
    };

    /**
     * The image data
     */
    HgBinary bin;

    /**
     * Construct uninitialized
     */
    HgImageData() = default;

    /**
     * Implicit conversion from binary file
     */
    HgImageData(HgBinary fileVal) : bin(fileVal) {}

    /**
     * Get the image info from the file
     *
     * Parameters
     * - format Where to store the format
     * - width Where to store the width
     * - height Where to store the height
     * - depth Where to store the depth
     *
     * Returns
     * - Whether the info could be found
     */
    bool getInfo(VkFormat* format, u32* width, u32* height, u32* depth);

    /**
     * Returns a pointer to the pixels, never nullptr
     */
    void* getPixels();
};

/**
 * Load an external image file into a resource in the Hurdy Gurdy format
 *
 * Parameters
 * - fences The fences to wait on
 * - fenceCount The number of fences
 * - id The resource to load into
 * - path The path of the file to import
 */
void hgImportPng(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path);

/**
 * Store a image resource onto disc in an external file format
 *
 * Parameters
 * - fences The fences to wait on
 * - fenceCount The number of fences
 * - id The resource to export
 * - path The path of the file to export to
 */
void hgExportPng(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path);

/**
 * A vertex in a model
 */
struct HgModelVertex
{
    /**
     * The vertex position
     */
    HgVec3 pos;
    /**
     * The vertex normal
     */
    HgVec3 norm;
    /**
     * The vertex tangent
     */
    HgVec4 tan;
    /**
     * The vertex uv coordinate
     */
    HgVec2 uv;
};

/**
 * A 3d model resource
 */
struct HgModelData
{
    /**
     * The identifier prepended to the info
     */
    static constexpr char modelIdentifier[] = "HGMODEL";

    /**
     * The info prendeded to a model resources
     */
    struct Info
    {
        /**
         * The identifier to ensure the file is a Hurdy Gurdy model
         */
        char identifier[sizeof(modelIdentifier)];
        /**
         * The number of vertices
         */
        u32 vertexCount;
        /**
         * The size of each vertex in bytes
         */
        u32 vertexWidth;
        /**
         * The number of indices (4 bytes each)
         */
        u32 indexCount;
        /**
         * How the vertices should be interpreted in sequence
         */
        VkPrimitiveTopology topology;
        /**
         * The file index of the first vertex
         */
        u64 verticesBegin;
        /**
         * The file index of the first geometry index
         */
        u64 indicesBegin;
    };

    /**
     * The model data
     */
    HgBinary file;

    /**
     * Construct uninitialized
     */
    HgModelData() = default;

    /**
     * Implicit conversion from binary file
     */
    HgModelData(HgBinary fileVal) : file{fileVal} {}

    /**
     * Get the model info from the file
     *
     * Parameters
     * - vertexCount How many vertices are in the model
     * - vertexWidth The size of each vertex in bytes
     * - indexCount How many indices are in the model
     * - topology How the vertices should be interpreted in sequence
     *
     * Returns
     * - Whether the info could be found
     */
    bool getInfo(u32* vertexCount, u32* vertexWidth, u32* indexCount, VkPrimitiveTopology* topology);

    /**
     * Returns a pointer to the vertices, never nullptr
     */
    void* getVertexData();

    /**
     * Returns a pointer to the vertices, or nullptr if it could not be found
     */
    void* getIndexData();
};

/**
 * Load an external model file into a resource in the Hurdy Gurdy format : TODO
 *
 * Parameters
 * - fences The fences to wait on
 * - fenceCount The number of fences
 * - id The resource to load to
 * - path The path of the file to import
 */
void hgImportGltf(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path);

/**
 * Store a model resource onto disc in an external file format : TODO
 *
 * Parameters
 * - fences The fences to wait on
 * - fenceCount The number of fences
 * - id The resource to export
 * - path The path of the file to export to
 */
void hgExportGltf(HgFence* fences, u32 fenceCount, HgResource id, HgStringView path);

/**
 * Initialize all gpu resource managers
 *
 * Resources
 * - HgGpuTexture
 * - HgGpuModel
 */
void hgInitGpuResources();

/**
 * Deinitialize all gpu resource managers
 */
void hgDeinitGpuResources();

/**
 * A texture stored on the GPU
 */
struct HgTextureResource
{
    /**
     * The allocation
     */
    VmaAllocation allocation;
    /**
     * The image
     */
    VkImage image;
    /**
     * The image view
     */
    VkImageView view;
    /**
     * The sampler
     */
    VkSampler sampler;
    /**
     * The format of each pixel
     */
    VkFormat format;
    /**
     * The width in pixels
     */
    u32 width;
    /**
     * The height in pixels
     */
    u32 height;
    /**
     * The depth in pixels
     */
    u32 depth;
};

/**
 * Initialize gpu textures
 */
void HgInitTextures();

/**
 * Deinitialize gpu textures
 */
void hgDeinitTextures();

/**
 * Load a texture from the cpu to the gpu
 *
 * Note, if the texture is not already loaded to the gpu, it must be available
 * on the cpu to load from
 *
 * Parameters
 * - id The resource to load
 * - sampler The sampler the texture should use
 */
void hgLoadTexture(HgResource id, VkSampler sampler);

/**
 * Unload a texture from the gpu
 */
void hgUnloadTexture(HgResource id);

/**
 * Gets a gpu texture resource
 *
 * Returns
 * - The gpu texture, or nullptr if it is not loaded
 */
HgTextureResource* hgGetTexture(HgResource id);

/**
 * A 3D model stored on the gpu
 */
struct HgModelResource
{
    /**
     * The allocation
     */
    VmaAllocation vertexAlloc;
    /**
     * The buffer
     */
    VkBuffer vertexBuffer;
    /**
     * The allocation
     */
    VmaAllocation indexAlloc;
    /**
     * The buffer
     */
    VkBuffer indexBuffer;
    /**
     * The number of vertices
     */
    u32 vertexCount;
    /**
     * The size of each vertex in bytes
     */
    u32 vertexWidth;
    /**
     * The number of indices (4 bytes each)
     */
    u32 indexCount;
};

/**
 * Initialize gpu models
 */
void hgInitModels();

/**
 * Deinitialize gpu models
 */
void hgDeinitModels();

/**
 * Load a model from the cpu to the gpu
 *
 * Note, if the model is not already loaded to the gpu, it must be available
 * on the cpu to load from
 */
void hgLoadModel(HgResource id);

/**
 * Unload a model from the gpu
 */
void hgUnloadModel(HgResource id);

/**
 * Gets a gpu model resource
 *
 * Returns
 * - The gpu model, or nullptr if it is not loaded
 */
HgModelResource* hgGetModel(HgResource id);

/**
 * Creates a new id for a component
 *
 * Note, should only be used by hgComponentID
 *
 * Parameters
 * - width The size in bytes of the component type
 */
u32 hgCreateComponentID(u32 width);

/**
 * The unique component id for a type
 */
template<typename T>
inline u32 hgComponentID = hgCreateComponentID(sizeof(T));

/**
 * An entity in the ecs
 */
struct HgEntity
{
    /**
     * The entity id, defaults to null
     */
    u32 id = (u32)-1;

    /**
     * Get the index from the id
     */
    u32 idx()
    {
        return id & 0x00ffffff;
    }

    /**
     * Get the generation from the id
     */
    u32 generation()
    {
        return (id & 0xff000000) >> 24;
    }

    /**
     * Increment this entity's generation count
     */
    void incrementGeneration()
    {
        id += 1 << 24;
    }
};

/**
 * Compare entities
 */
constexpr bool operator==(HgEntity lhs, HgEntity rhs)
{
    return lhs.id == rhs.id;
}

/**
 * Compare entities
 */
constexpr bool operator!=(HgEntity lhs, HgEntity rhs)
{
    return lhs.id != rhs.id;
}

/**
 * An entity component system
 */
struct HgECS
{
    /**
     * A system of components
     */
    struct System
    {
        /**
         * The component lookup from entity index
         */
        u32* indices;
        /**
         * The entity lookup from component index
         */
        HgEntity* entities;
        /**
         * The component data
         */
        void* components;
        /**
         * The number of components
         */
        u32 count;
        /**
         * The capacity of components
         */
        u32 capacity;
    };

    /**
     * The entities, ready to be spawned
     */
    HgEntity* pool;
    /**
     * The next entity to spawn
     */
    HgEntity next;
    /**
     * The capacity of the entity pool
     */
    u32 poolSize;
    /**
     * The component systems
     */
    System* systems;
    /**
     * The number of component systems
     */
    u32 systemCount;

    /**
     * Create a new entity component system
     *
     * Parameters
     * - maxEntities The maximum number of entities which can be spawned
     */
    static HgECS create(u32 maxEntities);

    /**
     * Destroy the entity component system
     */
    void destroy();

    /**
     * Reset the entity component system, despawning all entities
     */
    void reset();

    /**
     * Return a newly spawned entity
     */
    HgEntity spawn();

    /**
     * Despawn an entity
     *
     * Note, this function will invalidate iterators
     */
    void despawn(HgEntity e);

    /**
     * Return whether an entity is alive
     */
    bool alive(HgEntity e);

    /**
     * Add a component to an entity
     *
     * Note, the entity must not have a component of this type already
     *
     * Returns
     * - A pointer to the created component
     */
    void* add(HgEntity e, u32 componentID);

    /**
     * Add a component to an entity
     *
     * Note, the entity must not have a component of this type already
     *
     * Returns
     * - A reference to the created component
     */
    template<typename T>
    T& add(HgEntity e)
    {
        return *(T*)add(e, hgComponentID<T>);
    }

    /**
     * Remove a component from an entity
     *
     * Note, this function will invalidate iterators
     */
    void remove(HgEntity e, u32 componentID);

    /**
     * Remove a component from an entity
     *
     * Note, this function will invalidate iterators
     */
    template<typename T>
    void remove(HgEntity e)
    {
        remove(e, hgComponentID<T>);
    }

    /**
     * Check whether an entity has a component or not
     */
    bool has(HgEntity e, u32 componentID);

    /**
     * Check whether an entity has a component or not
     */
    template<typename T>
    bool has(HgEntity e)
    {
        return has(e, hgComponentID<T>);
    }

    /**
     * Return whether the entity has all given components
     */
    template<typename... Ts>
    bool hasAll(HgEntity e)
    {
        return (has<Ts>(e) && ...);
    }

    /**
     * Return whether the entity has any of the given components
     */
    template<typename... Ts>
    bool hasAny(HgEntity e)
    {
        return (has<Ts>(e) || ...);
    }

    /**
     * Get a pointer to the entity's component
     *
     * Note, the entity must have the component
     *
     * Returns
     * - A pointer to the entity's component, will never be nullptr
     */
    void* get(HgEntity e, u32 componentID);

    /**
     * Get a reference to the entity's component
     *
     * Note, the entity must have the component
     *
     * Returns
     * - A reference to the entity's component
     */
    template<typename T>
    T& get(HgEntity e)
    {
        return *(T*)get(e, hgComponentID<T>);
    }

    /**
     * Get the entity from it's component
     *
     * Parameters
     * - c The component to lookup, must be a valid component
     * - componentID The id of the component
     */
    HgEntity get(const void* c, u32 componentID);

    /**
     * Get the entity from it's component
     *
     * Parameters
     * - c The component to lookup, must be a valid component
     */
    template<typename T>
    HgEntity get(const T* c)
    {
        return get((void*)c, hgComponentID<T>);
    }

    /**
     * Return a pointer to all entities of type
     */
    template<typename T>
    HgEntity* entities()
    {
        return (HgEntity*)systems[hgComponentID<T>].entities;
    }

    /**
     * Return a pointer to all components of type
     */
    template<typename T>
    T* components()
    {
        return (T*)systems[hgComponentID<T>].components;
    }

    /**
     * Return the number of active components of a type
     */
    template<typename T>
    u32 count()
    {
        return systems[hgComponentID<T>].count;
    }

    /**
     * Find the id of the system with the fewest elements
     */
    u32 findSmallest(u32* ids, u32 idCount);

    /**
     * Find the id of the system with the fewest elements
     */
    template<typename... Ts>
    u32 findSmallest()
    {
        u32 index = 0;
        u32 ids[sizeof...(Ts)];
        ((ids[index++] = hgComponentID<Ts>), ...);
        return findSmallest(ids, sizeof...(Ts));
    }

    /**
     * Iterate over all entities with the single given component
     *
     * Note, specifying only one component allows deterministic ordering (such
     * as in the case of sorting), as well as extra optimization
     *
     * The function receives as parameters:
     * - The entity id
     * - A reference to the component
     *
     * Parameters
     * - function The function to call
     */
    template<typename T, typename Fn>
    void forEachSingle(Fn& fn)
    {
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity, T&>);

        HgEntity* e = entities<T>();
        HgEntity* end = e + count<T>();
        T* c = components<T>();
        for (; e != end; ++e, ++c)
        {
            fn(*e, *c);
        }
    }

    /**
     * Iterate over all entities with the given components
     *
     * The function receives as parameters:
     * - The entity id
     * - A reference to each component...
     *
     * Parameters
     * - function The function to call
     */
    template<typename... Ts, typename Fn>
    void forEachMulti(Fn& fn)
    {
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity, Ts&...>);

        u32 id = findSmallest<Ts...>();
        HgEntity* e = systems[id].entities;
        HgEntity* end = e + systems[id].count;
        for (; e != end; ++e)
        {
            if (hasAll<Ts...>(*e))
                fn(*e, get<Ts>(*e)...);
        }
    }

    /**
     * Iterate over all entities with the given components
     *
     * Note, calls the single or multi version from the number of components
     *
     * Parameters
     * - function The function to call
     */
    template<typename... Ts, typename Fn>
    void forEach(Fn fn)
    {
        static_assert(sizeof...(Ts) != 0);

        if constexpr (sizeof...(Ts) == 1)
        {
            forEachSingle<Ts...>(fn);
        } else {
            forEachMulti<Ts...>(fn);
        }
    }

    /**
     * Iterate over all entities with the single given component
     *
     * Note, specifying only one component allows deterministic ordering (such
     * as in the case of sorting), as well as extra optimization
     *
     * The function receives as parameters:
     * - The entity id
     * - A reference to the component
     *
     * Parameters
     * - chunkSize The number of executions per group
     * - fn The function to call
     */
    template<typename T, typename Fn>
    void forParSingle(u64 chunkSize, Fn& fn)
    {
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity, T&>);

        hgForPar(count<T>(), chunkSize, [&](u64 begin, u64 end)
        {
            HgEntity* e = entities<T>() + begin;
            HgEntity* eEnd = entities<T>() + end;
            T* c = components<T>() + begin;
            for (; e != eEnd; ++e, ++c)
            {
                fn(*e, *c);
            }
        });
    }

    /**
     * Iterate over all entities with the given components
     *
     * The function receives as parameters:
     * - The entity id
     * - A reference to each component...
     *
     * Parameters
     * - chunkSize The number of executions per group
     * - fn The function to call
     */
    template<typename... Ts, typename Fn>
    void forParMulti(u64 chunkSize, Fn& fn)
    {
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity, Ts&...>);

        u32 id = findSmallest<Ts...>();
        hgForPar(systems[id].count, chunkSize, [&](u64 begin, u64 end)
        {
            HgEntity* e = systems[id].entities + begin;
            HgEntity* eEnd = systems[id].entities + end;
            for (; e != eEnd; ++e)
            {
                if (hasAll<Ts...>(*e))
                    fn(*e, get<Ts>(*e)...);
            }
        });
    }

    /**
     * Iterate over all entities with the given components
     *
     * Note, calls the single of multi version from the number of components
     *
     * The function receives as parameters:
     * - The entity id
     * - A reference to each component...
     *
     * Parameters
     * - chunkSize The number of executions per group
     * - fn The function to call
     */
    template<typename... Ts, typename Fn>
    void forPar(u64 chunkSize, Fn fn)
    {
        static_assert(sizeof...(Ts) != 0);

        if constexpr (sizeof...(Ts) == 1)
        {
            forParSingle<Ts...>(chunkSize, fn);
        } else {
            forParMulti<Ts...>(chunkSize, fn);
        }
    }

    /**
     * Sort components
     *
     * Parameters
     * - componentID The component system to sort
     * - data The data passed to compare
     * - compare The comparison function
     */
    void sort(u32 componentID, void* data, bool (*compare)(void*, HgECS* ecs, HgEntity lhs, HgEntity rhs));

    /**
     * Sort components
     *
     * Parameters
     * - data The data passed to compare
     * - compare The comparison function
     */
    template<typename T>
    void sort(void* data, bool (*compare)(void*, HgECS* ecs, HgEntity lhs, HgEntity rhs))
    {
        sort(hgComponentID<T>, data, compare);
    }
};

/**
 * A node component for entities in a hierarchy
 */
struct HgHierarchy
{
    /**
     * The entity's parent, if any
     */
    HgEntity parent{};
    /**
     * The next child of this entity's parent
     */
    HgEntity nextSibling{};
    /**
     * The previous child of this entity's parent
     */
    HgEntity prevSibling{};
    /**
     * The first of this entity's children, forming a linked list
     */
    HgEntity firstChild{};
};

/**
 * Add a new child to an entity in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - parent The parent to add to, must be alive
 * - child The child to add, must be alive
 */
void hgAddChildEntity(HgECS* ecs, HgEntity parent, HgEntity child);

/**
 * Remove the entity from its hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to detach, must be alive
 */
void hgDetachEntity(HgECS* ecs, HgEntity e);

/**
 * Destroy the entity and all its children in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to destroy to, must be alive
 */
void hgDestroyEntity(HgECS* ecs, HgEntity e);

/**
 * The transform component for entities
 */
struct HgTransform
{
    /**
     * The entity's position in the world
     * - x: -left, +right
     * - y: -up, +down
     * - z: -backward, +forward
     */
    HgVec3 position{0.0f, 0.0f, 0.0f};
    /**
     * The entity's scaling
     * - x: horizonatal
     * - y: vertical
     * - z: depth
     */
    HgVec3 scale{1.0f, 1.0f, 1.0f};
    /**
     * The entity's rotation in the world
     */
    HgQuat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

/**
 * Set this transform and move all children by accordingly : TODO
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to set, must be alive
 * - pos The new position
 * - scale The new scale
 * - rot The new rotation
 */
void hgSetEntity(HgECS* ecs, HgEntity e, const HgVec3& pos, const HgVec3& scale, const HgQuat& rot);

/**
 * Move this transform and all children by a delta
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to move, must be alive
 * - dpos The change in position, added to current position
 * - dscale The change in scale, multiplied to current scale
 * - drot The change in rotation, applied to current rotation
 */
void hgMoveEntity(HgECS* ecs, HgEntity e, const HgVec3& dpos, const HgVec3& dscale, const HgQuat& drot);

/**
 * A sprite component rendered by the 2d pipeline
 */
struct HgSprite2D
{
    /**
     * The texture to draw from
     */
    HgResource texture;
    /**
     * The beginning coordinate to read from texture, [0.0, 1.0]
     */
    HgVec2 uvPos;
    /**
     * The size of the region to read from texture, [0.0, 1.0]
     */
    HgVec2 uvSize;
};

/**
 * Initialize the 2D pipeline
 *
 * Parameters
 * - arena The arena to allocate from
 * - maxTextures The maximum textures which can be added to the pipelin
 * - colorFormat The format of the color attachment, must not be UNDEFINED
 * - depthFormat The format of the depth attachment, if any
 */
void hgInitPipeline2D(
    HgArena* arena,
    u32 maxTextures,
    VkFormat colorFormat,
    VkFormat depthFormat);

/**
 * Deinitialize the 2D pipeline
 */
void hgDeinitPipeline2D();

/**
 * Add a texture to the 2D pipeline
 */
void hgAddTexture2D(HgResource textureID);

/**
 * Remove a texture from the 2D pipeline
 */
void hgRemoveTexture2D(HgResource textureID);

/**
 * Update the 2D pipeline's projection matrix
 */
void hgUpdateProjection2D(const HgMat4& projection);

/**
 * Updates the 2D pipeline's view matrix
 */
void hgUpdateView2D(const HgMat4& view);

/**
 * Issue draw commands for all HgSprite components in the ecs
 * 
 * Parameters
 * - ecs The ecs to draw
 * - cmd The command buffer to record to, must not be nullptr
 */
void hgDraw2D(HgECS* ecs, VkCommandBuffer cmd);

/**
 * A model component rendered by the 3d pipeline
 */
struct HgModel3D
{
    /**
     * The model to render
     */
    HgResource modelResource;
    // /**
    //  * The model's color map
    //  */
    // HgResource colorMap;
    // /**
    //  * The model's normal map
    //  */
    // HgResource normalMap;
};

/**
 * A direction light component rendered by the 3d pipeline
 */
struct HgDirLight3D
{
    /**
     * The direction of the light
     */
    HgVec3 dir;
    /**
     * The color of the light
     */
    HgVec4 color;
};

/**
 * A point light component rendered by the 3d pipeline
 */
struct HgPointLight3D
{
    /**
     * The color of the light
     */
    HgVec4 color;
};

/**
 * Initialize the 3D pipeline
 *
 * Parameters
 * - arena The arena to allocate from
 * - maxModels The maximum models which can be added to the pipeline
 * - colorFormat The format of the color attachment, must not be UNDEFINED
 * - depthFormat The format of the depth attachment, must not be UNDEFINED
 */
void hgInitPipeline3D(
    HgArena* arena,
    u32 maxModels,
    VkFormat colorFormat,
    VkFormat depthFormat);

/**
 * Deinitialize the 3D pipeline
 */
void hgDeinitPipeline3D();

/**
 * Add a model to the 3D pipeline
 *
 * Parameters
 * - modelID The model to add
 * - colorID The model's color texture
 * - normalID The model's normal texture, if any
 */
void hgAddModel3D(HgResource modelID, HgResource colorID, HgResource normalID);

/**
 * Remove a model from the 3D pipeline
 */
void hgRemoveModel3D(HgResource modelID);

/**
 * Update the 3D pipeline's projection matrix
 */
void hgUpdateProjection3D(const HgMat4& projection);

/**
 * Update the 3D pipeline's view matrix
 */
void hgUpdateView3D(const HgMat4& view);

/**
 * Issue draw commands for all HgModelComp components in the ecs
 * 
 * Parameters
 * - ecs The ecs to draw
 * - cmd The command buffer to record to, must not be nullptr
 */
void hgDraw3D(HgECS* ecs, VkCommandBuffer cmd);

/**
 * Initializes the graphics subsystem, loading all global Vulkan resources
 */
void hgInitGraphics();

/**
 * Deinitializes the graphics subsystem, unloading all global Vulkan resources
 */
void hgDeinitGraphics();

/**
 * Loads the Vulkan functions which use the instance
 *
 * Parameters
 * - instance The Vulkan instance, must not be nullptr
 */
void hgLoadVulkanInstanceFuncs(VkInstance instance);

/**
 * Loads the Vulkan functions which use the device
 *
 * Parameters
 * - device The Vulkan device, must not be nullptr
 */
void hgLoadVulkanDeviceFuncs(VkDevice device);

/**
 * The global Vulkan instance
 */
inline VkInstance hgVkInstance = nullptr;

/**
 * The global Vulkan physical device
 */
inline VkPhysicalDevice hgVkPhysicalDevice = nullptr;
/**
 * The global Vulkan logical device
 */
inline VkDevice hgVkDevice = nullptr;
/**
 * The global Vulkan memory allocator
 */
inline VmaAllocator hgVkVma = nullptr;

/**
 * The global Vulkan queue
 */
inline VkQueue hgVkQueue = nullptr;
/**
 * The global Vulkan queue family
 */
inline u32 hgVkQueueFamily = (u32)-1;
/**
 * The global Vulkan command pool
 */
inline VkCommandPool hgVkCmdPool = nullptr;

/**
 * Turns a VkResult into a string
 *
 * Parameters
 * - result The result enum to stringify
 *
 * Returns
 * - The string of the enum value's name
 */
const char* hgVkResultToStr(VkResult result);

/**
 * Turns a VkFormat into the size in bytes
 *
 * Parameters
 * - format The format to get the size of
 *
 * Returns
 * - The size of the format in bytes
 */
u32 hgVkFormatToSize(VkFormat format);

/**
 * Creates a Vulkan instance with sensible defaults
 *
 * In debug mode, enables debug messaging
 *
 * Parameters
 * - extensions The instance extensions to load
 * - extensionCount The number of extensions
 *
 * Returns
 * - The created VkInstance, will never be nullptr
 */
VkInstance hgCreateVkInstance(HgStringView* extensions, u32 extensionCount);

/**
 * Creates a Vulkan debug messenger
 *
 * Returns
 * - The created debug messenger, or nullptr if debug messenger is disabled
 */
VkDebugUtilsMessengerEXT hgCreateVkDebugUtilsMessenger();

/**
 * Find the first queue family index which supports the the queue flags
 *
 * Parameters
 * - gpu The physical device, must not be nullptr
 * - queueFamily Where to store the family, if found
 * - queueFlags The flags required of the queue family
 *
 * Returns
 * - Whether a queue family was found and stored in queueFamily
 */
bool hgFindVkQueueFamily(VkPhysicalDevice gpu, u32* queueFamily, VkQueueFlags queueFlags);

/**
 * Finds a Vulkan physical device with a general purpose queue family
 *
 * The physical device will have at least one queue family which supports both
 * graphics, transfer, and compute
 *
 * Returns
 * - The physical device
 * - nullptr if none was found
 */
VkPhysicalDevice hgFindVkPhysicalDevice();

/**
 * Creates a Vulkan logical device with a single general purpose queue
 *
 * The device will have queue 0 in hgVkQueueFamily
 *
 * Returns
 * - The physical device, will never be nullptr
 */
VkDevice hgCreateVkDevice();

/**
 * Attempts to find the index of a memory type which has the desired flags and
 * does not have the undesired flags
 *
 * Note, if no such memory type exists, the next best thing will be found
 *
 * The bitmask must not mask out all memory types
 *
 * Parameters
 * - bitmask A bitmask of which memory types cannot be used, must not be 0
 * - preferredFlags The flags which the type should have, though may not
 * - unpreferredFlags The flags which the type should not have, though may have
 *
 * Returns
 * - The found index of the memory type
 */
u32 hgFindVkMemoryTypeIndex(
    u32 bitmask,
    VkMemoryPropertyFlags preferredFlags = 0,
    VkMemoryPropertyFlags unpreferredFlags = 0);

// Vulkan allocator : TODO?

/**
 * Create a Vulkan buffer
 *
 * Parameters
 * - buffer A pointer to return the created buffer
 * - allocation A pointer to return the created allocation
 * - size The size in bytes of the buffer
 * - usage How the buffer will be used
 * - memory The memory flags for VMA
 */
void hgCreateVkBuffer(
    VkBuffer* buffer,
    VmaAllocation* allocation,
    u64 size,
    VkBufferUsageFlags usage,
    VmaAllocationCreateFlags memory = 0);

/**
 * How an HgBuffer will be accessed
 */
enum HgBufferMemoryUsage
{
    HgBufferMemoryUsage_deviceOnly,
    HgBufferMemoryUsage_frequentUpdate,
    HgBufferMemoryUsage_stageWrite,
    HgBufferMemoryUsage_stageRead,
};

/**
 * How an HgBuffer can be accessed
 */
enum HgBufferMemoryAccess
{
    HgBufferMemoryAccess_device = 0x1,
    HgBufferMemoryAccess_hostWrite = 0x2,
    HgBufferMemoryAccess_hostRead = 0x4,
};

/**
 * A gpu buffer
 */
struct HgBuffer
{
    /**
     * The Vulkan buffer
     */
    VkBuffer buffer;
    /**
     * The buffer's allocation
     */
    VmaAllocation alloc;
    /**
     * The size of the buffer
     */
    u64 size;
    /**
     * How the buffer can be accessed
     */
    HgBufferMemoryAccess access;
};

/**
 * Create a gpu buffer
 *
 * Parameters
 * - arena The arena to allocate from
 * - size The size in bytes of the buffer
 * - usage How the buffer will be used
 * - access How the buffer should be accessed
 */
HgBuffer* hgCreateBuffer(
    HgArena* arena,
    u64 size,
    VkBufferUsageFlags usage,
    HgBufferMemoryUsage access = HgBufferMemoryUsage_deviceOnly);

/**
 * Destroy a gpu buffer
 */
void hgDestroyBuffer(HgBuffer* buffer);

/**
 * Writes to a Vulkan device local buffer through a staging buffer
 *
 * Parameters
 * - dst The buffer to write to, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - src The data to write, must not be nullptr
 * - size The size in bytes to write
 */
void hgWriteVkBuffer(VkBuffer dst, u64 offset, const void* src, u64 size);

/**
 * Writes to a gpu buffer
 *
 * Parameters
 * - dst The buffer to write to, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - src The data to write, must not be nullptr
 * - size The size in bytes to write
 */
void hgWriteBuffer(HgBuffer* dst, u64 offset, const void* src, u64 size);

/**
 * Reads from a Vulkan device local buffer through a staging buffer
 *
 * Parameters
 * - dst The location to write to, must not be nullptr
 * - src The buffer to read from, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - size The size in bytes to read
 */
void hgReadVkBuffer(void* dst, VkBuffer src, u64 offset, u64 size);

/**
 * Reads from a Vulkan device local buffer through a staging buffer
 *
 * Parameters
 * - dst The location to write to, must not be nullptr
 * - src The buffer to read from, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - size The size in bytes to read
 */
void hgReadBuffer(void* dst, HgBuffer* src, u64 offset, u64 size);

/**
 * Config for hgCreateVkImage
 */
struct HgCreateVkImage
{
    /**
     * The dimensions of the image
     */
    VkImageType type = VK_IMAGE_TYPE_2D;
    /**
     * The width of the image
     */
    u32 width = 1;
    /**
     * The height of the image
     */
    u32 height = 1;
    /**
     * The depth of the image
     */
    u32 depth = 1;
    /**
     * The format of the image, must not be UNDEFINED
     */
    VkFormat format = VK_FORMAT_UNDEFINED;
    /**
     * The number of mip level
     */
    u32 mipLevels = 1;
    /**
     * The number of array layers
     */
    u32 arrayLayers = 1;
    /**
     * The number of MSAA samples
     */
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    /**
     * How the image will be used, must not be 0
     */
    VkImageUsageFlags usage = 0;
};

/**
 * Create a Vulkan image
 *
 * Parameters
 * - image A pointer to return the created image
 * - allocation A pointer to return the created allocation
 * - create The image create info
 */
void hgCreateVkImage(VkImage* image, VmaAllocation* allocation, const HgCreateVkImage& create);

/**
 * A gpu image
 */
struct HgImage
{
    /**
     * The Vulkan image
     */
    VkImage image;
    /**
     * The image's allocation
     */
    VmaAllocation alloc;
    /**
     * The type of image
     */
    VkImageType type;
    /**
     * The pixel format
     */
    VkFormat format;
    /**
     * The width in pixels
     */
    u32 width;
    /**
     * The height in pixels
     */
    u32 height;
    /**
     * The depth in pixels
     */
    u32 depth;
    /**
     * The number of mipmap levels
     */
    u32 mipLevels;
    /**
     * The number of array layers
     */
    u32 arrayLayers;
    /**
     * The number of msaa samples
     */
    VkSampleCountFlagBits msaaSamples;
};

/**
 * Config for hgCreateVkImage
 */
struct HgCreateImage
{
    /**
     * The dimensions of the image
     */
    VkImageType type = VK_IMAGE_TYPE_2D;
    /**
     * The width of the image
     */
    u32 width = 1;
    /**
     * The height of the image
     */
    u32 height = 1;
    /**
     * The depth of the image
     */
    u32 depth = 1;
    /**
     * The format of the image, must not be UNDEFINED
     */
    VkFormat format = VK_FORMAT_UNDEFINED;
    /**
     * The number of mip level
     */
    u32 mipLevels = 1;
    /**
     * The number of array layers
     */
    u32 arrayLayers = 1;
    /**
     * The number of MSAA samples
     */
    VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    /**
     * How the image will be used, must not be 0
     */
    VkImageUsageFlags usage = 0;
};

/**
 * Create a gpu image
 */
HgImage* hgCreateImage(HgArena* arena, const HgCreateImage& create);

/**
 * Destroy a gpu image
 */
void hgDestroyImage(HgImage* image);

/**
 * Config for hgVkCreateImage
 */
struct HgCreateVkImageView
{
    /**
     * The image to create a view of, must not be nullptr
     */
    VkImage image = nullptr;
    /**
     * The dimensions of the image
     */
    VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D;
    /**
     * The format of the image, must not be UNDEFINED
     */
    VkFormat format = VK_FORMAT_UNDEFINED;
    /**
     * The subresource, aspect must not be 0
     */
    VkImageSubresourceRange subresource = {0, 0, 1, 0, 1};
    /**
     * The component swizzles
     */
    VkComponentMapping components = {};
};

/**
 * Create a Vulkan image
 */
VkImageView hgCreateVkImageView(const HgCreateVkImageView& create);

/**
 * A view into a gpu image
 */
struct HgImageView
{
    /**
     * The view
     */
    VkImageView view;
    /**
     * The image viewed
     */
    HgImage* image;
    /**
     * The type of view
     */
    VkImageViewType type;
    /**
     * The image aspect
     */
    VkImageAspectFlags aspectFlags;
    /**
     * The first mip level
     */
    u32 baseMipLevel;
    /**
     * The number of mip levels including the base
     */
    u32 levelCount;
    /**
     * The first array layer
     */
    u32 baseArrayLayer;
    /**
     * The number of array layers including the base
     */
    u32 layerCount;
};

/**
 * Config for hgVkCreateImage
 */
struct HgCreateImageView
{
    /**
     * The image to create a view of, must not be nullptr
     */
    HgImage* image = nullptr;
    /**
     * The dimensions of the image
     */
    VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D;
    /**
     * The image aspect, must not be none
     */
    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_NONE;
    /**
     * The first mip level
     */
    u32 baseMipLevel = 0;
    /**
     * The number of mip levels including the base
     */
    u32 levelCount = 1;
    /**
     * The first array layer
     */
    u32 baseArrayLayer = 0;
    /**
     * The number of array layers including the base
     */
    u32 layerCount = 1;
};

/**
 * Create a Vulkan image
 */
HgImageView* hgCreateImageView(
    HgArena* arena,
    HgImage* image,
    VkImageViewType type,
    VkImageSubresourceRange subresource);

/**
 * Config for hgCreateVkSampler
 */
struct HgCreateVkSampler
{
    /**
     * The filter used for sampling
     */
    VkFilter filter;
    /**
     * How addresses past the edge are handled
     */
    VkSamplerAddressMode addressMode;
    /**
     * The border color, if addressMode uses a border
     */
    VkBorderColor borderColor;
    /**
     * The load bias
     */
    f32 mipLodBias = 0.0f;
    /**
     * The min clamp lod
     */
    f32 minLod = 0.0f;
    /**
     * The max clamp, or 1000.0f for no max
     */
    f32 maxLod = 1000.0f;
};

/**
 * Create a Vulkan sampler
 */
VkSampler hgCreateVkSampler(const HgCreateVkSampler& create);

/**
 * Config for hgWriteVkImage
 */
struct HgWriteVkImage
{
    /**
     * The image to write to, must not be nullptr
     */
    VkImage dstImage = nullptr;
    /**
     * The subresource of the image to write to
     */
    VkImageSubresourceLayers subresource = {0, 0, 0, 1};
    /**
     * The data to write, must not be nullptr
     */
    const void* srcData = nullptr;
    /**
     * The width of the image in pixels, must be greater than 0
     */
    u32 width = 1;
    /**
     * The width of the image in pixels, must be greater than 0
     */
    u32 height = 1;
    /**
     * The width of the image in pixels, must be greater than 0
     */
    u32 depth = 1;
    /**
     * The format of each pixel, must not be UNDEFINED
     */
    VkFormat format = VK_FORMAT_UNDEFINED;
    /**
     * The layout to transition to after transfering
     */
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
};

/**
 * Writes to a Vulkan device local image through a staging buffer
 */
void hgWriteVkImage(const HgWriteVkImage& config);

/**
 * Writes to a Vulkan device local cubemap image through a staging buffer
 *
 * Note, config.width and config.height are interpreted as the width and height
 * of each face of the cubemap and srcData is assumed to be layed out as:
 *  #
 * ####
 *  #
 */
void hgWriteVkImageCubemap(const HgWriteVkImage &config);

/**
 * Write to a gpu image
 *
 * Parameters
 * - dst The image to write to
 * - subresource The subresource of the image to write to
 * - src The data to read from
 * - layout The final layout to set the image to
 */
void hgWriteImage(
    HgImage* dst,
    VkImageSubresourceLayers subresource,
    const void* src,
    VkImageLayout layout);

/**
 * Write to a gpu image cubemap
 *
 * Note, dst should have 6 array layers, all of which will be filled
 *
 * Note, srcData is assumed to be layed out as:
 *  #
 * ####
 *  #
 *
 * Parameters
 * - dst The image to write to
 * - subresource The subresource of the image to write to
 * - src The data to read from
 * - layout The final layout to set the image to
 */
void hgWriteImageCubemap(
    HgImage* dst,
    VkImageSubresourceLayers subresource,
    const void* src,
    VkImageLayout layout);

/**
 * Config for hgReadVkImage
 */
struct HgReadVkImage
{
    /**
     * The location to write to, must not be nullptr
     */
    void* dst = nullptr;
    /**
     * The image to read from, must not be nullptr
     */
    VkImage srcImage = nullptr;
    /**
     * The layout the image was in before reading, must not be UNDEFINED
     */
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    /**
     * The subresource of the image to read from
     */
    VkImageSubresourceLayers subresource = {0, 0, 0, 1};
    /**
     * The width of the image in pixels, must be greater than 0
     */
    u32 width = 1;
    /**
     * The width of the image in pixels, must be greater than 0
     */
    u32 height = 1;
    /**
     * The width of the image in pixels, must be greater than 0
     */
    u32 depth = 1;
    /**
     * The format of each pixel, must not be UNDEFINED
     */
    VkFormat format = VK_FORMAT_UNDEFINED;
};

/**
 * Reads from a Vulkan device local image through a staging buffer
 */
void hgReadVkImage(const HgReadVkImage& config);

/**
 * Read from a gpu image
 *
 * Parameters
 * - src The pointer to write to
 * - dst The image to read from
 * - subresource The subresource of the image to read from
 * - layout The final layout to set the image to
 */
void hgReadImage(
    void* dst,
    HgImage* src,
    VkImageSubresourceLayers subresource,
    VkImageLayout layout);

/**
 * Config for hgGenerateVkImageMipmaps
 */
struct HgGenerateVkImageMipmaps
{
    /**
     * The image to generate mipmaps in, must not be nullptr
     */
    VkImage image = nullptr;
    /**
     * The image aspects to use, must not be 0
     */
    VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_NONE;
    /**
     * The layout the image was in before, must not be UNDEFINED
     */
    VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    /**
     * The layout the image will be set to, must not be UNDEFINED
     */
    VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    /**
     * The width of the base level, must be greater than 0
     */
    u32 width = 1;
    /**
     * The width of the base level, must be greater than 0
     */
    u32 height = 1;
    /**
     * The width of the base level, must be greater than 0
     */
    u32 depth = 1;
    /**
     * The total number of mips in the image, must be greater than 0
     */
    u32 mipCount = 0;
};

/**
 * Generates mipmaps from the base level
 */
void hgGenerateVkImageMipmaps(const HgGenerateVkImageMipmaps& config);

/**
 * Generates mipmaps from the base level
 *
 * Parameters
 * - image The image to generate mipmaps for
 * - aspectFlags The image aspect flags
 * - oldLayout The layout the image was in before
 * - newLayout The layout the image will be set to after
 */
void hgGenerateMipmaps(
    HgImage* image,
    VkImageAspectFlags aspectFlags,
    VkImageLayout oldLayout,
    VkImageLayout newLayout);

/**
 * Create a descriptor set layout
 *
 * Parameters
 * - bindings The bindings in the descriptor set
 * - bindingCount The number of bindings
 */
VkDescriptorSetLayout hgCreateVkDescriptorSetLayout(
    const VkDescriptorSetLayoutBinding* bindings,
    u32 bindingCount);

/**
 * Create a pipeline layout
 *
 * Parameters
 * - setLayouts The descriptor set layouts
 * - setLayoutCount The number of set layouts
 * - pushRanges The push constant ranges
 * - pushRangeCount The number of push ranges
 */
VkPipelineLayout hgCreateVkPipelineLayout(
    VkDescriptorSetLayout* setLayouts,
    u32 setLayoutCount,
    VkPushConstantRange* pushRanges,
    u32 pushRangeCount);

/**
 * Create a Vulkan shader module
 *
 * Parameters
 * - spirvCode The spirv bytecode of the shader
 * - codeSize The size of spirvCode in bytes
 */
VkShaderModule hgCreateVkShaderModule(const u8* spirvCode, u64 codeSize);

/**
 * Config for hgCreateVkGraphicsPipeline
 */
struct HgCreateVkGraphicsPipeline
{
    /**
     * The format of the color attachments, none can be UNDEFINED
     */
    const VkFormat* colorAttachmentFormats = nullptr;
    /**
     * The number of color attachment formats
     */
    u32 colorAttachmentCount = 0;
    /**
     * The format of the depth attachment, no depth attachment if UNDEFINED
     */
    VkFormat depthAttachmentFormat = VK_FORMAT_UNDEFINED;
    /**
     * The format of the stencil attachment, no stencil attachment if UNDEFINED
     */
    VkFormat stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    /**
     * The vertex shader code
     */
    VkShaderModule vertexShader = nullptr;
    /**
     * The fragment shader code
     */
    VkShaderModule fragmentShader = nullptr;
    /**
     * The pipeline layout
     */
    VkPipelineLayout layout = nullptr;
    /**
     * Descriptions of the vertex bindings, may be nullptr
     */
    const VkVertexInputBindingDescription* vertexBindings = nullptr;
    /**
     * The number of vertex bindings
     */
    u32 vertexBindingCount = 0;
    /**
     * Descriptions of the vertex attributes, may be nullptr
     */
    const VkVertexInputAttributeDescription* vertexAttributes = nullptr;
    /**
     * The number of vertex attributes
     */
    u32 vertexAttributeCount = 0;
    /**
     * How to interpret vertices into topology
     */
    VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    /**
     * The number of patch control points in the tesselation stage
     */
    u32 tesselationPatchControlPoints = 0;
    /**
     * How polygons are drawn
     */
    VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
    /**
     * Which face is treated as the front
     */
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    /**
     * How many samples are used in MSAA
     */
    VkSampleCountFlagBits multisampleCount = VK_SAMPLE_COUNT_1_BIT;
    /**
     * Enables back/front face culling
     */
    VkCullModeFlagBits cullMode = VK_CULL_MODE_NONE;
    /**
     * Enables color blending using pixel alpha values
     */
    bool enableColorBlend = false;
};

/**
 * Creates a graphics pipeline
 *
 * Parameters
 * - config The pipeline configuration
 */
VkPipeline hgCreateVkGraphicsPipeline(const HgCreateVkGraphicsPipeline& config);

/**
 * Creates a compute pipeline
 *
 * Parameters
 * - layout The pipeline layout, must not be nullptr
 * - shader The compute shader, must not be nullptr
 */
VkPipeline hgCreateVkComputePipeline(VkPipelineLayout layout, const VkShaderModule shader);

/**
 * Create a descriptor pool
 *
 * Parameters
 * - maxSets The max number of sets which can be allocated
 * - sizes The numbers of each descriptor type to allocate
 * - sizeCount The number of sizes
 * - flags Extra flags passed to the create info struct, if any
 */
VkDescriptorPool hgCreateVkDescriptorPool(
    u32 maxSets,
    VkDescriptorPoolSize* sizes,
    u32 sizeCount,
    VkDescriptorPoolCreateFlags flags = 0);

/**
 * Allocate a single descriptor set
 *
 * Parameters
 * - pool The descriptor pool to allocate from
 * - layout The layout of the set
 */
VkDescriptorSet hgCreateVkDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout);

/**
 * Update a descriptor set binding
 *
 * Parameters
 * - set The descriptor set to update
 * - binding The binding in the set
 * - type The descriptor type
 * - info The image infos to update to
 * - count The number of descriptors
 */
void hgUpdateVkDescriptorSet(
    VkDescriptorSet set,
    u32 binding,
    VkDescriptorType type,
    const VkDescriptorBufferInfo* bufferInfos,
    const VkDescriptorImageInfo* imageInfos,
    u32 count);

/**
 * Begin a command buffer to be executed once
 *
 * Returns
 * - The command buffer to record, will never be nullptr
 */
VkCommandBuffer hgBeginVkCmd();

/**
 * Execute the command buffer and wait for completion
 *
 * Parameters
 * - cmd The command buffer from hgvkBeginCommands, must not be nullptr
 */
void hgEndVkCmd(VkCommandBuffer cmd);

/**
 * The image id used by HgRenderer
 */
using HgImageRenderID = u64;

/**
 * The buffer id used by HgRenderer
 */
using HgBufferRenderID = u64;

/**
 * A rendering attachment
 */
struct HgRenderAttachment
{
    /**
     * The image attached
     */
    HgImageRenderID image = (u64)-1;
    /**
     * How the image will be loaded
     */
    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    /**
     * How the image will be stored
     */
    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    /**
     * What to clear the image to if cleared
     */
    VkClearValue clearValue = {};
};

/**
 * A render pass description
 */
struct HgRenderPass
{
    /**
     * The uniforms buffer dependencies
     */
    const HgBufferRenderID* uniformBuffers = nullptr;
    /**
     * The number of uniform buffers
     */
    u32 uniformBufferCount = 0;
    /**
     * The storage buffer dependencies
     */
    const HgBufferRenderID* storageBuffers = nullptr;
    /**
     * The number of storage buffers
     */
    u32 storageBufferCount = 0;
    /**
     * The sampled image dependencies
     */
    const HgImageRenderID* sampledImages = nullptr;
    /**
     * The number of sampled images
     */
    u32 sampledImageCount = 0;
    /**
     * The number of layers in each color attachment to write to
     */
    u32 layerCount = 1;
    /**
     * The color images to write to
     */
    const HgRenderAttachment* colorAttachments = nullptr;
    /**
     * The number of color attachments
     */
    u32 colorAttachmentCount = 0;
    /**
     * The depth attachment, if any
     */
    HgRenderAttachment depthAttachment = {};
    /**
     * The stencil attachment, if any
     */
    HgRenderAttachment stencilAttachment = {};
};

/**
 * Where resources used in rendering can be used
 */
enum HgRenderUsage
{
    HgRenderUsage_none = 0,
    HgRenderUsage_vertexBuffer,
    HgRenderUsage_indexBuffer,
    HgRenderUsage_graphicsShader,
    HgRenderUsage_graphicsUniformBuffer,
    HgRenderUsage_computeShader,
    HgRenderUsage_computeUniformBuffer,
    HgRenderUsage_colorAttachment,
    HgRenderUsage_depthAttachment,
    HgRenderUsage_stencilAttachment,
    HgRenderUsage_transfer,
    HgRenderUsage_presentSrc,
    HgRenderUsage_count,
};

/**
 * How resources used in rendering can be accessed
 */
enum HgRenderAccess
{
    HgRenderAccess_none = 0x0,
    HgRenderAccess_read = 0x1,
    HgRenderAccess_write = 0x2,
    HgRenderAccess_readWrite = HgRenderAccess_read | HgRenderAccess_write,
};

/**
 * An image dependency barrier
 */
struct HgImageBarrier
{
    /**
     * The image to sychronize
     */
    HgImageRenderID image = (u64)-1;
    /**
     * How the image will next be used
     */
    HgRenderUsage nextUsage = HgRenderUsage_none;
    /**
     * How the image will next be accessed
     */
    HgRenderAccess nextAccess = HgRenderAccess_none;
};

/**
 * A buffer dependency barrier
 */
struct HgBufferBarrier
{
    /**
     * The buffer to sychronize
     */
    HgBufferRenderID buffer = (u64)-1;
    /**
     * How the buffer will next be used
     */
    HgRenderUsage nextUsage = HgRenderUsage_none;
    /**
     * How the buffer will next be accessed
     */
    HgRenderAccess nextAccess = HgRenderAccess_none;
};

/**
 * A renderer to organize render passes and synchronize resources
 */
struct HgRenderer
{
    /**
     * An image resource
     */
    struct Image
    {
        /**
         * The image
         */
        VkImage handle;
        /**
         * The image view
         */
        VkImageView view;
        /**
         * The image subresource
         */
        VkImageSubresourceRange subresource;
        /**
         * The image's last usage
         */
        HgRenderUsage lastUsage;
        /**
         * The image's last access
         */
        HgRenderAccess lastAccess;
    };

    /**
     * A buffer resource
     */
    struct Buffer
    {
        /**
         * The buffer
         */
        VkBuffer handle;
        /**
         * The offset into the buffer in bytes
         */
        u64 offset;
        /**
         * The size of the buffer in bytes
         */
        u64 size;
        /**
         * The buffer's last usage
         */
        HgRenderUsage lastUsage;
        /**
         * The buffer's last access
         */
        HgRenderAccess lastAccess;
    };

    /**
     * The buffer resources
     */
    Buffer* buffers;
    /**
     * The number of active buffer resources
     */
    u32 bufferCount;
    /**
     * The max buffer resources
     */
    u32 bufferCapacity;
    /**
     * The image resources
     */
    Image* images;
    /**
     * The number of active image resources
     */
    u32 imageCount;
    /**
     * The max image resources
     */
    u32 imageCapacity;

    /**
     * Create a new renderer
     *
     * Parameters
     * - arena The arena to allocate from
     * - maxImages The max number of image resources
     * - maxBuffers The max number of buffer resources
     */
    static HgRenderer create(HgArena* arena, u32 maxImages, u32 maxBuffers);

    /**
     * Removes all resources
     */
    void reset();

    /**
     * Add a buffer resource
     *
     * Parameters
     * - buffer The buffer to add, must not be nullptr
     * - offset The offset into the buffer in bytes
     * - size The size of the buffer in bytes
     * - previousUsage The last usage of the buffer, if any
     * - previousAccess The last access of the buffer, if any
     *
     * Returns
     * - The buffer resource's id
     */
    HgBufferRenderID addBuffer(
        VkBuffer buffer,
        u64 offset,
        u64 size,
        HgRenderUsage previousUsage = HgRenderUsage_none,
        HgRenderAccess previousAccess = HgRenderAccess_none);

    /**
     * Add a image resource
     *
     * Parameters
     * - image The image to add, must not be nullptr
     * - view The image's view, must not be nullptr
     * - subresource The subresource of the image
     * - previousUsage The last usage of the image, if any
     * - previousAccess The last access of the image, if any
     *
     * Returns
     * - The image resource's id
     */
    HgImageRenderID addImage(
        VkImage image,
        VkImageView view,
        VkImageSubresourceRange subresource,
        HgRenderUsage previousUsage = HgRenderUsage_none,
        HgRenderAccess previousAccess = HgRenderAccess_none);

    /**
     * Creates a barrier for resource uses that are not part of a render pass
     *
     * Parameters
     * - cmd The command buffer
     * - bufferBarriers The buffer barriers
     * - bufferBarrierCount The number of buffer barriers
     * - imageBarriers The image barriers
     * - imageBarrierCount The number of image barriers
     */
    void barrier(
        VkCommandBuffer cmd,
        const HgBufferBarrier* bufferBarriers,
        u32 bufferBarrierCount,
        const HgImageBarrier* imageBarriers,
        u32 imageBarrierCount);

    /**
     * Begins a render pass
     *
     * Parameters
     * - cmd The command buffer
     * - width The width of the render area
     * - height The height of the render area
     * - pass The render pass description
     */
    void beginPass(VkCommandBuffer cmd, u32 width, u32 height, const HgRenderPass& pass);

    /**
     * Ends the render pass
     *
     * Parameters
     * - cmd The command buffer
     */
    void endPass(VkCommandBuffer cmd);
};

/**
 * Initializes global resources for windowing
 */
void hgInitPlatform();

/**
 * Deinitializes global resources for windowing
 */
void hgDeinitPlatform();

/**
 * Get the platform's required instance extensions for windowing
 *
 * Parameters
 * - arena The arena to allocate from
 * - extBuffer A pointer to store the extension names
 *
 * Returns
 * - The number of required extensions
 */
u32 hgVkGetPlatformExtensions(HgArena* arena, HgStringView** extBuffer);

// audio system : TODO

/**
 * A key on the keyboard or button on the mouse
 */
enum HgKey
{
    HgKey_none = 0,
    HgKey_k0,
    HgKey_k1,
    HgKey_k2,
    HgKey_k3,
    HgKey_k4,
    HgKey_k5,
    HgKey_k6,
    HgKey_k7,
    HgKey_k8,
    HgKey_k9,
    HgKey_q,
    HgKey_w,
    HgKey_e,
    HgKey_r,
    HgKey_t,
    HgKey_y,
    HgKey_u,
    HgKey_i,
    HgKey_o,
    HgKey_p,
    HgKey_a,
    HgKey_s,
    HgKey_d,
    HgKey_f,
    HgKey_g,
    HgKey_h,
    HgKey_j,
    HgKey_k,
    HgKey_l,
    HgKey_z,
    HgKey_x,
    HgKey_c,
    HgKey_v,
    HgKey_b,
    HgKey_n,
    HgKey_m,
    HgKey_semicolon,
    HgKey_colon,
    HgKey_apostrophe,
    HgKey_quotation,
    HgKey_comma,
    HgKey_period,
    HgKey_question,
    HgKey_grave,
    HgKey_tilde,
    HgKey_exclamation,
    HgKey_at,
    HgKey_hash,
    HgKey_dollar,
    HgKey_percent,
    HgKey_carot,
    HgKey_ampersand,
    HgKey_asterisk,
    HgKey_lparen,
    HgKey_rparen,
    HgKey_lbracket,
    HgKey_rbracket,
    HgKey_lbrace,
    HgKey_rbrace,
    HgKey_equal,
    HgKey_less,
    HgKey_greater,
    HgKey_plus,
    HgKey_minus,
    HgKey_slash,
    HgKey_backslash,
    HgKey_underscore,
    HgKey_bar,
    HgKey_up,
    HgKey_down,
    HgKey_left,
    HgKey_right,
    HgKey_mouse1,
    HgKey_mouse2,
    HgKey_mouse3,
    HgKey_mouse4,
    HgKey_mouse5,
    HgKey_lmouse = HgKey_mouse1,
    HgKey_rmouse = HgKey_mouse2,
    HgKey_mmouse = HgKey_mouse3,
    HgKey_escape,
    HgKey_space,
    HgKey_enter,
    HgKey_backspace,
    HgKey_kdelete,
    HgKey_insert,
    HgKey_tab,
    HgKey_home,
    HgKey_end,
    HgKey_f1,
    HgKey_f2,
    HgKey_f3,
    HgKey_f4,
    HgKey_f5,
    HgKey_f6,
    HgKey_f7,
    HgKey_f8,
    HgKey_f9,
    HgKey_f10,
    HgKey_f11,
    HgKey_f12,
    HgKey_lshift,
    HgKey_rshift,
    HgKey_lctrl,
    HgKey_rctrl,
    HgKey_lmeta,
    HgKey_rmeta,
    HgKey_lalt,
    HgKey_ralt,
    HgKey_lsuper,
    HgKey_rsuper,
    HgKey_capslock,
    HgKey_count,
};

/**
 * Configuration for a window
 */
struct HgWindowConfig
{
    /**
     * The title of the window
     */
    const char* title;
    /**
     * Whether the window should be windowed or fullscreen
     */
    bool windowed;
    /**
     * The width in pixels if windowed, otherwise ignored
     */
    u32 width;
    /**
     * The height in pixels if windowed, otherwise ignored
     */
    u32 height;
    /**
     * How the swapchain images will be used
     */
    VkImageUsageFlags imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    /**
     * How the swapchain images will be presented
     *
     * Note, will fall back to FIFO if unavailable
     */
    VkPresentModeKHR preferredPresentMode = VK_PRESENT_MODE_FIFO_KHR;
};

/**
 * A window
 */
struct HgWindow
{
    struct Internals;

    /**
     * Platform specific resources for a window
     */
    Internals* internals;
    /**
     * The window's Vulkan surface
     */
    VkSurfaceKHR surface;
    /**
     * The swapchain
     */
    VkSwapchainKHR swapchain;
    /**
     * The swapchain image format
     */
    VkFormat format;
    /**
     * The number of swapchain images
     */
    u32 imageCount;
    /**
     * The swapchain images
     */
    VkImage* images;
    /**
     * The swapchain image views
     */
    VkImageView* views;
    /**
     * How the swapchain images are used
     */
    VkImageUsageFlags imageUsage;
    /**
     * The swapchain image format
     */
    VkPresentModeKHR presentMode;

    /**
     * The command buffers per image
     */
    VkCommandBuffer* cmds;
    /**
     * The fences per image
     */
    VkFence* frameFinished;
    /**
     * The semaphores per image to signal availability
     */
    VkSemaphore* imageAvailable;
    /**
     * The semaphores per image to signal presentability
     */
    VkSemaphore* readyToPresent;
    /**
     * The current frame
     */
    u32 currentFrame;
    /**
     * The current image
     */
    u32 currentImage;

    /**
     * The window's current width
     */
    u32 width;
    /**
     * The window's current height
     */
    u32 height;
    /**
     * The current x position of the mouse
     */
    f64 mousePosX;
    /**
     * The current y position of the mouse
     */
    f64 mousePosY;
    /**
     * The x distance the mouse has travelled since last frame
     */
    f64 mouseDeltaX;
    /**
     * The y distance the mouse has travelled since last frame
     */
    f64 mouseDeltaY;
    /**
     * Which keys are currently being held down
     */
    bool isKeyDown[HgKey_count];
    /**
     * Which keys were pressed this frame
     */
    bool wasKeyPressed[HgKey_count];
    /**
     * Which keys were released this frame
     */
    bool wasKeyReleased[HgKey_count];
    /**
     * Whether this window has been closed
     */
    bool wasClosed;

    /**
     * Creates a window
     *
     * Parameters
     * - arena The arena to allocate from
     * - config The window configuration
     */
    static HgWindow* create(HgArena* arena, const HgWindowConfig& config);

    /**
     * Destroys the window
     */
    void destroy();

    /**
     * Acquires the next swapchain image and begins its command buffer
     *
     * Returns
     * - The command buffer to record this frame
     * - nullptr if the swapchain cannot be rendered to
     */
    VkCommandBuffer beginRecording();

    /**
     * Finishes recording the command buffer and presents the swapchain image
     *
     * Parameters
     * - cmd The command buffer given from beginRecording
     */
    void endAndPresent(VkCommandBuffer cmd);

    /**
     * Sets the window icon : TODO
     *
     * Parameters
     * - iconData The pixels of the image to set the icon to, must not be nullptr
     * - width The width in pixels of the icon
     * - height The height in pixels of the icon
     */
    void setIcon(u32* iconData, u32 width, u32 height);

    /**
     * Gets whether the window is fullscreen or not : TODO
     *
     * Returns
     * - Whether the window is fullscreen
     */
    bool isFullscreen();

    /**
     * Sets the window to fullscreen of windowed mode : TODO
     *
     * Parameters
     * - fullscreen Whether to set fullscreen, or set windowed
     */
    void setFullscreen(bool fullscreen);

    /**
     * The builtin cursor images
     */
    enum Cursor
    {
        Cursor_none = 0,
        Cursor_arrow,
        Cursor_text,
        Cursor_wait,
        Cursor_cross,
        Cursor_hand,
    };

    /**
     * Sets the window's cursor to a platform defined icon : TODO
     */
    void setCursor(Cursor cursor);

    /**
     * Sets the window's cursor to a custom image : TODO
     */
    void setCursorImage(u32* data, u32 width, u32 height);
};

/**
 * Processes all events since the last call to process events or startup
 *
 * Must be called every frame before querying input
 * Processes all events, so all windows must be given
 * Updates the each window's input state
 *
 * Parameters
 * - windows All open windows, must not be nullptr
 * - windowCount The number of windows
 */
void hgProcessWindowEvents(HgWindow** windows, u32 windowCount);

/**
 * Initialize ImGui platform backend
 *
 * Note, requires GLFW on Linux (for now)
 *
 * Parameters
 * - window The window for ImGui to use
 */
void ImGui_ImplHurdyGurdy_Init(
    HgWindow* window,
    u32 colorAttachmentCount,
    const VkFormat* colorFormats,
    VkFormat depthFormat = VK_FORMAT_UNDEFINED,
    VkFormat stencilFormat = VK_FORMAT_UNDEFINED);

/**
 * Deinitializes ImGui platform backend
 */
void ImGui_ImplHurdyGurdy_Shutdown();

/**
 * Create a new ImGui frame for the platform backend
 */
void ImGui_ImplHurdyGurdy_NewFrame();

/**
 * Draw the ImGui frame
 *
 * Parameters
 * - cmd The command buffer to record to
 */
void ImGui_ImplHurdyGurdy_Draw(VkCommandBuffer cmd);

#endif // HURDYGURDY_HPP

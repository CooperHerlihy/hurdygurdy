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

#include <algorithm>
#include <type_traits>

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
struct HgDefer {
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
#define hgDefer(code) [[maybe_unused]] HgDefer hgMacroConcat(hgDefer_, __LINE__){[&]{code;}};

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
 * The config for the HurdyGurdy library init
 */
struct HgInit {
    u64 arenaSize = UINT32_MAX;

    u32 maxFences = 2048;
    u32 threadPoolQueueSize = 2048;
    u32 ioRequestQueueSize = 2048;

    u32 maxFramesInFlight = 2;

    u32 maxWindows = 8;
    u32 maxWindowEvents = 2048;

    u32 maxBinaries = 256;
    u32 maxImages = 256;
    u32 maxTextures = 256;
    u32 maxMeshes = 256;
    u32 maxModels = 256;
};

/**
 * Initializes the HurdyGurdy library
 *
 * Subsystems initialized:
 * - Arena allocation
 * - Thread pool
 * - IO thread
 * - Resource managers
 * - Windowing/input
 * - Hardware graphics
 */
void hgInit(const HgInit* init);

/**
 * Shuts down the HurdyGurdy library
 */
void hgDeinit();

/**
 * Run Hurdy Gurdy tests, asserting success
 */
void hgTest();

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
 * Reverse the endianness of a 16 bit value
 */
constexpr u16 hgEndianReverse16(u16 val)
{
    return (val & 0xff00 >> 8) & (val & 0x00ff << 8);
}

/**
 * Reverse the endianness of a 32 bit value
 */
constexpr u32 hgEndianReverse32(u32 val)
{
    return (val & 0xff0000 >> 16) & (val & 0x00ff00) & (val & 0x0000ff << 16);
}

/**
 * Reverse the endianness of a 64 bit value
 */
constexpr u64 hgEndianReverse64(u64 val)
{
    return (val & 0xff000000 >> 24) &
           (val & 0x00ff0000 >> 8) &
           (val & 0x0000ff00 << 8) &
           (val & 0x000000ff << 24);
}

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
struct HgVec2 {
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
struct HgVec3 {
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
    explicit constexpr operator HgVec2()
    {
        return HgVec2{x, y};
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
struct HgVec4 {
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
    explicit constexpr operator HgVec2()
    {
        return HgVec2{x, y};
    }
    /**
     * Downsize to Vec3
     */
    explicit constexpr operator HgVec3()
    {
        return HgVec3{x, y, z};
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
struct HgMat2 {
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
struct HgMat3 {
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
struct HgMat4 {
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
struct HgComplex {
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
struct HgQuat {
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
void hgVecAdd(u32 size, f32* dst, const f32* lhs, const f32* rhs);

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
void hgVecSub(u32 size, f32* dst, const f32* lhs, const f32* rhs);

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
void hgVecMulPairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs);

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
void hgVecMulScalar(u32 size, f32* dst, f32 scalar, const f32* vec);

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
void hgVecDivPairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs);

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
void hgVecDivScalar(u32 size, f32* dst, const f32* vec, f32 scalar);

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
void hgVecDot(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Compute the dot product of 2D vectors
 */
constexpr f32 hgVecDot2(HgVec2 lhs, HgVec2 rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

/**
 * Compute the dot product of 3D vectors
 */
constexpr f32 hgVecDot3(HgVec3 lhs, HgVec3 rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

/**
 * Compute the dot product of 4D vectors
 */
constexpr f32 hgVecDot3(HgVec4 lhs, HgVec4 rhs)
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
void hgVecLen(u32 size, f32* dst, const f32* vec);

/**
 * Compute the length of a 2D vector
 */
f32 hgVecLen2(HgVec2 vec);

/**
 * Compute the length of a 3D vector
 */
f32 hgVecLen3(HgVec3 vec);

/**
 * Compute the length of a 4D vector
 */
f32 hgVecLen4(HgVec4 vec);

/**
 * Normalize a vector
 *
 * Note, cannot normalize 0
 */
void hgVecNorm(u32 size, f32* dst, const f32* vec);

/**
 * Normalize a 2D vector
 *
 * Note, cannot normalize 0
 */
HgVec2 hgVecNorm2(HgVec2 vec);

/**
 * Normalize a 3D vector
 *
 * Note, cannot normalize 0
 */
HgVec3 hgVecNorm3(HgVec3 vec);

/**
 * Normalize a 4D vector
 *
 * Note, cannot normalize 0
 */
HgVec4 hgVecNorm4(HgVec4 vec);

/**
 * Compute the cross product of 3D vectors
 *
 * Parameters
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hgVecCross(f32* dst, const f32* lhs, const f32* rhs);

/**
 * Compute the cross product of 3D vectors
 */
HgVec3 hgVecCross(HgVec3 lhs, HgVec3 rhs);

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
void hgMatAdd(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs);

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
void hgMatSub(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs);

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
void hgMatMul(f32* dst, u32 wl, u32 hl, const f32* lhs, u32 wr, u32 hr, const f32* rhs);

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
void hgMatMulVec(u32 width, u32 height, f32* dst, const f32* mat, const f32* vec);

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
constexpr HgQuat hgQuatConj(HgQuat quat)
{
    return HgQuat{quat.r, -quat.i, -quat.j, -quat.k};
}

/**
 * Create a rotation quaternion from an axis and angle
 */
HgQuat hgQuatAxisAngle(HgVec3 axis, f32 angle);

/**
 * Rotate a 3D vector using a quaternion
 */
HgVec3 hgVecRotate(HgQuat lhs, HgVec3 rhs);

/**
 * Rotate a 3x3 matrix using a quaternion
 */
HgMat3 hgMatRotate(HgQuat lhs, HgMat3 rhs);

/**
 * Creates a model matrix for 2D graphics
 *
 * Parameters
 * - position The position of the model
 * - scale The scale of the model
 * - rotation The rotation of the model
 */
HgMat4 hgMatModel2D(HgVec3 position, HgVec2 scale, f32 rotation);

/**
 * Creates a model matrix for 3D graphics
 *
 * Parameters
 * - position The position of the model
 * - scale The scale of the model
 * - rotation The rotation of the model
 */
HgMat4 hgMatModel3D(const HgVec3& position, const HgVec3& scale, const HgQuat& rotation);

/**
 * Creates a view matrix
 *
 * Parameters
 * - position The position of the camera
 * - zoom The zoom of the camera
 * - rotation The rotation of the camera
 */
HgMat4 hgMatView(const HgVec3& position, const HgVec3& zoom, const HgQuat& rotation);

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
HgMat4 hgMatOrthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far);

/**
 * Creates a perspective projection matrix
 *
 * Parameters
 * - fov The field of view of the projection in radians
 * - aspect The aspect ratio of the projection
 * - near The near plane of the projection, must be greater than 0.0f
 * - far The far plane of the projection, must be greater than near
 */
HgMat4 hgMatPerspective(f32 fov, f32 aspect, f32 near, f32 far);

/**
 * Generate white noise
 */
u32 hgNoise(u32 seed, u32 pos);

/**
 * Generate white noise
 */
u32 hgNoise2D(u32 seed, u32 x, u32 y);

/**
 * Generate white noise
 */
u32 hgNoise3D(u32 seed, u32 x, u32 y, u32 z);

/**
 * Generate white noise
 */
u32 hgNoise4D(u32 seed, u32 x, u32 y, u32 z, u32 w);

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
f32 hgNoiseNorm(u32 seed, f32 pos);

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
f32 hgNoiseNorm2D(u32 seed, HgVec2 pos);

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
f32 hgNoiseNorm3D(u32 seed, HgVec3 pos);

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
f32 hgNoiseNorm4D(u32 seed, HgVec4 pos);

/**
 * Generate white noise unit vector
 */
f32 hgNoiseVec1D(u32 seed, f32 pos);

/**
 * Generate white noise unit vector
 */
HgVec2 hgNoiseVec2D(u32 seed, HgVec2 pos);

// value and perlin noise : TODO

// sort and search algorithms : TODO

/**
 * Calculates the maximum number of mipmap levels that an image can have
 *
 * Parameters
 * - width The width of the image
 * - height The height of the image
 * - depth The depth of the image
 */
u32 hgGetMaxMipmaps(u32 width, u32 height, u32 depth);

/**
 * An arena allocator
 */
struct HgArena {
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
};

/**
 * Create a guard which restores an arena's head at the end of the scope
 */
struct HgArenaScope {
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
 * Initializes scratch arenas
 *
 * Parameters
 * - size The size of each arena in bytes
 */
void hgScratchInit(u64 size);

/**
 * Deinitializes scratch arenas
 */
void hgScratchDeinit();

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
HgArena* hgScratch(HgArena const* const* conflicts = nullptr, u32 count = 0);

/**
 * A span view into a string
 */
struct HgStringView {
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
struct HgString {
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
     * Implicit converts to a string view
     */
    constexpr operator HgStringView() const
    {
        return {chars, length};
    }
};

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
 * Creates a new string with empty capacity
 *
 * Parameters
 * - arena The arena to allocate from
 * - capacity The capacity to begin with
 */
HgString hgStringCreate(HgArena* arena, u64 capacity);

/**
 * Creates a new string copied from an existing string
 *
 * Parameters
 * - arena The arena to allocate from
 * - init The initial string to copy from
 */
HgString hgStringCopy(HgArena* arena, HgStringView str);

/**
 * Create a formatted string : TODO
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
HgString hgStringFormat(HgArena* arena, HgStringView fmt, ...);

/**
 * Change the capacity of a string
 *
 * Parameters
 * - arena The arena to allocate from
 * - str The string to reserve memory for
 * - newCapacity The new minimum capacity
 */
void hgStringReserve(HgArena* arena, HgString* str, u64 newCapacity);

/**
 * Increases the capacity of the string by a factor, or inits to 1
 *
 * Parameters
 * - arena The arena to allocate from
 * - str The string to reserve memory for
 * - factor The growth factor to increase by
 */
void hgStringGrow(HgArena* arena, HgString* str, f64 factor);

/**
 * Copies another string into this string at index
 *
 * Parameters
 * - arena The arena to allocate from
 * - dst The string to insert into
 * - idx The index into dst
 * - src The string to copy from
 *
 * Returns
 */
void hgStringInsert(HgArena* arena, HgString* dst, u64 idx, HgStringView src);

/**
 * Copies another string to the end of this string
 */
inline void hgStringAppend(HgArena* arena, HgString* dst, HgStringView src)
{
    hgStringInsert(arena, dst, dst->length, src);
}

/**
 * Copies another string to the beginning of this string
 */
inline void hgStringPrepend(HgArena* arena, HgString* dst, HgStringView src)
{
    hgStringInsert(arena, dst, 0, src);
}

/**
 * Copies another string into this string at index
 *
 * Parameters
 * - arena The arena to allocate from
 * - dst The string to insert into
 * - idx The index into dst
 * - src The string to copy from
 */
void hgStringInsertc(HgArena* arena, HgString* dst, u64 idx, char c);

/**
 * Copies another string to the end of this string
 */
inline void hgStringAppendc(HgArena* arena, HgString* dst, char c)
{
    hgStringInsertc(arena, dst, dst->length, c);
}

/**
 * Copies another string to the beginning of this string
 */
inline void hgStringPrependc(HgArena* arena, HgString* dst, char c)
{
    hgStringInsertc(arena, dst, 0, c);
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
bool hgIsInteger(HgStringView str);

/**
 * Check whether a string is a base 10 floating point number
 */
bool hgIsFloat(HgStringView str);

/**
 * Create an integer from a base 10 string
 */
i64 hgStringToInteger(HgStringView str);

/**
 * Create a float from a base 10 string
 */
f64 hgStringToFloat(HgStringView str);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 */
HgString hgIntegerToString(HgArena* arena, i64 num);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 * - decimalCount The number of trailing decimal digits
 */
HgString hgFloatToString(HgArena* arena, f64 num, u32 decimalCount);

// base 2 and 16 string-int conversions : TODO
// arbitrary base string-int conversions : TODO?

/**
 * An error contained in the json
 */
struct HgJsonError {
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
enum HgJsonType : u32 {
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
struct HgJsonField {
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
struct HgJsonStruct {
    /**
     * The first field
     */
    HgJsonField* fields;
};

/**
 * An element in an array
 */
struct HgJsonElem {
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
struct HgJsonArray {
    /**
     * The first element
     */
    HgJsonElem* elems;
};

/**
 * A node in the json file
 */
struct HgJsonNode {
    /**
     * The node's type
     */
    HgJsonType type;
    /**
     * The value in the node
     */
    union {
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
struct HgJson {
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
 * A generation counted handle
 */
struct HgHandle {
    /**
     * The handle id
     */
    u32 id = (u32)-1;
};

static constexpr u32 hgHandleIdxBits = 24;

/**
 * Get the index from a handle
 */
constexpr u32 hgHandleIdx(HgHandle handle)
{
    return handle.id & ((1 << hgHandleIdxBits) - 1);
}

/**
 * Get the generation from a handle
 */
constexpr u32 hgHandleGeneration(HgHandle handle)
{
    return handle.id & ~((1 << hgHandleIdxBits) - 1);
}

/**
 * Returns a new handle at the same index
 */
constexpr HgHandle hgHandleNextGeneration(HgHandle handle)
{
    return {handle.id + (1 << hgHandleIdxBits)};
}

/**
 * An object pool
 */
template<typename T>
struct HgPool {
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);

    /**
     * The values stored in the pool
     */
    T* vals;
    /**
     * The handle free list
     */
    HgHandle* freeList;
    /**
     * The next handle in the free list
     */
    HgHandle next;
    /**
     * The capacity of the pool
     */
    u32 capacity;
};

/**
 * Reset an object pool
 */
template<typename T>
constexpr void hgPoolReset(HgPool<T>* pool)
{
    hgAssert(pool != nullptr);

    for (u32 i = 0; i < pool->capacity; ++i)
    {
        pool->freeList[i] = {i + 1};
    }
    pool->next = {0};
}

/**
 * Create a new object pool
 */
template<typename T>
HgPool<T> hgPoolCreate(HgArena* arena, u32 capacity)
{
    hgAssert(arena != nullptr);
    hgAssert(capacity > 0);

    HgPool<T> pool{};
    pool.vals = hgAlloc<T>(arena, capacity);
    pool.freeList = hgAlloc<HgHandle>(arena, capacity);
    pool.capacity = capacity;
    hgPoolReset(&pool);

    return pool;
}

/**
 * Allocate an object from a pool
 *
 * Note, does not initialize the object
 */
template<typename T>
constexpr HgHandle hgPoolAlloc(HgPool<T>* pool)
{
    hgAssert(pool != nullptr);
    hgAssert(hgHandleIdx(pool->next) < pool->capacity);

    HgHandle handle = pool->next;
    pool->next = pool->freeList[hgHandleIdx(handle)];
    pool->freeList[hgHandleIdx(handle)] = handle;
    return handle;
}

/**
 * Returns whether a handle is alive in the pool
 */
template<typename T>
constexpr bool hgPoolAlive(HgPool<T>* pool, HgHandle handle)
{
    return hgHandleIdx(handle) < pool->capacity && pool->freeList[hgHandleIdx(handle)].id == handle.id;
}

/**
 * Free an object back into a pool
 *
 * Note, the object handle must be valid and alive
 */
template<typename T>
constexpr void hgPoolFree(HgPool<T>* pool, HgHandle handle)
{
    hgAssert(hgPoolAlive(pool, handle));
    pool->freeList[hgHandleIdx(handle)] = pool->next;
    pool->next = hgHandleNextGeneration(handle);
}

/**
 * Get an object from a pool
 */
template<typename T>
constexpr T* hgPoolGet(HgPool<T>* pool, HgHandle handle)
{
    hgAssert(hgPoolAlive(pool, handle));
    return &pool->vals[hgHandleIdx(handle)];
}

/**
 * A pool of indices
 */
typedef HgPool<void> HgIndexPool;

/**
 * A pool of indices
 */
template<>
struct HgPool<void> {
    /**
     * The handle free list
     */
    HgHandle* freeList;
    /**
     * The next handle in the free list
     */
    HgHandle next;
    /**
     * The capacity of the pool
     */
    u32 capacity;
};

/**
 * Create a new object pool
 */
template<>
inline HgPool<void> hgPoolCreate(HgArena* arena, u32 capacity)
{
    hgAssert(arena != nullptr);
    hgAssert(capacity > 0);

    HgPool<void> pool{};
    pool.freeList = hgAlloc<HgHandle>(arena, capacity);
    pool.capacity = capacity;
    hgPoolReset(&pool);

    return pool;
}

template<typename T>
constexpr u64 hgHash(T val);

/**
 * Hash map hashing for u8
 */
template<>
constexpr u64 hgHash(u8 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u16
 */
template<>
constexpr u64 hgHash(u16 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u32
 */
template<>
constexpr u64 hgHash(u32 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for u64
 */
template<>
constexpr u64 hgHash(u64 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i8
 */
template<>
constexpr u64 hgHash(i8 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i16
 */
template<>
constexpr u64 hgHash(i16 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i32
 */
template<>
constexpr u64 hgHash(i32 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for i64
 */
template<>
constexpr u64 hgHash(i64 val)
{
    return (u64)val;
}

/**
 * Hash map hashing for f32
 */
template<>
constexpr u64 hgHash(f32 val)
{
    union {
        f32 asFloat;
        u64 asHash;
    } u{};
    u.asFloat = val;
    return u.asHash;
}

/**
 * Hash map hashing for f64
 */
template<>
constexpr u64 hgHash(f64 val)
{
    union {
        f64 asFloat;
        u64 asHash;
    } u{};
    u.asFloat = val;
    return u.asHash;
}

/**
 * Hash map hashing for arbitrary pointer types
 */
template<typename T>
constexpr u64 hgPtrHash(T* val)
{
    union {
        T* asPtr;
        uptr asUptr;
    } u{};
    u.asPtr = val;
    return (u64)u.asUptr;
};

/**
 * Hash map hashing for void*
 */
template<>
constexpr u64 hgHash(void* val)
{
    return hgPtrHash<void>(val);
}

/**
 * Hash map hashing for HgHandle
 */
template<>
constexpr u64 hgHash(HgHandle handle)
{
    return handle.id;
}

/**
 * Hash map hashing for strings
 */
template<>
constexpr u64 hgHash(HgStringView str)
{
    u64 ret = 0;
    u64 mult = 1;
    for (u32 i = 0; i < str.length; ++i)
    {
        ret += (u64)str[i] * mult;
        mult *= 257;
    }
    return ret;
}

/**
 * Hash map hashing for HgString
 */
template<>
constexpr u64 hgHash(HgString str)
{
    return hgHash(HgStringView{str});
}

/**
 * Hash map hashing for C string
 */
template<>
constexpr u64 hgHash(const char* str)
{
    return hgHash(HgStringView{str});
}

/**
 * A hash set
 */
template<typename V>
struct HgSet {
    static_assert(std::is_trivially_copyable_v<V> && std::is_trivially_destructible_v<V>);

    /**
     * Whether each index has a value
     */
    bool* hasVal;
    /**
     * Where the values are stored;
     */
    V* vals;
    /**
     * The max number of vals
     */
    u32 capacity;
    /**
     * The current number of values that are stored
     */
    u32 count;
};

/**
 * Creates a new hash set
 *
 * Parameters
 * - arena The arena to allocate from
 * - slotCount The max number of slots to store values in
 */
template<typename V>
HgSet<V> hgSetCreate(HgArena* arena, u32 slotCount)
{
    hgAssert(slotCount > 0);

    HgSet<V> set;
    set.hasVal = hgAlloc<bool>(arena, slotCount);
    set.vals = hgAlloc<V>(arena, slotCount);
    set.capacity = slotCount;
    hgSetReset(&set);
    return set;
}

/**
 * Empties all slots
 */
template<typename V>
void hgSetReset(HgSet<V>* set)
{
    for (u32 i = 0; i < set->capacity; ++i)
    {
        set->hasVal[i] = false;
    }
    set->count = 0;
}

/**
 * Add a value to the set
 */
template<typename V, typename T = V>
void hgSetAdd(HgSet<V>* set, const T& val)
{
    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    hgAssert(set->count < set->capacity - 1);

    u32 idx = hgHash(v) % set->capacity;
    for (u32 dist = 0; set->hasVal[idx] && set->vals[idx] != v; ++dist)
    {
        u32 otherDist = hgHash(set->vals[idx]) % set->capacity - idx;
        if (otherDist > set->capacity)
            otherDist += set->capacity;

        if (otherDist < dist)
        {
            V valTmp = set->vals[idx];
            set->vals[idx] = v;
            v = valTmp;
            dist = otherDist;
        }

        idx = (idx + 1) % set->capacity;
    }

    set->hasVal[idx] = true;
    set->vals[idx] = v;
    ++set->count;
}

/**
 * Remove a value from the set
 */
template<typename V, typename T = V>
void hgSetRemove(HgSet<V>* set, const T& val)
{
    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    u32 idx = hgHash(v) % set->capacity;
    while (set->hasVal[idx])
    {
        if (set->vals[idx] == v)
            break;
        idx = (idx + 1) % set->capacity;
    }
    if (!set->hasVal[idx])
        return;

    u32 next = (idx + 1) % set->capacity;
    while (set->hasVal[next])
    {
        if (hgHash(set->vals[next]) % set->capacity != next)
        {
            set->vals[idx] = set->vals[next];
            idx = next;
        }
        next = (next + 1) % set->capacity;
    }
    set->hasVal[idx] = false;
    --set->count;
}

/**
 * Checks whether a value is contained in the set
 */
template<typename V, typename T = V>
bool hgSetHas(const HgSet<V>* set, const T& val)
{
    static_assert(std::is_convertible_v<T, V>);
    V v = (V)val;

    for (u32 idx = hgHash(v) % set->capacity; set->hasVal[idx]; idx = (idx + 1) % set->capacity)
    {
        if (set->vals[idx] == v)
            return true;
    }
    return false;
}

/**
 * A key-value hash map
 */
template<typename K, typename V>
struct HgMap {
    static_assert(std::is_trivially_copyable_v<K>
               && std::is_trivially_copyable_v<V>
               && std::is_trivially_destructible_v<K>
               && std::is_trivially_destructible_v<V>);

    /**
     * Whether each index has a value
     */
    bool* hasVal;
    /**
     * Where the keys are stored;
     */
    K* keys;
    /**
     * Where the values are stored
     */
    V* vals;
    /**
     * The max number of key value pairs
     */
    u32 capacity;
    /**
     * The current number of values that are stored
     */
    u32 count;
};

/**
 * Creates a new hash map
 *
 * Parameters
 * - arena The arena to allocate from
 * - slotCount The max number of slots to store values in
 */
template<typename K, typename V>
HgMap<K, V> hgMapCreate(HgArena* arena, u32 slotCount)
{
    hgAssert(slotCount > 0);

    HgMap<K, V> map;
    map.hasVal = hgAlloc<bool>(arena, slotCount);
    map.keys = hgAlloc<K>(arena, slotCount);
    map.vals = hgAlloc<V>(arena, slotCount);
    map.capacity = slotCount;
    hgMapReset(&map);
    return map;
}

/**
 * Empties all slots
 */
template<typename K, typename V>
void hgMapReset(HgMap<K, V>* map)
{
    for (u32 i = 0; i < map->capacity; ++i)
    {
        map->hasVal[i] = false;
    }
    map->count = 0;
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
template<typename K, typename V, typename T = K, typename U = V>
V* hgMapAdd(HgMap<K, V>* map, const T& key, const U& val)
{
    static_assert(std::is_convertible_v<T, K> && std::is_convertible_v<U, V>);
    K k = (K)key;
    V v = (V)val;

    hgAssert(map->count < map->capacity - 1);

    u32 idx = hgHash(k) % map->capacity;
    for (u32 dist = 0; map->hasVal[idx] && map->keys[idx] != k; ++dist)
    {
        u32 otherDist = hgHash(map->keys[idx]) % map->capacity - idx;
        if (otherDist > map->capacity)
            otherDist += map->capacity;

        if (otherDist < dist)
        {
            K keyTmp = map->keys[idx];
            V valTmp = map->vals[idx];
            map->keys[idx] = k;
            map->vals[idx] = v;
            k = keyTmp;
            v = valTmp;
            dist = otherDist;
        }

        idx = (idx + 1) % map->capacity;
    }

    map->hasVal[idx] = true;
    map->keys[idx] = k;
    map->vals[idx] = v;
    ++map->count;

    return map->vals + idx;
}

/**
 * Remove a key-value pair from the hash map, and stores it
 *
 * Parameters
 * - key The key to remove
 * - val A pointer to store the value, if found
 *
 * Returns
 * - Whether a value was found and stored in value
 */
template<typename K, typename V, typename T = K>
bool hgMapRemove(HgMap<K, V>* map, const T& key, V* val = nullptr)
{
    static_assert(std::is_convertible_v<T, K>);
    K k = (K)key;

    u32 idx = hgHash(k) % map->capacity;
    while (map->hasVal[idx])
    {
        if (map->keys[idx] == k)
            break;
        idx = (idx + 1) % map->capacity;
    }
    if (!map->hasVal[idx])
        return false;

    if (val != nullptr)
        *val = map->vals[idx];

    u32 next = (idx + 1) % map->capacity;
    while (map->hasVal[next])
    {
        if (hgHash(map->keys[next]) % map->capacity != next)
        {
            map->keys[idx] = map->keys[next];
            map->vals[idx] = map->vals[next];
            idx = next;
        }
        next = (next + 1)  % map->capacity;
    }
    map->hasVal[idx] = false;
    --map->count;

    return true;
}

/**
 * Gets the value stored at a key
 *
 * Returns
 * - A pointer to the value, or nullptr if it does not exist
 */
template<typename K, typename V, typename T = K>
V* hgMapGet(const HgMap<K, V>* map, const T& key)
{
    static_assert(std::is_convertible_v<T, K>);
    K k = (K)key;

    for (u32 idx = hgHash(key) % map->capacity; map->hasVal[idx]; idx = (idx + 1) % map->capacity)
    {
        if (map->keys[idx] == k)
            return map->vals + idx;
    }
    return nullptr;
}

/**
 * A high precision clock for timers and game deltas
 */
struct HgClock {
    /**
     * The begin time
     */
    f64 time;
};

/**
 * Resets the clock
 *
 * Returns
 * - The time in seconds since the last tick
 */
f64 hgClockTick(HgClock* clock);

/**
 * A simple performance measurement tool
 */
struct HgPerf {
    /**
     * The clock to keep track of each time
     */
    HgClock clock;
    /**
     * The measured time for each iteration
     */
    f64* times;
    /**
     * The max number of measurements
     */
    u32 count;
    /**
     * The current measurement
     */
    u32 current;
};

/**
 * Create a performance measurer
 */
HgPerf hgPerfCreate(HgArena* arena, u32 count);

/**
 * Begin the timer for a measurement
 */
void hgPerfBegin(HgPerf* perf);

/**
 * End the timer for a measurement
 *
 * Returns
 * - The time this measurement took
 */
f64 hgPerfEnd(HgPerf* perf);

/**
 * A set of statistics from performance measurements
 */
struct HgPerfStats {
    /**
     * The average time of all measurements
     */
    f64 avg;
    /**
     * The best case (the shortest time)
     */
    f64 best;
    /**
     * The worst case (the longest time)
     */
    f64 worst;
};

/**
 * Analyzes the performance measurements for statistics
 */
HgPerfStats hgPerfAnalyze(const HgPerf* perf);

/**
 * The scale to log performance metrics at
 */
enum HgPerfScale {
    HgPerfScale_seconds,
    HgPerfScale_milli,
    HgPerfScale_micro,
    HgPerfScale_nano,
};

/**
 * Logs performance statistics to stdout
 */
void hgPerfLog(HgStringView title, const HgPerfStats* stats, HgPerfScale scale);

/**
 * Returns the number of concurrent threads available in hardware
 */
u32 hgHardwareThreadCount();

/**
 * A spinlock fence for basic thread synchronization
 */
struct HgFence {
    /**
     * The handle to the fence
     */
    HgHandle handle;
};

/**
 * Initialize fences
 */
void hgFencesInit(HgArena* arena, u32 maxFences);

/**
 * Deinitialize fences
 */
void hgFencesDeinit();

/**
 * Create a new fence
 */
HgFence hgFenceCreate();

/**
 * Destroy a fence
 */
void hgFenceDestroy(HgFence fence);

/**
 * Add more events for the fence to wait on
 *
 * Parameters
 * - fence The fence to attach to
 * - count The number of added events
 */
void hgFenceAttach(HgFence fence, u32 count);

/**
 * Signal that events have completed
 *
 * Parameters
 * - fence The fence to signal
 * - count The number of signaled events
 */
void hgFenceSignal(HgFence fence, u32 count);

/**
 * Returns whether all work has been completed
 */
bool hgFenceIsComplete(HgFence fence);

/**
 * Spin waits for all work submissions to be completed
 */
bool hgFenceWait(HgFence fence, f64 timeoutSeconds);

/**
 * Spin waits for all work submissions to be completed
 */
void hgFenceWaitIndefinite(HgFence fence);

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
void hgThreadsInit(HgArena* arena, u32 queueSize, u32 threadCount);

/**
 * Deinitialize the thread pool
 */
void hgThreadsDeinit();

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
bool hgThreadsHelp(HgFence fence, f64 timeout);

/**
 * Pushes work to the thread pool queue to be executed
 *
 * Parameters
 * - fence The fences to signal upon completion
 * - data The data passed to the function
 * - work The function to be executed
 */
void hgThreadsCall(HgFence fence, void* data, void (*fn)(void* data));

/**
 * Iterates in parallel over a function n times using the thread pool
 *
 * Note, uses a fence internally to wait for all work to complete
 *
 * Parameters
 * - begin The first index to iterate from
 * - end The end index to iterate to
 * - data The data pointer passed to fn
 * - fn The function to use to iterate, takes begin and end indicces
 */
void hgThreadsFor(u64 begin, u64 end, void* data, void (*fn)(void* data, u64 idx));

/**
 * Initializes the IO thread
 *
 * Parameters
 * - arena The arena to allocate from
 * - queueSize The max concurrent request capacity
 */
void hgIoInit(HgArena* arena, u32 queueSize);

/**
 * Deinitializes the IO thread
 */
void hgIoDeinit();

/**
 * Make an asynchronous IO request on the IO thread
 *
 * Parameters
 * - fence The fence to signal on completion
 * - data The data pointer passed to fn
 * - path The path string passed to fn
 * - fn The function to execute
 */
void hgIoRequest(HgFence fence, void* data, HgStringView path, void (*fn)(void* data, HgStringView path));

/**
 * Gpu init config
 */
struct HgGpuInit {
    // u32 maxGpuBuffers = 512;
    // u32 maxGpuImages = 512;
    // u32 maxGpuView = 512;
    // u32 maxGpuPipelines = 512;

    u32 maxFramesInFlight = 2;
    u32 maxWindows = 8;
};

/**
 * Initializes the graphics subsystem, loading all global Vulkan resources
 */
void hgGpuInit(HgArena* arena, HgGpuInit* config);

/**
 * Deinitializes the graphics subsystem, unloading all global Vulkan resources
 */
void hgGpuDeinit();

/**
 * Wait for the GPU to finish work
 */
void hgGpuWaitIdle();

/**
 * Pixel formats
 */
enum HgFormat {
    HgFormat_undefined = 0,
    HgFormat_r4g4_unorm_pack8 = 1,
    HgFormat_r4g4b4a4_unorm_pack16 = 2,
    HgFormat_b4g4r4a4_unorm_pack16 = 3,
    HgFormat_r5g6b5_unorm_pack16 = 4,
    HgFormat_b5g6r5_unorm_pack16 = 5,
    HgFormat_r5g5b5a1_unorm_pack16 = 6,
    HgFormat_b5g5r5a1_unorm_pack16 = 7,
    HgFormat_a1r5g5b5_unorm_pack16 = 8,
    HgFormat_r8_unorm = 9,
    HgFormat_r8_snorm = 10,
    HgFormat_r8_uscaled = 11,
    HgFormat_r8_sscaled = 12,
    HgFormat_r8_uint = 13,
    HgFormat_r8_sint = 14,
    HgFormat_r8_srgb = 15,
    HgFormat_r8g8_unorm = 16,
    HgFormat_r8g8_snorm = 17,
    HgFormat_r8g8_uscaled = 18,
    HgFormat_r8g8_sscaled = 19,
    HgFormat_r8g8_uint = 20,
    HgFormat_r8g8_sint = 21,
    HgFormat_r8g8_srgb = 22,
    HgFormat_r8g8b8_unorm = 23,
    HgFormat_r8g8b8_snorm = 24,
    HgFormat_r8g8b8_uscaled = 25,
    HgFormat_r8g8b8_sscaled = 26,
    HgFormat_r8g8b8_uint = 27,
    HgFormat_r8g8b8_sint = 28,
    HgFormat_r8g8b8_srgb = 29,
    HgFormat_b8g8r8_unorm = 30,
    HgFormat_b8g8r8_snorm = 31,
    HgFormat_b8g8r8_uscaled = 32,
    HgFormat_b8g8r8_sscaled = 33,
    HgFormat_b8g8r8_uint = 34,
    HgFormat_b8g8r8_sint = 35,
    HgFormat_b8g8r8_srgb = 36,
    HgFormat_r8g8b8a8_unorm = 37,
    HgFormat_r8g8b8a8_snorm = 38,
    HgFormat_r8g8b8a8_uscaled = 39,
    HgFormat_r8g8b8a8_sscaled = 40,
    HgFormat_r8g8b8a8_uint = 41,
    HgFormat_r8g8b8a8_sint = 42,
    HgFormat_r8g8b8a8_srgb = 43,
    HgFormat_b8g8r8a8_unorm = 44,
    HgFormat_b8g8r8a8_snorm = 45,
    HgFormat_b8g8r8a8_uscaled = 46,
    HgFormat_b8g8r8a8_sscaled = 47,
    HgFormat_b8g8r8a8_uint = 48,
    HgFormat_b8g8r8a8_sint = 49,
    HgFormat_b8g8r8a8_srgb = 50,
    HgFormat_a8b8g8r8_unorm_pack32 = 51,
    HgFormat_a8b8g8r8_snorm_pack32 = 52,
    HgFormat_a8b8g8r8_uscaled_pack32 = 53,
    HgFormat_a8b8g8r8_sscaled_pack32 = 54,
    HgFormat_a8b8g8r8_uint_pack32 = 55,
    HgFormat_a8b8g8r8_sint_pack32 = 56,
    HgFormat_a8b8g8r8_srgb_pack32 = 57,
    HgFormat_a2r10g10b10_unorm_pack32 = 58,
    HgFormat_a2r10g10b10_snorm_pack32 = 59,
    HgFormat_a2r10g10b10_uscaled_pack32 = 60,
    HgFormat_a2r10g10b10_sscaled_pack32 = 61,
    HgFormat_a2r10g10b10_uint_pack32 = 62,
    HgFormat_a2r10g10b10_sint_pack32 = 63,
    HgFormat_a2b10g10r10_unorm_pack32 = 64,
    HgFormat_a2b10g10r10_snorm_pack32 = 65,
    HgFormat_a2b10g10r10_uscaled_pack32 = 66,
    HgFormat_a2b10g10r10_sscaled_pack32 = 67,
    HgFormat_a2b10g10r10_uint_pack32 = 68,
    HgFormat_a2b10g10r10_sint_pack32 = 69,
    HgFormat_r16_unorm = 70,
    HgFormat_r16_snorm = 71,
    HgFormat_r16_uscaled = 72,
    HgFormat_r16_sscaled = 73,
    HgFormat_r16_uint = 74,
    HgFormat_r16_sint = 75,
    HgFormat_r16_sfloat = 76,
    HgFormat_r16g16_unorm = 77,
    HgFormat_r16g16_snorm = 78,
    HgFormat_r16g16_uscaled = 79,
    HgFormat_r16g16_sscaled = 80,
    HgFormat_r16g16_uint = 81,
    HgFormat_r16g16_sint = 82,
    HgFormat_r16g16_sfloat = 83,
    HgFormat_r16g16b16_unorm = 84,
    HgFormat_r16g16b16_snorm = 85,
    HgFormat_r16g16b16_uscaled = 86,
    HgFormat_r16g16b16_sscaled = 87,
    HgFormat_r16g16b16_uint = 88,
    HgFormat_r16g16b16_sint = 89,
    HgFormat_r16g16b16_sfloat = 90,
    HgFormat_r16g16b16a16_unorm = 91,
    HgFormat_r16g16b16a16_snorm = 92,
    HgFormat_r16g16b16a16_uscaled = 93,
    HgFormat_r16g16b16a16_sscaled = 94,
    HgFormat_r16g16b16a16_uint = 95,
    HgFormat_r16g16b16a16_sint = 96,
    HgFormat_r16g16b16a16_sfloat = 97,
    HgFormat_r32_uint = 98,
    HgFormat_r32_sint = 99,
    HgFormat_r32_sfloat = 100,
    HgFormat_r32g32_uint = 101,
    HgFormat_r32g32_sint = 102,
    HgFormat_r32g32_sfloat = 103,
    HgFormat_r32g32b32_uint = 104,
    HgFormat_r32g32b32_sint = 105,
    HgFormat_r32g32b32_sfloat = 106,
    HgFormat_r32g32b32a32_uint = 107,
    HgFormat_r32g32b32a32_sint = 108,
    HgFormat_r32g32b32a32_sfloat = 109,
    HgFormat_r64_uint = 110,
    HgFormat_r64_sint = 111,
    HgFormat_r64_sfloat = 112,
    HgFormat_r64g64_uint = 113,
    HgFormat_r64g64_sint = 114,
    HgFormat_r64g64_sfloat = 115,
    HgFormat_r64g64b64_uint = 116,
    HgFormat_r64g64b64_sint = 117,
    HgFormat_r64g64b64_sfloat = 118,
    HgFormat_r64g64b64a64_uint = 119,
    HgFormat_r64g64b64a64_sint = 120,
    HgFormat_r64g64b64a64_sfloat = 121,
    HgFormat_b10g11r11_ufloat_pack32 = 122,
    HgFormat_e5b9g9r9_ufloat_pack32 = 123,
    HgFormat_d16_unorm = 124,
    HgFormat_x8_d24_unorm_pack32 = 125,
    HgFormat_d32_sfloat = 126,
    HgFormat_s8_uint = 127,
    HgFormat_d16_unorm_s8_uint = 128,
    HgFormat_d24_unorm_s8_uint = 129,
    HgFormat_d32_sfloat_s8_uint = 130,
    HgFormat_bc1_rgb_unorm_block = 131,
    HgFormat_bc1_rgb_srgb_block = 132,
    HgFormat_bc1_rgba_unorm_block = 133,
    HgFormat_bc1_rgba_srgb_block = 134,
    HgFormat_bc2_unorm_block = 135,
    HgFormat_bc2_srgb_block = 136,
    HgFormat_bc3_unorm_block = 137,
    HgFormat_bc3_srgb_block = 138,
    HgFormat_bc4_unorm_block = 139,
    HgFormat_bc4_snorm_block = 140,
    HgFormat_bc5_unorm_block = 141,
    HgFormat_bc5_snorm_block = 142,
    HgFormat_bc6h_ufloat_block = 143,
    HgFormat_bc6h_sfloat_block = 144,
    HgFormat_bc7_unorm_block = 145,
    HgFormat_bc7_srgb_block = 146,
    HgFormat_etc2_r8g8b8_unorm_block = 147,
    HgFormat_etc2_r8g8b8_srgb_block = 148,
    HgFormat_etc2_r8g8b8a1_unorm_block = 149,
    HgFormat_etc2_r8g8b8a1_srgb_block = 150,
    HgFormat_etc2_r8g8b8a8_unorm_block = 151,
    HgFormat_etc2_r8g8b8a8_srgb_block = 152,
    HgFormat_eac_r11_unorm_block = 153,
    HgFormat_eac_r11_snorm_block = 154,
    HgFormat_eac_r11g11_unorm_block = 155,
    HgFormat_eac_r11g11_snorm_block = 156,
    HgFormat_astc_4x4_unorm_block = 157,
    HgFormat_astc_4x4_srgb_block = 158,
    HgFormat_astc_5x4_unorm_block = 159,
    HgFormat_astc_5x4_srgb_block = 160,
    HgFormat_astc_5x5_unorm_block = 161,
    HgFormat_astc_5x5_srgb_block = 162,
    HgFormat_astc_6x5_unorm_block = 163,
    HgFormat_astc_6x5_srgb_block = 164,
    HgFormat_astc_6x6_unorm_block = 165,
    HgFormat_astc_6x6_srgb_block = 166,
    HgFormat_astc_8x5_unorm_block = 167,
    HgFormat_astc_8x5_srgb_block = 168,
    HgFormat_astc_8x6_unorm_block = 169,
    HgFormat_astc_8x6_srgb_block = 170,
    HgFormat_astc_8x8_unorm_block = 171,
    HgFormat_astc_8x8_srgb_block = 172,
    HgFormat_astc_10x5_unorm_block = 173,
    HgFormat_astc_10x5_srgb_block = 174,
    HgFormat_astc_10x6_unorm_block = 175,
    HgFormat_astc_10x6_srgb_block = 176,
    HgFormat_astc_10x8_unorm_block = 177,
    HgFormat_astc_10x8_srgb_block = 178,
    HgFormat_astc_10x10_unorm_block = 179,
    HgFormat_astc_10x10_srgb_block = 180,
    HgFormat_astc_12x10_unorm_block = 181,
    HgFormat_astc_12x10_srgb_block = 182,
    HgFormat_astc_12x12_unorm_block = 183,
    HgFormat_astc_12x12_srgb_block = 184,
    HgFormat_g8b8g8r8_422_unorm = 1000156000,
    HgFormat_b8g8r8g8_422_unorm = 1000156001,
    HgFormat_g8_b8_r8_3plane_420_unorm = 1000156002,
    HgFormat_g8_b8r8_2plane_420_unorm = 1000156003,
    HgFormat_g8_b8_r8_3plane_422_unorm = 1000156004,
    HgFormat_g8_b8r8_2plane_422_unorm = 1000156005,
    HgFormat_g8_b8_r8_3plane_444_unorm = 1000156006,
    HgFormat_r10x6_unorm_pack16 = 1000156007,
    HgFormat_r10x6g10x6_unorm_2pack16 = 1000156008,
    HgFormat_r10x6g10x6b10x6a10x6_unorm_4pack16 = 1000156009,
    HgFormat_g10x6b10x6g10x6r10x6_422_unorm_4pack16 = 1000156010,
    HgFormat_b10x6g10x6r10x6g10x6_422_unorm_4pack16 = 1000156011,
    HgFormat_g10x6_b10x6_r10x6_3plane_420_unorm_3pack16 = 1000156012,
    HgFormat_g10x6_b10x6r10x6_2plane_420_unorm_3pack16 = 1000156013,
    HgFormat_g10x6_b10x6_r10x6_3plane_422_unorm_3pack16 = 1000156014,
    HgFormat_g10x6_b10x6r10x6_2plane_422_unorm_3pack16 = 1000156015,
    HgFormat_g10x6_b10x6_r10x6_3plane_444_unorm_3pack16 = 1000156016,
    HgFormat_r12x4_unorm_pack16 = 1000156017,
    HgFormat_r12x4g12x4_unorm_2pack16 = 1000156018,
    HgFormat_r12x4g12x4b12x4a12x4_unorm_4pack16 = 1000156019,
    HgFormat_g12x4b12x4g12x4r12x4_422_unorm_4pack16 = 1000156020,
    HgFormat_b12x4g12x4r12x4g12x4_422_unorm_4pack16 = 1000156021,
    HgFormat_g12x4_b12x4_r12x4_3plane_420_unorm_3pack16 = 1000156022,
    HgFormat_g12x4_b12x4r12x4_2plane_420_unorm_3pack16 = 1000156023,
    HgFormat_g12x4_b12x4_r12x4_3plane_422_unorm_3pack16 = 1000156024,
    HgFormat_g12x4_b12x4r12x4_2plane_422_unorm_3pack16 = 1000156025,
    HgFormat_g12x4_b12x4_r12x4_3plane_444_unorm_3pack16 = 1000156026,
    HgFormat_g16b16g16r16_422_unorm = 1000156027,
    HgFormat_b16g16r16g16_422_unorm = 1000156028,
    HgFormat_g16_b16_r16_3plane_420_unorm = 1000156029,
    HgFormat_g16_b16r16_2plane_420_unorm = 1000156030,
    HgFormat_g16_b16_r16_3plane_422_unorm = 1000156031,
    HgFormat_g16_b16r16_2plane_422_unorm = 1000156032,
    HgFormat_g16_b16_r16_3plane_444_unorm = 1000156033,
    HgFormat_g8_b8r8_2plane_444_unorm = 1000330000,
    HgFormat_g10x6_b10x6r10x6_2plane_444_unorm_3pack16 = 1000330001,
    HgFormat_g12x4_b12x4r12x4_2plane_444_unorm_3pack16 = 1000330002,
    HgFormat_g16_b16r16_2plane_444_unorm = 1000330003,
    HgFormat_a4r4g4b4_unorm_pack16 = 1000340000,
    HgFormat_a4b4g4r4_unorm_pack16 = 1000340001,
    HgFormat_astc_4x4_sfloat_block = 1000066000,
    HgFormat_astc_5x4_sfloat_block = 1000066001,
    HgFormat_astc_5x5_sfloat_block = 1000066002,
    HgFormat_astc_6x5_sfloat_block = 1000066003,
    HgFormat_astc_6x6_sfloat_block = 1000066004,
    HgFormat_astc_8x5_sfloat_block = 1000066005,
    HgFormat_astc_8x6_sfloat_block = 1000066006,
    HgFormat_astc_8x8_sfloat_block = 1000066007,
    HgFormat_astc_10x5_sfloat_block = 1000066008,
    HgFormat_astc_10x6_sfloat_block = 1000066009,
    HgFormat_astc_10x8_sfloat_block = 1000066010,
    HgFormat_astc_10x10_sfloat_block = 1000066011,
    HgFormat_astc_12x10_sfloat_block = 1000066012,
    HgFormat_astc_12x12_sfloat_block = 1000066013,
    HgFormat_a1b5g5r5_unorm_pack16 = 1000470000,
    HgFormat_a8_unorm = 1000470001,
    HgFormat_pvrtc1_2bpp_unorm_block_img = 1000054000,
    HgFormat_pvrtc1_4bpp_unorm_block_img = 1000054001,
    HgFormat_pvrtc2_2bpp_unorm_block_img = 1000054002,
    HgFormat_pvrtc2_4bpp_unorm_block_img = 1000054003,
    HgFormat_pvrtc1_2bpp_srgb_block_img = 1000054004,
    HgFormat_pvrtc1_4bpp_srgb_block_img = 1000054005,
    HgFormat_pvrtc2_2bpp_srgb_block_img = 1000054006,
    HgFormat_pvrtc2_4bpp_srgb_block_img = 1000054007,
    HgFormat_r8_bool_arm = 1000460000,
    HgFormat_r16g16_sfixed5_nv = 1000464000,
    HgFormat_r10x6_uint_pack16_arm = 1000609000,
    HgFormat_r10x6g10x6_uint_2pack16_arm = 1000609001,
    HgFormat_r10x6g10x6b10x6a10x6_uint_4pack16_arm = 1000609002,
    HgFormat_r12x4_uint_pack16_arm = 1000609003,
    HgFormat_r12x4g12x4_uint_2pack16_arm = 1000609004,
    HgFormat_r12x4g12x4b12x4a12x4_uint_4pack16_arm = 1000609005,
    HgFormat_r14x2_uint_pack16_arm = 1000609006,
    HgFormat_r14x2g14x2_uint_2pack16_arm = 1000609007,
    HgFormat_r14x2g14x2b14x2a14x2_uint_4pack16_arm = 1000609008,
    HgFormat_r14x2_unorm_pack16_arm = 1000609009,
    HgFormat_r14x2g14x2_unorm_2pack16_arm = 1000609010,
    HgFormat_r14x2g14x2b14x2a14x2_unorm_4pack16_arm = 1000609011,
    HgFormat_g14x2_b14x2r14x2_2plane_420_unorm_3pack16_arm = 1000609012,
    HgFormat_g14x2_b14x2r14x2_2plane_422_unorm_3pack16_arm = 1000609013,
};

/**
 * Turns a HgFormat into the size in bytes
 *
 * Parameters
 * - format The format to get the size of
 *
 * Returns
 * - The size of the format in bytes
 */
u32 hgFormatToSize(HgFormat format);

// Vulkan allocator : TODO?

/**
 * Where in the pipeline a resource can be accessed
 */
enum HgGpuStage {
    HgGpuStage_none = 0,
    HgGpuStage_topOfPipe = 0x00000001,
    HgGpuStage_drawIndirect = 0x00000002,
    HgGpuStage_vertexInput = 0x00000004,
    HgGpuStage_vertexShader = 0x00000008,
    HgGpuStage_tessellationControlShader = 0x00000010,
    HgGpuStage_tessellationEvaluationShader = 0x00000020,
    HgGpuStage_geometryShader = 0x00000040,
    HgGpuStage_fragmentShader = 0x00000080,
    HgGpuStage_earlyFragmentTests = 0x00000100,
    HgGpuStage_lateFragmentTests = 0x00000200,
    HgGpuStage_colorAttachmentOutput = 0x00000400,
    HgGpuStage_computeShader = 0x00000800,
    HgGpuStage_transfer = 0x00001000,
    HgGpuStage_bottomOfPipe = 0x00002000,
    HgGpuStage_host = 0x00004000,
    HgGpuStage_allGraphics = 0x00008000,
    HgGpuStage_allCommands = 0x00010000,
};
typedef u32 HgGpuStageFlags;

/**
 * How a resource can be accessed
 */
enum HgGpuAccess {
    HgGpuAccess_none = 0,
    HgGpuAccess_indirectCommandRead = 0x00000001,
    HgGpuAccess_indexRead = 0x00000002,
    HgGpuAccess_vertexAttributeRead = 0x00000004,
    HgGpuAccess_uniformRead = 0x00000008,
    HgGpuAccess_inputAttachmentRead = 0x00000010,
    HgGpuAccess_shaderRead = 0x00000020,
    HgGpuAccess_shaderWrite = 0x00000040,
    HgGpuAccess_colorAttachmentRead = 0x00000080,
    HgGpuAccess_colorAttachmentWrite = 0x00000100,
    HgGpuAccess_depthStencilAttachmentRead = 0x00000200,
    HgGpuAccess_depthStencilAttachmentWrite = 0x00000400,
    HgGpuAccess_transferRead = 0x00000800,
    HgGpuAccess_transferWrite = 0x00001000,
    HgGpuAccess_hostRead = 0x00002000,
    HgGpuAccess_hostWrite = 0x00004000,
    HgGpuAccess_memoryRead = 0x00008000,
    HgGpuAccess_memoryWrite = 0x00010000,
};
typedef u32 HgGpuAccessFlags;

/**
 * A gpu buffer
 */
struct HgGpuBuffer;

/**
 * How a gpu buffer will be used
 */
enum HgGpuBufferUsage {
    HgGpuBufferUsage_transferSrc = 0x00000001,
    HgGpuBufferUsage_transferDst = 0x00000002,
    HgGpuBufferUsage_uniformTexelBuffer = 0x00000004,
    HgGpuBufferUsage_storageTexelBuffer = 0x00000008,
    HgGpuBufferUsage_uniformBuffer = 0x00000010,
    HgGpuBufferUsage_storageBuffer = 0x00000020,
    HgGpuBufferUsage_indexBuffer = 0x00000040,
    HgGpuBufferUsage_vertexBuffer = 0x00000080,
    HgGpuBufferUsage_indirectBuffer = 0x00000100,
};
typedef u32 HgGpuBufferUsageFlags;

/**
 * How a gpu buffer will be accessed
 */
enum HgGpuMemoryUsage {
    /**
     * It will only be accessed from the device
     */
    HgGpuMemoryUsage_deviceOnly = 0,
    /**
     * It will be used as a staging buffer to transfer from host to device
     */
    HgGpuMemoryUsage_stagingWrite = 1,
    /**
     * It will be used as a staging buffer to transfer from device to host
     */
    HgGpuMemoryUsage_stagingRead = 2,
    /**
     * It will be frequently written from the host and read on the device
     */
    HgGpuMemoryUsage_frequentUpdate = 3,
};

/**
 * How a gpu buffer can be accessed
 */
enum HgGpuMemoryHostAccess {
    /**
     * The buffer cannot be accessed by the host
     */
    HgGpuMemoryHostAccess_none = 0x0,
    /**
     * The buffer can be written to by the host
     */
    HgGpuMemoryHostAccess_write = 0x1,
    /**
     * The buffer can be read from by the host
     */
    HgGpuMemoryHostAccess_read = 0x2,
};

/**
 * Create a gpu buffer
 *
 * Parameters
 * - size The size in bytes of the buffer
 * - usageFlags How the buffer will be used
 * - access How the buffer should be accessed
 */
HgGpuBuffer* hgGpuBufferCreate(
    u64 size,
    HgGpuBufferUsageFlags usageFlags,
    HgGpuMemoryUsage access = HgGpuMemoryUsage_deviceOnly);

/**
 * Destroy a gpu buffer
 */
void hgGpuBufferDestroy(HgGpuBuffer* buffer);

/**
 * Writes to a gpu buffer
 *
 * Parameters
 * - dst The buffer to write to, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - src The data to write, must not be nullptr
 * - size The size in bytes to write
 */
void hgGpuBufferWrite(HgGpuBuffer* dst, u64 offset, const void* src, u64 size);

/**
 * Reads from a Vulkan device local buffer through a staging buffer
 *
 * Parameters
 * - dst The location to write to, must not be nullptr
 * - src The buffer to read from, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - size The size in bytes to read
 */
void hgGpuBufferRead(void* dst, HgGpuBuffer* src, u64 offset, u64 size);

/**
 * A gpu image
 */
struct HgGpuImage;

/**
 * How an image will be used
 */
enum HgGpuImageUsage {
    HgGpuImageUsage_transferSrc = 0x00000001,
    HgGpuImageUsage_transferDst = 0x00000002,
    HgGpuImageUsage_sampled = 0x00000004,
    HgGpuImageUsage_storage = 0x00000008,
    HgGpuImageUsage_colorAttachment = 0x00000010,
    HgGpuImageUsage_depthStencilAttachment = 0x00000020,
    HgGpuImageUsage_transientAttachment = 0x00000040,
    HgGpuImageUsage_inputAttachment = 0x00000080,
    HgGpuImageUsage_hostTransfer = 0x00400000,
};
typedef u32 HgGpuImageUsageFlags;

/**
 * The layout of an image
 */
enum HgGpuLayout {
    HgGpuLayout_undefined = 0,
    HgGpuLayout_general = 1,
    HgGpuLayout_colorAttachment = 2,
    HgGpuLayout_depthStencilAttachment = 3,
    HgGpuLayout_depthStencilReadOnly = 4,
    HgGpuLayout_shaderReadOnly = 5,
    HgGpuLayout_transferSrc = 6,
    HgGpuLayout_transferDst = 7,
    HgGpuLayout_preinitialized = 8,
    HgGpuLayout_presentSrc = 1000001002,
};

/**
 * Create a gpu image assuming most defaults
 */
HgGpuImage* hgGpuImageCreate(u32 width, u32 height, HgFormat format, HgGpuImageUsageFlags usage);

/**
 * Config for hgCreateVkImage
 */
struct HgGpuImageCreateEx {
    /**
     * The dimensions of the image
     */
    u32 dimensions = 2;
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
     * The format of the image, must not be undefined
     */
    HgFormat format = HgFormat_undefined;
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
    u32 msaaSamples = 1;
    /**
     * How the image will be used, must not be 0
     */
    HgGpuImageUsageFlags usage = 0;
};

/**
 * Create a gpu image with more options
 */
HgGpuImage* hgGpuImageCreateEx(const HgGpuImageCreateEx* create);

/**
 * Destroy a gpu image
 */
void hgGpuImageDestroy(HgGpuImage* image);

/**
 * A view into a gpu image
 */
struct HgGpuView;

/**
 * The dimensionality of an image
 */
enum HgGpuViewType {
    HgGpuViewType_1D = 0,
    HgGpuViewType_2D = 1,
    HgGpuViewType_3D = 2,
    HgGpuViewType_cube = 3,
    HgGpuViewType_1DArray = 4,
    HgGpuViewType_2DArray = 5,
    HgGpuViewType_cubeArray = 6,
};

/**
 * The aspect the image will be accessed in
 */
enum HgGpuAspect {
    HgGpuAspect_none = 0,
    HgGpuAspect_color = 0x00000001,
    HgGpuAspect_depth = 0x00000002,
    HgGpuAspect_stencil = 0x00000004,
    HgGpuAspect_metadata = 0x00000008,
    HgGpuAspect_plane0 = 0x00000010,
    HgGpuAspect_plane1 = 0x00000020,
    HgGpuAspect_plane2 = 0x00000040,
};
typedef u32 HgGpuAspectFlags;

/**
 * Create a gpu image view
 */
HgGpuView* hgGpuViewCreate(
    HgGpuImage* image,
    u32 baseMipLevel,
    u32 levelCount,
    u32 baseArrayLayer,
    u32 layerCount,
    HgGpuAspectFlags aspectFlags,
    HgGpuViewType type = HgGpuViewType_2D);

/**
 * Destroy a gpu image view
 */
void hgGpuViewDestroy(HgGpuView* view);

/**
 * Write to a gpu image
 *
 * Note, only fills the base mip level
 *
 * Parameters
 * - dst The image to write to
 * - src The data to read from
 */
void hgGpuImageWrite(HgGpuView* dst, const void* src);

/**
 * Write to a gpu image cubemap
 *
 * Note, dst must have at least 6 array layers to fill
 *
 * Only the base mip level is filled
 *
 * srcData is assumed to be layed out as:
 *  #
 * ####
 *  #
 *
 * Parameters
 * - dst The image to write to
 * - subresource The subresource of the image to write to
 * - src The data to read from
 */
void hgGpuImageWriteCubemap(HgGpuView* dst, const void* src);

/**
 * Read from a gpu image
 *
 * Note, only the base mip level is read
 *
 * Parameters
 * - src The pointer to write to
 * - dst The image to read from
 * - subresource The subresource of the image to read from
 */
void hgGpuImageRead(void* dst, HgGpuView* src);

/**
 * Generates mipmaps from the base level
 *
 * Note, dst should only have 1 array layer
 *
 * Parameters
 * - image The image to generate mipmaps for
 */
void hgGpuImageGenMipmaps(HgGpuView* dst);

/**
 * A sampler to access an image view
 */
struct HgGpuSampler;

/**
 * How a sampler interpolates between pixels
 */
enum HgGpuFilter {
    HgGpuFilter_nearest = 0,
    HgGpuFilter_linear = 1,
};

/**
 * How a sampler samples off the image's edge
 */
enum HgGpuSamplerEdgeMode {
    HgGpuSamplerEdgeMode_modeRepeat = 0,
    HgGpuSamplerEdgeMode_modeMirroredRepeat = 1,
    HgGpuSamplerEdgeMode_modeClampToEdge = 2,
    HgGpuSamplerEdgeMode_modeClampToBorder = 3,
    HgGpuSamplerEdgeMode_modeMirrorClampToEdge = 4,
};

/**
 * The border color if the sampler edge mode has a border
 */
enum HgSamplerBorderColor {
    HgGpuSamplerBorder_floatTransparentBlack = 0,
    HgGpuSamplerBorder_intTransparentBlack = 1,
    HgGpuSamplerBorder_floatOpaqueBlack = 2,
    HgGpuSamplerBorder_intOpaqueBlack = 3,
    HgGpuSamplerBorder_floatOpaqueWhite = 4,
    HgGpuSamplerBorder_intOpaqueWhite = 5,
};

/**
 * Create a Vulkan sampler
 *
 * Parameters
 * - filter How the sampler interpolates between image values
 * - addressMode How the sampler handles address off edges
 * - borderColor The border color if addressMode uses a border
 */
HgGpuSampler* hgGpuSamplerCreate(
    HgGpuFilter filter,
    HgGpuSamplerEdgeMode addressMode = HgGpuSamplerEdgeMode_modeRepeat,
    HgSamplerBorderColor borderColor = HgGpuSamplerBorder_floatTransparentBlack);

/**
 * Destroy a Vulkan sampler
 */
void hgGpuSamplerDestroy(HgGpuSampler* sampler);

/**
 * A gpu resource descriptor
 */
struct HgGpuDescriptor {
    /**
     * The descriptor id, defaults to null
     */
    u32 id = (u32)-1;
};

/**
 * Get the index from the id
 */
constexpr u32 hgGpuDescriptorIdx(HgGpuDescriptor desc)
{
    return desc.id & 0x0000ffff;
}

/**
 * Get the generation from the id
 */
constexpr u32 hgGpuDescriptorGeneration(HgGpuDescriptor desc)
{
    return (desc.id & 0x0fff0000) >> 16;
}

/**
 * The binding indices for the descriptor types in the bindless layout
 */
enum HgGpuDescriptorType {
    HgGpuDescriptorType_sampler = 0,
    HgGpuDescriptorType_combinedImageSampler = 1,
    HgGpuDescriptorType_sampledImage = 2,
    HgGpuDescriptorType_storageImage = 3,
    HgGpuDescriptorType_uniformTexelBuffer = 4,
    HgGpuDescriptorType_storageTexelBuffer = 5,
    HgGpuDescriptorType_uniformBuffer = 6,
    HgGpuDescriptorType_storageBuffer = 7,
    HgGpuDescriptorType_count,
};

/**
 * Get the descriptor type from the id
 */
constexpr HgGpuDescriptorType hgGpuDescriptorType(HgGpuDescriptor desc)
{
    return (HgGpuDescriptorType)((desc.id & 0xf0000000) >> 28);
}

/**
 * Create a new bindless descriptor
 */
HgGpuDescriptor hgGpuDescriptorCreate(HgGpuDescriptorType type);

/**
 * Destroy a bindless descriptor
 */
void hgGpuDescriptorDestroy(HgGpuDescriptor desc);

/**
 * The info to update a buffer descriptor
 */
struct HgGpuBufferDescriptorInfo {
    HgGpuBuffer* buffer;
    u64 offset;
    u64 range;
};

/**
 * The info to update an image descriptor
 */
struct HgGpuImageDescriptorInfo {
    HgGpuSampler* sampler;
    HgGpuView* imageView;
    HgGpuLayout imageLayout;
};

/**
 * Update a bindless descriptor
 *
 * Parameters
 * - descriptor The descriptor to update
 * - bufferInfo The buffer info, if the descriptor is a buffer type
 * - imageInfo The image info, if the descriptor is an image type
 */
void hgGpuDescriptorUpdate(
    HgGpuDescriptor descriptor,
    const HgGpuBufferDescriptorInfo* bufferInfo,
    const HgGpuImageDescriptorInfo* imageInfo);

/**
 * A shader pipeline
 */
struct HgGpuPipeline;

/**
 * A push constant range in a pipeline
 */
struct HgGpuPushRange {
    /**
     * The offset in bytes
     */
    u32 offset;
    /**
     * The size of the push in bytes
     */
    u32 size;
};

/**
 * A vertex binding description in a pipeline
 */
struct HgGpuVertexBinding {
    /**
     * The binding index
     */
    u32 binding;
    /**
     * The stride between vertices in bytes
     */
    u32 stride;
    /**
     * Whether the binding is instances or vertices
     */
    bool instanceRate;
};

/**
 * A vertex attribute description in a pipeline
 */
struct HgGpuVertexAttribute {
    /**
     * The location index
     */
    u32 location;
    /**
     * The vertex binding
     */
    u32 binding;
    /**
     * The format of the attribute
     */
    HgFormat format;
    /**
     * The offset into the binding vertex
     */
    u32 offset;
};

/**
 * How the vertex list is interpreted
 */
enum HgGpuTopology {
    HgGpuTopology_pointList = 0,
    HgGpuTopology_lineList = 1,
    HgGpuTopology_lineStrip = 2,
    HgGpuTopology_triangleList = 3,
    HgGpuTopology_triangleStrip = 4,
    HgGpuTopology_triangleFan = 5,
    HgGpuTopology_lineListWithAdjacency = 6,
    HgGpuTopology_lineStripWithAdjacency = 7,
    HgGpuTopology_triangleListWithAdjacency = 8,
    HgGpuTopology_triangleStripWithAdjacency = 9,
    HgGpuTopology_patchList = 10,
};

/**
 * How to treat vertices
 */
enum HgGpuPolygonMode {
    HgGpuPolygonMode_fill = 0,
    HgGpuPolygonMode_line = 1,
    HgGpuPolygonMode_point = 2,
};

enum HgGpuCull {
    HgGpuCull_none = 0,
    HgGpuCull_front = 0x00000001,
    HgGpuCull_back = 0x00000002,
    HgGpuCull_both = 0x00000003,
};
typedef u32 HgGpuCullFlags;

/**
 * Config for hgCreateGraphicsPipeline
 */
struct HgCreateGpuGraphicsPipeline {
    /**
     * The vertex shader code
     */
    const u8* vertexShader = nullptr;
    /**
     * The size in bytes of the vertex shader code
     */
    u64 vertexShaderSize = 0;
    /**
     * The fragment shader code
     */
    const u8* fragmentShader = nullptr;
    /**
     * The size in bytes of the fragment shader code
     */
    u64 fragmentShaderSize = 0;
    /**
     * The format of the color attachments, none can be UNDEFINED
     */
    const HgFormat* colorAttachmentFormats = nullptr;
    /**
     * The number of color attachment formats
     */
    u32 colorAttachmentCount = 0;
    /**
     * The format of the depth attachment, no depth attachment if UNDEFINED
     */
    HgFormat depthAttachmentFormat = HgFormat_undefined;
    /**
     * The format of the stencil attachment, no stencil attachment if UNDEFINED
     */
    HgFormat stencilAttachmentFormat = HgFormat_undefined;
    /**
     * The push constant ranges, if any
     */
    const HgGpuPushRange* pushRanges = nullptr;
    /**
     * The number of push constant ranges
     */
    u32 pushRangeCount;
    /**
     * Descriptions of the vertex bindings, may be nullptr
     */
    const HgGpuVertexBinding* vertexBindings = nullptr;
    /**
     * The number of vertex bindings
     */
    u32 vertexBindingCount = 0;
    /**
     * Descriptions of the vertex attributes, may be nullptr
     */
    const HgGpuVertexAttribute* vertexAttributes = nullptr;
    /**
     * The number of vertex attributes
     */
    u32 vertexAttributeCount = 0;
    /**
     * How to interpret vertices into topology
     */
    HgGpuTopology topology = HgGpuTopology_triangleList;
    /**
     * The number of patch control points in the tesselation stage
     */
    u32 tesselationPatchControlPoints = 0;
    /**
     * How polygons are drawn
     */
    HgGpuPolygonMode polygonMode = HgGpuPolygonMode_fill;
    /**
     * Enables back/front face culling
     */
    HgGpuCullFlags cullMode = HgGpuCull_none;
    /**
     * How many samples are used in MSAA
     */
    u32 multisampleCount = 1;
    /**
     * Enables culling fragments by comparing to a depth buffer
     */
    bool enableDepthRead = false;
    /**
     * Enables writing the depth to the depth buffer
     */
    bool enableDepthWrite = false;
    /**
     * Enables color blending using pixel alpha values for each color attachment
     */
    const bool* colorBlendEnables = nullptr;
};

/**
 * Create a graphics pipeline
 *
 * Parameters
 * - config The pipeline configuration
 */
HgGpuPipeline* hgGpuPipelineCreateGraphics(const HgCreateGpuGraphicsPipeline* config);

/**
 * Create a compute pipeline
 *
 * Parameters
 * - pushSize The size of the push constant
 * - shaderCode The compute shader, must not be nullptr
 * - shaderCodeSize The size in bytes of shaderCode
 */
HgGpuPipeline* hgGpuPipelineCreateCompute(u32 pushSize, const u8* shaderCode, u64 shaderCodeSize);

/**
 * Destroy a graphics or compute pipeline
 */
void hgGpuPipelineDestroy(HgGpuPipeline* pipeline);

/**
 * A gpu command buffer
 */
struct HgGpuCmd;

/**
 * Begin a command buffer to be executed once
 *
 * Returns
 * - The command buffer to record, will never be nullptr
 */
HgGpuCmd* hgGpuCmdBegin();

/**
 * Execute the command buffer and wait for completion
 *
 * Parameters
 * - cmd The command buffer from hgBeginGpuCommands, must not be nullptr
 */
void hgGpuCmdEnd(HgGpuCmd* cmd);

/**
 * Bind a graphics or compute pipeline
 */
void hgGpuBindPipeline(HgGpuCmd* cmd, HgGpuPipeline* pipeline);

/**
 * Push constants to the shader
 *
 * Parameters
 * - cmd The command buffer to record to
 * - pipeline The pipeline to push to
 * - offset The offset into the push range
 * - size The size of the data
 * - push The data to push
 */
void hgGpuPushConstants(HgGpuCmd* cmd, HgGpuPipeline* pipeline, u32 offset, void* push, u32 size);

/**
 * Bind an index buffer (assumed 32 bit)
 *
 * Parameters
 * - cmd The command buffer to record to
 * - buffer The buffer to bind
 * - offset The offset into the buffer
 */
void hgGpuBindIndexBuffer(HgGpuCmd* cmd, HgGpuBuffer* buffer, u64 offset = 0);

/**
 * Bind vertex buffers
 *
 * Parameters
 * - cmd The command buffer to record to
 * - bindingIdx The first binding index to bind to
 * - buffers The buffer to bind
 * - offsets The offsets into the buffers
 * - bufferCount The number of buffers
 */
void hgGpuBindVertexBuffers(HgGpuCmd* cmd, u32 bindingIdx, HgGpuBuffer** buffers, u64* offsets, u32 bufferCount);

/**
 * Issue a draw call
 *
 * Parameters
 * - cmd The command buffer to record to
 * - vertexBegin The index of the first vertex to draw
 * - vertexCount The number of vertices to draw
 * - instanceBegin The index of the first instance to draw
 * - instanceCount The number of instances to draw
 */
void hgGpuDraw(HgGpuCmd* cmd, u32 vertexBegin, u32 vertexCount, u32 instanceBegin, u32 instanceCount);

/**
 * Issue a draw call using an index buffer
 *
 * Parameters
 * - cmd The command buffer to record to
 * - vertexOffset The offset added to the indices in the index buffer
 * - indexBegin The index of the first index to draw
 * - indexCount The number of indices to draw
 * - instanceBegin The index of the first instance to draw
 * - instanceCount The number of instances to draw
 */
void hgGpuDrawIndexed(
    HgGpuCmd* cmd,
    i32 vertexOffset,
    u32 indexBegin,
    u32 indexCount,
    u32 instanceBegin,
    u32 instanceCount);

/**
 * Dispatch a compute shader
 *
 * Parameters
 * - cmd The command buffer to record to
 * - groupCountX The number of workgroups in the x dimension
 * - groupCountY The number of workgroups in the y dimension
 * - groupCountZ The number of workgroups in the z dimension
 */
void hgGpuCompute(HgGpuCmd* cmd, u32 groupCountX, u32 groupCountY, u32 groupCountZ);

/**
 * An image dependency barrier
 */
struct HgGpuImageBarrier {
    /**
     * The image to sychronize
     */
    HgGpuView* image;
    /**
     * Where the image will be used next
     */
    HgGpuStageFlags nextStage;
    /**
     * How the image will be accessed next
     */
    HgGpuAccessFlags nextAccess;
    /**
     * The next layout the image needs to be in
     */
    HgGpuLayout nextLayout;
};

/**
 * A buffer dependency barrier
 */
struct HgGpuBufferBarrier {
    /**
     * The buffer to sychronize
     */
    HgGpuBuffer* buffer;
    /**
     * Where the image will be used next
     */
    HgGpuStageFlags nextStage;
    /**
     * How the image will be accessed next
     */
    HgGpuAccessFlags nextAccess;
};

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
void hgGpuMemoryBarrier(
    HgGpuCmd* cmd,
    const HgGpuBufferBarrier* bufferBarriers,
    u32 bufferBarrierCount,
    const HgGpuImageBarrier* imageBarriers,
    u32 imageBarrierCount);

/**
 * A compute pass description
 */
struct HgGpuComputePass {
    /**
     * The uniforms buffer dependencies
     */
    HgGpuBuffer** uniformBuffers = nullptr;
    /**
     * The number of uniform buffers
     */
    u32 uniformBufferCount = 0;
    /**
     * The storage buffer dependencies
     */
    HgGpuBuffer** storageBuffers = nullptr;
    /**
     * The number of storage buffers
     */
    u32 storageBufferCount = 0;
    /**
     * The sampled image dependencies
     */
    HgGpuView** sampledImages = nullptr;
    /**
     * The number of sampled images
     */
    u32 sampledImageCount = 0;
    /**
     * The storage image dependencies
     */
    HgGpuView** storageImages = nullptr;
    /**
     * The number of storage images
     */
    u32 storageImageCount = 0;
};

/**
 * Performs memory barriers for compute shader resources
 *
 * Parameters
 * - cmd The command buffer
 * - pass The compute pass description
 */
void hgGpuComputePass(HgGpuCmd* cmd, const HgGpuComputePass* pass);

/**
 * The operation to load a render attachment
 */
enum HgGpuLoadOp {
    HgGpuLoadOp_load = 0,
    HgGpuLoadOp_clear = 1,
    HgGpuLoadOp_dontCare = 2,
};

/**
 * The operation to store a render attachment
 */
enum HgGpuStoreOp {
    HgGpuStoreOp_store = 0,
    HgGpuStoreOp_dontCare = 1,
};

/**
 * The value to clear color attachments to
 */
union HgGpuClearValueVolor {
    /**
     * The value as f32
     */
    f32 float32[4];
    /**
     * The value as i32
     */
    i32 int32[4];
    /**
     * The value as u32
     */
    u32 uint32[4];
};

/**
 * The value to clear depth and stencil attachments to
 */
struct HgGpuClearValueDepthStencil {
    /**
     * The depth value
     */
    f32 depth;
    /**
     * The stencil value
     */
    u32 stencil;
};

/**
 * The value to clear a render attachment to
 */
union HgGpuClearValue {
    /**
     * The value for color attachments
     */
    HgGpuClearValueVolor color;
    /**
     * The value for depth and stencil attachments
     */
    HgGpuClearValueDepthStencil depthStencil;
};

/**
 * A rendering attachment
 */
struct HgGpuRenderAttachment {
    /**
     * The image attached
     */
    HgGpuView* image = nullptr;
    /**
     * How the image will be loaded
     */
    HgGpuLoadOp loadOp = HgGpuLoadOp_clear;
    /**
     * How the image will be stored
     */
    HgGpuStoreOp storeOp = HgGpuStoreOp_store;
    /**
     * What to clear the image to if cleared
     */
    HgGpuClearValue clearValue = {};
};

/**
 * A render pass description
 */
struct HgGpuRenderPass {
    /**
     * The uniforms buffer dependencies
     */
    HgGpuBuffer** uniformBuffers = nullptr;
    /**
     * The number of uniform buffers
     */
    u32 uniformBufferCount = 0;
    /**
     * The storage buffer dependencies
     */
    HgGpuBuffer** storageBuffers = nullptr;
    /**
     * The number of storage buffers
     */
    u32 storageBufferCount = 0;
    /**
     * The sampled image dependencies
     */
    HgGpuView** sampledImages = nullptr;
    /**
     * The number of sampled images
     */
    u32 sampledImageCount = 0;
    /**
     * The storage image dependencies
     */
    HgGpuView** storageImages = nullptr;
    /**
     * The number of storage images
     */
    u32 storageImageCount = 0;
    /**
     * The color images to write to
     */
    const HgGpuRenderAttachment* colorAttachments = nullptr;
    /**
     * The number of color attachments
     */
    u32 colorAttachmentCount = 0;
    /**
     * The number of layers in each color attachment to write to
     */
    u32 layerCount = 1;
    /**
     * The depth attachment, if any
     */
    const HgGpuRenderAttachment* depthAttachment = nullptr;
    /**
     * The stencil attachment, if any
     */
    const HgGpuRenderAttachment* stencilAttachment = nullptr;
};

/**
 * Performs render barrier and begins a render pass
 *
 * Parameters
 * - cmd The command buffer
 * - width The width of the render area
 * - height The height of the render area
 * - pass The render pass description
 */
void hgGpuRenderPassBegin(HgGpuCmd* cmd, u32 width, u32 height, const HgGpuRenderPass* pass);

/**
 * Ends the render pass
 *
 * Parameters
 * - cmd The command buffer
 */
void hgGpuRenderPassEnd(HgGpuCmd* cmd);

/**
 * Initializes global resources for windowing
 *
 * Parameters
 * - arena The arena to allocate from
 * - maxWindows The maximum number of windows that can be created
 * - maxEvents The maximum number of events recorded per frame
 */
void hgPlatformInit(HgArena* arena, u32 maxWindows, u32 maxEvents);

/**
 * Deinitializes global resources for windowing
 */
void hgPlatformDeinit();

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
u32 hgPlatformGetVulkanExtensions(HgArena* arena, HgStringView** extBuffer);

/**
 * The present mode for the swapchain
 */
enum HgGpuPresentMode {
    HgGpuPresentMode_immediate = 0,
    HgGpuPresentMode_mailbox = 1,
    HgGpuPresentMode_fifo = 2,
    HgGpuPresentMode_fifoRelaxed = 3,
};

/**
 * Configuration for a window
 */
struct HgWindowConfig {
    /**
     * Whether the window can be resized
     */
    bool fixedSize = false;
    /**
     * Whether the window should be windowed or fullscreen
     */
    bool fullscreen = false;
    /**
     * How the swapchain images will be presented
     *
     * Note, will fall back to FIFO if preferred is unavailable
     */
    HgGpuPresentMode preferredPresentMode = HgGpuPresentMode_fifo;
    /**
     * How the swapchain images will be used
     */
    HgGpuImageUsageFlags imageUsage = HgGpuImageUsage_colorAttachment;
};

/**
 * A window
 */
struct HgWindow;

/**
 * Create a new window
 *
 * Note, width and height are ignored if fullscreen is enabled
 */
HgWindow* hgWindowCreate(const char* title, u32 width, u32 height, const HgWindowConfig* config);

/**
 * Destroy a window
 */
void hgWindowDestroy(HgWindow* window);

/**
 * Acquire an image from each swapchain and begin a command buffer
 *
 * Returns
 * - The command buffer to record this frame
 */
HgGpuCmd* hgGpuFrameBegin(HgWindow** windows, u32 windowCount);

/**
 * Finishes recording the command buffer and presents the window images
 *
 * Parameters
 * - cmd The command buffer given from beginFrame
 */
void hgGpuFrameEnd(HgGpuCmd* cmd);

/**
 * Returns the window's current image, or nullptr if it could not be acquired
 */
HgGpuView* hgWindowImageView(HgWindow* window);

/**
 * Get the window's width in pixels
 */
HgFormat hgWindowImageFormat(HgWindow* window);

/**
 * Processes all events since startup or the last call to process events
 */
void hgProcessEvents();

/**
 * The types of events
 */
enum HgWindowEventType {
    HgWindowEventType_none = 0,
    HgWindowEventType_buttonPress,
    HgWindowEventType_buttonRelease,
    HgWindowEventType_count,
};

/**
 * The button inputs
 */
enum HgButton {
    HgButton_none = 0,
    HgButton_k0,
    HgButton_k1,
    HgButton_k2,
    HgButton_k3,
    HgButton_k4,
    HgButton_k5,
    HgButton_k6,
    HgButton_k7,
    HgButton_k8,
    HgButton_k9,
    HgButton_q,
    HgButton_w,
    HgButton_e,
    HgButton_r,
    HgButton_t,
    HgButton_y,
    HgButton_u,
    HgButton_i,
    HgButton_o,
    HgButton_p,
    HgButton_a,
    HgButton_s,
    HgButton_d,
    HgButton_f,
    HgButton_g,
    HgButton_h,
    HgButton_j,
    HgButton_k,
    HgButton_l,
    HgButton_z,
    HgButton_x,
    HgButton_c,
    HgButton_v,
    HgButton_b,
    HgButton_n,
    HgButton_m,
    HgButton_semicolon,
    HgButton_colon,
    HgButton_apostrophe,
    HgButton_quotation,
    HgButton_comma,
    HgButton_period,
    HgButton_question,
    HgButton_grave,
    HgButton_tilde,
    HgButton_exclamation,
    HgButton_at,
    HgButton_hash,
    HgButton_dollar,
    HgButton_percent,
    HgButton_carot,
    HgButton_ampersand,
    HgButton_asterisk,
    HgButton_lparen,
    HgButton_rparen,
    HgButton_lbracket,
    HgButton_rbracket,
    HgButton_lbrace,
    HgButton_rbrace,
    HgButton_equal,
    HgButton_less,
    HgButton_greater,
    HgButton_plus,
    HgButton_minus,
    HgButton_slash,
    HgButton_backslash,
    HgButton_underscore,
    HgButton_bar,
    HgButton_up,
    HgButton_down,
    HgButton_left,
    HgButton_right,
    HgButton_mouse1,
    HgButton_mouse2,
    HgButton_mouse3,
    HgButton_mouse4,
    HgButton_mouse5,
    HgButton_lmouse = HgButton_mouse1,
    HgButton_rmouse = HgButton_mouse2,
    HgButton_mmouse = HgButton_mouse3,
    HgButton_escape,
    HgButton_space,
    HgButton_enter,
    HgButton_backspace,
    HgButton_kdelete,
    HgButton_insert,
    HgButton_tab,
    HgButton_home,
    HgButton_end,
    HgButton_f1,
    HgButton_f2,
    HgButton_f3,
    HgButton_f4,
    HgButton_f5,
    HgButton_f6,
    HgButton_f7,
    HgButton_f8,
    HgButton_f9,
    HgButton_f10,
    HgButton_f11,
    HgButton_f12,
    HgButton_lshift,
    HgButton_rshift,
    HgButton_lctrl,
    HgButton_rctrl,
    HgButton_lmeta,
    HgButton_rmeta,
    HgButton_lalt,
    HgButton_ralt,
    HgButton_lsuper,
    HgButton_rsuper,
    HgButton_capslock,
    HgButton_count,
};

/**
 * A button input event
 */
struct HgWindowButtonEvent {
    /**
     * The type of event
     */
    HgWindowEventType type;
    /**
     * The button which was pressed or released
     */
    HgButton button;
};

/**
 * Input event data
 */
union HgWindowEvent {
    /**
     * The type of event
     */
    HgWindowEventType type;
    /**
     * The button press or release event
     */
    HgWindowButtonEvent button;
};

/**
 * Returns whether the application was quit
 */
bool hgWasQuit();

/**
 * Returns whether the window was closed
 */
bool hgWindowWasClosed(HgWindow* window);

/**
 * Returns whether the mouse is focused on the window
 */
bool hgWindowIsFocused(HgWindow* window);

/**
 * Get the window's width in pixels
 */
u32 hgWindowWidth(HgWindow* window);

/**
 * Get the window's width in pixels
 */
u32 hgWindowHeight(HgWindow* window);

/**
 * Returns the current x position of the mouse relative to the window
 */
f32 hgMouseX(HgWindow* window);

/**
 * Returns the current y position of the mouse relative to the window
 */
f32 hgMouseY(HgWindow* window);

/**
 * Returns the change in x position of the mouse relative to the window height
 */
f32 hgMouseDeltaX(HgWindow* window);

/**
 * Returns the change in y position of the mouse relative to the window height
 */
f32 hgMouseDeltaY(HgWindow* window);

/**
 * Returns whether the key is currently down
 */
bool hgIsButtonDown(HgWindow* window, HgButton key);

/**
 * Get the key events since last event processing
 */
HgWindowEvent* hgWindowEvents(HgWindow* window, u32* count);

// audio system : TODO

// file system : TODO

/**
 * Initialize all default HurdyGurdy asset types
 *
 * Parameters
 * - arena The arena to allocate from
 * - maxBinaries The max number of binary assets
 */
void hgAssetsInit(HgArena* arena, u32 maxBinaries, u32 maxImages, u32 maxTextures, u32 maxMeshes, u32 maxModels);

/**
 * Typed handles for assets
 */
template<typename T>
struct HgAssetHandle {
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);

    /**
     * The handle value
     */
    HgHandle handle;
};

/**
 * The extra data associated with assets
 */
template<typename T>
struct HgAssetData {
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);

    /**
     * The asset
     */
    T asset;
    /**
     * The reference count
     */
    u32 refCount;
    /**
     * Whether the asset has finished loading
     */
    HgFence isLoaded;
    /**
     * The unique path for caching
     */
    HgStringView path;
};

/**
 * An asset manager
 */
template<typename T>
struct HgAssetManager {
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);

    /**
     * The asset lookup
     */
    HgMap<HgStringView, HgHandle> map;
    /**
     * The asset pool
     */
    HgPool<HgAssetData<T>> pool;
};

/**
 * Create an asset manager
 *
 * Parameters
 * - arena The arena to allocate from
 * - maxAssets The max number of assets which can be created in the manager
 *
 * Returns
 * - The created asset manager
 */
template<typename T>
HgAssetManager<T> hgAssetManagerCreate(HgArena* arena, u32 maxAssets)
{
    HgAssetManager<T> am{};
    am.map = hgMapCreate<HgStringView, HgHandle>(arena, maxAssets * 2);
    am.pool = hgPoolCreate<HgAssetData<T>>(arena, maxAssets);
    return am;
}

/**
 * Create an asset (or increment the ref count)
 *
 * Parameters
 * - assets The asset manager to create in
 * - path The unique path for caching
 * - out A pointer to return the handle
 *
 * Returns
 * - Whether the asset needs to be loaded
 */
template<typename T>
bool hgAssetCreate(HgAssetManager<T>* assets, HgStringView path, HgAssetHandle<T>* out)
{
    HgHandle* handle = hgMapGet(&assets->map, path);
    if (handle != nullptr)
    {
        *out = {*handle};
        return false;
    }

    char* pathStore = (char*)malloc(path.length);
    memcpy(pathStore, path.chars, path.length);

    handle = hgMapAdd(&assets->map, {pathStore, path.length}, hgPoolAlloc(&assets->pool));

    HgAssetData<T>* data = hgPoolGet(&assets->pool, *handle);
    data->path = {pathStore, path.length};
    data->isLoaded = hgFenceCreate();
    data->refCount = 1;
    data->asset = {};

    *out = {*handle};
    return true;
}

/**
 * Destroy an asset (or decrement the ref count)
 *
 * Parameters
 * - assets The asset manager to destroy from
 * - handle The asset to destroy
 * - asset A pointer to return the asset, if destroyed
 *
 * Returns
 * - Whether the asset needs to be unloaded
 */
template<typename T>
bool hgAssetDestroy(HgAssetManager<T>* assets, HgAssetHandle<T> handle, T* asset)
{
    hgAssert(hgPoolAlive(&assets->pool, handle.handle));

    HgAssetData<T>* data = hgPoolGet(&assets->pool, handle.handle);
    if (--data->refCount == 0)
    {
        *asset = data->asset;
        hgFenceDestroy(data->isLoaded);
        free((void*)data->path.chars);
        return true;
    }
    return false;
}

/**
 * Returns the asset's data
 */
template<typename T>
T* hgAssetGet(HgAssetManager<T>* assets, HgAssetHandle<T> handle)
{
    return &hgPoolGet(&assets->pool, handle.handle)->asset;
}

/**
 * Returns the reference count associated with an asset
 */
template<typename T>
u32 hgAssetRefCount(HgAssetManager<T>* assets, HgAssetHandle<T> handle)
{
    return hgPoolGet(&assets->pool, handle.handle)->refCount;
}

/**
 * Returns the fence associated with an asset, whether it is finished loading
 */
template<typename T>
HgFence hgAssetFence(HgAssetManager<T>* assets, HgAssetHandle<T> handle)
{
    return hgPoolGet(&assets->pool, handle.handle)->isLoaded;
}

/**
 * Returns the path associated with an asset
 */
template<typename T>
HgStringView hgAssetPath(HgAssetManager<T>* assets, HgAssetHandle<T> handle)
{
    return hgPoolGet(&assets->pool, handle.handle)->path;
}

/**
 * A binary asset
 */
struct HgBinary {
    /**
     * The data in the file
     */
    void* data;
    /**
     * The size of the file in bytes
     */
    u64 size;
};

/**
 * A binary file asset handle
 */
typedef HgAssetHandle<HgBinary> HgBinaryHandle;

/**
 * Initialize binary asset manager
 */
void hgAssetsInitBinaries(HgArena* arena, u32 maxBinaries);

/**
 * Load a binary file from disc asynchronously, or increment the ref count
 *
 * Parameters
 * - path The file path to the image
 *
 * Returns
 * - The binary
 */
HgBinaryHandle hgBinaryLoad(HgStringView path);

/**
 * Unload a binary file from memory, or decrement the ref count
 */
void hgBinaryUnload(HgBinaryHandle handle);

/**
 * Get the binary asset
 */
HgBinary* hgBinaryGet(HgBinaryHandle handle);

/**
 * Resize the file
 *
 * Parameters
 * - arena The arena to allocate from
 * - newSize The new size of the file in bytes
 */
HgBinary hgBinaryResize(HgArena* arena, const HgBinary* bin, u64 newSize);

/**
 * Read data at index into a buffer
 *
 * Parameters
 * - idx The index into the file in bytes to read from
 * - dst A pointer to store the read data
 * - size The size in bytes to read
 */
void hgBinaryRead(const HgBinary* bin, u64 idx, void* dst, u64 len);

/**
 * Read data of arbitrary type from the file
 *
 * Parameters
 * - idx The index into the file in bytes to read from
 */
template<typename T>
T hgBinaryRead(const HgBinary* bin, u64 idx)
{
    T ret;
    hgBinaryRead(bin, idx, &ret, sizeof(T));
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
void hgBinaryOverwrite(HgBinary* bin, u64 idx, const void* src, u64 len);

/**
 * Overwrite data of arbitrary type at the index
 *
 * Parameters
 * - idx The index into the file to overwrite
 * - src The data to write
 */
template<typename T>
void hgBinaryOverwrite(u64 idx, const T& src)
{
    overwrite(idx, &src, sizeof(T));
}

/**
 * Store a binary file to disc asynchronously
 *
 * Parameters
 * - bin The binary to store
 * - path The file path to store at
 */
void hgBinaryStore(HgBinary* bin, HgStringView path);

/**
 * An image asset
 */
struct HgImage {
    /**
     * The width of the image in pixels
     */
    u32 width;
    /**
     * The height of the image in pixels
     */
    u32 height;
    /**
     * The depth of the image in pixels
     */
    u32 depth;
    /**
     * The format of each pixel
     */
    HgFormat format;
    /**
     * The pixel data, aligned to 16 bytes
     */
    void* pixels;
};

/**
 * A handle to an image
 */
typedef HgAssetHandle<HgImage> HgImageHandle;

/**
 * Initialize image assets
 */
void hgAssetsInitImages(HgArena* arena, u32 maxImages);

/**
 * Load an image asset asynchronously, or increment the ref count
 */
HgImageHandle hgImageLoad(HgStringView path);

/**
 * Unload an image asset, or decrement the ref count
 */
void hgImageUnload(HgImageHandle handle);

/**
 * Get the image asset
 */
HgImage* hgImageGet(HgImageHandle handle);

/**
 * Store an image to disc in the png format
 */
void hgImageStorePng(HgImage* image, HgStringView path, HgFence fence);

/**
 * A texture asset stored on the GPU
 */
struct HgTexture {
    /**
     * The image
     */
    HgGpuImage* image;
    /**
     * The image view
     */
    HgGpuView* view;
    /**
     * The sampler
     */
    HgGpuSampler* sampler;
    /**
     * The descriptor
     */
    HgGpuDescriptor descriptor;
};

/**
 * A handle to a texture asset
 */
typedef HgAssetHandle<HgTexture> HgTextureHandle;

/**
 * Initialize gpu textures
 */
void hgAssetsInitTextures(HgArena* arena, u32 maxTextures);

/**
 * Load a texture, or increment the ref count (blocking)
 *
 * Note, image data can be preloaded asynchronously to speed up texture loading
 */
HgTextureHandle hgTextureLoad(HgStringView path);

/**
 * Unload a texture from the gpu
 */
void hgTextureUnload(HgTextureHandle handle);

/**
 * Gets a gpu texture resource
 *
 * Returns
 * - The gpu texture, or nullptr if it is not loaded
 */
HgTexture* hgTextureGet(HgTextureHandle handle);

/**
 * A vertex in a model
 */
struct HgModelVertex {
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
 * Data for a 3d model asset
 */
struct HgMesh {
    /**
     * The file index of the first vertex
     */
    HgModelVertex* vertices;
    /**
     * The file index of the first geometry index
     */
    u32* indices;
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
    HgGpuTopology topology;
};

/**
 * A handle to a model data asset
 */
typedef HgAssetHandle<HgMesh> HgMeshHandle;

/**
 * Initialize model data assets
 */
void hgAssetsInitMeshes(HgArena* arena, u32 maxModels);

/**
 * Load a model data asset asynchronously, or increment the ref count
 */
HgMeshHandle hgMeshLoad(HgStringView path);

/**
 * Unload a model data asset, or decrement the ref count
 */
void hgMeshUnload(HgMeshHandle handle);

/**
 * Get the model data asset
 */
HgMesh* hgMeshGet(HgMeshHandle handle);

/**
 * Store the model data to disc in gltf format : TODO
 */
void hgMeshStoreGltf(HgMesh* data, HgStringView path, HgFence fence);

/**
 * A 3D model stored on the gpu
 */
struct HgModel {
    /**
     * The vertex buffer
     */
    HgGpuBuffer* vertexBuffer;
    /**
     * The index buffer
     */
    HgGpuBuffer* indexBuffer;
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
 * A gpu model handle
 */
typedef HgAssetHandle<HgModel> HgModelHandle;

/**
 * Initialize gpu model assets
 */
void hgAssetsInitModels(HgArena* arena, u32 maxModels);

/**
 * Load a 3d model, or increment the ref count (blocking)
 *
 * Note, model data can be preloaded asynchronously to speed up model loading
 */
HgModelHandle hgModelLoad(HgStringView path);

/**
 * Unload a model asset from the gpu
 */
void hgUnloadModel(HgModelHandle handle);

/**
 * Gets a gpu model resource
 *
 * Returns
 * - The gpu model, or nullptr if it is not loaded
 */
HgModel* hgGetModel(HgModelHandle handle);

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
struct HgEntity {
    /**
     * The entity handle
     */
    HgHandle handle;
};

/**
 * An entity component system
 */
struct HgEcs {
    /**
     * A system of components
     */
    struct System {
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
     * The entity pool
     */
    HgIndexPool entityPool;
    /**
     * The component systems
     */
    HgMap<u32, System> systems;

    /**
     * Create a new entity component system
     *
     * Parameters
     * - arena The arena to allocate from
     * - maxEntities The maximum number of entities which can be spawned
     * - maxComponentTypes The maximum number of types which can be created
     */
    static HgEcs create(HgArena* arena, u32 maxEntities, u32 maxComponentTypes);

    /**
     * Create a new component type in the ECS
     *
     * Parameters
     * - arena The arena to allocate from
     * - maxCount the maximum number of components in the component system
     * - width The width of the component type in bytes
     * - align The alignment of the component type in bytes
     * - componentID The ID of the component to create
     */
    void createComponent(HgArena* arena, u32 maxCount, u32 width, u32 align, u32 componentID);

    /**
     * Create a new component type in the ECS
     *
     * Parameters
     * - arena The arena to allocate from
     * - maxCount the maximum number of components in the component system
     */
    template<typename T>
    void createComponent(HgArena* arena, u32 maxCount)
    {
        createComponent(arena, maxCount, sizeof(T), alignof(T), hgComponentID<T>);
    }

    /**
     * Remove all component types and despawn all entities
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
        hgAssert(hgMapGet(&systems, hgComponentID<T>) != nullptr);
        return get((void*)c, hgComponentID<T>);
    }

    /**
     * Return a pointer to all entities of type
     */
    template<typename T>
    HgEntity* entities()
    {
        hgAssert(hgMapGet(&systems, hgComponentID<T>) != nullptr);
        return (HgEntity*)hgMapGet(&systems, hgComponentID<T>)->entities;
    }

    /**
     * Return a pointer to all components of type
     */
    template<typename T>
    T* components()
    {
        hgAssert(hgMapGet(&systems, hgComponentID<T>) != nullptr);
        return (T*)hgMapGet(&systems, hgComponentID<T>)->components;
    }

    /**
     * Return the number of active components of a type
     */
    template<typename T>
    u32 count()
    {
        hgAssert(hgMapGet(&systems, hgComponentID<T>) != nullptr);
        return hgMapGet(&systems, hgComponentID<T>)->count;
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
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity, T*>);

        HgEntity* e = entities<T>();
        HgEntity* end = e + count<T>();
        T* c = components<T>();
        for (; e != end; ++e, ++c)
        {
            fn(*e, c);
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
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity, Ts*...>);

        u32 id = findSmallest<Ts...>();
        System* system = hgMapGet(&systems, id);
        hgAssert(system != nullptr);

        HgEntity* e = system->entities;
        HgEntity* end = e + system->count;
        for (; e != end; ++e)
        {
            if (hasAll<Ts...>(*e))
                fn(*e, &get<Ts>(*e)...);
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
     * - fn The function to call
     */
    template<typename T, typename Fn>
    void forParSingle(Fn& fn)
    {
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity, T*>);

        struct Capture {
            HgEcs* ecs;
            Fn* fn;
        };
        Capture capture{this, &fn};

        hgThreadsFor(0, count<T>(), &capture, [](void* pcapture, u64 idx)
        {
            Capture* capture = (Capture*)pcapture;
            (*capture->fn)(
                capture->ecs->template entities<T>()[idx],
                &capture->ecs->template components<T>()[idx]);
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
     * - fn The function to call
     */
    template<typename... Ts, typename Fn>
    void forParMulti(Fn& fn)
    {
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity, Ts*...>);

        System* system = hgMapGet(&systems, findSmallest<Ts...>());
        hgAssert(system != nullptr);

        struct Capture {
            HgEcs* ecs;
            System* system;
            Fn* fn;
        };
        Capture capture{this, system, &fn};

        hgThreadsFor(0, system->count, &capture, [](void* pcapture, u64 idx)
        {
            Capture* capture = (Capture*)pcapture;
            HgEntity e = capture->system->entities[idx];
            if (capture->ecs->template hasAll<Ts...>(e))
                (*capture->fn)(e, &capture->ecs->template get<Ts>(e)...);
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
    void forPar(Fn fn)
    {
        static_assert(sizeof...(Ts) != 0);

        if constexpr (sizeof...(Ts) == 1)
        {
            forParSingle<Ts...>(fn);
        } else {
            forParMulti<Ts...>(fn);
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
    void sort(u32 componentID, void* data, bool (*compare)(void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs));

    /**
     * Sort components
     *
     * Parameters
     * - data The data passed to compare
     * - compare The comparison function
     */
    template<typename T>
    void sort(void* data, bool (*compare)(void*, HgEcs* ecs, HgEntity lhs, HgEntity rhs))
    {
        sort(hgComponentID<T>, data, compare);
    }
};

/**
 * A node component for entities in a hierarchy
 */
struct HgHierarchy {
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
void hgAddChildEntity(HgEcs* ecs, HgEntity parent, HgEntity child);

/**
 * Remove the entity from its hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to detach, must be alive
 */
void hgDetachEntity(HgEcs* ecs, HgEntity e);

/**
 * Destroy the entity and all its children in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to destroy to, must be alive
 */
void hgDestroyEntity(HgEcs* ecs, HgEntity e);

/**
 * The transform component for entities
 */
struct HgTransform {
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
void hgSetEntity(HgEcs* ecs, HgEntity e, const HgVec3& pos, const HgVec3& scale, const HgQuat& rot);

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
void hgMoveEntity(HgEcs* ecs, HgEntity e, const HgVec3& dpos, const HgVec3& dscale, const HgQuat& drot);

/**
 * A sprite component rendered by the 2d pipeline
 */
struct HgSprite2D {
    /**
     * The texture to draw from
     */
    HgTexture* texture;
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
 * - colorFormat The format of the color attachment, must not be UNDEFINED
 * - depthFormat The format of the depth attachment, if any
 */
void hgInitPipeline2D(
    HgFormat colorFormat,
    HgFormat depthFormat);

/**
 * Deinitialize the 2D pipeline
 */
void hgDeinitPipeline2D();

/**
 * Update the 2D pipeline's projection matrix
 */
void hgUpdateProjection2D(const HgMat4* projection);

/**
 * Updates the 2D pipeline's view matrix
 */
void hgUpdateView2D(const HgMat4* view);

/**
 * Issue draw commands for all HgSprite components in the ecs
 *
 * Parameters
 * - ecs The ecs to draw
 * - cmd The command buffer to record to, must not be nullptr
 */
void hgDraw2D(HgEcs* ecs, HgGpuCmd* cmd);

/**
 * A model component rendered by the 3d pipeline
 */
struct HgModel3D {
    /**
     * The model to render
     */
    HgModel* modelResource;
    /**
     * The model's color map
     */
    HgTexture* colorMap;
    /**
     * The model's normal map
     */
    HgTexture* normalMap;
};

/**
 * A direction light component rendered by the 3d pipeline
 */
struct HgDirLight3D {
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
struct HgPointLight3D {
    /**
     * The color of the light
     */
    HgVec4 color;
};

/**
 * Initialize the 3D pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be UNDEFINED
 * - depthFormat The format of the depth attachment, must not be UNDEFINED
 */
void hgInitPipeline3D(
    HgFormat colorFormat,
    HgFormat depthFormat);

/**
 * Deinitialize the 3D pipeline
 */
void hgDeinitPipeline3D();

/**
 * Update the 3D pipeline's projection matrix
 */
void hgUpdateProjection3D(const HgMat4* projection);

/**
 * Update the 3D pipeline's view matrix
 */
void hgUpdateView3D(const HgMat4* view);

/**
 * Issue draw commands for all HgModelComp components in the ecs
 *
 * Parameters
 * - ecs The ecs to draw
 * - cmd The command buffer to record to, must not be nullptr
 */
void hgDraw3D(HgEcs* ecs, HgGpuCmd* cmd);

/**
 * Initialize ImGui platform backend
 *
 * Note, requires GLFW on Linux (for now)
 *
 * Parameters
 * - window The window for ImGui to use
 */
void hgImGuiInit(
    HgWindow* window,
    const HgFormat* colorFormats,
    u32 colorAttachmentCount,
    HgFormat depthFormat = HgFormat_undefined,
    HgFormat stencilFormat = HgFormat_undefined);

/**
 * Deinitializes ImGui platform backend
 */
void hgImGuiDeinit();

/**
 * Create an ImGui texture
 */
void* hgImGuiTextureCreate(HgGpuView* view, HgGpuSampler* sampler, HgGpuLayout layout);

/**
 * Create an ImGui texture
 */
void hgImGuiTextureDestroy(void* texture);

/**
 * Create a new ImGui frame for the platform backend
 */
void hgImGuiNewFrame();

/**
 * Draw the ImGui frame
 *
 * Parameters
 * - cmd The command buffer to record to
 */
void hgImGuiDraw(HgGpuCmd* cmd);

#endif // HURDYGURDY_HPP

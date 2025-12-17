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

#ifndef HURDYGURDY_H
#define HURDYGURDY_H

#include <cassert>
#include <cfloat>
#include <cinttypes>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <chrono>
#include <new>
#include <type_traits>
#include <utility>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using usize = std::size_t;
using isize = std::intptr_t;

using f32 = std::float_t;
using f64 = std::double_t;

/**
 * The struct used to declare and run tests
 */
struct HgTest {
    /**
     * The name of the test
     */
    const char *name;
    /**
     * A pointer to the test function
     */
    bool (*function)();

    HgTest() {};

    /**
     * Creates and registers the test globally
     *
     * Parameters
     * - test_name The name of the test
     * - test_function A pointer to the test function
     */
    HgTest(const char *test_name, bool (*test_function)());
};

/**
 * Automatically declares and registers a test function
 *
 * Example:
 * hg_test(hg_example_test) {
 *     bool success = true;
 *     return success;
 * }
 */
#define hg_test(name) \
    static bool hg_test_function_##name(); \
    static HgTest hg_test_struct_##name{#name, hg_test_function_##name}; \
    static bool hg_test_function_##name() 

#define hg_test_assert(cond) { \
    if (!(cond)) { \
        std::printf("HurdyGurdy Test assertion failed: " #cond "\n"); \
        return false; \
    } \
}


/**
 * Runs all tests registered globally
 *
 * Note, tests are rendered through static init, so this should be called after
 * static init is completed
 *
 * Returns:
 * - Whether all tests passed
 */
bool hg_run_tests();

/**
 * Initializes the HurdyGurdy library
 *
 * Run this function before any Vulkan, windowing, or audio functions
 *
 * Note, calls hg_vk_load
 */
void hg_init();

/**
 * Shuts down the HurdyGurdy library
 */
void hg_exit();

#ifdef NDEBUG

/**
 * Runs code only in debug mode
 */
#define hg_debug_mode(code)

#else // NDEBUG

/**
 * Runs code only in debug mode
 */
#define hg_debug_mode(code) code

#endif // NDEBUG

template<typename F>
struct HgDeferInternal {
    F fn;
    HgDeferInternal(F defer_function) : fn(defer_function) {}
    ~HgDeferInternal() { fn(); }
};

#define hg_concat_macros_internal(x, y) x##y
#define hg_concat_macros(x, y) hg_concat_macros_internal(x, y)

/**
 * Defers a piece of code until the end of the scope
 *
 * Parameters
 * - code The code to run, may be placed inside braces or not
 */
#define hg_defer(code) [[maybe_unused]] auto hg_concat_macros(hg_defer_, __COUNTER__) = HgDeferInternal{[&]{code;}}

/**
 * Gets the number of elements in a stack allocated array
 *
 * Parameters
 * - array The array to take the count of
 * Returns
 * - The nuymber of elements
 */
#define hg_countof(array) (sizeof(array) / sizeof((array)[0]))

/**
 * Formats and logs a debug message to stderr in debug mode only
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_debug(...) do { hg_debug_mode({ (void)std::fprintf(stderr, "HurdyGurdy Debug: " __VA_ARGS__); }) } while(0)

/**
 * Formats and logs an info message to stderr
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_info(...) do { (void)std::fprintf(stderr, "HurdyGurdy Info: " __VA_ARGS__); } while(0)

/**
 * Formats and logs a warning message to stderr
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_warn(...) do { (void)std::fprintf(stderr, "HurdyGurdy Warning: " __VA_ARGS__); } while(0)

/**
 * Formats and logs an error message to stderr and aborts the program
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_error(...) do { (void)std::fprintf(stderr, "HurdyGurdy Error: " __VA_ARGS__); abort(); } while(0)

/**
 * A high precision clock for timers and game deltas
 */
struct HgClock {
    std::chrono::time_point<std::chrono::high_resolution_clock> time;

    /**
     * Resets the clock and gets the delta since the last tick in seconds
     *
     * Parameters
     * - clock The clock to tick, must not be nullptr
     * Returns
     * - Seconds since last tick
     */
    f64 tick();
};

/**
 * Aligns a pointer to an alignment
 *
 * Parameters
 * - value The value to align
 * - alignment The alignment, must be a multiple of 2
 * Returns
 * - The aligned size
 */
constexpr usize hg_align(usize value, usize alignment) {
    assert(alignment > 0 && (alignment & (alignment - 1)) == 0);
    return (value + alignment - 1) & ~(alignment - 1);
}

static constexpr f64 HgPi    = 3.1415926535897932;
static constexpr f64 HgTau   = 6.2831853071795864;
static constexpr f64 HgE     = 2.7182818284590452;
static constexpr f64 HgRoot2 = 1.4142135623730951;
static constexpr f64 HgRoot3 = 1.7320508075688772;

/**
 * A 2D vector
 */
struct HgVec2 {
    f32 x, y;
};

constexpr bool operator==(const HgVec2& lhs, const HgVec2& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

constexpr bool operator!=(const HgVec2& lhs, const HgVec2& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

/**
 * A 3D vector
 */
struct HgVec3 {
    f32 x, y, z;
};

constexpr bool operator==(const HgVec3& lhs, const HgVec3& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

constexpr bool operator!=(const HgVec3& lhs, const HgVec3& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}

/**
 * A 4D vector
 */
struct HgVec4 {
    f32 x, y, z, w;
};

constexpr bool operator==(const HgVec4& lhs, const HgVec4& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

constexpr bool operator!=(const HgVec4& lhs, const HgVec4& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
}

/**
 * A 2x2 matrix
 */
struct HgMat2 {
    HgVec2 x, y;
};

constexpr bool operator==(const HgMat2& lhs, const HgMat2& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

constexpr bool operator!=(const HgMat2& lhs, const HgMat2& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

/**
 * A 3x3 matrix
 */
struct HgMat3 {
    HgVec3 x, y, z;
};

constexpr bool operator==(const HgMat3& lhs, const HgMat3& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

constexpr bool operator!=(const HgMat3& lhs, const HgMat3& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}

/**
 * A 4x4 matrix
 */
struct HgMat4 {
    HgVec4 x, y, z, w;
};

constexpr bool operator==(const HgMat4& lhs, const HgMat4& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

constexpr bool operator!=(const HgMat4& lhs, const HgMat4& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
}

/**
 * A complex number
 */
struct HgComplex {
    f32 r, i;
};

constexpr bool operator==(const HgComplex& lhs, const HgComplex& rhs) {
    return lhs.r == rhs.r && lhs.i == rhs.i;
}

constexpr bool operator!=(const HgComplex& lhs, const HgComplex& rhs) {
    return lhs.r != rhs.r || lhs.i != rhs.i;
}

/**
 * A quaternion
 *
 * r is the real part
 */
struct HgQuat {
    f32 r, i, j, k;
};

constexpr bool operator==(const HgQuat& lhs, const HgQuat& rhs) {
    return lhs.r == rhs.r && lhs.i == rhs.i && lhs.j == rhs.j && lhs.k == rhs.k;
}

constexpr bool operator!=(const HgQuat& lhs, const HgQuat& rhs) {
    return lhs.r != rhs.r || lhs.i != rhs.i || lhs.j != rhs.j || lhs.k != rhs.k;
}

/**
 * Creates a 2D vector with the given scalar
 *
 * Parameters
 * - scalar The scalar to create the vector with
 * Returns
 * - The created vector
 */
constexpr HgVec2 hg_svec2(f32 scalar) {
    return {scalar, scalar};
}

/**
 * Creates a 3D vector with the given scalar
 *
 * Parameters
 * - scalar The scalar to create the vector with
 * Returns
 * - The created vector
 */
constexpr HgVec3 hg_svec3(f32 scalar) {
    return {scalar, scalar, scalar};
}

/**
 * Creates a 4D vector with the given scalar
 *
 * Parameters
 * - scalar The scalar to create the vector with
 * Returns
 * - The created vector
 */
constexpr HgVec4 hg_svec4(f32 scalar) {
    return {scalar, scalar, scalar, scalar};
}

/**
 * Creates a 2x2 matrix with the given scalar
 *
 * Stores the scalar in the diagonal only
 *
 * Parameters
 * - scalar The scalar to create the matrix with
 * Returns
 * - The created matrix
 */
constexpr HgMat2 hg_smat2(f32 scalar) {
    return {
        {scalar, 0.0f},
        {0.0f, scalar},
    };
}

/**
 * Creates a 3x3 matrix with the given scalar
 *
 * Stores the scalar in the diagonal only
 *
 * Parameters
 * - scalar The scalar to create the matrix with
 * Returns
 * - The created matrix
 */
constexpr HgMat3 hg_smat3(f32 scalar) {
    return {
        {scalar, 0.0f, 0.0f},
        {0.0f, scalar, 0.0f},
        {0.0f, 0.0f, scalar},
    };
}

/**
 * Creates a 4x4 matrix with the given scalar
 *
 * Stores the scalar in the diagonal only
 *
 * Parameters
 * - scalar The scalar to create the matrix with
 * Returns
 * - The created matrix
 */
constexpr HgMat4 hg_smat4(f32 scalar) {
    return {
        {scalar, 0.0f, 0.0f, 0.0f},
        {0.0f, scalar, 0.0f, 0.0f},
        {0.0f, 0.0f, scalar, 0.0f},
        {0.0f, 0.0f, 0.0f, scalar},
    };
}

/**
 * Creates a 3D vector from a 2D vector and 0 for the z component
 *
 * Parameters
 * - lhs The vector to convert
 * Returns
 * - The converted vector
 */
constexpr HgVec3 hg_vec2to3(const HgVec2& lhs) {
    return {lhs.x, lhs.y, 0.0f};
}

/**
 * Creates a 4D vector from a 2D vector and 0 for the z and w components
 *
 * Parameters
 * - lhs The vector to convert
 * Returns
 * - The converted vector
 */
constexpr HgVec4 hg_vec2to4(const HgVec2& lhs) {
    return {lhs.x, lhs.y, 0.0f, 0.0f};
}

/**
 * Creates a 3D vector from a 3D vector and 0 for the w component
 *
 * Parameters
 * - lhs The vector to convert
 * Returns
 * - The converted vector
 */
constexpr HgVec4 hg_vec3to4(const HgVec3& lhs) {
    return {lhs.x, lhs.y, lhs.z, 0.0f};
}

/**
 * Scales a 2x2 matrix to a 3x3 matrix with 1 on the diagonal
 *
 * Parameters
 * - lhs The matrix to convert
 * Returns
 * - The converted matrix
 */
constexpr HgMat3 hg_mat2to3(const HgMat2& lhs) {
    return {
        {lhs.x.x, lhs.x.y, 0.0f},
        {lhs.y.x, lhs.y.y, 0.0f},
        {0.0f,    0.0f,    1.0f},
    };
}

/**
 * Scales a 2x2 matrix to a 4x4 matrix with 1 on the diagonal
 *
 * Parameters
 * - lhs The matrix to convert
 * Returns
 * - The converted matrix
 */
constexpr HgMat4 hg_mat2to4(const HgMat2& lhs) {
    return {
        {lhs.x.x, lhs.x.y, 0.0f, 0.0f},
        {lhs.y.x, lhs.y.y, 0.0f, 0.0f},
        {0.0f,    0.0f,    1.0f, 0.0f},
        {0.0f,    0.0f,    0.0f, 1.0f},
    };
}

/**
 * Scales a 3x3 matrix to a 4x4 matrix with 1 on the diagonal
 *
 * Parameters
 * - lhs The matrix to convert
 * Returns
 * - The converted matrix
 */
constexpr HgMat4 hg_mat3to4(const HgMat3& lhs) {
    return {
        {lhs.x.x, lhs.x.y, lhs.x.z, 0.0f},
        {lhs.y.x, lhs.y.y, lhs.y.z, 0.0f},
        {lhs.z.x, lhs.z.y, lhs.z.z, 0.0f},
        {0.0f,    0.0f,    0.0f,    1.0f},
    };
}

/**
 * Adds two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
constexpr void hg_vadd(u32 size, f32 *dst, const f32 *lhs, const f32 *rhs) {
    assert(dst != nullptr);
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = lhs[i] + rhs[i];
    }
}

/**
 * Adds two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The added vector
 */
constexpr HgVec2 hg_vadd2(const HgVec2& lhs, const HgVec2& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

constexpr HgVec2 operator+(const HgVec2& lhs, const HgVec2& rhs) {
    return hg_vadd2(lhs, rhs);
}

/**
 * Adds two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The added vector
 */
constexpr HgVec3 hg_vadd3(const HgVec3& lhs, const HgVec3& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

constexpr HgVec3 operator+(const HgVec3& lhs, const HgVec3& rhs) {
    return hg_vadd3(lhs, rhs);
}

/**
 * Adds two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The added vector
 */
constexpr HgVec4 hg_vadd4(const HgVec4& lhs, const HgVec4& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

constexpr HgVec4 operator+(const HgVec4& lhs, const HgVec4& rhs) {
    return hg_vadd4(lhs, rhs);
}

/**
 * Subtracts two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
constexpr void hg_vsub(u32 size, f32 *dst, const f32 *lhs, const f32 *rhs) {
    assert(dst != nullptr);
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = lhs[i] - rhs[i];
    }
}

/**
 * Subtracts two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The subtracted vector
 */
constexpr HgVec2 hg_vsub2(const HgVec2& lhs, const HgVec2& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

constexpr HgVec2 operator-(const HgVec2& lhs, const HgVec2& rhs) {
    return hg_vsub2(lhs, rhs);
}

/**
 * Subtracts two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The subtracted vector
 */
constexpr HgVec3 hg_vsub3(const HgVec3& lhs, const HgVec3& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

constexpr HgVec3 operator-(const HgVec3& lhs, const HgVec3& rhs) {
    return hg_vsub3(lhs, rhs);
}

/**
 * Subtracts two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The subtracted vector
 */
constexpr HgVec4 hg_vsub4(const HgVec4& lhs, const HgVec4& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

constexpr HgVec4 operator-(const HgVec4& lhs, const HgVec4& rhs) {
    return hg_vsub4(lhs, rhs);
}

/**
 * Multiplies pairwise two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
constexpr void hg_vmul(u32 size, f32 *dst, const f32 *lhs, const f32 *rhs) {
    assert(dst != nullptr);
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = lhs[i] * rhs[i];
    }
}

/**
 * Multiplies pairwise two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The multiplied vector
 */
constexpr HgVec2 hg_vmul2(const HgVec2& lhs, const HgVec2& rhs) {
    return {lhs.x * rhs.x, lhs.y * rhs.y};
}

constexpr HgVec2 operator*(const HgVec2& lhs, const HgVec2& rhs) {
    return hg_vmul2(lhs, rhs);
}

/**
 * Multiplies pairwise two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The multiplied vector
 */
constexpr HgVec3 hg_vmul3(const HgVec3& lhs, const HgVec3& rhs) {
    return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

constexpr HgVec3 operator*(const HgVec3& lhs, const HgVec3& rhs) {
    return hg_vmul3(lhs, rhs);
}

/**
 * Multiplies pairwise two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The multiplied vector
 */
constexpr HgVec4 hg_vmul4(const HgVec4& lhs, const HgVec4& rhs) {
    return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w};
}

constexpr HgVec4 operator*(const HgVec4& lhs, const HgVec4& rhs) {
    return hg_vmul4(lhs, rhs);
}

/**
 * Multiplies a scalar and a vector
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be nullptr
 * - scalar The scalar to multiply with
 * - vec The vector to multiply with, must not be nullptr
 */
constexpr void hg_svmul(u32 size, f32 *dst, f32 scalar, const f32 *vec) {
    assert(dst != nullptr);
    assert(vec != nullptr);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = scalar * vec[i];
    }
}

/**
 * Multiplies a scalar and a 2D vector
 *
 * Parameters
 * - scalar The scalar to multiply with
 * - vec The vector to multiply with
 * Returns
 * - The multiplied vector
 */
constexpr HgVec2 hg_svmul2(f32 scalar, const HgVec2& vec) {
    return {scalar * vec.x, scalar * vec.y};
}

constexpr HgVec2 operator*(f32 lhs, const HgVec2& rhs) {
    return hg_svmul2(lhs, rhs);
}

constexpr HgVec2 operator*(const HgVec2& lhs, f32 rhs) {
    return hg_svmul2(rhs, lhs);
}

/**
 * Multiplies a scalar and a 3D vector
 *
 * Parameters
 * - scalar The scalar to multiply with
 * - vec The vector to multiply with
 * Returns
 * - The multiplied vector
 */
constexpr HgVec3 hg_svmul3(f32 scalar, const HgVec3& vec) {
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z};
}

constexpr HgVec3 operator*(f32 lhs, const HgVec3& rhs) {
    return hg_svmul3(lhs, rhs);
}

constexpr HgVec3 operator*(const HgVec3& lhs, f32 rhs) {
    return hg_svmul3(rhs, lhs);
}

/**
 * Multiplies a scalar and a 4D vector
 *
 * Parameters
 * - scalar The scalar to multiply with
 * - vec The vector to multiply with
 * Returns
 * - The multiplied vector
 */
constexpr HgVec4 hg_svmul4(f32 scalar, const HgVec4& vec) {
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
}

constexpr HgVec4 operator*(f32 lhs, const HgVec4& rhs) {
    return hg_svmul4(lhs, rhs);
}

constexpr HgVec4 operator*(const HgVec4& lhs, f32 rhs) {
    return hg_svmul4(rhs, lhs);
}

/**
 * Divides pairwise two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
constexpr void hg_vdiv(u32 size, f32 *dst, const f32 *lhs, const f32 *rhs) {
    assert(dst != nullptr);
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = lhs[i] / rhs[i];
    }
}

/**
 * Divides pairwise two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The divided vector
 */
constexpr HgVec2 hg_vdiv2(const HgVec2& lhs, const HgVec2& rhs) {
    return {lhs.x / rhs.x, lhs.y / rhs.y};
}

constexpr HgVec2 operator/(const HgVec2& lhs, const HgVec2& rhs) {
    return hg_vdiv2(lhs, rhs);
}

/**
 * Divides pairwise two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The divided vector
 */
constexpr HgVec3 hg_vdiv3(const HgVec3& lhs, const HgVec3& rhs) {
    return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
}

constexpr HgVec3 operator/(const HgVec3& lhs, const HgVec3& rhs) {
    return hg_vdiv3(lhs, rhs);
}

/**
 * Divides pairwise two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The divided vector
 */
constexpr HgVec4 hg_vdiv4(const HgVec4& lhs, const HgVec4& rhs) {
    return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w};
}

constexpr HgVec4 operator/(const HgVec4& lhs, const HgVec4& rhs) {
    return hg_vdiv4(lhs, rhs);
}

/**
 * Divides a vector by a scalar
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be nullptr
 * - vec The vector to divide, must not be nullptr
 * - scalar The scalar to divide by
 */
constexpr void hg_svdiv(u32 size, f32 *dst, const f32 *vec, f32 scalar) {
    assert(dst != nullptr);
    assert(vec != nullptr);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = vec[i] / scalar;
    }
}

/**
 * Divides a 2D vector by a scalar
 *
 * Parameters
 * - vec The vector to divide
 * - scalar The scalar to divide by
 * Returns
 * - The divided vector
 */
constexpr HgVec2 hg_svdiv2(const HgVec2& vec, f32 scalar) {
    return {vec.x / scalar, vec.y / scalar};
}

constexpr HgVec2 operator/(const HgVec2& lhs, f32 rhs) {
    return hg_svdiv2(lhs, rhs);
}

/**
 * Divides a 3D vector by a scalar
 *
 * Parameters
 * - vec The vector to divide
 * - scalar The scalar to divide by
 * Returns
 * - The divided vector
 */
constexpr HgVec3 hg_svdiv3(const HgVec3& vec, f32 scalar) {
    return {vec.x / scalar, vec.y / scalar, vec.z / scalar};
}

constexpr HgVec3 operator/(const HgVec3& lhs, f32 rhs) {
    return hg_svdiv3(lhs, rhs);
}

/**
 * Divides a 4D vector by a scalar
 *
 * Parameters
 * - vec The vector to divide
 * - scalar The scalar to divide by
 * Returns
 * - The divided vector
 */
constexpr HgVec4 hg_svdiv4(const HgVec4& vec, f32 scalar) {
    return {vec.x / scalar, vec.y / scalar, vec.z / scalar, vec.w / scalar};
}

constexpr HgVec4 operator/(const HgVec4& lhs, f32 rhs) {
    return hg_svdiv4(lhs, rhs);
}

/**
 * Computes the dot product of two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
constexpr void hg_dot(u32 size, f32 *dst, const f32 *lhs, const f32 *rhs) {
    assert(dst != nullptr);
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    *dst = 0.0f;
    for (u32 i = 0; i < size; ++i) {
        *dst += lhs[i] * rhs[i];
    }
}

/**
 * Computes the dot product of two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The dot product
 */
constexpr float hg_dot(const HgVec2& lhs, const HgVec2& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

/**
 * Computes the dot product of two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The dot product
 */
constexpr float hg_dot(const HgVec3& lhs, const HgVec3& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

/**
 * Computes the dot product of two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The dot product
 */
constexpr float hg_dot(const HgVec4& lhs, const HgVec4& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

/**
 * Computes the length of a vector
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be nullptr
 * - vec The vector to compute the length of, must not be nullptr
 */
constexpr void hg_len(u32 size, f32 *dst, const f32 *vec) {
    assert(dst != nullptr);
    assert(vec != nullptr);
    hg_dot(size, dst, vec, vec);
    *dst = sqrtf(*dst);
}

/**
 * Computes the length of a 2D vector
 *
 * Parameters
 * - lhs The vector to compute the length of
 * Returns
 * - The length of the vector
 */
constexpr float hg_len(const HgVec2& vec) {
    return sqrtf(hg_dot(vec, vec));
}

/**
 * Computes the length of a 3D vector
 *
 * Parameters
 * - lhs The vector to compute the length of
 * Returns
 * - The length of the vector
 */
constexpr float hg_len(const HgVec3& vec) {
    return sqrtf(hg_dot(vec, vec));
}

/**
 * Computes the length of a 4D vector
 *
 * Parameters
 * - lhs The vector to compute the length of
 * Returns
 * - The length of the vector
 */
constexpr float hg_len(const HgVec4& vec) {
    return sqrtf(hg_dot(vec, vec));
}

/**
 * Normalizes a vector
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be nullptr
 * - vec The vector to normalize, must not be nullptr
 */
constexpr void hg_norm(u32 size, f32 *dst, const f32 *vec) {
    assert(dst != nullptr);
    assert(vec != nullptr);
    f32 len = 0.0f;
    hg_len(size, &len, vec);
    for (u32 i = 0; i < size; ++i) {
        dst[i] = vec[i] / len;
    }
}

/**
 * Normalizes a 2D vector
 *
 * Parameters
 * - vec The vector to normalize
 * Returns
 * - The normalized vector
 */
constexpr HgVec2 hg_norm(const HgVec2& vec) {
    f32 len = hg_len(vec);
    return {vec.x / len, vec.y / len};
}

/**
 * Normalizes a 3D vector
 *
 * Parameters
 * - vec The vector to normalize
 * Returns
 * - The normalized vector
 */
constexpr HgVec3 hg_norm(const HgVec3& vec) {
    f32 len = hg_len(vec);
    return {vec.x / len, vec.y / len, vec.z / len};
}

/**
 * Normalizes a 4D vector
 *
 * Parameters
 * - vec The vector to normalize
 * Returns
 * - The normalized vector
 */
constexpr HgVec4 hg_norm(const HgVec4& vec) {
    f32 len = hg_len(vec);
    return {vec.x / len, vec.y / len, vec.z / len, vec.w / len};
}

/**
 * Computes the cross product of two 3D vectors
 *
 * Parameters
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
constexpr void hg_cross(f32 *dst, const f32 *lhs, const f32 *rhs) {
    assert(dst != nullptr);
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
    dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
    dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
}

/**
 * Computes the cross product of two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The cross product
 */
constexpr HgVec3 hg_cross(const HgVec3& lhs, const HgVec3& rhs) {
    return {lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x};
}

/**
 * Adds two arbitrary size matrices
 *
 * Parameters
 * - width The width of the matrices
 * - height The height of the matrices
 * - dst The destination matrix, must not be nullptr
 * - lhs The left-hand side matrix, must not be nullptr
 * - rhs The right-hand side matrix, must not be nullptr
 */
constexpr void hg_madd(u32 width, u32 height, f32 *dst, const f32 *lhs, const f32 *rhs) {
    assert(dst != nullptr);
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    for (u32 i = 0; i < width; ++i) {
        for (u32 j = 0; j < height; ++j) {
            dst[i * width + j] = lhs[i * width + j] + rhs[i * width + j];
        }
    }
}

/**
 * Adds two 2x2 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The added matrix
 */
constexpr HgMat2 hg_madd2(const HgMat2& lhs, const HgMat2& rhs) {
    HgMat2 result{};
    hg_madd(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

constexpr HgMat2 operator+(const HgMat2& lhs, const HgMat2& rhs) {
    return hg_madd2(lhs, rhs);
}

/**
 * Adds two 3x3 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The added matrix
 */
constexpr HgMat3 hg_madd3(const HgMat3& lhs, const HgMat3& rhs) {
    HgMat3 result{};
    hg_madd(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

constexpr HgMat3 operator+(const HgMat3& lhs, const HgMat3& rhs) {
    return hg_madd3(lhs, rhs);
}

/**
 * Adds two 4x4 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The added matrix
 */
constexpr HgMat4 hg_madd4(const HgMat4& lhs, const HgMat4& rhs) {
    HgMat4 result{};
    hg_madd(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

constexpr HgMat4 operator+(const HgMat4& lhs, const HgMat4& rhs) {
    return hg_madd4(lhs, rhs);
}

/**
 * Subtracts two arbitrary size matrices
 *
 * Parameters
 * - width The width of the matrices
 * - height The height of the matrices
 * - dst The destination matrix, must not be nullptr
 * - lhs The left-hand side matrix, must not be nullptr
 * - rhs The right-hand side matrix, must not be nullptr
 */
constexpr void hg_msub(u32 width, u32 height, f32 *dst, const f32 *lhs, const f32 *rhs) {
    assert(dst != nullptr);
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    for (u32 i = 0; i < width; ++i) {
        for (u32 j = 0; j < height; ++j) {
            dst[i * width + j] = lhs[i * width + j] - rhs[i * width + j];
        }
    }
}

/**
 * Subtracts two 2x2 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The subtracted matrix
 */
constexpr HgMat2 hg_msub2(const HgMat2& lhs, const HgMat2& rhs) {
    HgMat2 result{};
    hg_msub(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

constexpr HgMat2 operator-(const HgMat2& lhs, const HgMat2& rhs) {
    return hg_msub2(lhs, rhs);
}

/**
 * Subtracts two 3x3 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The subtracted matrix
 */
constexpr HgMat3 hg_msub3(const HgMat3& lhs, const HgMat3& rhs) {
    HgMat3 result{};
    hg_msub(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

constexpr HgMat3 operator-(const HgMat3& lhs, const HgMat3& rhs) {
    return hg_msub3(lhs, rhs);
}

/**
 * Subtracts two 4x4 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The subtracted matrix
 */
constexpr HgMat4 hg_msub4(const HgMat4& lhs, const HgMat4& rhs) {
    HgMat4 result{};
    hg_msub(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

constexpr HgMat4 operator-(const HgMat4& lhs, const HgMat4& rhs) {
    return hg_msub4(lhs, rhs);
}

/**
 * Multiplies two arbitrary size matrices
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
constexpr void hg_mmul(f32 *dst, u32 wl, u32 hl, const f32 *lhs, u32 wr, u32 hr, const f32 *rhs) {
    assert(hr == wl);
    assert(dst != nullptr);
    assert(lhs != nullptr);
    assert(rhs != nullptr);
    (void)hr;
    for (u32 i = 0; i < wl; ++i) {
        for (u32 j = 0; j < wr; ++j) {
            dst[i * wl + j] = 0.0f;
            for (u32 k = 0; k < hl; ++k) {
                dst[i * wl + j] += lhs[k * wl + j] * rhs[i * wr + k];
            }
        }
    }
}

/**
 * Multiplies two 2x2 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The multiplied matrix
 */
constexpr HgMat2 hg_mmul2(const HgMat2& lhs, const HgMat2& rhs) {
    HgMat2 result{};
    hg_mmul(&result.x.x, 2, 2, &lhs.x.x, 2, 2, &rhs.x.x);
    return result;
}

constexpr HgMat2 operator*(const HgMat2& lhs, const HgMat2& rhs) {
    return hg_mmul2(lhs, rhs);
}

/**
 * Multiplies two 3x3 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The multiplied matrix
 */
constexpr HgMat3 hg_mmul3(const HgMat3& lhs, const HgMat3& rhs) {
    HgMat3 result{};
    hg_mmul(&result.x.x, 3, 3, &lhs.x.x, 3, 3, &rhs.x.x);
    return result;
}

constexpr HgMat3 operator*(const HgMat3& lhs, const HgMat3& rhs) {
    return hg_mmul3(lhs, rhs);
}

/**
 * Multiplies two 4x4 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The multiplied matrix
 */
constexpr HgMat4 hg_mmul4(const HgMat4& lhs, const HgMat4& rhs) {
    HgMat4 result{};
    hg_mmul(&result.x.x, 4, 4, &lhs.x.x, 4, 4, &rhs.x.x);
    return result;
}

constexpr HgMat4 operator*(const HgMat4& lhs, const HgMat4& rhs) {
    return hg_mmul4(lhs, rhs);
}

/**
 * Multiplies a matrix and a vector
 *
 * Parameters
 * - width The width of the matrix
 * - height The height of the matrix
 * - dst The destination vector, must not be nullptr
 * - mat The matrix to multiply with, must not be nullptr
 * - vec The vector to multiply with, must not be nullptr
 */
constexpr void hg_mvmul(u32 width, u32 height, f32 *dst, const f32 *mat, const f32 *vec) {
    assert(dst != nullptr);
    assert(mat != nullptr);
    assert(vec != nullptr);
    for (u32 i = 0; i < height; ++i) {
        dst[i] = 0.0f;
        for (u32 j = 0; j < width; ++j) {
            dst[i] += mat[j * width + i] * vec[j];
        }
    }
}

/**
 * Multiplies a 2x2 matrix and a 2D vector
 *
 * Parameters
 * - lhs The matrix to multiply with
 * - rhs The vector to multiply with
 * Returns
 * - The multiplied vector
 */
constexpr HgVec2 hg_mvmul2(const HgMat2& lhs, const HgVec2& rhs) {
    HgVec2 result{};
    hg_mvmul(2, 2, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

constexpr HgVec2 operator*(const HgMat2& lhs, const HgVec2& rhs) {
    return hg_mvmul2(lhs, rhs);
}

/**
 * Multiplies a 3x3 matrix and a 3D vector
 *
 * Parameters
 * - lhs The matrix to multiply with
 * - rhs The vector to multiply with
 * Returns
 * - The multiplied vector
 */
constexpr HgVec3 hg_mvmul3(const HgMat3& lhs, const HgVec3& rhs) {
    HgVec3 result{};
    hg_mvmul(3, 3, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

constexpr HgVec3 operator*(const HgMat3& lhs, const HgVec3& rhs) {
    return hg_mvmul3(lhs, rhs);
}

/**
 * Multiplies a 4x4 matrix and a 4D vector
 *
 * Parameters
 * - lhs The matrix to multiply with
 * - rhs The vector to multiply with
 * Returns
 * - The multiplied vector
 */
constexpr HgVec4 hg_mvmul4(const HgMat4& lhs, const HgVec4& rhs) {
    HgVec4 result{};
    hg_mvmul(4, 4, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

constexpr HgVec4 operator*(const HgMat4& lhs, const HgVec4& rhs) {
    return hg_mvmul4(lhs, rhs);
}

/**
 * Adds two complex numbers
 *
 * Parameters
 * - lhs The left-hand side complex number
 * - rhs The right-hand side complex number
 * Returns
 * - The added complex number
 */
constexpr HgComplex hg_cadd(const HgComplex& lhs, const HgComplex& rhs) {
    return {lhs.r + rhs.r, lhs.i + rhs.i};
}

constexpr HgComplex operator+(const HgComplex& lhs, const HgComplex& rhs) {
    return hg_cadd(lhs, rhs);
}

/**
 * Subtracts two complex numbers
 *
 * Parameters
 * - lhs The left-hand side complex number
 * - rhs The right-hand side complex number
 * Returns
 * - The subtracted complex number
 */
constexpr HgComplex hg_csub(const HgComplex& lhs, const HgComplex& rhs) {
    return {lhs.r - rhs.r, lhs.i - rhs.i};
}

constexpr HgComplex operator-(const HgComplex& lhs, const HgComplex& rhs) {
    return hg_csub(lhs, rhs);
}

/**
 * Multiplies two complex numbers
 *
 * Parameters
 * - lhs The left-hand side complex number
 * - rhs The right-hand side complex number
 * Returns
 * - The multiplied complex number
 */
constexpr HgComplex hg_cmul(const HgComplex& lhs, const HgComplex& rhs) {
    return {lhs.r * rhs.r - lhs.i * rhs.i, lhs.r * rhs.i + lhs.i * rhs.r};
}

constexpr HgComplex operator*(const HgComplex& lhs, const HgComplex& rhs) {
    return hg_cmul(lhs, rhs);
}

/**
 * Adds two quaternions
 *
 * Parameters
 * - lhs The left-hand side quaternion
 * - rhs The right-hand side quaternion
 * Returns
 * - The added quaternion
 */
constexpr HgQuat hg_qadd(const HgQuat& lhs, const HgQuat& rhs) {
    return {lhs.r + rhs.r, lhs.i + rhs.i, lhs.j + rhs.j, lhs.k + rhs.k};
}

constexpr HgQuat operator+(const HgQuat& lhs, const HgQuat& rhs) {
    return hg_qadd(lhs, rhs);
}

/**
 * Subtracts two quaternions
 *
 * Parameters
 * - lhs The left-hand side quaternion
 * - rhs The right-hand side quaternion
 * Returns
 * - The subtracted quaternion
 */
constexpr HgQuat hg_qsub(const HgQuat& lhs, const HgQuat& rhs) {
    return {lhs.r - rhs.r, lhs.i - rhs.i, lhs.j - rhs.j, lhs.k - rhs.k};
}

constexpr HgQuat operator-(const HgQuat& lhs, const HgQuat& rhs) {
    return hg_qsub(lhs, rhs);
}

/**
 * Multiplies two quaternions
 *
 * Parameters
 * - lhs The left-hand side quaternion
 * - rhs The right-hand side quaternion
 * Returns
 * - The multiplied quaternion
 */
constexpr HgQuat hg_qmul(const HgQuat& lhs, const HgQuat& rhs) {
    return {
        lhs.r * rhs.r - lhs.i * rhs.i - lhs.j * rhs.j - lhs.k * rhs.k,
        lhs.r * rhs.i + lhs.i * rhs.r + lhs.j * rhs.k - lhs.k * rhs.j,
        lhs.r * rhs.j - lhs.i * rhs.k + lhs.j * rhs.r + lhs.k * rhs.i,
        lhs.r * rhs.k + lhs.i * rhs.j - lhs.j * rhs.i + lhs.k * rhs.r,
    };
}

constexpr HgQuat operator*(const HgQuat& lhs, const HgQuat& rhs) {
    return hg_qmul(lhs, rhs);
}

/**
 * Computes the conjugate of a quaternion
 *
 * Parameters
 * - quat The quaternion to compute the conjugate of
 * Returns
 * - The conjugate of the quaternion
 */
constexpr HgQuat hg_qconj(const HgQuat& quat) {
    return {quat.r, -quat.i, -quat.j, -quat.k};
}

/**
 * Creates a rotation quaternion from an axis and angle
 *
 * Parameters
 * - axis The axis of the rotation
 * - angle The angle of the rotation
 * Returns
 * - The created quaternion
 */
constexpr HgQuat hg_axis_angle(const HgVec3& axis, f32 angle) {
    f32 half_angle = angle / 2.0f;
    f32 sin_half_angle = sinf(half_angle);
    return {
        cosf(half_angle),
        axis.x * sin_half_angle,
        axis.y * sin_half_angle,
        axis.z * sin_half_angle,
    };
}

/**
 * Rotates a 3D vector using a quaternion
 *
 * Parameters
 * - lhs The quaternion to rotate with
 * - rhs The vector to rotate
 * Returns
 * - The rotated vector
 */
constexpr HgVec3 hg_rotate(const HgQuat& lhs, const HgVec3& rhs) {
    HgQuat q = hg_qmul(lhs, hg_qmul({0.0f, rhs.x, rhs.y, rhs.z}, hg_qconj(lhs)));
    return {q.i, q.j, q.k};
}

/**
 * Rotates a 3x3 matrix using a quaternion
 *
 * Parameters
 * - lhs The quaternion to rotate with
 * - rhs The matrix to rotate
 * Returns
 * - The rotated matrix
 */
constexpr HgMat3 hg_rotate(const HgQuat& lhs, const HgMat3& rhs) {
    return {
        hg_rotate(lhs, rhs.x),
        hg_rotate(lhs, rhs.y),
        hg_rotate(lhs, rhs.z),
    };
}

/**
 * Creates a model matrix for 2D graphics
 *
 * Parameters
 * - position The position of the model
 * - scale The scale of the model
 * - rotation The rotation of the model
 * Returns
 * - The created matrix
 */
HgMat4 hg_model_matrix_2d(const HgVec3& position, const HgVec2& scale, f32 rotation);

/**
 * Creates a model matrix for 3D graphics
 *
 * Parameters
 * - position The position of the model
 * - scale The scale of the model
 * - rotation The rotation of the model
 * Returns
 * - The created matrix
 */
HgMat4 hg_model_matrix_3d(const HgVec3& position, const HgVec3& scale, const HgQuat& rotation);

/**
 * Creates a view matrix
 *
 * Parameters
 * - position The position of the camera
 * - zoom The zoom of the camera
 * - rotation The rotation of the camera
 * Returns
 * - The created matrix
 */
HgMat4 hg_view_matrix(const HgVec3& position, f32 zoom, const HgQuat& rotation);

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
 * Returns
 * - The created matrix
 */
HgMat4 hg_projection_orthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far);

/**
 * Creates a perspective projection matrix
 *
 * Parameters
 * - fov The field of view of the projection in radians
 * - aspect The aspect ratio of the projection
 * - near The near plane of the projection, must be greater than 0.0f
 * - far The far plane of the projection, must be greater than near
 * Returns
 * - The created matrix
 */
HgMat4 hg_projection_perspective(f32 fov, f32 aspect, f32 near, f32 far);

// random number generators : TODO

// hash functions : TODO

// sort and search algorithms : TODO

/**
 * Calculates the maximum number of mipmap levels that an image can have
 *
 * Parameters
 * - width The width of the image
 * - height The height of the image
 * - depth The depth of the image
 * Returns
 * - The maximum number of mipmap levels the image can have
 */
constexpr u32 hg_max_mipmaps(u32 width, u32 height, u32 depth) {
    return (u32)std::log2f((f32)std::max({width, height, depth})) + 1;
}

/**
 * A pointer-count pair
 */
template<typename T>
struct HgSpan {
    /**
     * The array
     */
    T *data;
    /**
     * The number of items in the array
     */
    usize count;

    /**
     * The size of the array in bytes
     *
     * Returns
     * - The size of the array in bytes
     */
    constexpr usize size() {
        return count * sizeof(*data);
    }

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr T& operator[](usize index) {
        assert(data != nullptr);
        assert(index < count);
        return data[index];
    }

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr const T& operator[](usize index) const {
        assert(data != nullptr);
        assert(index < count);
        return data[index];
    }

    /**
     * Implicit conversion can add const
     */
    constexpr operator HgSpan<std::add_const<T>>() const {
        return {(const T*)data, count};
    }
};

template<>
struct HgSpan<const void> {
    /**
     * The array
     */
    const void *data;
    /**
     * The number of items in the array
     */
    usize count;

    /**
     * The size of the array in bytes
     *
     * Returns
     * - The size of the array in bytes
     */
    constexpr usize size() {
        return count;
    }
};

template<>
struct HgSpan<void> {
    /**
     * The array
     */
    void *data;
    /**
     * The number of items in the array
     */
    usize count;

    /**
     * The size of the array in bytes
     *
     * Returns
     * - The size of the array in bytes
     */
    constexpr usize size() {
        return count;
    }

    /**
     * Implicit conversion can add const
     */
    constexpr operator HgSpan<const void>() const {
        return {(const void*)data, count};
    }
};

template<typename T>
constexpr bool operator==(HgSpan<T> lhs, HgSpan<T> rhs) {
    return lhs.data == rhs.data && lhs.count == rhs.count;
}

template<typename T>
constexpr bool operator!=(HgSpan<T> lhs, HgSpan<T> rhs) {
    return lhs.data != rhs.data || lhs.count != rhs.count;
}

template<typename T>
constexpr bool operator==(HgSpan<T> lhs, std::nullptr_t rhs) {
    return lhs.data == rhs && lhs.count == 0;
}

template<typename T>
constexpr bool operator==(std::nullptr_t lhs, HgSpan<T> rhs) {
    return rhs.data == lhs && rhs.count == 0;
}

template<typename T>
constexpr bool operator!=(HgSpan<T> lhs, std::nullptr_t rhs) {
    return lhs.data != rhs;
}

template<typename T>
constexpr bool operator!=(std::nullptr_t lhs, HgSpan<T> rhs) {
    return rhs.data != lhs;
}

/**
 * The interface for generic allocators
 */
struct HgAllocator {
    /**
     * Allocates memory
     *
     * Parameters
     * - size The size to allocate in bytes
     * - alignment The alignment in bytes of the allocation
     */
    virtual void *alloc_fn(usize size, usize alignment) = 0;

    /**
     * Changes the size of an allocation, potentially returning a different
     * allocation, with the data copied over
     *
     * Parameters
     * - allocation The allocation to free
     * - old_size The original size of the allocation in bytes
     * - new_size The size to allocate in bytes
     * - alignment The alignment in bytes of the allocation
     */
    virtual void *realloc_fn(void *allocation, usize old_size, usize new_size, usize alignment) = 0;

    /**
     * Frees allocated memory
     *
     * Parameters
     * - allocation The allocation to free
     * - size The size of the allocation in bytes
     * - alignment The alignment in bytes of the allocation
     */
    virtual void free_fn(void *allocation, usize size, usize alignment) = 0;

    /**
     * A convenience to allocate using a type
     *
     * Returns
     * - The allocated item
     */
    template<typename T>
    T *alloc() {
        static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);
        return (T *)alloc_fn(sizeof(T), alignof(T));
    }

    /**
     * A convenience to allocate an array using a type
     *
     * Parameters
     * - count The number of T to allocate
     * Returns
     * - The allocated array
     */
    template<typename T>
    HgSpan<T> alloc(usize count) {
        static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);
        HgSpan<T> span;
        span.data = (T *)alloc_fn(count * sizeof(T), alignof(T));
        span.count = span.data != nullptr ? count : 0;
        return span;
    }

    /**
     * A convenience to allocate a void array
     *
     * Parameters
     * - size The size in bytes to allocate
     * Returns
     * - The allocated array
     */
    HgSpan<void> alloc(usize size, usize alignment) {
        HgSpan<void> span;
        span.data = alloc_fn(size, alignment);
        span.count = span.data != nullptr ? size : 0;
        return span;
    }

    /**
     * A convenience to reallocate an array using a type
     *
     * Parameters
     * - allocation The allocation to reallocate
     * - count The new number of T to allocate
     * Returns
     * - The rallocated array
     */
    template<typename T>
    HgSpan<T> realloc(HgSpan<T> allocation, usize count) {
        static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);
        HgSpan<T> span;
        span.data = (T *)realloc_fn(allocation.data, allocation.count * sizeof(T), count * sizeof(T), alignof(T));
        span.count = span.data != nullptr ? count : 0;
        return span;
    }

    /**
     * A convenience to reallocate a void array
     *
     * Parameters
     * - allocation The alloation to reallocate
     * - size The new size to allocate
     * Returns
     * - The rallocated array
     */
    HgSpan<void> realloc(HgSpan<void> allocation, usize size, usize alignment) {
        HgSpan<void> span;
        span.data = realloc_fn(allocation.data, allocation.count, size, alignment);
        span.count = span.data != nullptr ? size : 0;
        return span;
    }

    /**
     * A convenience to free using a type
     *
     * Parameters
     * - allocation The allocation to free
     */
    template<typename T>
    void free(T *allocation) {
        static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);
        free_fn(allocation, sizeof(T), alignof(T));
    }

    /**
     * A convenience to free an array using a type
     *
     * Parameters
     * - allocation The allocation to free
     */
    template<typename T>
    void free(HgSpan<T> allocation) {
        static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);
        free_fn(allocation.data, allocation.count * sizeof(T), alignof(T));
    }

    /**
     * A convenience to free a void array
     *
     * Parameters
     * - allocation The allocation to free
     */
    void free(HgSpan<void> allocation, usize alignment) {
        free_fn(allocation.data, allocation.count, alignment);
    }
};

/**
 * Get the global allocator for malloc/free
 *
 * Returns
 * - The global allocator
 */
HgAllocator& hg_persistent_allocator();

/**
 * Gets the current temporary allocator on this thread
 *
 * Returns
 * - This thread's temporary allocator
 */
HgAllocator& hg_temp_allocator();

/**
 * Sets the current temporary allocator on this thread
 *
 * Note, if this function is never called, the default temp allocator is the
 * global allocator
 *
 * Note, the temp allocator will remain stable between library calls, i.e. even
 * if it is changed within a function it will be changed by before returning
 *
 * Parameters
 * - The allocator to set for this thread
 */
void hg_temp_allocator_set(HgAllocator *allocator);

/**
 * C malloc, realloc, and free in the HgAllocator interface
 */
struct HgStdAllocator : public HgAllocator {
    /**
     * Calls malloc, checking for nullptr in debug mode
     *
     * Parameters
     * - size The size of the allocation in bytes
     * - alignment The alignment in bytes of the allocation
     * Returns
     * - The allocated memory
     */
    void *alloc_fn(usize size, usize alignment) override;

    /**
     * Calls realloc, checking for nullptr in debug mode
     *
     * Parameters
     * - allocation The allocation to resize
     * - old_size The size of the original allocation in bytes
     * - new_size The size of the new allocation in bytes
     * - alignment The alignment in bytes of the allocation
     * Returns
     * - The allocated memory
     */
    void *realloc_fn(void *allocation, usize old_size, usize new_size, usize alignment) override;

    /**
     * Calls free
     *
     * Parameters
     * - allocation The allocation to free
     * - size The size of the allocation in bytes
     * - alignment The alignment in bytes of the allocation
     */
    void free_fn(void *allocation, usize size, usize alignment) override;
};

/**
 * An arena allocator
 *
 * Allocations are made very quickly, and are not freed individually, instead
 * the whole block is freed at once
 */
struct HgArena : public HgAllocator {
    /**
     * The allocator used to create the arena
     */
    HgAllocator *allocator;
    /**
     * A pointer to the memory being allocated
     */
    HgSpan<void> memory;
    /**
     * The next allocation to be given out
     */
    void *head;

    /**
     * Allocates an arena with capacity
     *
     * Parameters
     * - allocator The allocator to use to create the arena
     * - capacity The size of the block to allocate and use
     * Returns
     * - The allocated arena
     */
    static HgArena create(HgAllocator& allocator, usize capacity);

    /**
     * Frees an arena's memory
     */
    void destroy();

    /**
     * Frees all allocations from an arena
     */
    void reset();

    /**
     * Allocates memory from an arena
     *
     * Allocations are not individually freed, hg_arena() is
     * called instead to free all allocations at once
     *
     * Parameters
     * - size The size in bytes of the allocation
     * - alignment The required alignment of the allocation in bytes
     * Returns
     * - The allocation if successful
     * - nullptr if the allocation exceeds capacity, or size is 0
     */
    void *alloc_fn(usize size, usize alignment) override;

    /**
     * Reallocates memory from a arena
     *
     * Simply increases the size if allocation is the most recent allocation
     *
     * Parameters
     * - allocation The allocation to grow, must be the last allocation made
     * - old_size The original size in bytes of the allocation
     * - new_size The new size in bytes of the allocation
     * - alignment The required alignment of the allocation in bytes
     * Returns
     * - The allocation if successful
     * - nullptr if the allocation exceeds capacity
     */
    void *realloc_fn(void *allocation, usize old_size, usize new_size, usize alignment) override;

    /**
     * Does nothing, only exists to fit allocator interface
     *
     * Parameters
     * - allocation The allocation to free, must be the last allocation made
     * - size The size of the allocation
     * - alignment The required alignment of the allocation in bytes
     */
    void free_fn(void *allocation, usize size, usize alignment) override;
};

/**
 * A stack allocator
 *
 * Allocations are made very quickly, but must be freed in reverse order
 */
struct HgStack : public HgAllocator {
    /**
     * The allocator used to create the stack
     */
    HgAllocator *allocator;
    /*
     * A pointer to the memory being allocated
     */
    HgSpan<void> memory;
    /*
     * The next allocation to be given out
     */
    usize head;

    /**
     * Allocates a stack allocator with capacity
     *
     * Parameters
     * - allocator The allocator to allocate memory from
     * - capacity The size of the block to allocate and use
     * Returns
     * - The allocated stack
     */
    static HgStack create(HgAllocator& allocator, usize capacity);

    /**
     * Frees a stack's memory
     */
    void destroy();

    /**
     * Frees all allocations from an stack
     */
    void reset();

    /**
     * Allocates memory from a stack
     *
     * Parameters
     * - size The size in bytes of the allocation
     * - alignment The required alignment of the allocation in bytes
     * Returns
     * - The allocation if successful
     * - nullptr if the allocation exceeds capacity, or size is 0
     */
    void *alloc_fn(usize size, usize alignment) override;

    /**
     * Reallocates memory from a stack
     *
     * Simply increases the size if allocation is the most recent allocation
     *
     * Parameters
     * - allocation The allocation to grow, must be the last allocation made
     * - old_size The original size in bytes of the allocation
     * - new_size The new size in bytes of the allocation
     * - alignment The required alignment of the allocation in bytes
     * Returns
     * - The allocation if successful
     * - nullptr if the allocation exceeds capacity
     */
    void *realloc_fn(void *allocation, usize old_size, usize new_size, usize alignment) override;

    /**
     * Frees an allocation from a stack
     *
     * Can only deallocate the most recent allocation, otherwise does nothing
     *
     * Parameters
     * - allocation The allocation to free, must be the last allocation made
     * - size The size of the allocation
     * - alignment The required alignment of the allocation in bytes
     */
    void free_fn(void *allocation, usize size, usize alignment) override;
};

/**
 * A dynamically sized array
 */
template<typename T>
struct HgArray {
    static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);

    /**
     * The allocator used to create, destroy, and resize the array
     */
    HgAllocator *allocator;
    /**
     * The allocated space for the array
     */
    HgSpan<T> items;
    /**
     * The number of active items in the array
     */
    usize count;

    /**
     * Access using the index operator
     */
    T& operator[](usize index) {
        return items[index];
    }

    /**
     * Access using the index operator in a const context
     */
    const T& operator[](usize index) const {
        assert(index < count);
        return items[index];
    }

    /**
     * Allocate a new dynamic array
     *
     * Parameters
     * - allocator The allocator to use
     * - count The number of active items to begin with
     * - capacity The max number of items before reallocating
     * Returns
     * - The allocated array
     */
    static HgArray<T> create(HgAllocator& allocator, usize count, usize capacity) {
        assert(capacity > 0);
        assert(count <= capacity);

        HgArray arr{};
        arr.allocator = &allocator;
        arr.items = allocator.alloc<T>(capacity);
        arr.count = count;
        std::memset(arr.items.data, 0, count * sizeof(T));
        return arr;
    }

    /**
     * Free the dynamic array
     */
    void destroy() {
        allocator->free(items);
    }

    /**
     * Push an item to the end to the array, resizing if needed
     *
     * Parameters
     * - args The arguments to use to construct the new item
     */
    template<typename... U>
    void push(U&&... args) {
        if (count >= items.count) {
            assert(items.count > 0);
            items = allocator->realloc(items, items.count * 2);
        }
        new (&items[count]) T{std::forward<U>(args)...};
        ++count;
    }

    /**
     * Pops an item off the end of the array
     */
    void pop() {
        assert(count > 0);
        --count;
    }

    /**
     * Inserts an item into the array, resizing if needed
     *
     * Parameters
     * - index The index the new item will be placed at
     * - args The arguments to use to construct the new item
     */
    template<typename... U>
    void insert(usize index, U&&... args) {
        assert(index <= count);
        if (count >= items.count) {
            assert(items.count > 0);
            items = allocator->realloc(items, items.count * 2);
        }
        std::memmove(items.data + index + 1, items.data + index, (count - index) * sizeof(*items.data));
        ++count;
        new (&items[index]) T{std::forward<U>(args)...};
    }

    /**
     * Removes the item at the index, subsequent items to taking its place
     *
     * Parameters
     * - index The index of the item to remove
     */
    void remove(usize index) {
        assert(index < count);
        --count;
        std::memmove(items.data + index, items.data + index + 1, (count - index) * sizeof(*items.data));
    }
};

/**
 * An option type wrapper, where the value may or may not exist
 */
template<typename T>
struct HgOption {
    static_assert(std::is_trivially_copyable_v<T> && std::is_standard_layout_v<T>);

    /**
     * The value stored, may not be valid if has_value == false
     */
    T value;
    /**
     * Whether the container contains an item
     */
    bool has_value;

    /**
     * Access the value like a pointer
     */
    constexpr T& operator*() {
        assert(has_value);
        return value;
    }

    /**
     * Access the value like a pointer in a const context
     */
    constexpr const T& operator*() const {
        assert(has_value);
        return value;
    }

    /**
     * Access the value like a pointer
     */
    constexpr T& operator->() {
        assert(has_value);
        return value;
    }

    /**
     * Access the value like a pointer in a const context
     */
    constexpr const T& operator->() const {
        assert(has_value);
        return value;
    }

    /**
     * Implicitly cast to bool
     */
    constexpr operator bool() const {
        return has_value;
    }
};

/**
 * Loads a binary file
 *
 * Parameters
 * - allocator Where to allocate the memory from
 * - path The null terminated path to the file to load, must not be nullptr
 * Returns
 * - The loaded file data if successful
 * - nullptr if the file was not found or could not be read
 */
HgSpan<void> hg_file_load_binary(HgAllocator& allocator, const char *path);

/**
 * Unloads a binary file
 *
 * Parameters
 * - allocator Where to free the memory to
 * - data The data to unload, noop if nullptr
 */
void hg_file_unload_binary(HgAllocator& allocator, HgSpan<void> data);

/**
 * Saves a binary file
 *
 * Parameters
 * - data The data to save, may be nullptr if size is 0
 * - path The path to the file to save, must not be empty
 * Returns
 * - true if the file was saved successfully
 * - false if the file could not be written
 */
bool hg_file_save_binary(HgSpan<const void> data, const char *path);

// text files : TODO
// json files : TODO
// image files : TODO
// 3d model files : TODO
// audio files : TODO

// thread pool : TODO

/**
 * A key on the keyboard or button on the mouse
 */
typedef enum HgKey {
    HG_KEY_NONE = 0,
    HG_KEY_0,
    HG_KEY_1,
    HG_KEY_2,
    HG_KEY_3,
    HG_KEY_4,
    HG_KEY_5,
    HG_KEY_6,
    HG_KEY_7,
    HG_KEY_8,
    HG_KEY_9,
    HG_KEY_Q,
    HG_KEY_W,
    HG_KEY_E,
    HG_KEY_R,
    HG_KEY_T,
    HG_KEY_Y,
    HG_KEY_U,
    HG_KEY_I,
    HG_KEY_O,
    HG_KEY_P,
    HG_KEY_A,
    HG_KEY_S,
    HG_KEY_D,
    HG_KEY_F,
    HG_KEY_G,
    HG_KEY_H,
    HG_KEY_J,
    HG_KEY_K,
    HG_KEY_L,
    HG_KEY_Z,
    HG_KEY_X,
    HG_KEY_C,
    HG_KEY_V,
    HG_KEY_B,
    HG_KEY_N,
    HG_KEY_M,
    HG_KEY_SEMICOLON,
    HG_KEY_COLON,
    HG_KEY_APOSTROPHE,
    HG_KEY_QUOTATION,
    HG_KEY_COMMA,
    HG_KEY_PERIOD,
    HG_KEY_QUESTION,
    HG_KEY_GRAVE,
    HG_KEY_TILDE,
    HG_KEY_EXCLAMATION,
    HG_KEY_AT,
    HG_KEY_HASH,
    HG_KEY_DOLLAR,
    HG_KEY_PERCENT,
    HG_KEY_CAROT,
    HG_KEY_AMPERSAND,
    HG_KEY_ASTERISK,
    HG_KEY_LPAREN,
    HG_KEY_RPAREN,
    HG_KEY_LBRACKET,
    HG_KEY_RBRACKET,
    HG_KEY_LBRACE,
    HG_KEY_RBRACE,
    HG_KEY_EQUAL,
    HG_KEY_LESS,
    HG_KEY_GREATER,
    HG_KEY_PLUS,
    HG_KEY_MINUS,
    HG_KEY_SLASH,
    HG_KEY_BACKSLASH,
    HG_KEY_UNDERSCORE,
    HG_KEY_BAR,
    HG_KEY_UP,
    HG_KEY_DOWN,
    HG_KEY_LEFT,
    HG_KEY_RIGHT,
    HG_KEY_MOUSE1,
    HG_KEY_MOUSE2,
    HG_KEY_MOUSE3,
    HG_KEY_MOUSE4,
    HG_KEY_MOUSE5,
    HG_KEY_LMOUSE = HG_KEY_MOUSE1,
    HG_KEY_RMOUSE = HG_KEY_MOUSE2,
    HG_KEY_MMOUSE = HG_KEY_MOUSE3,
    HG_KEY_ESCAPE,
    HG_KEY_SPACE,
    HG_KEY_ENTER,
    HG_KEY_BACKSPACE,
    HG_KEY_DELETE,
    HG_KEY_INSERT,
    HG_KEY_TAB,
    HG_KEY_HOME,
    HG_KEY_END,
    HG_KEY_F1,
    HG_KEY_F2,
    HG_KEY_F3,
    HG_KEY_F4,
    HG_KEY_F5,
    HG_KEY_F6,
    HG_KEY_F7,
    HG_KEY_F8,
    HG_KEY_F9,
    HG_KEY_F10,
    HG_KEY_F11,
    HG_KEY_F12,
    HG_KEY_LSHIFT,
    HG_KEY_RSHIFT,
    HG_KEY_LCTRL,
    HG_KEY_RCTRL,
    HG_KEY_LMETA,
    HG_KEY_RMETA,
    HG_KEY_LALT,
    HG_KEY_RALT,
    HG_KEY_LSUPER,
    HG_KEY_RSUPER,
    HG_KEY_CAPSLOCK,
    HG_KEY_COUNT,
} HgKey;

struct HgWindow {
    struct Internals;

    /**
     * Platform specific resources for a window
     */
    Internals *internals;

    /**
     * Configuration for a window
     */
    struct Config {
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
    };

    /**
     * Creates a window
     *
     * Parameters
     * - config The window configuration
     * Returns
     * - The created window, will never be nullptr
     */
    static HgWindow create(const Config& config);

    /**
     * Destroys a window
     */
    void destroy();

    /**
     * Sets the windows icons : TODO
     *
     * Parameters
     * - icon_data The pixels of the image to set the icon to, must not be nullptr
     * - width The width in pixels of the icon
     * - height The height in pixels of the icon
     */
    void set_icon(u32 *icon_data, u32 width, u32 height);

    /**
     * Gets whether the window is fullscreen or not : TODO
     *
     * Returns
     * - Whether the window is fullscreen
     */
    bool is_fullscreen();

    /**
     * Sets the window to fullscreen of windowed mode : TODO
     *
     * Parameters
     * - fullscreen Whether to set fullscreen, or set windowed
     */
    void set_fullscreen(bool fullscreen);

    /**
     * The builtin cursor images
     */
    typedef enum Cursor {
        HG_CURSOR_NONE = 0,
        HG_CURSOR_ARROW,
        HG_CURSOR_TEXT,
        HG_CURSOR_WAIT,
        HG_CURSOR_CROSS,
        HG_CURSOR_HAND,
    } Cursor;

    /**
     * Sets the window's cursor to a platform defined icon : TODO
     */
    void set_cursor(Cursor cursor);

    /**
     * Sets the window's cursor to a custom image : TODO
     */
    void set_cursor_image(u32 *data, u32 width, u32 height);

    /**
     * Checks if the window was closed via close button or window manager
     *
     * hg_window_close() is not automatically called when this function returns
     * true, and may be called manually
     *
     * Returns
     * - whether the window was closed
     */
    bool was_closed();

    /**
     * Checks if the window was resized
     *
     * Returns
     * - whether the window was resized
     */
    bool was_resized();

    /**
     * Gets the size of the window in pixels
     *
     * Parameters
     * - width A pointer to store the width, must not be nullptr
     * - height A pointer to store the height, must not be nullptr
     */
    void get_size(u32 *width, u32 *height);

    /**
     * Gets the most recent mouse position
     *
     * Parameters
     * - x A pointer to store the x position
     * - y A pointer to store the y position
     */
    void get_mouse_pos(f64& x, f64& y);

    /**
     * Gets the most recent mouse delta
     *
     * Parameters
     * - x A pointer to store the x delta
     * - y A pointer to store the y delta
     */
    void get_mouse_delta(f64& x, f64& y);

    /**
     * Checks if a key is being held down
     *
     * Parameters
     * - key The key to check
     * Returns
     * - whether the key is being held down
     */
    bool is_key_down(HgKey key);

    /**
     * Checks if a key was pressed this frame
     *
     * Parameters
     * - key The key to check
     * Returns
     * - whether the key was pressed this frame
     */
    bool was_key_pressed(HgKey key);

    /**
     * Checks if a key was released this frame
     *
     * Parameters
     * - key The key to check
     * Returns
     * - whether the key was released this frame
     */
    bool was_key_released(HgKey key);
};

/**
 * Create a Vulkan surface for the window, according to the platform
 *
 * Parameters
 * - instance The Vulkan instance, must not be nullptr
 * - window The window to create a surface for
 * Returns
 * - The created Vulkan surface, will never be nullptr
 */
VkSurfaceKHR hg_vk_create_surface(VkInstance instance, HgWindow window);

/**
 * Processes all events since the last call to process events or startup
 *
 * Must be called every frame before querying input
 * Processes all events, so all windows must be given
 * Updates the each window's input state
 *
 * Parameters
 * - hg The hg context, must not be nullptr
 * - windows All open windows
 */
void hg_window_process_events(HgSpan<const HgWindow> windows);

// audio system : TODO

/**
 * Loads the Vulkan library and the functions required to create an instance
 */
void hg_vk_load();

/**
 * Loads the Vulkan functions which use the instance
 *
 * Note, this function is automatically called from hg_vk_create_instance
 *
 * Parameters
 * - instance The Vulkan instance, must not be nullptr
 */
void hg_vk_load_instance(VkInstance instance);

/**
 * Loads the Vulkan functions which use the Device
 *
 * Note, this function is automatically called from
 * hg_vk_create_single_queue_device
 *
 * Parameters
 * - device The Vulkan device, must not be nullptr
 */
void hg_vk_load_device(VkDevice device);

/**
 * Turns a VkResult into a string
 *
 * Parameters
 * - result The result enum to stringify
 * Returns
 * - The string of the enum value's name
 */
const char *hg_vk_result_string(VkResult result);

/**
 * Turns a VkFormat into the size in bytes
 * 
 * Parameters
 * - format The format to get the size of
 * Returns
 * - The size of the format in bytes
 */
u32 hg_vk_format_to_size(VkFormat format);

/**
 * Creates a Vulkan instance with sensible defaults
 * 
 * In debug mode, enables debug messaging
 *
 * Note, loads Vulkan function pointers automatically
 *
 * Parameters
 * - app_name The name of the application, may be nullptr
 * Returns
 * - The created VkInstance, will never be nullptr
 */
VkInstance hg_vk_create_instance(const char *app_name);

/**
 * Creates a Vulkan debug messenger
 *
 * Parameters
 * - instance The Vulkan instance, must not be nullptr
 * Returns
 * - The created debug messenger, will never be nullptr
 */
VkDebugUtilsMessengerEXT hg_vk_create_debug_messenger(VkInstance instance);

/**
 * Find the first queue family index which supports the the queue flags
 *
 * Parameters
 * - gpu The physical device, must not be nullptr
 * - queue_flags The flags required of the queue family
 * Returns
 * - The queue family if found
 * - No value if not found
 */
HgOption<u32> hg_vk_find_queue_family(VkPhysicalDevice gpu, VkQueueFlags queue_flags);

// find gpu with multiple potential queues : TODO
// create device with multiple potential queues : TODO

/**
 * A Vulkan device with a single general-purpose queue
 */
struct HgSingleQueueDeviceData {
    /**
     * The handle to the Vulkan device object
     */
    VkDevice handle;
    /**
     * The handle to the associated Vulkan physical device
     */
    VkPhysicalDevice gpu;
    /**
     * The created Vulkan queue
     */
    VkQueue queue;
    /**
     * The index of the queue family that the queue is from
     */
    u32 queue_family;
};

/**
 * Creates a Vulkan device with a single general-purpose queue
 *
 * Enables the swapchain extension, and the synchronization 2 and dynamic
 * rendering features
 *
 * Note, loads Vulkan function pointers automatically
 *
 * Parameters
 * - gpu The physical device, must not be nullptr
 * - queue_family Which family to create the queue in
 * Returns
 * - The created Vulkan device, will never be nullptr
 */
HgSingleQueueDeviceData hg_vk_create_single_queue_device(VkInstance instance);

/**
 * Configuration for Vulkan pipelines
 */
struct HgVkPipelineConfig {
    /**
     * The format of the color attachments, none can be UNDEFINED
     */
    HgSpan<const VkFormat> color_attachment_formats;
    /**
     * The format of the depth attachment, if UNDEFINED no depth attachment
     */
    VkFormat depth_attachment_format;
    /**
     * The format of the stencil attachment, if UNDEFINED no stencil attachment
     */
    VkFormat stencil_attachment_format;
    /**
     * The shaders
     */
    HgSpan<const VkPipelineShaderStageCreateInfo> shader_stages;
    /**
     * The pipeline layout
     */
    VkPipelineLayout layout;
    /**
     * Descriptions of the vertex bindings, may be nullptr
     */
    HgSpan<const VkVertexInputBindingDescription> vertex_bindings;
    /**
     * Descriptions of the vertex attributes, may be nullptr
     */
    HgSpan<const VkVertexInputAttributeDescription> vertex_attributes;
    /**
     * How to interpret vertices into topology, defaults to POINT_LIST
     */
    VkPrimitiveTopology topology;
    /**
     * The number of patch control points in the tesselation stage
     */
    u32 tesselation_patch_control_points;
    /**
     * How polygons are drawn, defaults to FILL
     */
    VkPolygonMode polygon_mode;
    /**
     * Which face is treated as the front, defaults to COUNTER_CLOCKWISE
     */
    VkFrontFace front_face;
    /**
     * How many samples are used in MSAA, 0 defaults to 1
     */
    VkSampleCountFlagBits multisample_count;
    /**
     * Enables back/front face culling, defaults to NONE
     */
    VkCullModeFlagBits cull_mode;
    /**
     * Enables color blending using pixel alpha values
     */
    bool enable_color_blend;
};

/**
 * Creates a graphics pipeline
 *
 * Parameters
 * - device The Vulkan device, must not be nullptr
 * - config The pipeline configuration
 * Returns
 * - The created graphics pipeline, will never be nullptr
 */
VkPipeline hg_vk_create_graphics_pipeline(VkDevice device, const HgVkPipelineConfig& config);

/**
 * Creates a compute pipeline
 *
 * There cannot be any attachments
 * There can only be one shader stage, COMPUTE
 * There cannot be any vertex inputs
 *
 * Parameters
 * - device The Vulkan device, must not be nullptr
 * - config The pipeline configuration
 * Returns
 * - The created compute pipeline, will never be nullptr
 */
VkPipeline hg_vk_create_compute_pipeline(VkDevice device, const HgVkPipelineConfig& config);

/**
 * Attempts to find the index of a memory type which has the desired flags and
 * does not have the undesired flags
 *
 * Note, if no such memory type exists, the next best thing will be found
 *
 * The bitmask must not mask out all memory types
 *
 * Parameters
 * - gpu The Vulkan physical device, must not be nullptr
 * - bitmask A bitmask of which memory types cannot be used, must not be 0
 * - desired_flags The flags which the type should 
 * - undesired_flags The flags which the type should not have, though may have
 * Returns
 * - The found index of the memory type
 */
u32 hg_vk_find_memory_type_index(
    VkPhysicalDevice gpu,
    u32 bitmask,
    VkMemoryPropertyFlags desired_flags,
    VkMemoryPropertyFlags undesired_flags);

// Vulkan allocator : TODO?

/**
 * A Vulkan swapchain and associated data
 */
struct HgSwapchainData {
    /**
     * The handle to the Vulkan swapchain object
     */
    VkSwapchainKHR handle;
    /**
     * The width of the swapchain's images
     */
    u32 width;
    /**
     * The height of the swapchain's images
     */
    u32 height;
    /**
     * The pixel format of the swapchain's images
     */
    VkFormat format;
};

/**
 * Creates a Vulkan swapchain
 *
 * Parameters
 * - device The Vulkan device, must not be nullptr
 * - gpu The physical device to query capabilities, must not be nullptr
 * - old_swapchain The old swapchain, may be nullptr
 * - surface The surface to create from
 * - image_usage How the swapchain's images will be used
 * - desired_mode The preferred present mode (fallback to FIFO)
 * Returns
 * - The created Vulkan swapchain
 */
HgSwapchainData hg_vk_create_swapchain(
    VkDevice device,
    VkPhysicalDevice gpu,
    VkSwapchainKHR old_swapchain,
    VkSurfaceKHR surface,
    VkImageUsageFlags image_usage,
    VkPresentModeKHR desired_mode);

/**
 * A system to synchronize frames rendering to multiple swapchain images at once
 */
struct HgSwapchainCommands {
    VkCommandPool cmd_pool;
    VkSwapchainKHR swapchain;
    VkCommandBuffer *cmds;
    VkFence *frame_finished;
    VkSemaphore *image_available;
    VkSemaphore *ready_to_present;
    u32 frame_count;
    u32 current_frame;
    u32 current_image;

    /**
     * Creates a swapchain command buffer system
     *
     * Parameters
     * - device The Vulkan device, must not be nullptr
     * - swapchain The Vulkan swapchain to create frames for, must not be nullptr
     * - cmd_pool The Vulkan command pool to allocate cmds from, must not be nullptr
     * Returns
     * - The created swaphchain command buffer system
     */
    static HgSwapchainCommands create(VkDevice device, VkSwapchainKHR swapchain, VkCommandPool cmd_pool);

    /**
     * Destroys a swaphchain command buffer system
     *
     * Parameters
     * - device The Vulkan device, must not be nullptr
     */
    void destroy(VkDevice device);

    /**
     * Acquires the next swapchain image and begins its command buffer
     *
     * Parameters
     * - device The Vulkan device, must not be nullptr
     * Returns
     * - The command buffer to record this frame
     * - nullptr if the swapchain is out of date
     */
    VkCommandBuffer acquire_and_record(VkDevice device);

    /**
     * Finishes recording the command buffer and presents the swapchain image
     *
     * Parameters
     * - queue The Vulkan queue, must not be nullptr
     */
    void end_and_present(VkQueue queue);
};

/**
 * Writes to a Vulkan device local buffer through a staging buffer
 *
 * Parameters
 * - device The Vulkan device, must not be nullptr
 * - allocator The Vulkan allocator, must not be nullptr
 * - cmd_pool The command pool for the queue, must not be nullptr
 * - transfer_queue The Vulkan queue to transfer on, must not be nullptr
 * - dst The buffer to write to, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - src The data to write, must not be nullptr
 */
void hg_vk_buffer_staging_write(
    VkDevice device,
    VmaAllocator allocator,
    VkCommandPool cmd_pool,
    VkQueue transfer_queue,
    VkBuffer dst,
    usize offset,
    HgSpan<const void> src);

/**
 * Reads from a Vulkan device local buffer through a staging buffer
 *
 * Parameters
 * - device The Vulkan device, must not be nullptr
 * - allocator The Vulkan allocator, must not be nullptr
 * - cmd_pool The command pool for the queue, must not be nullptr
 * - transfer_queue The Vulkan queue to transfer on, must not be nullptr
 * - dst The location to write to, must not be nullptr
 * - src The buffer to read from, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 */
void hg_vk_buffer_staging_read(
    VkDevice device,
    VmaAllocator allocator,
    VkCommandPool cmd_pool,
    VkQueue transfer_queue,
    HgSpan<void> dst,
    VkBuffer src,
    usize offset);

/**
 * Configuration for a staging image write
 */
struct HgVkImageStagingWriteConfig {
    /**
     * The image to write to, must not be nullptr
     */
    VkImage dst_image;
    /**
     * The subresource of the image to write to
     */
    VkImageSubresourceLayers subresource;
    /**
     * The data to write, must not be nullptr
     */
    void *src_data;
    /**
     * The width of the image in pixels, must be greater than 0
     */
    u32 width;
    /**
     * The width of the image in pixels, must be greater than 0
     */
    u32 height;
    /**
     * The width of the image in pixels, must be greater than 0
     */
    u32 depth;
    /**
     * The format of each pixel, must not be UNDEFINED
     */
    VkFormat format;
    /**
     * The layout to transition to after transfering
     */
    VkImageLayout layout;
};

/**
 * Writes to a Vulkan device local image through a staging buffer
 *
 * Parameters
 * - device The Vulkan device, must not be nullptr
 * - allocator The Vulkan allocator, must not be nullptr
 * - cmd_pool The command pool for the queue, must not be nullptr
 * - transfer_queue The Vulkan queue to transfer on, must not be nullptr
 * - config The configuration for the write
 */
void hg_vk_image_staging_write(
    VkDevice device,
    VmaAllocator allocator,
    VkCommandPool cmd_pool,
    VkQueue transfer_queue,
    const HgVkImageStagingWriteConfig& config);

/**
 * Configuration for a staging image write
 */
struct HgVkImageStagingReadConfig {
    /**
     * The location to write to, must not be nullptr
     */
    void *dst;
    /**
     * The image to read from, must not be nullptr
     */
    VkImage src_image;
    /**
     * The layout the image was in before reading, must not be UNDEFINED
     */
    VkImageLayout layout;
    /**
     * The subresource of the image to read from
     */
    VkImageSubresourceLayers subresource;
    /**
     * The width of the image in pixels, must be greater than 0
     */
    u32 width;
    /**
     * The width of the image in pixels, must be greater than 0
     */
    u32 height;
    /**
     * The width of the image in pixels, must be greater than 0
     */
    u32 depth;
    /**
     * The format of each pixel, must not be UNDEFINED
     */
    VkFormat format;
};

/**
 * Reads from a Vulkan device local image through a staging buffer
 *
 * Parameters
 * - device The Vulkan device, must not be nullptr
 * - allocator The Vulkan allocator, must not be nullptr
 * - cmd_pool The command pool for the queue, must not be nullptr
 * - transfer_queue The Vulkan queue to transfer on, must not be nullptr
 * - config The configuration for the read, must not be nullptr
 */
void hg_vk_image_staging_read(
    VkDevice device,
    VmaAllocator allocator,
    VkCommandPool cmd_pool,
    VkQueue transfer_queue,
    const HgVkImageStagingReadConfig& config);

// cubemap read/write : TODO

// mipmap generation : TODO

// ecs multithreading support : TODO

/**
 * The handle for an ECS entity
 */
using HgEntityID = u64;

/**
 * An ECS system index with compile time type info
 */
template<typename SystemType, typename ComponentType>
struct HgSystemID {
    static_assert(std::is_trivially_copyable_v<ComponentType> && std::is_standard_layout_v<ComponentType>);
    static_assert((std::is_trivially_copyable_v<SystemType> && std::is_standard_layout_v<SystemType>) || std::is_void_v<SystemType>);

    u32 index;

    operator u32() const {
        return index;
    }
};

/**
 * An entity component system
 */
struct HgECS {
    /**
     * A system of components in an entity component system
     */
    struct System {
        /**
         * Unique data used by the system
         */
        HgSpan<void> system_data;
        /**
         * The alignment of the system's unique data
         */
        usize system_data_alignment;
        /**
         * entity_indices[entity id] is the index into components and
         * component_entities for that entity id
         */
        HgSpan<u32> entity_indices;
        /**
         * Each index is the entity associated with components[index]
         */
        HgSpan<HgEntityID> component_entities;
        /**
         * The data for the components
         */
        HgSpan<void> components;
        /**
         * The size of each component in bytes
         */
        u32 component_size;
        /**
         * The memory alignment of each component
         */
        u32 component_alignment;
        /**
         * The current number of components
         */
        u32 component_count;
    };

    /**
     * The allocator used by the ECS
     */
    HgAllocator *allocator;
    /**
     * The pool of entity ids
     */
    HgSpan<HgEntityID> entity_pool;
    /**
     * The next entity in the pool
     */
    HgEntityID entity_next;
    /**
     * The component systems
     */
    HgSpan<System> systems;
    /**
     * The number of systems
     */
    u32 system_count;

    /**
     * Creates an entity component system
     *
     * Parameters
     * - allocator The allocator the ecs will use
     * - max_entities The max entities that can exist, must be greater than 0
     * - max_systems The max systems that can exist, must be greater than 0
     * Returns
     * - The created entity component system
     */
    static HgECS create(HgAllocator& allocator, u32 max_entities, u32 max_systems);

    /**
     * Destroys an entity component system
     */
    void destroy();

    /**
     * Resets an entity component system, removing all entities and components
     */
    void reset();

    /**
     * Adds a component system to the ecs
     *
     * Parameters
     * - data_size The size in bytes of the unique data used by the system
     * - data_alignment The alignment of the unique data used by the system
     * - component_size The size in bytes of each component
     * - component_alignment The alignment of each component
     * - max_component The maximum number of components in the system
     * Result
     * - The index of the new system
     */
    u32 add_system_untyped(
        usize data_size,
        usize data_alignment,
        u32 component_size,
        u32 component_alignment,
        u32 max_components);

    /**
     * Adds a component system using the given type
     *
     * Parameters
     * - max_component The maximum number of components in the system
     * Result
     * - The index of the new system with type info
     */
    template<typename SystemType, typename ComponentType>
    HgSystemID<SystemType, ComponentType> add_system(u32 max_components) {
        if constexpr (std::is_void_v<SystemType>) {
            return {add_system_untyped(
                0, 0,
                sizeof(ComponentType),
                alignof(ComponentType),
                max_components)};
        } else {
            return {add_system_untyped(
                sizeof(SystemType),
                alignof(SystemType),
                sizeof(ComponentType),
                alignof(ComponentType),
                max_components)};
        }
    }

    /**
     * Gets a pointer to the system's unique data
     *
     * Parameters
     * - system_index The system to get the data of
     * Returns
     * - The system's data
     */
    void *get_system_untyped(u32 system);

    /**
     * Gets a typed pointer to the system's unique data
     *
     * Parameters
     * - system_id The system to get the data of
     * Returns
     * - The system's data
     */
    template<typename SystemType, typename ComponentType>
    SystemType& get_system(HgSystemID<SystemType, ComponentType> system) {
        return *(SystemType *)get_system_untyped(system);
    }

    /**
     * Flushes component removals in an ecs system
     *
     * When a component is removed, it is only flagged dead, so removals must be
     * flushed to keep components contiguous
     *
     * Note, this function moves components, so should not be used while iterating
     * or in a multithreaded context, while simple removal can be
     *
     * Parameters
     * - system_index The system to flush
     */
    void flush_system(u32 system);

    /**
     * Gets the next iterator, the next entity in a system from an ecs
     *
     * Parameters
     * - system_index The index of the system to traverse
     * - iterator The previous entity, point to nullptr to begin, iter must not be nullptr
     * Returns
     * - true if another component is available
     * - false if iteration is complete
     */
    bool iterate_system(u32 system_index, HgEntityID **iterator);

    /**
     * Creates an entity in an ECS, and returns its id
     *
     * Returns
     * - The id of the created entity, will never be 0
     */
    HgEntityID create_entity();

    /**
     * Destroys an entity in an ECS
     *
     * Parameters
     * - entity The id of the entity to destroy
     */
    void destroy_entity(HgEntityID entity);

    /**
     * Checks whether an entity id is alive and can be used
     *
     * Parameters
     * - entity The id of the entity to check
     * Returns
     * - Whether the entity is alive and can be used
     */
    bool is_entity_alive(HgEntityID entity);

    /**
     * Adds a component to an entity
     *
     * Note, the component must not already exist
     *
     * Parameters
     * - entity The id of the entity, must not be 0
     * - system_index The system to add the component in
     * Returns
     * - A pointer to the created component
     */
    void *add_component_untyped(HgEntityID entity, u32 system);

    /**
     * Casts the component to the given type
     *
     * Note, the component must not already exist
     *
     * Parameters
     * - entity The id of the entity, must not be 0
     * - system_index The system to add the component in
     * Returns
     * - A pointer to the created component
     */
    template<typename SystemType, typename ComponentType>
    ComponentType& add_component(HgEntityID entity, HgSystemID<SystemType, ComponentType> system) {
        return *(ComponentType *)add_component_untyped(entity, system);
    }

    /**
     * Removes a component from an entity
     *
     * Note, the component must already exist
     *
     * Parameters
     * - entity The id of the entity, must not be 0
     * - system_index The system to remove the component from
     */
    void remove_component(HgEntityID entity, u32 system);

    /**
     * Checks whether an entity has a component or not
     *
     * Parameters
     * - entity The id of the entity, must not be 0
     * - system_index The system to check
     * Returns
     * - Whether the entity has a component in the system
     */
    bool has_component(HgEntityID entity, u32 system);

    /**
     * Gets a pointer to the entity's component
     *
     * Note, the entity must have a component in the system
     *
     * Parameters
     * - entity The id of the entity, must not be 0
     * - system_index The system to get the component from
     * Returns
     * - The entity's component, will never be 0
     */
    void *get_component_untyped(HgEntityID entity, u32 system);

    /**
     * Casts the pointer to the given type
     *
     * Note, the entity must have a component in the system
     *
     * Parameters
     * - entity The id of the entity, must not be 0
     * - system_index The system to get the component from
     * Returns
     * - The entity's component, will never be 0
     */
    template<typename SystemType, typename ComponentType>
    ComponentType& get_component(HgEntityID entity, HgSystemID<SystemType, ComponentType> system) {
        return *(ComponentType *)get_component_untyped(entity, system);
    }

    /**
     * Gets the entity id from it's component
     *
     * Parameters
     * - component The component to lookup, must be a valid component
     * - system_index The system to get the component from
     * Returns
     * - The components's entity, will never be 0
     */
    HgEntityID get_entity(void *component, u32 system_index);
};

/**
 * A pipeline to render 2D sprites
 */
struct HgPipelineSprite {
    VkDevice device;
    VmaAllocator allocator;
    VkDescriptorSetLayout vp_layout;
    VkDescriptorSetLayout image_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet vp_set;
    VkBuffer vp_buffer;
    VmaAllocation vp_buffer_allocation;
};

/**
 * Creates a pipeline abstraction to render 2D sprites
 * 
 * Parameters
 * - device The Vulkan device, must not be nullptr
 * - allocator The VMA allocator, must not be nullptr,
 * - color_format The format of the color attachment which will be rendered to,
 *   must not be VK_FORMAT_UNDEFINED
 * - depth_format The format of the depth attachment, may be VK_FORMAT_UNDEFINED
 * Returns
 * - The created pipeline
 */
HgPipelineSprite hg_pipeline_sprite_create(
    VkDevice device,
    VmaAllocator allocator,
    VkFormat color_format,
    VkFormat depth_format);

/**
 * Destroys the sprite pipeline
 *
 * Parameters
 * - pipeline The pipeline to destroy
 */
void hg_pipeline_sprite_destroy(HgPipelineSprite *pipeline);

/**
 * Updates the sprite pipeline's projection matrix
 *
 * Parameters
 * - pipeline The pipeline to update, must not be nullptr
 * - projection The value to update to, must not be nullptr
 */
void hg_pipeline_sprite_update_projection(HgPipelineSprite *pipeline, HgMat4 *projection);

/**
 * Updates the sprite pipeline's view matrix
 *
 * Parameters
 * - pipeline The pipeline to update, must not be nullptr
 * - view The value to update to, must not be nullptr
 */
void hg_pipeline_sprite_update_view(HgPipelineSprite *pipeline, HgMat4 *view);

/**
 * The texture resources used by HgPipelineSprite
 */
struct HgPipelineSpriteTexture {
    VmaAllocation allocation;
    VkImage image;
    VkImageView view;
    VkSampler sampler;
    VkDescriptorSet set;
};

/**
 * Configuration for a hgSpritePipelineTexture
 */
struct HgPipelineSpriteTextureConfig {
    /**
     * The pixel data to use, must not be nullptr
     */
    void *tex_data;
    /**
     * The width of the texture in pixels, must be greater than 0
     */
    u32 width;
    /**
     * The height of the texture in pixels, must be greater than 0
     */
    u32 height;
    /**
     * The Vulkan format of each pixel, must not be UNDEFINED
     */
    VkFormat format;
    /**
     * The filter to use when sampling the texture
     */
    VkFilter filter;
    /**
     * How to sample beyond the edge of the texture
     */
    VkSamplerAddressMode edge_mode;
};

/**
 * Creates a texture for HgPipelineSprite
 *
 * Note, if for some reason there are multiple HgPipelineSprite objects,
 * textures are compatible between them, so need not be duplicated
 *
 * Parameters
 * - pipeline The pipeline to create for, must not be nullptr
 * - cmd_pool The command pool to get a command buffer from, must not be nullptr
 * - transfer_queue The queue to transfer the data on, must not be nullptr
 * - config The configuration for the texture, must not be nullptr
 * Returns
 * - The created texture
 */
HgPipelineSpriteTexture hg_pipeline_sprite_create_texture(
    HgPipelineSprite *pipeline,
    VkCommandPool cmd_pool,
    VkQueue transfer_queue,
    HgPipelineSpriteTextureConfig *config);

/**
 * Destroys a texture for HgPipelineSprite
 *
 * Parameters
 * - pipeline The pipeline, must not be nullptr
 * - texture The texture to destroy, must not be nullptr
 */
void hg_pipeline_sprite_destroy_texture(HgPipelineSprite *pipeline, HgPipelineSpriteTexture *texture);

/**
 * Binds the pipeline in a command buffer
 *
 * Note, the command buffer should be in a render pass, and the dynamic viewport
 * and scissor must be set
 *
 * Parameters
 * - pipeline The pipeline to bind, must not be nullptr
 * - cmd The command buffer, must not be nullptr
 */
void hg_pipeline_sprite_bind(HgPipelineSprite *pipeline, VkCommandBuffer cmd);

/**
 * The data pushed to each sprite draw call
 */
struct HgPipelineSpritePush {
    /**
     * The sprite's model matrix (position, scale, rotation, etc.)
     */
    HgMat4 model;
    /**
     * The beginning coordinates of the texture to read from (0.0f to 1.0f)
     */
    HgVec2 uv_pos;
    /**
     * The size within the texture to read (0.0f to 1.0f)
     */
    HgVec2 uv_size;
};

/**
 * Draws a sprite using the sprite pipeline
 *
 * The HpPipelineSprite must already be bound
 *
 * Parameters
 * - pipeline The pipeline, must not be nullptr
 * - cmd The command buffer, must not be nullptr
 * - texture The texture to read from, must not be nullptr
 * - push_data The data to push to the draw call, must not be nullptr
 */
void hg_pipeline_sprite_draw(
    HgPipelineSprite *pipeline,
    VkCommandBuffer cmd,
    HgPipelineSpriteTexture *texture,
    HgPipelineSpritePush *push_data);

#endif // HURDYGURDY_H

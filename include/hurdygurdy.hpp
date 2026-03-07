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
#include <cstddef>
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

#define hg_macro_concat_internal(x, y) x##y

/**
 * Concatenate two macros
 */
#define hg_macro_concat(x, y) hg_macro_concat_internal(x, y)

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
    HgDefer(F fn_val) : fn(fn_val) {}

    /**
     * Execute the function
     */
    ~HgDefer() {
        fn();
    }
};

/**
 * Defers a piece of code until the end of the scope
 *
 * Parameters
 * - code The code to run, may be placed inside braces or not
 */
#define hg_defer(code) [[maybe_unused]] HgDefer hg_macro_concat(hg_defer_, __COUNTER__){[&]{code;}};

#ifdef HG_LOGGING

/**
 * Formats and logs a debug message to stderr for debugging purposes
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_debug(...) do { { (void)fprintf(stderr, "HurdyGurdy Debug: " __VA_ARGS__); } } while(0)

/**
 * Formats and logs a warning message to stderr
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_warn(...) do { (void)fprintf(stderr, "HurdyGurdy Warning: " __VA_ARGS__); } while(0)

/**
 * Formats and logs an error message to stderr and aborts the program
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_error(...) do { (void)fprintf(stderr, "HurdyGurdy Error: " __VA_ARGS__); abort(); } while(0)

#else

/**
 * Formats and logs a debug message to stderr for debugging puposes
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_debug(...) do {} while(0)

/**
 * Formats and logs a warning message to stderr
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_warn(...) do {} while(0)

/**
 * Formats and logs an error message to stderr and aborts the program
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_error(...) do { abort(); } while(0)

#endif

#ifdef HG_ASSERTIONS

/**
 * Asserts condition, or aborts the program
 *
 * Parameters
 * - cond The condition to assert
 */
#define hg_assert(cond) do { \
    if (!(cond)) { \
        hg_error("Assertion failed in " __FILE__ ":%d %s() " #cond "\n", __LINE__, __func__); \
    } \
} while(0)

#else

/**
 * Asserts condition, or aborts the program
 *
 * Parameters
 * - cond The condition to assert
 */
#define hg_assert(cond) do {} while(0)

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
 * An unsigned integer used for indexing
 */
typedef size_t usize;
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
 * - Entity component system
 * - Hardware graphics
 * - OS windowing
 */
void hg_init();

/**
 * Shuts down the HurdyGurdy library
 */
void hg_exit();

/**
 * Create and register a test globally
 *
 * Parameters
 * - test_name The name of the test
 * - test_function A pointer to the test function
 */
void hg_tests_register(const char* name, bool (*function)());

/**
 * Runs all tests registered globally
 *
 * This should be called after initialization, some tests require it
 *
 * Returns:
 * - Whether all tests passed
 */
bool hg_tests_run();

/**
 * Automatically declares and registers a test function
 *
 * Example:
 * hg_test(example_test) {
 *     bool success = true;
 *     hg_test_assert(success != false);
 *     return success;
 * }
 *
 * Parameters
 * - name The name of the test, should not be a string
 */
#define hg_test(name) \
    static bool hg_test_function_##name(); \
    namespace { \
        struct HgTestDummy_##name { \
            HgTestDummy_##name() { \
                hg_tests_register(#name, hg_test_function_##name); \
            } \
        } hg_test_dummy_##name; \
    } \
    static bool hg_test_function_##name() 

/**
 * Asserts a condition in a test
 *
 * Returns false to fail the test
 *
 * Parameters
 * - cond The condition to check
 */
#define hg_test_assert(cond) do { \
    if (!(cond)) { \
        hg_warn(__FILE__ ":%d Test assertion failed: " #cond "\n", __LINE__); \
        return false; \
    } \
} while(0)

/**
 * The value of Pi
 */
static constexpr f64 hg_pi = 3.1415926535897932;
/**
 * The value of Tau (2pi)
 */
static constexpr f64 hg_tau = 6.2831853071795864;
/**
 * The value of Euler's number
 */
static constexpr f64 hg_euler = 2.7182818284590452;
/**
 * The value of the square root of 2
 */
static constexpr f64 hg_root2 = 1.4142135623730951;
/**
 * The value of the square root of 3
 */
static constexpr f64 hg_root3 = 1.7320508075688772;

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
    constexpr HgVec2(f32 scalar) : x{scalar}, y{scalar} {}
    /**
     * Construct from a list of values
     */
    constexpr HgVec2(f32 x_val, f32 y_val) : x{x_val}, y{y_val} {}

    /**
     * Access with index
     */
    constexpr f32& operator[](usize index) {
        hg_assert(index < 2);
        return *(&x + index);
    }

    /**
     * Access with index
     */
    constexpr const f32& operator[](usize index) const {
        hg_assert(index < 2);
        return *(&x + index);
    }

    /**
     * Add another vector in place
     */
    const HgVec2& operator+=(const HgVec2& other);
    /**
     * Subtract another vector in place
     */
    const HgVec2& operator-=(const HgVec2& other);
    /**
     * Multiply another vector in place
     */
    const HgVec2& operator*=(const HgVec2& other);
    /**
     * Divide another vector in place
     */
    const HgVec2& operator/=(const HgVec2& other);
};

/**
 * Compare vectors
 */
constexpr bool operator==(const HgVec2& lhs, const HgVec2& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

/**
 * Compare vectors
 */
constexpr bool operator!=(const HgVec2& lhs, const HgVec2& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

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
    constexpr HgVec3(f32 scalar) : x{scalar}, y{scalar}, z{scalar} {}
    /**
     * Construct from a list of values
     */
    constexpr HgVec3(f32 x_val, f32 y_val) : x{x_val}, y{y_val}, z{0} {}
    /**
     * Construct from a list of values
     */
    constexpr HgVec3(f32 x_val, f32 y_val, f32 z_val) : x{x_val}, y{y_val}, z{z_val} {}
    /**
     * Construct from a Vec2
     */
    constexpr HgVec3(const HgVec2& other) : x{other.x}, y{other.y}, z{0} {}

    /**
     * Access with index
     */
    constexpr f32& operator[](usize index) {
        hg_assert(index < 3);
        return *(&x + index);
    }

    /**
     * Access with index
     */
    constexpr const f32& operator[](usize index) const {
        hg_assert(index < 3);
        return *(&x + index);
    }

    /**
     * Add another vector in place
     */
    const HgVec3& operator+=(const HgVec3& other);
    /**
     * Subtract another vector in place
     */
    const HgVec3& operator-=(const HgVec3& other);
    /**
     * Multiply another vector in place
     */
    const HgVec3& operator*=(const HgVec3& other);
    /**
     * Divide another vector in place
     */
    const HgVec3& operator/=(const HgVec3& other);
};

/**
 * Compare vectors
 */
constexpr bool operator==(const HgVec3& lhs, const HgVec3& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

/**
 * Compare vectors
 */
constexpr bool operator!=(const HgVec3& lhs, const HgVec3& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}

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
    constexpr HgVec4(f32 scalar) : x{scalar}, y{scalar}, z{scalar}, w{scalar} {}
    /**
     * Construct from a list of values
     */
    constexpr HgVec4(f32 x_val, f32 y_val) : x{x_val}, y{y_val}, z{0}, w{0} {}
    /**
     * Construct from a list of values
     */
    constexpr HgVec4(f32 x_val, f32 y_val, f32 z_val) : x{x_val}, y{y_val}, z{z_val}, w{0} {}
    /**
     * Construct from a list of values
     */
    constexpr HgVec4(f32 x_val, f32 y_val, f32 z_val, f32 w_val) : x{x_val}, y{y_val}, z{z_val}, w{w_val} {}
    /**
     * Construct from a Vec2
     */
    constexpr HgVec4(const HgVec2& other) : x{other.x}, y{other.y}, z{0}, w{0} {}
    /**
     * Construct from a Vec3
     */
    constexpr HgVec4(const HgVec3& other) : x{other.x}, y{other.y}, z{other.z}, w{0} {}

    /**
     * Access with index
     */
    constexpr f32& operator[](usize index) {
        hg_assert(index < 4);
        return *(&x + index);
    }

    /**
     * Access with index
     */
    constexpr const f32& operator[](usize index) const {
        hg_assert(index < 4);
        return *(&x + index);
    }

    /**
     * Add another vector in place
     */
    const HgVec4& operator+=(const HgVec4& other);
    /**
     * Subtract another vector in place
     */
    const HgVec4& operator-=(const HgVec4& other);
    /**
     * Multiply another vector in place
     */
    const HgVec4& operator*=(const HgVec4& other);
    /**
     * Divide another vector in place
     */
    const HgVec4& operator/=(const HgVec4& other);
};

/**
 * Compare vectors
 */
constexpr bool operator==(const HgVec4& lhs, const HgVec4& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

/**
 * Compare vectors
 */
constexpr bool operator!=(const HgVec4& lhs, const HgVec4& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
}

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
    constexpr HgMat2(f32 scalar) : x{scalar, 0}, y{0, scalar} {}
    /**
     * Construct from a list of values
     */
    constexpr HgMat2(f32 xx, f32 xy, f32 yx, f32 yy) : x{xx, xy}, y{yx, yy} {}
    /**
     * Construct from a list of vectors
     */
    constexpr HgMat2(const HgVec2& x_val, const HgVec2& y_val) : x{x_val}, y{y_val} {}

    /**
     * Access with index
     */
    constexpr HgVec2& operator[](usize index) {
        hg_assert(index < 2);
        return *(&x + index);
    }

    /**
     * Access with index
     */
    constexpr const HgVec2& operator[](usize index) const {
        hg_assert(index < 2);
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
 * Compare two matrices
 */
constexpr bool operator==(const HgMat2& lhs, const HgMat2& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

/**
 * Compare two matrices
 */
constexpr bool operator!=(const HgMat2& lhs, const HgMat2& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

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
    constexpr HgMat3(f32 scalar)
        : x{scalar, 0, 0}, y{0, scalar, 0}, z{0, 0, scalar} {}
    /**
     * Construct from a list of vectors
     */
    constexpr HgMat3(const HgVec2& x_val, const HgVec2& y_val)
        : x{x_val}, y{y_val}, z{0, 0, 1} {}
    /**
     * Construct from a list of vectors
     */
    constexpr HgMat3(const HgVec3& x_val, const HgVec3& y_val, const HgVec3& z_val)
        : x{x_val}, y{y_val}, z{z_val} {}
    /**
     * Construct from a Mat2
     */
    constexpr HgMat3(const HgMat2& other)
        : x{other.x}, y{other.y}, z{0, 0, 1} {}

    /**
     * Access with index
     */
    constexpr HgVec3& operator[](usize index) {
        hg_assert(index < 3);
        return *(&x + index);
    }

    /**
     * Access with index
     */
    constexpr const HgVec3& operator[](usize index) const {
        hg_assert(index < 3);
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
 * Compare two matrices
 */
constexpr bool operator==(const HgMat3& lhs, const HgMat3& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

/**
 * Compare two matrices
 */
constexpr bool operator!=(const HgMat3& lhs, const HgMat3& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}

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
    constexpr HgMat4(f32 scalar)
        : x{scalar, 0, 0, 0}, y{0, scalar, 0, 0}, z{0, 0, scalar, 0}, w{0, 0, 0, scalar} {}
    /**
     * Construct from a list of vectors
     */
    constexpr HgMat4(const HgVec2& x_val, const HgVec2& y_val)
        : x{x_val}, y{y_val}, z{0, 0, 1, 0}, w{0, 0, 0, 1} {}
    /**
     * Construct from a list of vectors
     */
    constexpr HgMat4(const HgVec3& x_val, const HgVec3& y_val, const HgVec3& z_val)
        : x{x_val}, y{y_val}, z{z_val}, w{0, 0, 0, 1} {}
    /**
     * Construct from a list of vectors
     */
    constexpr HgMat4(const HgVec4& x_val, const HgVec4& y_val, const HgVec4& z_val, const HgVec4& w_val)
        : x{x_val}, y{y_val}, z{z_val}, w{w_val} {}
    /**
     * Construct from a Mat2
     */
    constexpr HgMat4(const HgMat2& other)
        : x{other.x}, y{other.y}, z{0, 0, 1, 0}, w{0, 0, 0, 1} {}
    /**
     * Construct from a Mat3
     */
    constexpr HgMat4(const HgMat3& other)
        : x{other.x}, y{other.y}, z{other.z}, w{0, 0, 0, 1} {}

    /**
     * Access with index
     */
    constexpr HgVec4& operator[](usize index) {
        hg_assert(index < 4);
        return *(&x + index);
    }

    /**
     * Access with index
     */
    constexpr const HgVec4& operator[](usize index) const {
        hg_assert(index < 4);
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
 * Compare two matrices
 */
constexpr bool operator==(const HgMat4& lhs, const HgMat4& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

/**
 * Compare two matrices
 */
constexpr bool operator!=(const HgMat4& lhs, const HgMat4& rhs) {
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
}

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
    constexpr HgComplex(f32 r_val) : r{r_val}, i{0} {}
    /**
     * Construct from a list of values
     */
    constexpr HgComplex(f32 r_val, f32 i_val) : r{r_val}, i{i_val} {}

    /**
     * Access with index
     */
    constexpr f32& operator[](usize index) {
        hg_assert(index < 2);
        return *(&r + index);
    }

    /**
     * Access with index
     */
    constexpr const f32& operator[](usize index) const {
        hg_assert(index < 2);
        return *(&r + index);
    }

    /**
     * Add another complex number in place
     */
    const HgComplex& operator+=(const HgComplex& other);
    /**
     * Subtract another complex number in place
     */
    const HgComplex& operator-=(const HgComplex& other);
};

/**
 * Compare two complex numbers
 */
constexpr bool operator==(const HgComplex& lhs, const HgComplex& rhs) {
    return lhs.r == rhs.r && lhs.i == rhs.i;
}

/**
 * Compare two complex numbers
 */
constexpr bool operator!=(const HgComplex& lhs, const HgComplex& rhs) {
    return lhs.r != rhs.r || lhs.i != rhs.i;
}

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
    constexpr HgQuat(f32 r_val) : r{r_val}, i{0}, j{0}, k{0} {}
    /**
     * Construct from a list of values
     */
    constexpr HgQuat(f32 r_val, f32 i_val, f32 j_val, f32 k_val) : r{r_val}, i{i_val}, j{j_val}, k{k_val} {}

    /**
     * Access with index
     */
    constexpr f32& operator[](usize index) {
        hg_assert(index < 4);
        return *(&r + index);
    }

    /**
     * Access with index
     */
    constexpr const f32& operator[](usize index) const {
        hg_assert(index < 4);
        return *(&r + index);
    }

    /**
     * Add another quaternion in place
     */
    const HgQuat& operator+=(const HgQuat& other);
    /**
     * Subtract another quaternion in place
     */
    const HgQuat& operator-=(const HgQuat& other);
};

/**
 * Compare two quaternions
 */
constexpr bool operator==(const HgQuat& lhs, const HgQuat& rhs) {
    return lhs.r == rhs.r && lhs.i == rhs.i && lhs.j == rhs.j && lhs.k == rhs.k;
}

/**
 * Compare two quaternions
 */
constexpr bool operator!=(const HgQuat& lhs, const HgQuat& rhs) {
    return lhs.r != rhs.r || lhs.i != rhs.i || lhs.j != rhs.j || lhs.k != rhs.k;
}

/**
 * Add two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hg_vec_add(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Add two 2D vectors
 */
constexpr HgVec2 operator+(const HgVec2& lhs, const HgVec2& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

/**
 * Add two 3D vectors
 */
constexpr HgVec3 operator+(const HgVec3& lhs, const HgVec3& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

/**
 * Add two 4D vectors
 */
constexpr HgVec4 operator+(const HgVec4& lhs, const HgVec4& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

/**
 * Subtract two arbitrary size vectors
 *
 * parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hg_vec_sub(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Subtract two 2D vectors
 */
constexpr HgVec2 operator-(const HgVec2& lhs, const HgVec2& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

/**
 * Subtract two 3D vectors
 */
constexpr HgVec3 operator-(const HgVec3& lhs, const HgVec3& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

/**
 * Subtract two 4D vectors
 */
constexpr HgVec4 operator-(const HgVec4& lhs, const HgVec4& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

/**
 * Multiply pairwise two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hg_vec_mul_pairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Multiply pairwise two 2D vectors
 */
constexpr HgVec2 operator*(const HgVec2& lhs, const HgVec2& rhs) {
    return {lhs.x * rhs.x, lhs.y * rhs.y};
}

/**
 * Multiply pairwise two 3D vectors
 */
constexpr HgVec3 operator*(const HgVec3& lhs, const HgVec3& rhs) {
    return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

/**
 * Multiply pairwise two 4D vectors
 */
constexpr HgVec4 operator*(const HgVec4& lhs, const HgVec4& rhs) {
    return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w};
}

/**
 * Multiply a scalar and a vector
 */
void hg_vec_scalar_mul(u32 size, f32* dst, f32 scalar, const f32* vec);

/**
 * Multiply a scalar and a 2D vector
 */
constexpr HgVec2 operator*(f32 scalar, const HgVec2& vec) {
    return {scalar * vec.x, scalar * vec.y};
}

/**
 * Multiply a scalar and a 2D vector
 */
constexpr HgVec2 operator*(const HgVec2& vec, f32 scalar) {
    return {scalar * vec.x, scalar * vec.y};
}

/**
 * Multiply a scalar and a 3D vector
 */
constexpr HgVec3 operator*(f32 scalar, const HgVec3& vec) {
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z};
}

/**
 * Multiply a scalar and a 3D vector
 */
constexpr HgVec3 operator*(const HgVec3& vec, f32 scalar) {
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z};
}

/**
 * Multiply a scalar and a 4D vector
 */
constexpr HgVec4 operator*(f32 scalar, const HgVec4& vec) {
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
}

/**
 * Multiply a scalar and a 4D vector
 */
constexpr HgVec4 operator*(const HgVec4& vec, f32 scalar) {
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
}

/**
 * Divide pairwise two arbitrary size vectors
 *
 * Note, cannot divide by 0
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hg_vec_div(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Divide pairwise two 2D vectors
 *
 * Note, cannot divide by 0
 */
constexpr HgVec2 operator/(const HgVec2& lhs, const HgVec2& rhs) {
    hg_assert(rhs.x != 0 && rhs.y != 0);
    return {lhs.x / rhs.x, lhs.y / rhs.y};
}

/**
 * Divide pairwise two 3D vectors
 *
 * Note, cannot divide by 0
 */
constexpr HgVec3 operator/(const HgVec3& lhs, const HgVec3& rhs) {
    hg_assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
    return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
}

/**
 * Divide pairwise two 4D vectors
 *
 * Note, cannot divide by 0
 */
constexpr HgVec4 operator/(const HgVec4& lhs, const HgVec4& rhs) {
    hg_assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0 && rhs.w != 0);
    return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w};
}

/**
 * Divide a vector by a scalar
 *
 * Note, cannot divide by 0
 */
void hg_vec_scalar_div(u32 size, f32* dst, const f32* vec, f32 scalar);

/**
 * Divide a 2D vector by a scalar
 *
 * Note, cannot divide by 0
 */
constexpr HgVec2 operator/(const HgVec2& vec, f32 scalar) {
    hg_assert(scalar != 0);
    return {vec.x / scalar, vec.y / scalar};
}

/**
 * Divide a 3D vector by a scalar
 *
 * Note, cannot divide by 0
 */
constexpr HgVec3 operator/(const HgVec3& vec, f32 scalar) {
    hg_assert(scalar != 0);
    return {vec.x / scalar, vec.y / scalar, vec.z / scalar};
}

/**
 * Divide a 4D vector by a scalar
 *
 * Note, cannot divide by 0
 */
constexpr HgVec4 operator/(const HgVec4& vec, f32 scalar) {
    hg_assert(scalar != 0);
    return {vec.x / scalar, vec.y / scalar, vec.z / scalar, vec.w / scalar};
}

/**
 * Compute the dot product of two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hg_dot(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Compute the dot product of two 2D vectors
 */
constexpr f32 hg_dot(const HgVec2& lhs, const HgVec2& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

/**
 * Compute the dot product of two 3D vectors
 */
constexpr f32 hg_dot(const HgVec3& lhs, const HgVec3& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

/**
 * Compute the dot product of two 4D vectors
 */
constexpr f32 hg_dot(const HgVec4& lhs, const HgVec4& rhs) {
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
void hg_len(u32 size, f32* dst, const f32* vec);

/**
 * Compute the length of a 2D vector
 */
f32 hg_len(const HgVec2& vec);

/**
 * Compute the length of a 3D vector
 */
f32 hg_len(const HgVec3& vec);

/**
 * Compute the length of a 4D vector
 */
f32 hg_len(const HgVec4& vec);

/**
 * Normalize a vector
 *
 * Note, cannot normalize 0
 */
void hg_norm(u32 size, f32* dst, const f32* vec);

/**
 * Normalize a 2D vector
 *
 * Note, cannot normalize 0
 */
HgVec2 hg_norm(const HgVec2& vec);

/**
 * Normalize a 3D vector
 *
 * Note, cannot normalize 0
 */
HgVec3 hg_norm(const HgVec3& vec);

/**
 * Normalize a 4D vector
 *
 * Note, cannot normalize 0
 */
HgVec4 hg_norm(const HgVec4& vec);

/**
 * Compute the cross product of two 3D vectors
 *
 * Parameters
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hg_cross(f32* dst, const f32* lhs, const f32* rhs);

/**
 * Compute the cross product of two 3D vectors
 */
HgVec3 hg_cross(const HgVec3& lhs, const HgVec3& rhs);

/**
 * Add two arbitrary size matrices
 *
 * Parameters
 * - width The width of the matrices
 * - height The height of the matrices
 * - dst The destination matrix, must not be nullptr
 * - lhs The left-hand side matrix, must not be nullptr
 * - rhs The right-hand side matrix, must not be nullptr
 */
void hg_mat_add(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Add two 2x2 matrices
 */
HgMat2 operator+(const HgMat2& lhs, const HgMat2& rhs);

/**
 * Add two 3x3 matrices
 */
HgMat3 operator+(const HgMat3& lhs, const HgMat3& rhs);

/**
 * Add two 4x4 matrices
 */
HgMat4 operator+(const HgMat4& lhs, const HgMat4& rhs);

/**
 * Subtract two arbitrary size matrices
 *
 * Parameters
 * - width The width of the matrices
 * - height The height of the matrices
 * - dst The destination matrix, must not be nullptr
 * - lhs The left-hand side matrix, must not be nullptr
 * - rhs The right-hand side matrix, must not be nullptr
 */
void hg_mat_sub(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Subtract two 2x2 matrices
 */
HgMat2 operator-(const HgMat2& lhs, const HgMat2& rhs);

/**
 * Subtract two 3x3 matrices
 */
HgMat3 operator-(const HgMat3& lhs, const HgMat3& rhs);

/**
 * Subtract two 4x4 matrices
 */
HgMat4 operator-(const HgMat4& lhs, const HgMat4& rhs);

/**
 * Multiply two arbitrary size matrices
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
void hg_mat_mul(f32* dst, u32 wl, u32 hl, const f32* lhs, u32 wr, u32 hr, const f32* rhs);

/**
 * Multiply two 2x2 matrices
 */
HgMat2 operator*(const HgMat2& lhs, const HgMat2& rhs);

/**
 * Multiply two 3x3 matrices
 */
HgMat3 operator*(const HgMat3& lhs, const HgMat3& rhs);

/**
 * Multiply two 4x4 matrices
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
void hg_mat_vec_mul(u32 width, u32 height, f32* dst, const f32* mat, const f32* vec);

/**
 * Multiply a 2x2 matrix and a 2D vector
 */
HgVec2 operator*(const HgMat2& lhs, const HgVec2& rhs);

/**
 * Multiply a 3x3 matrix and a 3D vector
 */
HgVec3 operator*(const HgMat3& lhs, const HgVec3& rhs);

/**
 * Multiply a 4x4 matrix and a 4D vector
 */
HgVec4 operator*(const HgMat4& lhs, const HgVec4& rhs);

/**
 * Add two complex numbers
 */
constexpr HgComplex operator+(const HgComplex& lhs, const HgComplex& rhs) {
    return {lhs.r + rhs.r, lhs.i + rhs.i};
}

/**
 * Subtract two complex numbers
 */
constexpr HgComplex operator-(const HgComplex& lhs, const HgComplex& rhs) {
    return {lhs.r - rhs.r, lhs.i - rhs.i};
}

/**
 * Multiply two complex numbers
 */
constexpr HgComplex operator*(const HgComplex& lhs, const HgComplex& rhs) {
    return {lhs.r * rhs.r - lhs.i * rhs.i, lhs.r * rhs.i + lhs.i * rhs.r};
}

/**
 * Add two quaternions
 */
constexpr HgQuat operator+(const HgQuat& lhs, const HgQuat& rhs) {
    return {lhs.r + rhs.r, lhs.i + rhs.i, lhs.j + rhs.j, lhs.k + rhs.k};
}

/**
 * Subtract two quaternions
 */
constexpr HgQuat operator-(const HgQuat& lhs, const HgQuat& rhs) {
    return {lhs.r - rhs.r, lhs.i - rhs.i, lhs.j - rhs.j, lhs.k - rhs.k};
}

/**
 * Multiply two quaternions
 */
HgQuat operator*(const HgQuat& lhs, const HgQuat& rhs);

/**
 * Compute the conjugate of a quaternion
 */
constexpr HgQuat hg_conj(const HgQuat& quat) {
    return {quat.r, -quat.i, -quat.j, -quat.k};
}

/**
 * Create a rotation quaternion from an axis and angle
 */
HgQuat hg_axis_angle(const HgVec3& axis, f32 angle);

/**
 * Rotate a 3D vector using a quaternion
 */
HgVec3 hg_rotate(const HgQuat& lhs, const HgVec3& rhs);

/**
 * Rotate a 3x3 matrix using a quaternion
 */
HgMat3 hg_rotate(const HgQuat& lhs, const HgMat3& rhs);

/**
 * Creates a model matrix for 2D graphics
 *
 * Parameters
 * - position The position of the model
 * - scale The scale of the model
 * - rotation The rotation of the model
 *
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
 *
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
 *
 * Returns
 * - The created matrix
 */
HgMat4 hg_view_matrix(const HgVec3& position, const HgVec3& zoom, const HgQuat& rotation);

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
 *
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
 *
 * Returns
 * - The created matrix
 */
HgMat4 hg_projection_perspective(f32 fov, f32 aspect, f32 near, f32 far);

// random number generators : TODO

// sort and search algorithms : TODO

/**
 * Calculates the maximum number of mipmap levels that an image can have
 *
 * At least one dimension must be greater than 0
 *
 * Parameters
 * - width The width of the image
 * - height The height of the image
 * - depth The depth of the image
 *
 * Returns
 * - The maximum number of mipmap levels the image can have
 */
u32 hg_max_mipmaps(u32 width, u32 height, u32 depth);

/**
 * Aligns a pointer to an alignment
 *
 * Parameters
 * - value The value to align
 * - alignment The alignment, must be a power of two
 *
 * Returns
 * - The aligned size
 */
constexpr usize hg_align(uptr value, usize alignment) {
    hg_assert(alignment > 0 && (alignment & (alignment - 1)) == 0);
    return (value + alignment - 1) & ~(alignment - 1);
}

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
    usize capacity;
    /**
     * The next allocation to be given out
     */
    usize head;

    /**
     * Construct uninitialized
     */
    HgArena() = default;

    /**
     * Create an arena from a block of memory
     */
    HgArena(void* memory_val, usize capacity_val) : memory{memory_val}, capacity{capacity_val}, head{0} {}

    /**
     * Allocates memory from an arena
     *
     * Parameters
     * - size The size in bytes of the allocation
     * - alignment The required alignment of the allocation in bytes
     *
     * Returns
     * - The allocation, never nullptr
     */
    void* alloc(usize size, usize alignment);

    /**
     * A convenience to allocate an array of a type
     *
     * Note, objects are not initialized
     *
     * Parameters
     * - count The number of T to allocate
     *
     * Returns
     * - The allocated array, never nullptr
     */
    template<typename T>
    T* alloc(usize count) {
        return (T*)alloc(count * sizeof(T), alignof(T));
    }

    /**
     * Reallocates memory from a arena
     *
     * Simply increases the size if allocation is the most recent allocation
     *
     * Parameters
     * - allocation The allocation to grow
     * - new_size The new size in bytes of the allocation
     * - alignment The required alignment of the allocation in bytes
     *
     * Returns
     * - The allocation, never nullptr
     */
    void* realloc(void* allocation, usize old_size, usize new_size, usize alignment);

    /**
     * A convenience to reallocate an array of a type
     *
     * Note, objects are default constructed if possible, otherwise they are
     * left uninitialized
     *
     * Parameters
     * - allocation The allocation to reallocate
     * - count The new number of T to allocate
     *
     * Returns
     * - The reallocated array, never nullptr
     */
    template<typename T>
    T* realloc(T* allocation, usize old_count, usize new_count) {
        static_assert(std::is_trivially_copyable_v<T>);
        return (T*)realloc(allocation, old_count * sizeof(T), new_count * sizeof(T), alignof(T));
    }
};

/**
 * Create a guard which restores an arena's head at the end of the scope
 */
struct HgArenaScope {
    /**
     * The arena to restore
     */
    HgArena& arena;
    /**
     * The arena's original head
     */
    usize head;

    /**
     * Constructs the scope guard
     */
    HgArenaScope(HgArena& arena_val) : arena{arena_val}, head{arena_val.head} {}

    /**
     * Restores the arena's head
     */
    ~HgArenaScope() {
        arena.head = head;
    }
};

/**
 * Initializes scratch arenas
 */
void hg_scratch_memory_init();

/**
 * Deinitializes scratch arenas
 */
void hg_scratch_memory_deinit();

/**
 * Get a scratch arena for temporary allocations, assuming no conflicts
 *
 * Returns
 * - A scratch arena
 */
HgArena& hg_get_scratch();

/**
 * Get a scratch arena for temporary allocations, accounting for a conflict
 *
 * Parameters
 * - conflict If an arena is being used, the returned arena will be different
 *
 * Returns
 * - A scratch arena
 */
HgArena& hg_get_scratch(const HgArena& conflict);

/**
 * Get a scratch arena for temporary allocations, accounting for conflicts
 *
 * Parameters
 * - conflict If arenas are being used, the returned arena will be different
 * - count The number of conflicts
 *
 * Returns
 * - A scratch arena
 */
HgArena& hg_get_scratch(const HgArena** conflicts, usize count);

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
    usize length;

    /**
     * The size of the string in bytes
     *
     * Returns
     * - The size of the string in bytes
     */
    constexpr usize size() const {
        return length;
    }

    /**
     * Construct uninitialized
     */
    HgStringView() = default;

    /**
     * Create a string view from a pointer and length
     */
    constexpr HgStringView(const char* chars_val, usize length_val) : chars{chars_val}, length{length_val} {}

    /**
     * Create a string view from begin and end pointers
     */
    constexpr HgStringView(const char* chars_begin, const char* chars_end)
        : chars{chars_begin}
        , length{(uptr)(chars_end - chars_begin)}
    {
        hg_assert(chars_begin <= chars_end);
    }

    /**
     * Implicit constexpr conversion from c string
     *
     * Potentially dangerous, c string should be at most 4096 chars
     */
    constexpr HgStringView(const char* c_str) : chars{c_str}, length{0} {
        while (c_str[length] != '\0') {
            ++length;
            hg_assert(length <= 4096);
        }
    }

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr const char& operator[](usize index) const {
        hg_assert(chars != nullptr);
        hg_assert(index < length);
        return chars[index];
    }

    /**
     * For c++ ranged based for
     */
    constexpr const char* begin() const {
        return chars;
    }

    /**
     * For c++ ranged based for
     */
    constexpr const char* end() const {
        return chars + length;
    }
};

/**
 * Create a null terminated string for C interop
 *
 * Parameters
 * - arena The arena to allocate from
 * - str The string to create from
 *
 * Returns
 * - a null terminated string
 */
char* hg_c_string(HgArena& arena, HgStringView str);

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
    usize capacity;
    /**
     * The number of characters currently in the string
     */
    usize length;

    /**
     * Returns the size of the in bytes
     */
    constexpr usize size() const {
        return length;
    }

    /**
     * Creates a new string with empty capacity
     *
     * Parameters
     * - arena The arena to allocate from
     * - capacity The capacity to begin with
     *
     * Returns
     * - The created empty string
     */
    static HgString create(HgArena& arena, usize capacity);

    /**
     * Creates a new string copied from an existing string
     *
     * Parameters
     * - arena The arena to allocate from
     * - init The initial string to copy from
     *
     * Returns
     * - The created copied string
     */
    static HgString create(HgArena& arena, HgStringView init);

    /**
     * Removes all characters
     */
    constexpr void reset() {
        length = 0;
    }

    /**
     * Changes the capacity
     *
     * Parameters
     * - arena The arena to allocate from
     * - new_capacity The new minimum capacity
     */
    void reserve(HgArena& arena, usize new_capacity);

    /**
     * Increases the capacity of the string, or inits to 1
     *
     * Parameters
     * - arena The arena to allocate from
     * - factor The growth factor to increase by
     */
    void grow(HgArena& arena, f32 factor = 2.0f);

    /**
     * Access using the index operator
     */
    constexpr char& operator[](usize index) {
        hg_assert(index < length);
        return chars[index];
    }

    /**
     * Access using the index operator in a const context
     */
    constexpr const char& operator[](usize index) const {
        hg_assert(index < length);
        return chars[index];
    }

    /**
     * For c++ ranged based for loop
     */
    constexpr char* begin() const {
        return chars;
    }

    /**
     * For c++ ranged based for loop
     */
    constexpr char* end() const {
        return chars + length;
    }

    /**
     * Inserts a char into this string at index
     *
     * Parameters
     * - arena The arena to allocate from
     * - c The char to insert
     */
    HgString& insert(HgArena& arena, usize index, char c);

    /**
     * Appends a char to the end of this string
     *
     * Parameters
     * - arena The arena to allocate from
     * - c The char to append
     */
    HgString& append(HgArena& arena, char c) {
        return insert(arena, length, c);
    }

    /**
     * Prepends a char to the beginning of this string
     *
     * Parameters
     * - arena The arena to allocate from
     * - c The char to prepend
     */
    HgString& prepend(HgArena& arena, char c) {
        return insert(arena, 0, c);
    }

    /**
     * Copies another string into this string at index
     *
     * Parameters
     * - arena The arena to allocate from
     * - str The string to copy from
     */
    HgString& insert(HgArena& arena, usize index, HgStringView str);

    /**
     * Copies another string to the end of this string
     *
     * Parameters
     * - arena The arena to allocate from
     * - str The string to copy from
     */
    HgString& append(HgArena& arena, HgStringView str) {
        return insert(arena, length, str);
    }

    /**
     * Copies another string to the beginning of this string
     *
     * Parameters
     * - arena The arena to allocate from
     * - str The string to copy from
     */
    HgString& prepend(HgArena& arena, HgStringView str) {
        return insert(arena, 0, str);
    }

    /**
     * Implicit converts to string_view
     */
    constexpr operator HgStringView() const {
        return {chars, length};
    }
};

/**
 * Compare string views
 */
constexpr bool operator==(HgStringView lhs, HgStringView rhs) {
    return lhs.length == rhs.length && memcmp(lhs.chars, rhs.chars, lhs.length) == 0;
}

/**
 * Compare string views
 */
constexpr bool operator!=(HgStringView lhs, HgStringView rhs) {
    return !(lhs == rhs);
}

/**
 * Compare strings
 */
inline bool operator==(const HgString& lhs, const HgString& rhs) {
    return lhs.length == rhs.length && memcmp(lhs.chars, rhs.chars, lhs.length) == 0;
}

/**
 * Compare strings
 */
inline bool operator!=(const HgString& lhs, const HgString& rhs) {
    return !(lhs == rhs);
}

/**
 * Check whether a character is whitespace
 *
 * Parameters
 * - c The character to check
 *
 * Returns
 * - Whether it is a space, tab, or newline, or not
 */
bool hg_is_whitespace(char c);

/**
 * Check whether a character is a base 10 numeral
 *
 * Parameters
 * - c The character to check
 *
 * Returns
 * - Whether it is 0-9 or not
 */
bool hg_is_numeral_base10(char c);

/**
 * Check whether a string is a base 10 integer
 *
 * Parameters
 * - str The string to check
 *
 * Returns
 * - Whether it is valid integer, positive or negative
 */
bool hg_is_integer_base10(HgStringView str);

/**
 * Check whether a string is a base 10 floating point number
 *
 * Parameters
 * - str The string to check
 *
 * Returns
 * - Whether it is valid floating point number
 */
bool hg_is_float_base10(HgStringView str);

/**
 * Create an i64 from a base 10 string
 *
 * Parameters
 * - str The string to create from
 *
 * Returns
 * - The created integer
 */
i64 hg_str_to_int_base10(HgStringView str);

/**
 * Create a integer from a base 10 string
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 *
 * Returns
 * - The created string
 */
f64 hg_str_to_float_base10(HgStringView str);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 *
 * Returns
 * - The created string
 */
HgString hg_int_to_str_base10(HgArena& arena, i64 num);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 *
 * Returns
 * - The created string
 */
HgString hg_float_to_str_base10(HgArena& arena, f64 num, u64 decimal_count);

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
 *
 * Returns
 * - The created string
 */
HgString hg_format_string(HgArena& arena, HgStringView fmt, ...);

/**
 * A parsed Json file
 */
struct HgJson {
    /**
     * An error contained in the json
     */
    struct Error {
        /**
         * The next error
         */
        Error* next;
        /**
         * The error message
         */
        HgString msg;
    };

    /**
     * A node in the json file
     */
    struct Node;

    /**
     * The types contained in nodes
     */
    enum Type : u32 {
        none = 0,
        jstruct,
        field,
        array,
        string,
        floating,
        integer,
        boolean,
    };

    /**
     * A field in a struct
     */
    struct Field {
        /**
         * The next field
         */
        Field* next;
        /**
         * The name of the field
         */
        HgString name;
        /**
         * The value stored in the field
         */
        Node* value;
    };

    /**
     * A struct contained in the json
     */
    struct Struct {
        /**
         * The first field
         */
        Field* fields;
    };

    /**
     * An element in an array
     */
    struct Elem {
        /**
         * The next element
         */
        Elem* next;
        /**
         * The value stored in the element
         */
        Node* value;
    };

    /**
     * An array contained in the json
     */
    struct Array {
        /**
         * The first element
         */
        Elem* elems;
    };

    /**
     * A node in the json file
     */
    struct Node {
        /**
         * The node's type
         */
        Type type;
        /**
         * The value in the node
         */
        union {
            Struct jstruct;
            Field field;
            Array array;
            HgString string;
            f64 floating;
            i64 integer;
            bool boolean;
        };
    };

    /**
     * The successfully parsed nodes
     */
    Node* file;
    /**
     * The errors found
     */
    Error* errors;

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
    static HgJson parse(HgArena& arena, HgStringView text);
};

/**
 * Hash map hashing for u8
 */
constexpr usize hg_hash(u8 val) {
    return (usize)val;
}

/**
 * Hash map hashing for u16
 */
constexpr usize hg_hash(u16 val) {
    return (usize)val;
}

/**
 * Hash map hashing for u32
 */
constexpr usize hg_hash(u32 val) {
    return (usize)val;
}

/**
 * Hash map hashing for u64
 */
constexpr usize hg_hash(u64 val) {
    return (usize)val;
}

/**
 * Hash map hashing for i8
 */
constexpr usize hg_hash(i8 val) {
    return (usize)val;
}

/**
 * Hash map hashing for i16
 */
constexpr usize hg_hash(i16 val) {
    return (usize)val;
}

/**
 * Hash map hashing for i32
 */
constexpr usize hg_hash(i32 val) {
    return (usize)val;
}

/**
 * Hash map hashing for i64
 */
constexpr usize hg_hash(i64 val) {
    return (usize)val;
}

/**
 * Hash map hashing for f32
 */
constexpr usize hg_hash(f32 val) {
    union {
        f32 as_float;
        usize as_usize;
    } u{};
    u.as_float = val;
    return u.as_usize;
}

/**
 * Hash map hashing for f64
 */
constexpr usize hg_hash(f64 val) {
    union {
        f64 as_float;
        usize as_usize;
    } u{};
    u.as_float = val;
    return u.as_usize;
}

/**
 * Hash map hashing for void*
 */
constexpr usize hg_hash(void* val) {
    union {
        void* as_ptr;
        uptr as_uptr;
    } u{};
    u.as_ptr = val;
    return (usize)u.as_uptr;
}

/**
 * Hash map hashing for strings
 */
constexpr usize hg_hash(HgStringView str) {
    u64 ret = 0;
    u64 mult = 1;
    for (char c : str) {
        ret += (u64)c * mult;
        mult *= 257;
    }
    return (usize)ret;
}

/**
 * Hash map hashing for HgString
 */
constexpr usize hg_hash(HgString str) {
    return hg_hash(HgStringView{str});
}

/**
 * Hash map hashing for C string
 */
constexpr usize hg_hash(const char* str) {
    return hg_hash(HgStringView{str});
}

/**
 * A key-value hash map
 */
template<typename Key, typename Value, usize (*hash_fn)(Key) = hg_hash>
struct HgHashMap {
    static_assert(std::is_trivially_copyable_v<Key>
               && std::is_trivially_copyable_v<Value>
               && std::is_trivially_destructible_v<Key>
               && std::is_trivially_destructible_v<Value>);

    /**
     * Whether each index has a value
     */
    bool* has_val;
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
    usize capacity;
    /**
     * The current number of values that are stored
     */
    usize load;

    /**
     * Creates a new hash map
     *
     * Parameters
     * - arena The arena to allocate from
     * - slot_count The max number of slots to store values in
     *
     * Returns
     * - The created empty hash map
     */
    static HgHashMap create(HgArena& arena, usize slot_count) {
        hg_assert(slot_count > 0);

        HgHashMap map;
        map.has_val = arena.alloc<bool>(slot_count);
        map.keys = arena.alloc<Key>(slot_count);
        map.vals = arena.alloc<Value>(slot_count);
        map.capacity = slot_count;
        map.reset();
        return map;
    }

    /**
     * Empties all slots
     */
    void reset() {
        for (usize i = 0; i < capacity; ++i) {
            has_val[i] = false;
        }
        load = 0;
    }

    /**
     * Returns whether the hash map is full
     */
    bool is_full() {
        return load == capacity;
    }

    /**
     * Returns whether the hash map is full
     */
    bool is_near_full() {
        return (f32)load >= (f32)capacity * 0.8f;
    }

    /**
     * Returns whether the hash map is full
     */
    bool is_half_full() {
        return (f32)load >= (f32)capacity * 0.5f;
    }

    /**
     * Resizes the slots and rehashes all entries
     *
     * Parameters
     * - arena The arena to allocate from
     * - new_size The new number of slots
     */
    void resize(HgArena& arena, usize new_size) {
        HgHashMap old = *this;
        *this = create(arena, new_size);

        for (usize i = 0; i < old.capacity; ++i) {
            if (old.has_val[i])
                insert(old.keys[i], old.vals[i]);
        }
    }

    /**
     * Grows the number of slots by a factor
     *
     * Parameters
     * - arena The arena to allocate from
     * - factor The factor to increase by
     */
    void grow(HgArena& arena, f32 factor = 2) {
        resize(arena, (usize)((f32)capacity * factor));
    }

    /**
     * Inserts a value into the hash map
     *
     * Parameters
     * - key The key to store at
     * - value The value to store
     *
     * Returns
     * - A reference to the constructed object
     */
    Value& insert(Key key, Value value) {
        hg_assert(load < capacity - 1);

        usize idx = hash_fn(key) % capacity;
        usize dist = 0;
        while (has_val[idx] && keys[idx] != key) {
            usize other_dist = hash_fn(keys[idx]) % capacity;
            if (other_dist < idx)
                other_dist += capacity;
            other_dist -= idx;

            if (other_dist < dist) {
                Key key_tmp = keys[idx];
                Value value_tmp = vals[idx];
                keys[idx] = key;
                vals[idx] = value;
                key = key_tmp;
                value = value_tmp;

                dist = other_dist;
            }

            idx = (idx + 1) % capacity;
            ++dist;
        }

        ++load;
        has_val[idx] = true;
        keys[idx] = key;
        vals[idx] = value;
        return vals[idx];
    }

    /**
     * Removes a value from the hash map, and stores it
     *
     * Parameters
     * - key The key to remove from
     * - value A pointer to store the value, if found
     *
     * Returns
     * - Whether a value was found and stored in value
     */
    bool remove(const Key& key, Value* value) {
        usize idx = get_idx(key);
        if (idx == (usize)-1)
            return false;

        if (value != nullptr)
            *value = vals[idx];

        usize next = (idx + 1) % capacity;
        while (has_val[next]) {
            if (hash_fn(keys[next]) != idx) {
                keys[idx] = keys[next];
                vals[idx] = vals[next];
                idx = next;
            }
            next = (next + 1)  % capacity;
        }
        has_val[idx] = false;
        --load;

        return true;
    }

    /**
     * Removes a value from the hash map
     *
     * Parameters
     * - key The key to remove from
     */
    void remove(const Key& key) {
        remove(key, nullptr);
    }

    /**
     * Checks whether a value exists
     *
     * Parameters
     * - key The key to check at
     *
     * Returns
     * - Whether a value exists at the key
     */
    bool has(const Key& key) {
        return get_idx(key) != (usize)-1;
    }

    /**
     * Gets the index of a key
     *
     * Parameters
     * - key The key to lookup
     *
     * Returns
     * - The index the key is stored at
     * - -1 if the key is not in the hash map
     */
    usize get_idx(const Key& key) {
        usize idx = hash_fn(key) % capacity;
        while (has_val[idx] && keys[idx] != key) {
            idx = (idx + 1) % capacity;
        }
        return has_val[idx] ? idx : (usize)-1;
    }

    /**
     * Gets a pointer to the value at key, or nullptr if it does not exist
     *
     * Parameters
     * - key The key to get from
     *
     * Returns
     * - A pointer to the value
     * - nullptr if it does not exist
     */
    Value* get(const Key& key) {
        usize idx = get_idx(key);
        return idx == (usize)-1 ? nullptr : vals + idx;
    }

    /**
     * Execute a function for every key-value pair
     */
    template<typename Fn>
    void for_each(Fn fn) {
        static_assert(std::is_invocable_r_v<void, Fn, Key&, Value&>);
        for (usize i = 0; i < capacity; ++i) {
            if (has_val[i])
                fn(keys[i], vals[i]);
        }
    }
};

/**
 * A hash set
 */
template<typename Value, usize (*hash_fn)(Value) = hg_hash>
struct HgHashSet {
    static_assert(std::is_trivially_copyable_v<Value> && std::is_trivially_destructible_v<Value>);

    /**
     * Whether each index has a value
     */
    bool* has_val;
    /**
     * Where the values are stored;
     */
    Value* vals;
    /**
     * The max number of vals
     */
    usize capacity;
    /**
     * The current number of values that are stored
     */
    usize load;

    /**
     * Creates a new hash set
     *
     * Parameters
     * - arena The arena to allocate from
     * - slot_count The max number of slots to store values in
     *
     * Returns
     * - The created empty hash set
     */
    static HgHashSet create(HgArena& arena, usize slot_count) {
        hg_assert(slot_count > 0);

        HgHashSet set;
        set.has_val = arena.alloc<bool>(slot_count);
        set.vals = arena.alloc<Value>(slot_count);
        set.capacity = slot_count;
        set.reset();
        return set;
    }

    /**
     * Empties all slots
     */
    void reset() {
        for (usize i = 0; i < capacity; ++i) {
            has_val[i] = false;
        }
        load = 0;
    }

    /**
     * Returns whether the hash set is full
     */
    bool is_full() {
        return load == capacity;
    }

    /**
     * Returns whether the hash set is full
     */
    bool is_near_full() {
        return (f32)load >= (f32)capacity * 0.8f;
    }

    /**
     * Resizes the slots and rehashes all entries
     *
     * Parameters
     * - arena The arena to allocate from
     * - new_size The new number of slots
     */
    void resize(HgArena& arena, usize new_size) {
        HgHashSet old = *this;
        *this = create(arena, new_size);

        for (usize i = 0; i < old.capacity; ++i) {
            if (old.has_val[i])
                insert(old.vals[i]);
        }
    }

    /**
     * Grows the number of slots by a factor
     *
     * Parameters
     * - arena The arena to allocate from
     * - factor The factor to increase by
     */
    void grow(HgArena& arena, f32 factor = 2) {
        resize(arena, (usize)((f32)capacity * factor));
    }

    /**
     * Inserts a value into the hash set
     *
     * Parameters
     * - val The value to store
     */
    void insert(Value val) {
        hg_assert(load < capacity - 1);

        usize idx = hash_fn(val) % capacity;
        usize dist = 0;
        while (has_val[idx] && vals[idx] != val) {
            usize other_dist = hash_fn(vals[idx]) % capacity;
            if (other_dist < idx)
                other_dist += capacity;
            other_dist -= idx;

            if (other_dist < dist) {
                Value val_tmp = vals[idx];
                vals[idx] = val;
                val = val_tmp;

                dist = other_dist;
            }

            idx = (idx + 1) % capacity;
            ++dist;
        }

        ++load;
        has_val[idx] = true;
        vals[idx] = val;
    }

    /**
     * Removes a value from the hash set, and stores it
     *
     * Parameters
     * - val The value to remove from
     */
    void remove(const Value& val) {
        usize idx = get_idx(val);
        if (idx == (usize)-1)
            return;

        usize next = (idx + 1) % capacity;
        while (has_val[next]) {
            if (hash_fn(vals[next]) != idx) {
                vals[idx] = vals[next];
                idx = next;
            }
            next = (next + 1)  % capacity;
        }
        has_val[idx] = false;
        --load;
    }

    /**
     * Checks whether a value exists
     *
     * Parameters
     * - val The value to check at
     *
     * Returns
     * - Whether a value exists at the val
     */
    bool has(const Value& val) {
        return get_idx(val) != (usize)-1;
    }

    /**
     * Gets the index of a val
     *
     * Parameters
     * - val The value to lookup
     *
     * Returns
     * - The index the val is stored at
     * - -1 if the val is not in the hash set
     */
    usize get_idx(const Value& val) {
        usize idx = hash_fn(val) % capacity;
        while (has_val[idx] && vals[idx] != val) {
            idx = (idx + 1) % capacity;
        }
        return has_val[idx] ? idx : (usize)-1;
    }

    /**
     * Execute a function for every value
     */
    template<typename Fn>
    void for_each(Fn fn) {
        static_assert(std::is_invocable_r_v<void, Fn, Value&>);
        for (usize i = 0; i < capacity; ++i) {
            if (has_val[i])
                fn(vals[i]);
        }
    }
};

/**
 * A high precision clock for timers and game deltas
 */
struct HgClock {
    std::chrono::time_point<std::chrono::high_resolution_clock> time = std::chrono::high_resolution_clock::now();

    /**
     * Resets the clock and returns the delta since the last tick in seconds
     *
     * Returns
     * - Seconds since last tick
     */
    f64 tick();
};

/**
 * Returns the number of concurrent threads available in hardware
 */
usize hg_hardware_concurrency();

/**
 * A spinlock fence for basic thread synchronization
 */
struct HgFence {
    /**
     * The number of events the fence is waiting on
     */
    std::atomic<u64> counter{0};

    /**
     * Add more events for the fence to wait on
     *
     * Parameters
     * - count The number of added events
     */
    void add(usize count);

    /**
     * Signal that events have completed
     *
     * Parameters
     * - count The number of signaled events
     */
    void signal(usize count);

    /**
     * Returns whether all work has been completed
     */
    bool is_complete();

    /**
     * Spin waits for all work submissions to be completed
     *
     * Parameters
     * - timeout_seconds The time in seconds to wait before timing out
     */
    bool wait(f64 seconds);
};

/**
 * Initialize the thread pool
 *
 * Note, the recommended thread is the hardware thread count minus dedicated
 * threads such as main thread, IO thread, etc.
 *
 * Parameters
 * - arena The arena to allocate from
 * - queue_size The max capacity of the thread work queue
 * - thread_count The number of threads to spawn in the pool
 */
void hg_thread_pool_init(HgArena& arena, usize queue_size, usize thread_count);

/**
 * Deinitialize the thread pool
 */
void hg_thread_pool_deinit();

/**
 * Wait on a fence, and help complete work in the meantime
 *
 * Parameters
 * - fence The fence to wait on
 * - timeout_seconds The max time to spend working
 *
 * Returns
 * - true if the fence was completed
 * - false if the timeout was reached
 */
bool hg_thread_pool_help(HgFence& fence, f64 timeout_seconds);

/**
 * Pushes work to the thread pool queue to be executed
 *
 * Parameters
 * - fences The fences to signal upon completion
 * - fence_count The number of fences
 * - data The data passed to the function
 * - work The function to be executed
 */
void hg_call_par(HgFence* fences, usize fence_count, void* data, void (*fn)(void*));

/**
 * Iterates in parallel over a function n times using the thread pool
 *
 * Note, uses a fence internally to wait for all work to complete
 *
 * Parameters
 * - n The number of elements to iterate [0, count)
 * - chunk_size The number of elements to iterate per chunk
 * - fn The function to use to iterate, takes begin and end indicces
 */
template<typename Fn>
void hg_for_par(usize n, usize chunk_size, Fn fn) {
    static_assert(std::is_invocable_r_v<void, Fn, usize, usize>);

    HgArena& scratch = hg_get_scratch();
    HgArenaScope scratch_scope{scratch};

    HgFence fence;
    for (usize i = 0; i < n; i += chunk_size) {
        auto iter = [begin = i, end = std::min(i + chunk_size, n), &fn] {
            fn(begin, end);
        };
        void* data = scratch.alloc<decltype(iter)>(1);
        memcpy(data, &iter, sizeof(iter));

        hg_call_par(&fence, 1, data, [](void* piter) {
            (*(decltype(iter)*)piter)();
        });
    }
    hg_thread_pool_help(fence, INFINITY);
}

/**
 * Initializes the IO thread
 *
 * Parameters
 * - arena The arena to allocate from
 * - queue_size The max concurrent request capacity
 */
void hg_io_thread_init(HgArena& arena, usize queue_size);

/**
 * Deinitializes the IO thread
 */
void hg_io_thread_deinit();

/**
 * Make an asynchronous IO request on the IO thread
 */
void hg_io_request(
    HgFence* fences,
    usize fence_count,
    void* resource,
    HgStringView path,
    void (*fn)(void* resource, HgStringView path));

/**
 * The uuid derived from the resource's name/path
 */
using HgResource = usize;

/**
 * Get the resource uuid from the name/path
 */
constexpr HgResource hg_resource_id(HgStringView name) {
    return hg_hash(name);
}

/**
 * A loaded binary file
 */
struct HgBinary {
    /**
     * The data in the file
     */
    void* data;
    /**
     * The size of the file in bytes
     */
    usize size;

    /**
     * Construct uninitialized
     */
    HgBinary() = default;

    /**
     * Construct from a data pointer
     */
    constexpr HgBinary(void* file_val, usize size_val) : data{file_val}, size{size_val} {}

    /**
     * Load a binary file from disc
     *
     * Note, this is blocking
     *
     * Parameters
     * - arena The arena to allocate from
     * - path The file path to the image
     */
    static HgBinary load(HgArena& arena, HgStringView path);

    /**
     * Store a binary file to disc
     *
     * Note, this is blocking
     *
     * Parameters
     * - path The file path to store at
     */
    void store(HgStringView path);

    /**
     * Resize the file
     *
     * Parameters
     * - arena The arena to allocate from
     * - new_size The new size of the file in bytes
     */
    void resize(HgArena& arena, usize new_size) {
        data = arena.realloc(data, size, new_size, 1);
        size = new_size;
    }

    /**
     * Increase the size of the file by count
     *
     * Parameters
     * - arena The arena to allocate from
     * - count The size in bytes to increase by
     */
    void grow(HgArena& arena, usize count) {
        resize(arena, size + count);
    }

    /**
     * Read data at index into a buffer
     *
     * Parameters
     * - index The index into the file in bytes to read from
     * - dst A pointer to store the read data
     * - size The size in bytes to read
     */
    void read(usize idx, void* dst, usize len) const {
        hg_assert(idx + len <= size);
        memcpy(dst, (u8*)data + idx, len);
    }

    /**
     * Read data of arbitrary type from the file
     *
     * Parameters
     * - index The index into the file in bytes to read from
     *
     * Returns
     * - The data read
     */
    template<typename T>
    T read(usize index) const {
        T ret;
        read(index, &ret, sizeof(T));
        return ret;
    }

    /**
     * Overwrite data at the index
     *
     * Parameters
     * - src The data to write
     * - size The size of the data in bytes
     */
    void overwrite(usize idx, const void* src, usize len) {
        hg_assert(idx + len <= size);
        memcpy((u8*)data + idx, src, len);
    }

    /**
     * Overwrite data of arbitrary type at the index
     *
     * Parameters
     * - src The data to write
     */
    template<typename T>
    void overwrite(usize idx, const T& src) {
        overwrite(idx, &src, sizeof(T));
    }
};

/**
 * A texture resource
 */
struct HgTexture {
    static constexpr char texture_identifier[] = "HGT"; // hurdy gurdy texture

    /**
     * The info prepended to a texture resource
     */
    struct Info {
        /**
         * The identifier to ensure the file is a Hurdy Gurdy texture
         */
        char identifier[sizeof(texture_identifier)];
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
     * The texture data
     */
    HgBinary file;

    /**
     * Construct uninitialized
     */
    HgTexture() = default;

    /**
     * Implicit conversion from binary file
     */
    HgTexture(HgBinary file_val) : file(file_val) {}

    /**
     * Get the texture info from the file
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
    bool get_info(VkFormat& format, u32& width, u32& height, u32& depth);

    /**
     * Returns a pointer to the pixels
     */
    void* get_pixels();
};

/**
 * Initialize the resource manager (or reinitialize to reset)
 *
 * Parameters
 * - arena The arena to allocate from
 * - max_resources The max number of resources which can be allocated
 */
void hg_resources_init(HgArena& arena, usize max_resources);

/**
 * Unload and dealloc all resources
 */
void hg_resources_reset();

/**
 * Allocate a new empty resource
 *
 * Parameters
 * - id The id of the resource
 */
void hg_alloc_resource(HgResource id);

/**
 * Deallocates a resource (unloading if needed)
 */
void hg_dealloc_resource(HgResource id);

/**
 * Loads a resource (or just increments the reference count)
 *
 * Note, automatically allocates a new resource if it does not exist
 *
 * Parameters
 * - fence The fences to signal on completion
 * - fence_count The number of fences
 * - id The resource to load into
 * - path The filepath to load from
 */
void hg_load_resource(HgFence* fences, usize fence_count, HgResource id, HgStringView path);

/**
 * Unloads a resource (or just decrements the reference count)
 *
 * Note, does not automatically deallocate the resource after unloading
 *
 * Parameters
 * - fence The fences to signal on completion
 * - fence_count The number of fences
 * - id The resource to load into
 */
void hg_unload_resource(HgFence* fences, usize fence_count, HgResource id);

/**
 * Stores a resource to disc
 *
 * Parameters
 * - fence The fences to signal on completion
 * - fence_count The number of fences
 * - id The resource to load into
 * - path The filepath to store to
 */
void hg_store_resource(HgFence* fences, usize fence_count, HgResource id, HgStringView path);

/**
 * Get a resource from the global store
 *
 * Parameters
 * - id The resource to get
 *
 * Returns
 * - A pointer to the resource binary, may be empty if not loaded
 * - nullptr, if the resource is not allocated
 */
HgBinary* hg_get_resource(HgResource id);

/**
 * Load an external texture file into a resource in the Hurdy Gurdy format
 *
 * Parameters
 * - fences The fences to wait on
 * - fence_count The number of fences
 * - id The resource to load to
 * - path The path of the file to import
 */
void hg_import_png(HgFence* fences, usize fence_count, HgResource id, HgStringView path);

/**
 * Store a texture resource onto disc in an external file format
 *
 * Parameters
 * - fences The fences to wait on
 * - fence_count The number of fences
 * - id The resource to export
 * - path The path of the file to export to
 */
void hg_export_png(HgFence* fences, usize fence_count, HgResource id, HgStringView path);

/**
 * A buffer stored on the GPU
 */
struct HgGpuBuffer {
    /**
     * The allocation
     */
    VmaAllocation allocation;
    /**
     * The buffer
     */
    VkBuffer buffer;
    /**
     * The size of the buffer in bytes
     */
    VkDeviceSize size;
};

/**
 * A texture stored on the GPU
 */
struct HgGpuTexture {
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
 * Initialize the gpu resource manager (or reinitialize to reset)
 *
 * Parameters
 * - arena The arena to allocate from
 * - max_resources The max number of resources which can be allocated
 */
void hg_gpu_resources_init(HgArena& arena, usize max_resources);

/**
 * Unload and dealloc all gpu resources
 */
void hg_gpu_resources_reset();

/**
 * Allocate a new empty gpu buffer
 *
 * Parameters
 * - id The id of the resource
 */
void hg_alloc_gpu_buffer(HgResource id);

/**
 * Allocate a new empty gpu texture
 *
 * Parameters
 * - id The id of the resource
 */
void hg_alloc_gpu_texture(HgResource id);

/**
 * Deallocates a gpu resource (unloading if needed)
 */
void hg_dealloc_gpu_resource(HgResource id);

/**
 * Loads a gpu buffer (or just increments the reference count)
 *
 * Note, allocates the gpu buffer if needed, but expects the cpu side loaded
 *
 * Parameters
 * - id The resource to load into
 */
void hg_load_gpu_buffer(HgResource id);

/**
 * Loads a gpu texture (or just increments the reference count)
 *
 * Note, allocates the gpu texture if needed, but expects the cpu side loaded
 *
 * Parameters
 * - id The resource to load into
 * - filter The filter the texture should use
 */
void hg_load_gpu_texture(HgResource id, VkFilter filter);

/**
 * Unloads a gpu resource (or just decrement the reference count)
 *
 * Note, does not automatically deallocate the resource after unloading
 *
 * Parameters
 * - id The resource to load into
 */
void hg_unload_gpu_resource(HgResource id);

/**
 * Returns whether a gpu resource is currently loaded
 */
bool hg_is_gpu_resource_loaded(HgResource id);

/**
 * Get a gpu buffer from the global store
 *
 * Parameters
 * - id The resource to get
 *
 * Returns
 * - A pointer to the resource binary, may be empty if not loaded
 * - nullptr, if the resource is not allocated
 */
HgGpuBuffer* hg_get_gpu_buffer(HgResource id);

/**
 * Get a gpu texture from the global store
 *
 * Parameters
 * - id The resource to get
 *
 * Returns
 * - A pointer to the resource binary, may be empty if not loaded
 * - nullptr, if the resource is not allocated
 */
HgGpuTexture* hg_get_gpu_texture(HgResource id);

/**
 * Creates a new id for each component
 *
 * Should only be used by hg_component_id
 *
 * Parameters
 * - width The size in bytes of the component
 */
u32 hg_create_component_id(u32 width);

/**
 * The unique component id for a type, keeping const and non-const the same
 */
template<typename T>
inline u32 hg_component_id = hg_create_component_id(sizeof(T));

/**
 * An entity in the ecs
 */
struct HgEntity {
    /**
     * The entity id, defaults to null
     */
    u32 id = (u32)-1;

    /**
     * Get the index from the id
     */
    u32 idx() {
        return id & 0x00ffffff;
    }

    /**
     * Get the generation from the id
     */
    u32 generation() {
        return (id & 0xff000000) >> 24;
    }

    /**
     * Increment this entity's generation count
     */
    void increment_generation() {
        id += 1 << 24;
    }
};

/**
 * Compare entities
 */
constexpr bool operator==(HgEntity lhs, HgEntity rhs) {
    return lhs.id == rhs.id;
}

/**
 * Compare entities
 */
constexpr bool operator!=(HgEntity lhs, HgEntity rhs) {
    return lhs.id != rhs.id;
}

/**
 * An entity component system
 */
struct HgECS {
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
    u32 pool_size;

    /**
     * The component systems
     */
    System* systems;
    /**
     * The number of component systems
     */
    u32 system_count;

    /**
     * Create a new entity component system
     *
     * Parameters
     * - max_entities The maximum number of entities which can be spawned
     *
     * Returns
     * - The created ECS with no entities or components
     */
    static HgECS create(u32 max_entities);

    /**
     * Destroy the entity component system
     */
    void destroy();

    /**
     * Resets the entity component system, despawning all entities
     */
    void reset();

    /**
     * Returns a newly spawned entity
     */
    HgEntity spawn();

    /**
     * Despawn an entity
     *
     * Note, this function will invalidate iterators
     *
     * Parameters
     * - e The entity to despawn, must be alive
     */
    void despawn(HgEntity e);

    /**
     * Returns whether an entity is alive
     */
    bool alive(HgEntity e);

    /**
     * Adds a component to an entity
     *
     * Note, the entity must not have a component of this type already
     *
     * Parameters
     * - e The entity to add to, must be alive
     * - component_id The id of the component
     *
     * Returns
     * - A pointer to the created component
     */
    void* add(HgEntity e, u32 component_id);

    /**
     * Adds a component to an entity
     *
     * Note, the entity must not have a component of this type already
     *
     * Parameters
     * - e The entity to add to, must be alive
     * - component_id The id of the component
     *
     * Returns
     * - A pointer to the created component
     */
    template<typename T>
    T& add(HgEntity e) {
        return *(T*)add(e, hg_component_id<T>);
    }

    /**
     * Removes a component from an entity
     *
     * Note, this function will invalidate iterators
     *
     * Parameters
     * - e The entity to remove from, must be alive
     * - component_id The id of the component
     */
    void remove(HgEntity e, u32 component_id);

    /**
     * Removes a component from an entity
     *
     * Note, this function will invalidate iterators
     *
     * Parameters
     * - e The entity to remove from, must be alive
     */
    template<typename T>
    void remove(HgEntity e) {
        remove(e, hg_component_id<T>);
    }

    /**
     * Checks whether an entity has a component or not
     *
     * Parameters
     * - e The entity to check, must be alive
     * - component_id The id of the component
     *
     * Returns
     * - Whether the entity has the component
     */
    bool has(HgEntity e, u32 component_id);

    /**
     * Checks whether an entity has a component or not
     *
     * Parameters
     * - e The entity to check, must be alive
     *
     * Returns
     * - Whether the entity has the component
     */
    template<typename T>
    bool has(HgEntity e) {
        return has(e, hg_component_id<T>);
    }

    /**
     * Returns whether the entity has all given components
     *
     * Parameters
     * - e The entity to check, must be alive
     *
     * Returns
     * - Whether the entity has the components
     */
    template<typename... Ts>
    bool has_all(HgEntity e) {
        return (has<Ts>(e) && ...);
    }

    /**
     * Gets a pointer to the entity's component
     *
     * Note, the entity must have the component
     *
     * Parameters
     * - e The id of the entity, must be alive
     * - component_id The id of the component
     *
     * Returns
     * - The entity's component, will never be nullptr
     */
    void* get(HgEntity e, u32 component_id);

    /**
     * Gets a reference to the entity's component
     *
     * Note, the entity must have the component
     *
     * Parameters
     * - e The id of the entity, must be alive
     *
     * Returns
     * - The entity's component
     */
    template<typename T>
    T& get(HgEntity e) {
        return *(T*)get(e, hg_component_id<T>);
    }

    /**
     * Gets the entity from it's component
     *
     * Parameters
     * - c The component to lookup, must be a valid component
     * - component_id The id of the component
     *
     * Returns
     * - The components's entity, will always be alive
     */
    HgEntity get(const void* c, u32 component_id);

    /**
     * Gets the entity from it's component
     *
     * Parameters
     * - c The component to lookup, must be a valid component
     *
     * Returns
     * - The components's entity, will always be alive
     */
    template<typename T>
    HgEntity get(const T* c) {
        return get((void*)c, hg_component_id<T>);
    }

    /**
     * Returns a pointer to all entities of type
     */
    template<typename T>
    HgEntity* entities() {
        return (HgEntity*)systems[hg_component_id<T>].entities;
    }

    /**
     * Returns a pointer to all components of type
     */
    template<typename T>
    T* components() {
        return (T*)systems[hg_component_id<T>].components;
    }

    /**
     * Returns the number of active components of a type
     */
    template<typename T>
    u32 count() {
        return systems[hg_component_id<T>].count;
    }

    /**
     * Finds the id of the system with the fewest elements
     *
     * Parameters
     * - ids The ids to check, must be registered
     *
     * Returns
     * - The id of the smallest in the array
     */
    u32 find_smallest(u32* ids, usize id_count);

    /**
     * Finds the id of the system with the fewest elements
     *
     * Returns
     * - The id of the smallest in the array
     */
    template<typename... Ts>
    u32 find_smallest() {
        u32 index = 0;
        u32 ids[sizeof...(Ts)];
        ((ids[index++] = hg_component_id<Ts>), ...);
        return find_smallest(ids, sizeof...(Ts));
    }

    /**
     * Iterates over all entities with the single given component
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
    void for_each_single(Fn& fn) {
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity, T&>);

        HgEntity* e = entities<T>();
        HgEntity* end = e + count<T>();
        T* c = components<T>();
        for (; e != end; ++e, ++c) {
            fn(*e, *c);
        }
    }

    /**
     * Iterates over all entities with the given components
     *
     * The function receives as parameters:
     * - The entity id
     * - A reference to each component...
     *
     * Parameters
     * - function The function to call
     */
    template<typename... Ts, typename Fn>
    void for_each_multi(Fn& fn) {
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity, Ts&...>);

        u32 id = find_smallest<Ts...>();
        HgEntity* e = systems[id].entities;
        HgEntity* end = e + systems[id].count;
        for (; e != end; ++e) {
            if (has_all<Ts...>(*e))
                fn(*e, get<Ts>(*e)...);
        }
    }

    /**
     * Iterates over all entities with the given components
     *
     * Note, calls the single or multi version from the number of components
     *
     * Parameters
     * - function The function to call
     */
    template<typename... Ts, typename Fn>
    void for_each(Fn fn) {
        static_assert(sizeof...(Ts) != 0);

        if constexpr (sizeof...(Ts) == 1) {
            for_each_single<Ts...>(fn);
        } else {
            for_each_multi<Ts...>(fn);
        }
    }

    /**
     * Iterates over all entities with the single given component
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
    void for_par_single(u32 chunk_size, Fn& fn) {
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity, T&>);

        hg_for_par(count<T>(), chunk_size, [&](usize begin, usize end) {
            HgEntity* e = entities<T>() + begin;
            HgEntity* e_end = entities<T>() + end;
            T* c = components<T>() + begin;
            for (; e != e_end; ++e, ++c) {
                fn(*e, *c);
            }
        });
    }

    /**
     * Iterates over all entities with the given components
     *
     * The function receives as parameters:
     * - The entity id
     * - A reference to each component...
     *
     * Parameters
     * - function The function to call
     */
    template<typename... Ts, typename Fn>
    void for_par_multi(u32 chunk_size, Fn& fn) {
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity, Ts&...>);

        u32 id = find_smallest<Ts...>();
        hg_for_par(systems[id].count, chunk_size, [&](usize begin, usize end) {
            HgEntity* e = systems[id].entities + begin;
            HgEntity* e_end = systems[id].entities + end;
            for (; e != e_end; ++e) {
                if (has_all<Ts...>(*e))
                    fn(*e, get<Ts>(*e)...);
            }
        });
    }

    /**
     * Iterates over all entities with the given components
     *
     * Note, calls the single of multi version from the number of components
     *
     * Parameters
     * - function The function to call
     */
    template<typename... Ts, typename Fn>
    void for_par(u32 chunk_size, Fn fn) {
        static_assert(sizeof...(Ts) != 0);

        if constexpr (sizeof...(Ts) == 1) {
            for_par_single<Ts...>(chunk_size, fn);
        } else {
            for_par_multi<Ts...>(chunk_size, fn);
        }
    }

    /**
     * Sorts components
     *
     * Parameters
     * - begin The index to begin sorting
     * - end The index to end sorting
     * - component_id The component system to sort
     * - compare The comparison function
     */
    void sort_untyped(
        u32 begin,
        u32 end,
        u32 component_id,
        void* data,
        bool (*compare)(void*, HgECS& ecs, HgEntity lhs, HgEntity rhs));

    /**
     * Sorts components
     *
     * Parameters
     * - compare The comparison function
     */
    template<typename T>
    void sort(void* data, bool (*compare)(void*, HgECS& ecs, HgEntity lhs, HgEntity rhs)) {
        sort_untyped(0, count<T>(), hg_component_id<T>, data, compare);
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
    HgEntity next_sibling{};
    /**
     * The previous child of this entity's parent
     */
    HgEntity prev_sibling{};
    /**
     * The first of this entity's children, forming a linked list
     */
    HgEntity first_child{};
};

/**
 * Adds a new child to an entity in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - parent The parent to add to, must be alive
 * - child The child to add, must be alive
 */
void hg_add_child_entity(HgECS& ecs, HgEntity parent, HgEntity new_child);

/**
 * Removes the entity from its hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to detach, must be alive
 */
void hg_detach_entity(HgECS& ecs, HgEntity e);

/**
 * Destroys the entity and all its children in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to destroy to, must be alive
 */
void hg_destroy_entity(HgECS& ecs, HgEntity e);

/**
 * The transform component for entities
 */
struct HgTransform {
    /**
     * The entity's position in the world
     *
     * x: -left, +right
     *
     * y: -up, +down
     *
     * z: -backward, +forward
     */
    HgVec3 position = {0.0f, 0.0f, 0.0f};
    /**
     * The entity's scaling
     *
     * x: horizonatal
     *
     * y: vertical
     *
     * z: depth
     */
    HgVec3 scale = {1.0f, 1.0f, 1.0f};
    /**
     * The entity's rotation in the world
     */
    HgQuat rotation = {1.0f, 0.0f, 0.0f, 0.0f};
};

/**
 * Set this transform and move all children by accordingly : TODO
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to set, must be alive
 * - p The new position
 * - s The new scale
 * - r The new rotation
 */
void hg_set_entity(HgECS& ecs, HgEntity e, const HgVec3& p, const HgVec3& s, const HgQuat& r);

/**
 * Move this transform and all children by a delta
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to move, must be alive
 * - dp The change in position, added to current position
 * - ds The change in scale, multiplied to current scale
 * - dr The change in rotation, applied to current rotation
 */
void hg_move_entity(HgECS& ecs, HgEntity e, const HgVec3& dp, const HgVec3& ds, const HgQuat& dr);

/**
 * A sprite component rendered by the 2d pipeline
 */
struct HgSprite {
    /**
     * The texture to draw from
     */
    HgResource texture;
    /**
     * The beginning coordinate to read from texture, [0.0, 1.0]
     */
    HgVec2 uv_pos;
    /**
     * The size of the region to read from texture, [0.0, 1.0]
     */
    HgVec2 uv_size;
};

/**
 * Initialize the 2D pipeline
 *
 * Parameters
 * - arena The arena to allocate from
 * - max_textures The maximum textures which can be added to the pipelin
 * - color_format The format of the color attachment, must not be UNDEFINED
 * - depth_format The format of the depth attachment, if any
 */
void hg_pipeline_2d_init(
    HgArena& arena,
    usize max_textures,
    VkFormat color_format,
    VkFormat depth_format);

/**
 * Deinitialize the 2D pipeline
 */
void hg_pipeline_2d_deinit();

/**
 * Add a texture to the 2D pipeline
 *
 * Parameters
 * - texture_id The texture to add
 */
void hg_pipeline_2d_add_texture(HgResource texture_id);

/**
 * Remove a texture from the 2D pipeline
 *
 * Parameters
 * - texture_id The texture to remove
 */
void hg_pipeline_2d_remove_texture(HgResource texture_id);

/**
 * Returns whether the texture has been added to the pipeline
 *
 * Parameters
 * - texture_id The texture to check
 *
 * Returns
 * - Whether the texture has been added to the pipeline
 */
bool hg_pipeline_2d_has_texture(HgResource texture_id);

/**
 * Updates the 2D pipeline's projection matrix
 *
 * Parameters
 * - projection The new projection matrix
 */
void hg_pipeline_2d_update_projection(const HgMat4& projection);

/**
 * Updates the 2D pipeline's view matrix
 *
 * Parameters
 * - projection The new view matrix
 */
void hg_pipeline_2d_update_view(const HgMat4& view);

/**
 * Issue draw commands for all HgSprite components in the ecs
 * 
 * Parameters
 * - ecs The ecs to draw
 * - cmd The command buffer to record to, must not be nullptr
 */
void hg_draw_2d(HgECS& ecs, VkCommandBuffer cmd);

/**
 * Initializes the graphics subsystem
 */
void hg_graphics_init();

/**
 * Deinitializes the graphics subsystem
 */
void hg_graphics_deinit();

/**
 * The global Vulkan instance
 */
inline VkInstance hg_vk_instance = nullptr;
/**
 * The global Vulkan physical device
 */
inline VkPhysicalDevice hg_vk_physical_device = nullptr;
/**
 * The global Vulkan logical device
 */
inline VkDevice hg_vk_device = nullptr;
/**
 * The global Vulkan memory allocator
 */
inline VmaAllocator hg_vk_vma = nullptr;

/**
 * The global Vulkan queue
 */
inline VkQueue hg_vk_queue = nullptr;
/**
 * The global Vulkan queue family
 */
inline u32 hg_vk_queue_family = (u32)-1;
/**
 * The global Vulkan command pool
 */
inline VkCommandPool hg_vk_cmd_pool = nullptr;

/**
 * Turns a VkResult into a string
 *
 * Parameters
 * - result The result enum to stringify
 *
 * Returns
 * - The string of the enum value's name
 */
const char* hg_vk_result_string(VkResult result);

/**
 * Turns a VkFormat into the size in bytes
 *
 * Parameters
 * - format The format to get the size of
 *
 * Returns
 * - The size of the format in bytes
 */
u32 hg_vk_format_to_size(VkFormat format);

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
 * Creates a Vulkan instance with sensible defaults
 *
 * In debug mode, enables debug messaging
 *
 * Parameters
 * - extensions The instance extensions to load
 * - extension_count The number of extensions
 *
 * Returns
 * - The created VkInstance, will never be nullptr
 */
VkInstance hg_vk_create_instance(HgStringView* extensions, u32 extension_count);

/**
 * Creates a Vulkan debug messenger
 *
 * Returns
 * - The created debug messenger, will never be nullptr
 */
VkDebugUtilsMessengerEXT hg_vk_create_debug_messenger();

/**
 * Find the first queue family index which supports the the queue flags
 *
 * Parameters
 * - gpu The physical device, must not be nullptr
 * - queue_flags The flags required of the queue family
 * - queue_family Where to store the family, if found
 *
 * Returns
 * - Whether a queue family was found and stored in queue_family
 */
bool hg_vk_find_queue_family(VkPhysicalDevice gpu, u32& queue_family, VkQueueFlags queue_flags);

/**
 * Finds a Vulkan physical device with a general purpose queue family
 *
 * The physical device will have at least one queue family which supports both
 * graphics, transfer, and compute
 *
 * Note, hg_vk_instance must be initialized
 *
 * Returns
 * - The physical device
 * - nullptr if none was found
 */
VkPhysicalDevice hg_vk_find_single_queue_physical_device();

/**
 * Creates a Vulkan logical device with a single general purpose queue
 *
 * The device will have queue 0 in hg_vk_queue_family
 *
 * Note, hg_vk_physical_device must be initialized
 *
 * Returns
 * - The physical device, will never be nullptr
 */
VkDevice hg_vk_create_single_queue_device();

// find gpu with multiple potential queues : TODO
// create device with multiple potential queues : TODO

/**
 * Configuration for Vulkan pipelines
 */
struct HgVkPipelineConfig {
    /**
     * The format of the color attachments, none can be UNDEFINED
     */
    const VkFormat* color_attachment_formats;
    /**
     * The number of color attachment formats
     */
    u32 color_attachment_format_count;
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
    const VkPipelineShaderStageCreateInfo* shader_stages;
    /**
     * The number of shaders
     */
    u32 shader_count;
    /**
     * The pipeline layout
     */
    VkPipelineLayout layout;
    /**
     * Descriptions of the vertex bindings, may be nullptr
     */
    const VkVertexInputBindingDescription* vertex_bindings;
    /**
     * The number of vertex bindings
     */
    u32 vertex_binding_count;
    /**
     * Descriptions of the vertex attributes, may be nullptr
     */
    const VkVertexInputAttributeDescription* vertex_attributes;
    /**
     * The number of vertex attributes
     */
    u32 vertex_attribute_count;
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
 * - config The pipeline configuration
 *
 * Returns
 * - The created graphics pipeline, will never be nullptr
 */
VkPipeline hg_vk_create_graphics_pipeline(const HgVkPipelineConfig& config);

/**
 * Creates a compute pipeline
 *
 * There cannot be any attachments
 * There can only be one shader stage, COMPUTE
 * There cannot be any vertex inputs
 *
 * Parameters
 * - config The pipeline configuration
 *
 * Returns
 * - The created compute pipeline, will never be nullptr
 */
VkPipeline hg_vk_create_compute_pipeline(const HgVkPipelineConfig& config);

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
 * - desired_flags The flags which the type should
 * - undesired_flags The flags which the type should not have, though may have
 *
 * Returns
 * - The found index of the memory type
 */
u32 hg_vk_find_memory_type_index(
    u32 bitmask,
    VkMemoryPropertyFlags desired_flags,
    VkMemoryPropertyFlags undesired_flags);

// Vulkan allocator : TODO?

/**
 * Writes to a Vulkan device local buffer through a staging buffer
 *
 * Parameters
 * - dst The buffer to write to, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - src The data to write, must not be nullptr
 * - size The size in bytes to write
 */
void hg_vk_buffer_staging_write(
    VkBuffer dst,
    usize offset,
    const void* src,
    usize size);

/**
 * Reads from a Vulkan device local buffer through a staging buffer
 *
 * Parameters
 * - dst The location to write to, must not be nullptr
 * - src The buffer to read from, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - size The size in bytes to read
 */
void hg_vk_buffer_staging_read(
    void* dst,
    VkBuffer src,
    usize offset,
    usize size);

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
    void* src_data;
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
 * - config The configuration for the write
 */
void hg_vk_image_staging_write(
    const HgVkImageStagingWriteConfig& config);

/**
 * Writes to a Vulkan device local cubemap image through a staging buffer
 *
 * Note, config.width and config.height are interpreted as the width and height
 * of each face of the cubemap and src_data is assumed to be layed out as:
 *  #
 * ####
 *  #
 *
 * Parameters
 * - config The configuration for the write
 */
void hg_vk_image_staging_write_cubemap(
    const HgVkImageStagingWriteConfig &config);

/**
 * Configuration for a staging image write
 */
struct HgVkImageStagingReadConfig {
    /**
     * The location to write to, must not be nullptr
     */
    void* dst;
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
 * - config The configuration for the read, must not be nullptr
 */
void hg_vk_image_staging_read(
    const HgVkImageStagingReadConfig& config);

/**
 * Generates mipmaps from the base level
 *
 * Parameters
 * - image The image to generate mipmaps in, must not be nullptr
 * - aspect_mask The image aspects to use, must not be 0
 * - old_layout The layout the image was in before, must not be UNDEFINED
 * - new_layout The layout the image will be set to, must not be UNDEFINED
 * - width The width of the base level, must be greater than 0
 * - height The width of the base level, must be greater than 0
 * - depth The width of the base level, must be greater than 0
 * - mip_count The total number of mips in the image, must be greater than 0
 */
void hg_vk_image_generate_mipmaps(
    VkImage image,
    VkImageAspectFlags aspect_mask,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    u32 width,
    u32 height,
    u32 depth,
    u32 mip_count);

/**
 * Initializes global resources for windowing
 */
void hg_platform_init();

/**
 * Deinitializes global resources for windowing
 */
void hg_platform_deinit();

/**
 * Get the platform's required instance extensions for windowing
 *
 * Parameters
 * - arena The arena to allocate from
 * - ext_buffer A pointer to store the extension names
 *
 * Returns
 * - The number of required extensions
 */
u32 hg_vk_get_platform_extensions(HgArena& arena, HgStringView** ext_buffer);

// audio system : TODO

/**
 * A key on the keyboard or button on the mouse
 */
enum class HgKey {
    none = 0,
    k0,
    k1,
    k2,
    k3,
    k4,
    k5,
    k6,
    k7,
    k8,
    k9,
    q,
    w,
    e,
    r,
    t,
    y,
    u,
    i,
    o,
    p,
    a,
    s,
    d,
    f,
    g,
    h,
    j,
    k,
    l,
    z,
    x,
    c,
    v,
    b,
    n,
    m,
    semicolon,
    colon,
    apostrophe,
    quotation,
    comma,
    period,
    question,
    grave,
    tilde,
    exclamation,
    at,
    hash,
    dollar,
    percent,
    carot,
    ampersand,
    asterisk,
    lparen,
    rparen,
    lbracket,
    rbracket,
    lbrace,
    rbrace,
    equal,
    less,
    greater,
    plus,
    minus,
    slash,
    backslash,
    underscore,
    bar,
    up,
    down,
    left,
    right,
    mouse1,
    mouse2,
    mouse3,
    mouse4,
    mouse5,
    lmouse = mouse1,
    rmouse = mouse2,
    mmouse = mouse3,
    escape,
    space,
    enter,
    backspace,
    kdelete,
    insert,
    tab,
    home,
    end,
    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12,
    lshift,
    rshift,
    lctrl,
    rctrl,
    lmeta,
    rmeta,
    lalt,
    ralt,
    lsuper,
    rsuper,
    capslock,
    count,
};

/**
 * Configuration for a window
 */
struct HgWindowConfig {
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
    VkImageUsageFlags image_usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    /**
     * How the swapchain images will be presented
     *
     * Note, will fall back to FIFO if unavailable
     */
    VkPresentModeKHR preferred_present_mode = VK_PRESENT_MODE_FIFO_KHR;
};

/**
 * A window
 */
struct HgWindow {
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
    u32 image_count;
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
    VkImageUsageFlags image_usage;
    /**
     * The swapchain image format
     */
    VkPresentModeKHR present_mode;

    /**
     * The command buffers per image
     */
    VkCommandBuffer* cmds;
    /**
     * The fences per image
     */
    VkFence* frame_finished;
    /**
     * The semaphores per image to signal availability
     */
    VkSemaphore* image_available;
    /**
     * The semaphores per image to signal presentability
     */
    VkSemaphore* ready_to_present;
    /**
     * The current frame
     */
    u32 current_frame;
    /**
     * The current image
     */
    u32 current_image;

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
    f64 mouse_pos_x;
    /**
     * The current y position of the mouse
     */
    f64 mouse_pos_y;
    /**
     * The x distance the mouse has travelled since last frame
     */
    f64 mouse_delta_x;
    /**
     * The y distance the mouse has travelled since last frame
     */
    f64 mouse_delta_y;
    /**
     * Which keys are currently being held down
     */
    bool is_key_down[(u32)HgKey::count];
    /**
     * Which keys were pressed this frame
     */
    bool was_key_pressed[(u32)HgKey::count];
    /**
     * Which keys were released this frame
     */
    bool was_key_released[(u32)HgKey::count];
    /**
     * Whether this window has been closed
     */
    bool was_closed;

    /**
     * Creates a window
     *
     * Parameters
     * - arena The arena to allocate from
     * - config The window configuration
     *
     * Returns
     * - The created window, will never be nullptr
     */
    static HgWindow* create(HgArena& arena, const HgWindowConfig& config);

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
    VkCommandBuffer begin_recording();

    /**
     * Finishes recording the command buffer and presents the swapchain image
     *
     * Parameters
     * - cmd The command buffer given from begin_recording
     */
    void end_and_present(VkCommandBuffer cmd);

    /**
     * Sets the window icon : TODO
     *
     * Parameters
     * - icon_data The pixels of the image to set the icon to, must not be nullptr
     * - width The width in pixels of the icon
     * - height The height in pixels of the icon
     */
    void set_icon(u32* icon_data, u32 width, u32 height);

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
    enum class Cursor {
        none = 0,
        arrow,
        text,
        wait,
        cross,
        hand,
    };

    /**
     * Sets the window's cursor to a platform defined icon : TODO
     */
    void set_cursor(Cursor cursor);

    /**
     * Sets the window's cursor to a custom image : TODO
     */
    void set_cursor_image(u32* data, u32 width, u32 height);
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
 */
void hg_process_window_events(HgWindow** windows, usize window_count);

/**
 * Initialize ImGui platform backend
 *
 * Note, requires GLFW on Linux (for now)
 *
 * Parameters
 * - window The window for ImGui to use
 */
void ImGui_ImplHurdyGurdy_Init(HgWindow* window);

/**
 * Deinitializes ImGui platform backend
 */
void ImGui_ImplHurdyGurdy_Shutdown();

/**
 * Create a new ImGui frame for the platform backend
 */
void ImGui_ImplHurdyGurdy_NewFrame();

#endif // HURDYGURDY_HPP

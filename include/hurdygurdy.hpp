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
#include <cinttypes>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <chrono>
#include <optional>

#include <atomic>
#include <mutex>
#include <thread>

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
#define HG_VK_DEBUG_MESSENGER 1
#endif

#ifdef HG_RELEASE_MODE
#define HG_NO_ASSERTIONS 1
#endif

/**
 * Initializes the HurdyGurdy library
 *
 * Subsystems initialized:
 * - Threading
 * - Resources
 * - Graphics
 * - Windowing
 */
void hg_init();

/**
 * Shuts down the HurdyGurdy library
 */
void hg_exit();

#ifdef HG_DEBUG_MODE

/**
 * Runs code only in debug mode
 */
#define hg_debug_mode(code) code

#else

/**
 * Runs code only in debug mode
 */
#define hg_debug_mode(code)

#endif

#define hg_concat_macros_internal(x, y) x##y
#define hg_concat_macros(x, y) hg_concat_macros_internal(x, y)

template<typename F>
struct HgDeferInternal {
    F fn;
    HgDeferInternal(F defer_function) : fn(defer_function) {}
    ~HgDeferInternal() { fn(); }
};

/**
 * Defers a piece of code until the end of the scope
 *
 * Parameters
 * - code The code to run, may be placed inside braces or not
 */
#define hg_defer(code) [[maybe_unused]] HgDeferInternal hg_concat_macros(hg_defer_, __COUNTER__){[&]{code;}};

/**
 * Gets the number of elements in a stack allocated array
 *
 * Parameters
 * - array The array to get the count of
 *
 * Returns
 * - The number of elements
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

#ifdef HG_NO_ASSERTIONS

#define hg_assert(cond) do {} while(0)

#else

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

#endif

/**
 * The struct used to declare and run tests
 */
struct HgTest {
    /**
     * The name of the test
     */
    const char* name;
    /**
     * A pointer to the test function
     */
    bool (*function)();

    /**
     * Creates and registers the test globally
     *
     * Parameters
     * - test_name The name of the test
     * - test_function A pointer to the test function
     */
    HgTest(const char* test_name, decltype(function) test_function);
};

/**
 * Automatically declares and registers a test function
 *
 * Example:
 * hg_test(hg_example_test) {
 *     bool success = true;
 *     hg_test_assert(success != false);
 *     return success;
 * }
 *
 * Parameters
 * - name The name of the test, should not be a string
 */
#define hg_test(name) \
    inline bool hg_test_function_##name(); \
    inline HgTest hg_test_struct_##name{#name, hg_test_function_##name}; \
    inline bool hg_test_function_##name() 

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
 * Runs all tests registered globally
 *
 * Note, tests are rendered through static init, so this should be called after
 * static init is completed
 *
 * Returns:
 * - Whether all tests passed
 */
bool hg_run_tests();

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using usize = std::size_t;
using uptr = std::uintptr_t;
using iptr = std::intptr_t;

using f32 = std::float_t;
using f64 = std::double_t;

template<typename T>
static constexpr bool hg_is_memmove_safe = std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>;

template<typename T>
struct hg_remove_cvref {
    using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

template<typename T>
using hg_remove_cvref_t = typename hg_remove_cvref<T>::type;

/**
 * An object which may or may not exist
 */
template<typename T>
using HgOption = std::optional<T>;

/**
 * A pointer-count view into memory
 */
template<typename T>
struct HgSpan {
    /**
     * The array
     */
    T* data;
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
    constexpr usize size() const {
        return count * sizeof(*data);
    }

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr T& operator[](usize index) {
        hg_assert(data != nullptr);
        hg_assert(index < count);
        return data[index];
    }

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr const T& operator[](usize index) const {
        hg_assert(data != nullptr);
        hg_assert(index < count);
        return data[index];
    }

    /**
     * For c++ ranged based for
     */
    constexpr T* begin() {
        return data;
    }

    /**
     * For c++ ranged based for
     */
    constexpr T* end() {
        return data + count;
    }

    /**
     * For c++ ranged based for
     */
    constexpr const T* begin() const {
        return data;
    }

    /**
     * For c++ ranged based for
     */
    constexpr const T* end() const {
        return data + count;
    }

    /**
     * Implicit conversion can add const
     */
    constexpr operator HgSpan<const T>() const {
        return {(const T*)data, count};
    }
};

template<>
struct HgSpan<const void> {
    /**
     * The array
     */
    const void* data;
    /**
     * The size of the array in bytes
     */
    usize count;
    /**
     * The alignment of the array
     */
    usize alignment;

    /**
     * The size of the array in bytes
     *
     * Returns
     * - The size of the array in bytes
     */
    constexpr usize size() const {
        return count;
    }
};

template<>
struct HgSpan<void> {
    /**
     * The array
     */
    void* data;
    /**
     * The size of the array in bytes
     */
    usize count;
    /**
     * The alignment of the array
     */
    usize alignment;

    /**
     * The size of the array in bytes
     *
     * Returns
     * - The size of the array in bytes
     */
    constexpr usize size() const {
        return count;
    }

    /**
     * Implicit conversion can add const
     */
    constexpr operator HgSpan<const void>() const {
        return {(const void*)data, count, alignment};
    }
};

template<typename T>
constexpr bool operator==(HgSpan<T> lhs, HgSpan<T> rhs) {
    return lhs.data == rhs.data && lhs.count == rhs.count;
}

template<typename T>
constexpr bool operator!=(HgSpan<T> lhs, HgSpan<T> rhs) {
    return !(lhs == rhs);
}

template<typename T>
constexpr bool operator==(HgSpan<T> lhs, std::nullptr_t rhs) {
    if (lhs.data == rhs)
        hg_assert(lhs.count == 0);
    return lhs.data == rhs;
}

template<typename T>
constexpr bool operator!=(HgSpan<T> lhs, std::nullptr_t rhs) {
    return !(lhs == rhs);
}

template<typename T>
constexpr bool operator==(std::nullptr_t lhs, HgSpan<T> rhs) {
    if (lhs == rhs.data)
        hg_assert(rhs.count == 0);
    return lhs == rhs.data;
}

template<typename T>
constexpr bool operator!=(std::nullptr_t lhs, HgSpan<T> rhs) {
    return !(lhs == rhs);
}

/**
 * A span view into a string
 */
struct HgStringView : public HgSpan<const char> {
    HgStringView() = default;
    constexpr HgStringView(const char* data_val, usize count_val) : HgSpan{data_val, count_val} {}

    /**
     * Implicit constexpr conversion from c string
     *
     * Potentially dangerous, c string must be at most 4096 chars
     */
    constexpr HgStringView(const char* c_str) : HgSpan{c_str, 0} {
        while (c_str[count] != '\0') {
            ++count;
            hg_assert(count <= 4096);
        }
    }
};

static constexpr f64 hg_pi    = 3.1415926535897932;
static constexpr f64 hg_tau   = 6.2831853071795864;
static constexpr f64 hg_euler = 2.7182818284590452;
static constexpr f64 hg_root2 = 1.4142135623730951;
static constexpr f64 hg_root3 = 1.7320508075688772;

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
 * A 2D vector
 */
struct HgVec2 {
    f32 x, y;

    HgVec2() = default;
    constexpr HgVec2(f32 scalar) : x{scalar}, y{scalar} {}
    constexpr HgVec2(f32 x_val, f32 y_val) : x{x_val}, y{y_val} {}

    constexpr f32& operator[](usize index) {
        hg_assert(index < 2);
        return* (&x + index);
    }

    constexpr const f32& operator[](usize index) const {
        hg_assert(index < 2);
        return* (&x + index);
    }

    const HgVec2& operator+=(const HgVec2& other);
    const HgVec2& operator-=(const HgVec2& other);
    const HgVec2& operator*=(const HgVec2& other);
    const HgVec2& operator/=(const HgVec2& other);
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

    HgVec3() = default;
    constexpr HgVec3(f32 scalar) : x{scalar}, y{scalar}, z{scalar} {}
    constexpr HgVec3(f32 x_val, f32 y_val) : x{x_val}, y{y_val}, z{0} {}
    constexpr HgVec3(f32 x_val, f32 y_val, f32 z_val) : x{x_val}, y{y_val}, z{z_val} {}
    constexpr HgVec3(const HgVec2& other) : x{other.x}, y{other.y}, z{0} {}

    constexpr f32& operator[](usize index) {
        hg_assert(index < 3);
        return* (&x + index);
    }

    constexpr const f32& operator[](usize index) const {
        hg_assert(index < 3);
        return* (&x + index);
    }

    const HgVec3& operator+=(const HgVec3& other);
    const HgVec3& operator-=(const HgVec3& other);
    const HgVec3& operator*=(const HgVec3& other);
    const HgVec3& operator/=(const HgVec3& other);
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
 
    HgVec4() = default;
    constexpr HgVec4(f32 scalar) : x{scalar}, y{scalar}, z{scalar}, w{scalar} {}
    constexpr HgVec4(f32 x_val, f32 y_val) : x{x_val}, y{y_val}, z{0}, w{0} {}
    constexpr HgVec4(f32 x_val, f32 y_val, f32 z_val) : x{x_val}, y{y_val}, z{z_val}, w{0} {}
    constexpr HgVec4(f32 x_val, f32 y_val, f32 z_val, f32 w_val) : x{x_val}, y{y_val}, z{z_val}, w{w_val} {}
    constexpr HgVec4(const HgVec2& other) : x{other.x}, y{other.y}, z{0}, w{0} {}
    constexpr HgVec4(const HgVec3& other) : x{other.x}, y{other.y}, z{other.z}, w{0} {}

    constexpr f32& operator[](usize index) {
        hg_assert(index < 4);
        return* (&x + index);
    }

    constexpr const f32& operator[](usize index) const {
        hg_assert(index < 4);
        return* (&x + index);
    }

    const HgVec4& operator+=(const HgVec4& other);
    const HgVec4& operator-=(const HgVec4& other);
    const HgVec4& operator*=(const HgVec4& other);
    const HgVec4& operator/=(const HgVec4& other);
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

    HgMat2() = default;
    constexpr HgMat2(f32 scalar) : x{scalar, 0}, y{0, scalar} {}
    constexpr HgMat2(f32 xx, f32 xy, f32 yx, f32 yy) : x{xx, xy}, y{yx, yy} {}
    constexpr HgMat2(const HgVec2& x_val, const HgVec2& y_val) : x{x_val}, y{y_val} {}

    constexpr HgVec2& operator[](usize index) {
        hg_assert(index < 2);
        return* (&x + index);
    }

    constexpr const HgVec2& operator[](usize index) const {
        hg_assert(index < 2);
        return* (&x + index);
    }

    const HgMat2& operator+=(const HgMat2& other);
    const HgMat2& operator-=(const HgMat2& other);
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

    HgMat3() = default;
    constexpr HgMat3(f32 scalar)
        : x{scalar, 0, 0}, y{0, scalar, 0}, z{0, 0, scalar} {}
    constexpr HgMat3(const HgVec2& x_val, const HgVec2& y_val)
        : x{x_val}, y{y_val}, z{0, 0, 1} {}
    constexpr HgMat3(const HgVec3& x_val, const HgVec3& y_val, const HgVec3& z_val)
        : x{x_val}, y{y_val}, z{z_val} {}
    constexpr HgMat3(const HgMat2& other)
        : x{other.x}, y{other.y}, z{0, 0, 1} {}

    constexpr HgVec3& operator[](usize index) {
        hg_assert(index < 3);
        return* (&x + index);
    }

    constexpr const HgVec3& operator[](usize index) const {
        hg_assert(index < 3);
        return* (&x + index);
    }

    const HgMat3& operator+=(const HgMat3& other);
    const HgMat3& operator-=(const HgMat3& other);
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

    HgMat4() = default;
    constexpr HgMat4(f32 scalar)
        : x{scalar, 0, 0, 0}, y{0, scalar, 0, 0}, z{0, 0, scalar, 0}, w{0, 0, 0, scalar} {}
    constexpr HgMat4(const HgVec2& x_val, const HgVec2& y_val)
        : x{x_val}, y{y_val}, z{0, 0, 1, 0}, w{0, 0, 0, 1} {}
    constexpr HgMat4(const HgVec3& x_val, const HgVec3& y_val, const HgVec3& z_val)
        : x{x_val}, y{y_val}, z{z_val}, w{0, 0, 0, 1} {}
    constexpr HgMat4(const HgVec4& x_val, const HgVec4& y_val, const HgVec4& z_val, const HgVec4& w_val)
        : x{x_val}, y{y_val}, z{z_val}, w{w_val} {}
    constexpr HgMat4(const HgMat2& other)
        : x{other.x}, y{other.y}, z{0, 0, 1, 0}, w{0, 0, 0, 1} {}
    constexpr HgMat4(const HgMat3& other)
        : x{other.x}, y{other.y}, z{other.z}, w{0, 0, 0, 1} {}

    constexpr HgVec4& operator[](usize index) {
        hg_assert(index < 4);
        return* (&x + index);
    }

    constexpr const HgVec4& operator[](usize index) const {
        hg_assert(index < 4);
        return* (&x + index);
    }

    const HgMat4& operator+=(const HgMat4& other);
    const HgMat4& operator-=(const HgMat4& other);
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
    /**
     * The real part
     */
    f32 r;
    /**
     * The imaginary part
     */
    f32 i;

    HgComplex() = default;
    constexpr HgComplex(f32 r_val) : r{r_val}, i{0} {}
    constexpr HgComplex(f32 r_val, f32 i_val) : r{r_val}, i{i_val} {}

    constexpr f32& operator[](usize index) {
        hg_assert(index < 2);
        return* (&r + index);
    }

    constexpr const f32& operator[](usize index) const {
        hg_assert(index < 2);
        return* (&r + index);
    }

    const HgComplex& operator+=(const HgComplex& other);
    const HgComplex& operator-=(const HgComplex& other);
};

constexpr bool operator==(const HgComplex& lhs, const HgComplex& rhs) {
    return lhs.r == rhs.r && lhs.i == rhs.i;
}

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
 
    HgQuat() = default;
    constexpr HgQuat(f32 r_val) : r{r_val}, i{0}, j{0}, k{0} {}
    constexpr HgQuat(f32 r_val, f32 i_val, f32 j_val, f32 k_val) : r{r_val}, i{i_val}, j{j_val}, k{k_val} {}

    constexpr f32& operator[](usize index) {
        hg_assert(index < 4);
        return* (&r + index);
    }

    constexpr const f32& operator[](usize index) const {
        hg_assert(index < 4);
        return* (&r + index);
    }

    const HgQuat& operator+=(const HgQuat& other);
    const HgQuat& operator-=(const HgQuat& other);
};

constexpr bool operator==(const HgQuat& lhs, const HgQuat& rhs) {
    return lhs.r == rhs.r && lhs.i == rhs.i && lhs.j == rhs.j && lhs.k == rhs.k;
}

constexpr bool operator!=(const HgQuat& lhs, const HgQuat& rhs) {
    return lhs.r != rhs.r || lhs.i != rhs.i || lhs.j != rhs.j || lhs.k != rhs.k;
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
void hg_vec_add(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Adds two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The added vector
 */
constexpr HgVec2 operator+(const HgVec2& lhs, const HgVec2& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

/**
 * Adds two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The added vector
 */
constexpr HgVec3 operator+(const HgVec3& lhs, const HgVec3& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

/**
 * Adds two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The added vector
 */
constexpr HgVec4 operator+(const HgVec4& lhs, const HgVec4& rhs) {
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
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
void hg_vec_sub(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Subtracts two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The subtracted vector
 */
constexpr HgVec2 operator-(const HgVec2& lhs, const HgVec2& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

/**
 * Subtracts two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The subtracted vector
 */
constexpr HgVec3 operator-(const HgVec3& lhs, const HgVec3& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

/**
 * Subtracts two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The subtracted vector
 */
constexpr HgVec4 operator-(const HgVec4& lhs, const HgVec4& rhs) {
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
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
void hg_vec_mul_pairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Multiplies pairwise two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The multiplied vector
 */
constexpr HgVec2 operator*(const HgVec2& lhs, const HgVec2& rhs) {
    return {lhs.x * rhs.x, lhs.y * rhs.y};
}

/**
 * Multiplies pairwise two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The multiplied vector
 */
constexpr HgVec3 operator*(const HgVec3& lhs, const HgVec3& rhs) {
    return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

/**
 * Multiplies pairwise two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The multiplied vector
 */
constexpr HgVec4 operator*(const HgVec4& lhs, const HgVec4& rhs) {
    return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w};
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
void hg_vec_scalar_mul(u32 size, f32* dst, f32 scalar, const f32* vec);

/**
 * Multiplies a scalar and a 2D vector
 *
 * Parameters
 * - scalar The scalar to multiply with
 * - vec The vector to multiply with
 *
 * Returns
 * - The multiplied vector
 */
constexpr HgVec2 operator*(f32 scalar, const HgVec2& vec) {
    return {scalar * vec.x, scalar * vec.y};
}

/**
 * Multiplies a scalar and a 2D vector
 *
 * Parameters
 * - vec The vector to multiply with
 * - scalar The scalar to multiply with
 *
 * Returns
 * - The multiplied vector
 */
constexpr HgVec2 operator*(const HgVec2& vec, f32 scalar) {
    return {scalar * vec.x, scalar * vec.y};
}

/**
 * Multiplies a scalar and a 3D vector
 *
 * Parameters
 * - scalar The scalar to multiply with
 * - vec The vector to multiply with
 *
 * Returns
 * - The multiplied vector
 */
constexpr HgVec3 operator*(f32 scalar, const HgVec3& vec) {
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z};
}

/**
 * Multiplies a scalar and a 3D vector
 *
 * Parameters
 * - vec The vector to multiply with
 * - scalar The scalar to multiply with
 *
 * Returns
 * - The multiplied vector
 */
constexpr HgVec3 operator*(const HgVec3& vec, f32 scalar) {
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z};
}

/**
 * Multiplies a scalar and a 4D vector
 *
 * Parameters
 * - scalar The scalar to multiply with
 * - vec The vector to multiply with
 *
 * Returns
 * - The multiplied vector
 */
constexpr HgVec4 operator*(f32 scalar, const HgVec4& vec) {
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
}

/**
 * Multiplies a scalar and a 4D vector
 *
 * Parameters
 * - vec The vector to multiply with
 * - scalar The scalar to multiply with
 *
 * Returns
 * - The multiplied vector
 */
constexpr HgVec4 operator*(const HgVec4& vec, f32 scalar) {
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
}

/**
 * Divides pairwise two arbitrary size vectors
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
 * Divides pairwise two 2D vectors
 *
 * Note, cannot divide by 0
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The divided vector
 */
constexpr HgVec2 operator/(const HgVec2& lhs, const HgVec2& rhs) {
    hg_assert(rhs.x != 0 && rhs.y != 0);
    return {lhs.x / rhs.x, lhs.y / rhs.y};
}

/**
 * Divides pairwise two 3D vectors
 *
 * Note, cannot divide by 0
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The divided vector
 */
constexpr HgVec3 operator/(const HgVec3& lhs, const HgVec3& rhs) {
    hg_assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
    return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
}

/**
 * Divides pairwise two 4D vectors
 *
 * Note, cannot divide by 0
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The divided vector
 */
constexpr HgVec4 operator/(const HgVec4& lhs, const HgVec4& rhs) {
    hg_assert(rhs.x != 0 && rhs.y != 0 && rhs.z != 0 && rhs.w != 0);
    return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w};
}

/**
 * Divides a vector by a scalar
 *
 * Note, cannot divide by 0
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be nullptr
 * - vec The vector to divide, must not be nullptr
 * - scalar The scalar to divide by
 */
void hg_vec_scalar_div(u32 size, f32* dst, const f32* vec, f32 scalar);

/**
 * Divides a 2D vector by a scalar
 *
 * Note, cannot divide by 0
 *
 * Parameters
 * - vec The vector to divide
 * - scalar The scalar to divide by
 *
 * Returns
 * - The divided vector
 */
constexpr HgVec2 operator/(const HgVec2& vec, f32 scalar) {
    hg_assert(scalar != 0);
    return {vec.x / scalar, vec.y / scalar};
}

/**
 * Divides a 3D vector by a scalar
 *
 * Note, cannot divide by 0
 *
 * Parameters
 * - vec The vector to divide
 * - scalar The scalar to divide by
 *
 * Returns
 * - The divided vector
 */
constexpr HgVec3 operator/(const HgVec3& vec, f32 scalar) {
    hg_assert(scalar != 0);
    return {vec.x / scalar, vec.y / scalar, vec.z / scalar};
}

/**
 * Divides a 4D vector by a scalar
 *
 * Note, cannot divide by 0
 *
 * Parameters
 * - vec The vector to divide
 * - scalar The scalar to divide by
 *
 * Returns
 * - The divided vector
 */
constexpr HgVec4 operator/(const HgVec4& vec, f32 scalar) {
    hg_assert(scalar != 0);
    return {vec.x / scalar, vec.y / scalar, vec.z / scalar, vec.w / scalar};
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
void hg_dot(u32 size, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Computes the dot product of two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The dot product
 */
constexpr f32 hg_dot(const HgVec2& lhs, const HgVec2& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

/**
 * Computes the dot product of two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The dot product
 */
constexpr f32 hg_dot(const HgVec3& lhs, const HgVec3& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

/**
 * Computes the dot product of two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The dot product
 */
constexpr f32 hg_dot(const HgVec4& lhs, const HgVec4& rhs) {
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
void hg_len(u32 size, f32* dst, const f32* vec);

/**
 * Computes the length of a 2D vector
 *
 * Parameters
 * - lhs The vector to compute the length of
 *
 * Returns
 * - The length of the vector
 */
f32 hg_len(const HgVec2& vec);

/**
 * Computes the length of a 3D vector
 *
 * Parameters
 * - lhs The vector to compute the length of
 *
 * Returns
 * - The length of the vector
 */
f32 hg_len(const HgVec3& vec);

/**
 * Computes the length of a 4D vector
 *
 * Parameters
 * - lhs The vector to compute the length of
 *
 * Returns
 * - The length of the vector
 */
f32 hg_len(const HgVec4& vec);

/**
 * Normalizes a vector
 *
 * Note, cannot normalize 0
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be nullptr
 * - vec The vector to normalize, must not be nullptr
 */
void hg_norm(u32 size, f32* dst, const f32* vec);

/**
 * Normalizes a 2D vector
 *
 * Note, cannot normalize 0
 *
 * Parameters
 * - vec The vector to normalize
 *
 * Returns
 * - The normalized vector
 */
HgVec2 hg_norm(const HgVec2& vec);

/**
 * Normalizes a 3D vector
 *
 * Note, cannot normalize 0
 *
 * Parameters
 * - vec The vector to normalize
 *
 * Returns
 * - The normalized vector
 */
HgVec3 hg_norm(const HgVec3& vec);

/**
 * Normalizes a 4D vector
 *
 * Note, cannot normalize 0
 *
 * Parameters
 * - vec The vector to normalize
 *
 * Returns
 * - The normalized vector
 */
HgVec4 hg_norm(const HgVec4& vec);

/**
 * Computes the cross product of two 3D vectors
 *
 * Parameters
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void hg_cross(f32* dst, const f32* lhs, const f32* rhs);

/**
 * Computes the cross product of two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 *
 * Returns
 * - The cross product
 */
HgVec3 hg_cross(const HgVec3& lhs, const HgVec3& rhs);

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
void hg_mat_add(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Adds two 2x2 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 *
 * Returns
 * - The added matrix
 */
HgMat2 operator+(const HgMat2& lhs, const HgMat2& rhs);

/**
 * Adds two 3x3 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 *
 * Returns
 * - The added matrix
 */
HgMat3 operator+(const HgMat3& lhs, const HgMat3& rhs);

/**
 * Adds two 4x4 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 *
 * Returns
 * - The added matrix
 */
HgMat4 operator+(const HgMat4& lhs, const HgMat4& rhs);

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
void hg_mat_sub(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs);

/**
 * Subtracts two 2x2 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 *
 * Returns
 * - The subtracted matrix
 */
HgMat2 operator-(const HgMat2& lhs, const HgMat2& rhs);

/**
 * Subtracts two 3x3 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 *
 * Returns
 * - The subtracted matrix
 */
HgMat3 operator-(const HgMat3& lhs, const HgMat3& rhs);

/**
 * Subtracts two 4x4 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 *
 * Returns
 * - The subtracted matrix
 */
HgMat4 operator-(const HgMat4& lhs, const HgMat4& rhs);

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
void hg_mat_mul(f32* dst, u32 wl, u32 hl, const f32* lhs, u32 wr, u32 hr, const f32* rhs);

/**
 * Multiplies two 2x2 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 *
 * Returns
 * - The multiplied matrix
 */
HgMat2 operator*(const HgMat2& lhs, const HgMat2& rhs);

/**
 * Multiplies two 3x3 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 *
 * Returns
 * - The multiplied matrix
 */
HgMat3 operator*(const HgMat3& lhs, const HgMat3& rhs);

/**
 * Multiplies two 4x4 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 *
 * Returns
 * - The multiplied matrix
 */
HgMat4 operator*(const HgMat4& lhs, const HgMat4& rhs);

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
void hg_mat_vec_mul(u32 width, u32 height, f32* dst, const f32* mat, const f32* vec);

/**
 * Multiplies a 2x2 matrix and a 2D vector
 *
 * Parameters
 * - lhs The matrix to multiply with
 * - rhs The vector to multiply with
 *
 * Returns
 * - The multiplied vector
 */
HgVec2 operator*(const HgMat2& lhs, const HgVec2& rhs);

/**
 * Multiplies a 3x3 matrix and a 3D vector
 *
 * Parameters
 * - lhs The matrix to multiply with
 * - rhs The vector to multiply with
 *
 * Returns
 * - The multiplied vector
 */
HgVec3 operator*(const HgMat3& lhs, const HgVec3& rhs);

/**
 * Multiplies a 4x4 matrix and a 4D vector
 *
 * Parameters
 * - lhs The matrix to multiply with
 * - rhs The vector to multiply with
 *
 * Returns
 * - The multiplied vector
 */
HgVec4 operator*(const HgMat4& lhs, const HgVec4& rhs);

/**
 * Adds two complex numbers
 *
 * Parameters
 * - lhs The left-hand side complex number
 * - rhs The right-hand side complex number
 *
 * Returns
 * - The added complex number
 */
constexpr HgComplex operator+(const HgComplex& lhs, const HgComplex& rhs) {
    return {lhs.r + rhs.r, lhs.i + rhs.i};
}

/**
 * Subtracts two complex numbers
 *
 * Parameters
 * - lhs The left-hand side complex number
 * - rhs The right-hand side complex number
 *
 * Returns
 * - The subtracted complex number
 */
constexpr HgComplex operator-(const HgComplex& lhs, const HgComplex& rhs) {
    return {lhs.r - rhs.r, lhs.i - rhs.i};
}

/**
 * Multiplies two complex numbers
 *
 * Parameters
 * - lhs The left-hand side complex number
 * - rhs The right-hand side complex number
 *
 * Returns
 * - The multiplied complex number
 */
constexpr HgComplex operator*(const HgComplex& lhs, const HgComplex& rhs) {
    return {lhs.r * rhs.r - lhs.i * rhs.i, lhs.r * rhs.i + lhs.i * rhs.r};
}

/**
 * Adds two quaternions
 *
 * Parameters
 * - lhs The left-hand side quaternion
 * - rhs The right-hand side quaternion
 *
 * Returns
 * - The added quaternion
 */
constexpr HgQuat operator+(const HgQuat& lhs, const HgQuat& rhs) {
    return {lhs.r + rhs.r, lhs.i + rhs.i, lhs.j + rhs.j, lhs.k + rhs.k};
}

/**
 * Subtracts two quaternions
 *
 * Parameters
 * - lhs The left-hand side quaternion
 * - rhs The right-hand side quaternion
 *
 * Returns
 * - The subtracted quaternion
 */
constexpr HgQuat operator-(const HgQuat& lhs, const HgQuat& rhs) {
    return {lhs.r - rhs.r, lhs.i - rhs.i, lhs.j - rhs.j, lhs.k - rhs.k};
}

/**
 * Multiplies two quaternions
 *
 * Parameters
 * - lhs The left-hand side quaternion
 * - rhs The right-hand side quaternion
 *
 * Returns
 * - The multiplied quaternion
 */
HgQuat operator*(const HgQuat& lhs, const HgQuat& rhs);

/**
 * Computes the conjugate of a quaternion
 *
 * Parameters
 * - quat The quaternion to compute the conjugate of
 *
 * Returns
 * - The conjugate of the quaternion
 */
constexpr HgQuat hg_conj(const HgQuat& quat) {
    return {quat.r, -quat.i, -quat.j, -quat.k};
}

/**
 * Creates a rotation quaternion from an axis and angle
 *
 * Parameters
 * - axis The axis of the rotation
 * - angle The angle of the rotation
 *
 * Returns
 * - The created quaternion
 */
HgQuat hg_axis_angle(const HgVec3& axis, f32 angle);

/**
 * Rotates a 3D vector using a quaternion
 *
 * Parameters
 * - lhs The quaternion to rotate with
 * - rhs The vector to rotate
 *
 * Returns
 * - The rotated vector
 */
HgVec3 hg_rotate(const HgQuat& lhs, const HgVec3& rhs);

/**
 * Rotates a 3x3 matrix using a quaternion
 *
 * Parameters
 * - lhs The quaternion to rotate with
 * - rhs The matrix to rotate
 *
 * Returns
 * - The rotated matrix
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
 * The interface for generic allocators
 */
struct HgAllocator {
    /**
     * Allocates memory
     *
     * Parameters
     * - size The size to allocate in bytes
     * - alignment The alignment of the allocation, must be a power of 2
     *
     * Returns
     * - The allocated memory
     * - nullptr on failure
     */
    virtual void* alloc_fn(usize size, usize alignment) = 0;

    /**
     * Changes the size of an allocation, potentially returning a different
     * allocation, with the data copied over
     *
     * Parameters
     * - allocation The allocation to free
     * - old_size The original size of the allocation in bytes
     * - new_size The size to allocate in bytes
     * - alignment The alignment of the allocation, must be a power of 2
     *
     * Returns
     * - The reallocated memory
     * - nullptr on failure
     */
    virtual void* realloc_fn(void* allocation, usize old_size, usize new_size, usize alignment) = 0;

    /**
     * Frees allocated memory
     *
     * Parameters
     * - allocation The allocation to free
     * - size The size of the allocation in bytes
     * - alignment The alignment of the allocation, must be a power of 2
     */
    virtual void free_fn(void* allocation, usize size, usize alignment) = 0;

    /**
     * A convenience to allocate a type
     *
     * Returns
     * - The allocated item
     * - nullptr on failure
     */
    template<typename T>
    T* alloc() {
        return new ((T* )alloc_fn(sizeof(T), alignof(T))) T;
    }

    /**
     * A convenience to allocate an array of a type
     *
     * Note, objects are default constructed if possible, otherwise they are
     * left uninitialized
     *
     * Parameters
     * - count The number of T to allocate
     *
     * Returns
     * - The allocated array
     * - nullptr on failure
     */
    template<typename T>
    HgSpan<T> alloc(usize count) {
        HgSpan<T> span;
        span.data = (T* )alloc_fn(count * sizeof(T), alignof(T));
        span.count = span.data != nullptr ? count : 0;

        if constexpr (std::is_default_constructible_v<T>) {
            for (usize i = 0; i < span.count; ++i) {
                new (&span[i]) T;
            }
        }

        return span;
    }

    /**
     * A convenience to allocate a void array
     *
     * Parameters
     * - size The size in bytes to allocate
     *
     * Returns
     * - The allocated array
     * - nullptr on failure
     */
    HgSpan<void> alloc(usize size, usize alignment) {
        HgSpan<void> span;
        span.data = alloc_fn(size, alignment);
        span.count = span.data != nullptr ? size : 0;
        span.alignment = alignment;
        return span;
    }

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
     * - The reallocated array
     * - nullptr on failure
     */
    template<typename T>
    HgSpan<T> realloc(HgSpan<T> allocation, usize count) {
        static_assert(hg_is_memmove_safe<T>);

        HgSpan<T> span;
        span.data = (T* )realloc_fn(allocation.data, allocation.count * sizeof(T), count * sizeof(T), alignof(T));
        span.count = span.data != nullptr ? count : 0;

        if constexpr (std::is_default_constructible_v<T>) {
            for (usize i = allocation.count; i < span.count; ++i) {
                new (&span[i]) T;
            }
        }

        return span;
    }

    /**
     * A convenience to reallocate a void array
     *
     * Parameters
     * - allocation The allocation to reallocate
     * - size The new size to allocate
     *
     * Returns
     * - The reallocated array
     * - nullptr on failure
     */
    HgSpan<void> realloc(HgSpan<void> allocation, usize size) {
        HgSpan<void> span;
        span.data = realloc_fn(allocation.data, allocation.count, size, allocation.alignment);
        span.count = span.data != nullptr ? size : 0;
        return span;
    }

    /**
     * A convenience to free a type
     *
     * Parameters
     * - allocation The allocation to free
     */
    template<typename T>
    void free(T* allocation) {
        if constexpr (std::is_destructible_v<T>) {
            allocation->~T();
        }
        free_fn(allocation, sizeof(T), alignof(T));
    }

    /**
     * A convenience to free an array of a type
     *
     * Parameters
     * - allocation The allocation to free
     */
    template<typename T>
    void free(HgSpan<T> allocation) {
        if constexpr (std::is_destructible_v<T>) {
            for (usize i = 0; i < allocation.count; ++i) {
                allocation[i].~T();
            }
        }
        free_fn(allocation.data, allocation.count * sizeof(T), alignof(T));
    }

    /**
     * A convenience to free a void array
     *
     * Parameters
     * - allocation The allocation to free
     */
    void free(HgSpan<void> allocation) {
        free_fn(allocation.data, allocation.count, allocation.alignment);
    }
};

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
     *
     * Returns
     * - The allocated memory
     */
    void* alloc_fn(usize size, usize alignment) override;

    /**
     * Calls realloc, checking for nullptr in debug mode
     *
     * Parameters
     * - allocation The allocation to resize
     * - old_size The size of the original allocation in bytes
     * - new_size The size of the new allocation in bytes
     * - alignment The alignment in bytes of the allocation
     *
     * Returns
     * - The allocated memory
     */
    void* realloc_fn(void* allocation, usize old_size, usize new_size, usize alignment) override;

    /**
     * Calls free
     *
     * Parameters
     * - allocation The allocation to free
     * - size The size of the allocation in bytes
     * - alignment The alignment in bytes of the allocation
     */
    void free_fn(void* allocation, usize size, usize alignment) override;

    /**
     * Convenience to grab a statically constructed HgStdAllocator vtable
     *
     * Returns
     * - The global standard allocator, no different from any other
     */
    static HgStdAllocator& get() {
        static HgStdAllocator mem;
        return mem;
    }
};

/**
 * An arena allocator
 *
 * Allocations are made very quickly, and are not freed individually, instead
 * the whole block is freed at once
 */
struct HgArena : public HgAllocator {
    /**
     * A pointer to the memory being allocated
     */
    HgSpan<void> memory;
    /**
     * The next allocation to be given out
     */
    void* head;

    /**
     * Allocates an arena with capacity
     *
     * Parameters
     * - parent The allocator to create the arena from
     * - capacity The size of the block to allocate and use
     *
     * Returns
     * - The allocated arena
     * - nullopt if allocation failed
     */
    static HgOption<HgArena> create(HgAllocator& parent, usize capacity);

    /**
     * Frees an arena's memory
     */
    void destroy(HgAllocator& parent);

    /**
     * Frees all allocations from an arena
     */
    void reset() {
        head = memory.data;
    }

    /**
     * Returns the state of the arena
     */
    void* save() {
        return head;
    }

    /**
     * Loads the saved state of the arena
     */
    void load(void* save_state) {
        hg_assert((uptr)head <= (uptr)save_state && (uptr)save_state <= (uptr)memory.data + memory.count);
        head = save_state;
    }

    /**
     * Allocates memory from an arena
     *
     * Allocations are not individually freed, hg_arena() is
     * called instead to free all allocations at once
     *
     * Parameters
     * - size The size in bytes of the allocation
     * - alignment The required alignment of the allocation in bytes
     *
     * Returns
     * - The allocation if successful
     * - nullptr if the allocation exceeds capacity
     */
    void* alloc_fn(usize size, usize alignment) override;

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
     *
     * Returns
     * - The allocation if successful
     * - nullptr if the allocation exceeds capacity
     */
    void* realloc_fn(void* allocation, usize old_size, usize new_size, usize alignment) override;

    /**
     * Does nothing, only exists to fit allocator interface
     *
     * Parameters
     * - allocation The allocation to free, must be the last allocation made
     * - size The size of the allocation
     * - alignment The required alignment of the allocation in bytes
     */
    void free_fn(void* allocation, usize size, usize alignment) override;
};

/**
 * A stack allocator
 *
 * Allocations are made very quickly, but must be freed in reverse order
 */
struct HgStack : public HgAllocator {
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
     * - parent The allocator to create the stack from
     * - capacity The size of the block to allocate and use
     *
     * Returns
     * - The allocated stack
     * - nullopt if allocation failed
     */
    static HgOption<HgStack> create(HgAllocator& parent, usize capacity);

    /**
     * Frees a stack's memory
     */
    void destroy(HgAllocator& parent);

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
     *
     * Returns
     * - The allocation if successful
     * - nullptr if the allocation exceeds capacity
     */
    void* alloc_fn(usize size, usize alignment) override;

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
     *
     * Returns
     * - The allocation if successful
     * - nullptr if the allocation exceeds capacity
     */
    void* realloc_fn(void* allocation, usize old_size, usize new_size, usize alignment) override;

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
    void free_fn(void* allocation, usize size, usize alignment) override;
};

/**
 * A dynamically sized array
 */
template<typename T>
struct HgArray {
    static_assert(hg_is_memmove_safe<T>);

    /**
     * The allocated space for the array
     */
    T* items;
    /**
     * The max number of items that can be stored in the array
     */
    usize capacity;
    /**
     * The number of active items in the array
     */
    usize count;

    /**
     * Returns the size in bytes of the array
     */
    constexpr usize size() const {
        return count * sizeof(T);
    }

    /**
     * Allocate a new dynamic array
     *
     * Parameters
     * - mem The allocator to use
     * - count The number of active items to begin with, must be <= capacity
     * - capacity The max number of items before reallocating
     *
     * Returns
     * - The allocated dynamic array
     * - nullopt if allocation failed
     */
    static HgOption<HgArray<T>> create(HgAllocator& mem, usize count, usize capacity) {
        hg_assert(count <= capacity);

        HgOption<HgArray> arr{std::in_place};

        arr->items = (T* )mem.alloc_fn(capacity * sizeof(T), alignof(T));
        if (arr->items == nullptr)
            return std::nullopt;

        arr->capacity = capacity;
        arr->count = count;

        if constexpr (std::is_default_constructible_v<T>) {
            for (usize i = 0; i < count; ++i) {
                new (&arr->items[i]) T;
            }
        } else {
            hg_assert(count == 0);
        }

        return arr;
    }

    /**
     * Free the dynamic array
     */
    void destroy(HgAllocator& mem) {
        mem.free_fn(items, capacity * sizeof(T), alignof(T));
    }

    /**
     * Removes all contained objects, emptying the array
     */
    constexpr void reset() {
        count = 0;
    }

    /**
     * Returns whether capacity is filled
     */
    constexpr bool is_full() const {
        return count == capacity;
    }

    /**
     * Returns whether items can be added safely
     */
    constexpr bool has_space() const {
        return count < capacity;
    }

    /**
     * Increases the capacity to a minimum value
     *
     * Parameters
     * - mem The allocator to use
     * - min The new minimum capacity
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool reserve(HgAllocator& mem, usize min) {
        if (min > capacity) {
            void* new_items = mem.realloc_fn(items, capacity * sizeof(T), min * sizeof(T), alignof(T));
            if (new_items == nullptr)
                return false;
            items = (T* )new_items;
            capacity = min;
        }
        return true;
    }

    /**
     * Increases the capacity of the array, or inits to 1
     *
     * Parameters
     * - mem The allocator to use
     * - factor The growth factor to increase by
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool grow(HgAllocator& mem, f32 factor = 2.0f) {
        hg_assert(factor > 1.0f);
        hg_assert(capacity <= (usize)((f32)SIZE_MAX / factor));
        return reserve(mem, capacity == 0 ? 1 : (usize)((f32)capacity * factor));
    }

    /**
     * Access the value at index
     *
     * Parameters
     * - index The index to get from, must be < count
     *
     * Returns
     * - A reference to the gotten value
     */
    constexpr T& get(usize index) {
        hg_assert(index < count);
        return items[index];
    }

    /**
     * Access the value at index in a const context
     *
     * Parameters
     * - index The index to get from, must be < count
     *
     * Returns
     * - A reference to the gotten value
     */
    constexpr const T& get(usize index) const {
        hg_assert(index < count);
        return items[index];
    }

    /**
     * Access using the index operator
     */
    constexpr T& operator[](usize index) {
        return get(index);
    }

    /**
     * Access using the index operator in a const context
     */
    constexpr const T& operator[](usize index) const {
        return get(index);
    }

    /**
     * Returns a reference to the first item in the array
     */
    constexpr T& first() {
        return get(0);
    }

    /**
     * Returns a reference to the first item in the array in a const context
     */
    constexpr const T& first() const {
        return get(0);
    }

    /**
     * Returns a reference to the last item in the array
     */
    constexpr T& last() {
        return get(count - 1);
    }

    /**
     * Returns a reference to the last item in the array in a const context
     */
    constexpr const T& last() const {
        return get(count - 1);
    }

    /**
     * For c++ ranged based for loop
     */
    constexpr T* begin() {
        return items;
    }

    /**
     * For c++ ranged based for loop in a const context
     */
    constexpr const T* begin() const {
        return items;
    }

    /**
     * For c++ ranged based for loop
     */
    constexpr T* end() {
        return items + count;
    }

    /**
     * For c++ ranged based for loop in a const context
     */
    constexpr const T* end() const {
        return items + count;
    }

    /**
     * Push an item to the end to the array
     *
     * Note, space must be available
     *
     * Parameters
     * - args The arguments to use to construct the new item, if any
     *
     * Returns
     * - A reference to the created object
     */
    template<typename... Args>
    constexpr T& push(Args&&... args) {
        hg_assert(count < capacity);
        return* (new (&items[count++]) T{std::forward<Args>(args)...});
    }

    /**
     * Pops an item off the end of the array
     */
    constexpr void pop() {
        hg_assert(count > 0);
        --count;
    }

    /**
     * Inserts an item into the array, moving items back
     *
     * Note, space must be available
     *
     * Parameters
     * - index The index the new item will be placed at, must be <= count
     * - args The arguments to use to construct the new item, if any
     *
     * Returns
     * - A reference to the created object
     */
    template<typename... Args>
    T& insert(usize index, Args&&... args) {
        hg_assert(index <= count);
        hg_assert(count < capacity);

        std::memmove((void* )&items[index + 1], (void* )&items[index], (count++ - index) * sizeof(T));
        return* (new (&items[index]) T{std::forward<Args>(args)...});
    }

    /**
     * Removes the item at the index, subsequent items to taking its place
     *
     * Parameters
     * - index The index of the item to remove
     */
    void remove(usize index) {
        hg_assert(index < count);

        std::memmove((void* )&items[index], (void* )&items[index + 1], (count - index - 1) * sizeof(T));
        --count;
    }

    /**
     * Inserts an item into the array, moving the item at index to the end
     *
     * Parameters
     * - index The index the new item will be placed at, must be <= count
     * - args The arguments to use to construct the new item, if any
     *
     * Returns
     * - A reference to the created object
     */
    template<typename... Args>
    T& swap_insert(usize index, Args&&... args) {
        hg_assert(index <= count);
        hg_assert(count < capacity);
        if (index == count)
            return push(std::forward<Args>(args)...);

        std::memmove((void* )&items[count++], (void* )&items[index], sizeof(T));
        return* (new (&items[index]) T{std::forward<Args>(args)...});
    }

    /**
     * Removes the item at the index, moving the last item to take its place
     *
     * Parameters
     * - index The index of the item to remove
     */
    void swap_remove(usize index) {
        hg_assert(index < count);
        if (index == count - 1) {
            pop();
            return;
        }

        std::memmove((void* )&items[index], (void* )&items[count - 1], sizeof(T));
        --count;
    }
};

/**
 * A type erased dynamically sized array
 */
struct HgArrayAny {
    /**
     * The allocated space for the array
     */
    void* items;
    /**
     * The size in bytes of the items
     */
    u32 width;
    /**
     * The alignment of the items
     */
    u32 alignment;
    /**
     * The max number of items that can be stored in the array
     */
    usize capacity;
    /**
     * The number of active items in the array
     */
    usize count;

    /**
     * Returns the size in bytes of the array
     */
    constexpr usize size() {
        return count * width;
    }

    /**
     * Allocate a new dynamic array
     *
     * Parameters
     * - allocator The allocator to use
     * - width The size of the items in bytes
     * - alignment The alignment of the items
     * - count The number of active items to begin with, must be <= capacity
     * - capacity The max number of items before reallocating
     *
     * Returns
     * - The allocated dynamic array
     * - nullopt if allocation failed
     */
    static HgOption<HgArrayAny> create(
        HgAllocator& mem,
        u32 width,
        u32 alignment,
        usize count,
        usize capacity
    );

    /**
     * Allocate a new dynamic array using a type
     *
     * Note, does not construct items
     *
     * Parameters
     * - allocator The allocator to use
     * - count The number of active items to begin with, must be <= capacity
     * - capacity The max number of items before reallocating
     *
     * Returns
     * - The allocated dynamic array
     * - nullopt if allocation failed
     */
    template<typename T>
    static HgOption<HgArrayAny> create(HgAllocator& mem, usize count, usize capacity) {
        static_assert(hg_is_memmove_safe<T>);
        return create(mem, sizeof(T), alignof(T), count, capacity);
    }

    /**
     * Free the dynamic array
     */
    void destroy(HgAllocator& mem) {
        mem.free_fn(items, capacity * width, alignment);
    }

    /**
     * Destroys all contained objects, emptying the array
     */
    constexpr void reset() {
        count = 0;
    }

    /**
     * Returns whether capacity is filled
     */
    constexpr bool is_full() const {
        return count == capacity;
    }

    /**
     * Returns whether items can be added safely
     */
    constexpr bool has_space() const {
        return count < capacity;
    }

    /**
     * Increases the capacity to a minimum value
     *
     * Parameters
     * - mem The allocator to use
     * - min The new minimum capacity
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool reserve(HgAllocator& mem, usize min);

    /**
     * Increases the capacity of the array, or inits to 1
     *
     * Parameters
     * - mem The allocator to use
     * - factor The growth factor to increase by
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool grow(HgAllocator& mem, f32 factor = 2.0f) {
        hg_assert(factor > 1.0f);
        hg_assert(capacity <= (usize)((f32)SIZE_MAX / factor));
        return reserve(mem, capacity == 0 ? 1 : (usize)((f32)capacity * factor));
    }

    /**
     * Access the value at index
     *
     * Parameters
     * - index The index to get from, must be < count
     *
     * Returns
     * - A pointer to the gotten value
     */
    constexpr void* get(usize index) {
        hg_assert(index < count);
        return (u8* )items + index * width;
    }

    /**
     * Access the value at index in a const context
     *
     * Parameters
     * - index The index to get from, must be < count
     *
     * Returns
     * - A pointer to the gotten value
     */
    constexpr const void* get(usize index) const {
        hg_assert(index < count);
        return (u8* )items + index * width;
    }

    /**
     * Access using the index operator
     */
    constexpr void* operator[](usize index) {
        return get(index);
    }

    /**
     * Access using the index operator in a const context
     */
    constexpr const void* operator[](usize index) const {
        return get(index);
    }

    /**
     * Returns a reference to the first item in the array
     */
    constexpr void* first() {
        return get(0);
    }

    /**
     * Returns a reference to the first item in the array in a const context
     */
    constexpr const void* first() const {
        return get(0);
    }

    /**
     * Returns a reference to the last item in the array
     */
    constexpr void* last() {
        return get(count - 1);
    }

    /**
     * Returns a reference to the last item in the array in a const context
     */
    constexpr const void* last() const {
        return get(count - 1);
    }

    /**
     * Push an item to the end to the array, asserting space is available
     *
     * Returns
     * - A pointer to the created object
     */
    constexpr void* push() {
        hg_assert(count < capacity);
        return get(count++);
    }

    /**
     * Pops an item off the end of the array
     */
    constexpr void pop() {
        hg_assert(count > 0);
        --count;
    }

    /**
     * Inserts an item into the array, moving subsequent items back
     *
     * Parameters
     * - index The index the new item will be placed at, must be <= count
     *
     * Returns
     * - A pointer to the created object
     */
    void* insert(usize index) {
        hg_assert(index <= count);
        hg_assert(count < capacity);

        std::memmove(get(index + 1), get(index), (count++ - index) * width);
        return get(index);
    }

    /**
     * Removes the item at the index, subsequent items to taking its place
     *
     * Parameters
     * - index The index of the item to remove
     */
    void remove(usize index) {
        hg_assert(index < count);

        std::memmove(get(index), get(index + 1), (count - index - 1) * width);
        --count;
    }

    /**
     * Inserts an item into the array, moving the item at index to the end
     *
     * Parameters
     * - index The index the new item will be placed at, must be <= count
     *
     * Returns
     * - A reference to the created object
     */
    void* swap_insert(usize index) {
        hg_assert(index <= count);
        hg_assert(count < capacity);
        if (index == count)
            return push();

        std::memcpy(get(count++), get(index), width);
        return get(index);
    }

    /**
     * Removes the item at the index, moving the last item to take its place
     *
     * Parameters
     * - index The index of the item to remove
     */
    void swap_remove(usize index) {
        hg_assert(index < count);
        if (index == count - 1) {
            pop();
            return;
        }

        std::memcpy(get(index), get(count - 1), width);
        --count;
    }
};

/**
 * A lock free, thread safe, multi consumer, multi producer queue
 */
template<typename T>
struct HgThreadQueue {
};

/**
 * A dynamic FIFO queue
 */
template<typename T>
struct HgQueue {
    static_assert(hg_is_memmove_safe<T>);

    /**
     * The allocated space for the queue
     */
    T* items;
    /**
     * The max number of items that can be stored in the queue
     */
    usize capacity;
    /**
     * The end of the queue, where items are added
     */
    usize head;
    /**
     * The beginning of the queue, where items are removed
     */
    usize tail;

    /**
     * Returns the number of items in the queue
     */
    constexpr usize count() const {
        return (head >= tail ? 0 : capacity) + head - tail;
    }

    /**
     * Returns the size in bytes of the queue
     */
    constexpr usize size() const {
        return count() * sizeof(T);
    }

    /**
     * Allocate a new dynamic queue
     *
     * Parameters
     * - mem The allocator to use
     * - capacity The max number of items before reallocating
     *
     * Returns
     * - The allocated dynamic queue
     * - nullopt if allocation failed
     */
    static HgOption<HgQueue<T>> create(HgAllocator& mem, usize capacity) {
        hg_assert(capacity > 1);

        HgOption<HgQueue> arr{std::in_place};

        arr->items = (T* )mem.alloc_fn(capacity * sizeof(T), alignof(T));
        if (arr->items == nullptr)
            return std::nullopt;

        arr->capacity = capacity;
        arr->head = 0;
        arr->tail = 0;

        return arr;
    }

    /**
     * Free the dynamic queue
     */
    void destroy(HgAllocator& mem) {
        mem.free_fn(items, capacity * sizeof(T), alignof(T));
    }

    /**
     * Removes all contained objects, emptying the queue
     */
    constexpr void reset() {
        head = 0;
        tail = 0;
    }

    /**
     * Returns whether capacity is filled
     */
    constexpr bool is_full() const {
        return (head + 1) % capacity == tail;
    }

    /**
     * Returns whether items can be added safely
     */
    constexpr bool has_space() const {
        return !is_full();
    }

    /**
     * Returns whether any items are contained
     */
    constexpr bool is_empty() const {
        return head == tail;
    }

    /**
     * Increases the capacity to a minimum value
     *
     * Parameters
     * - mem The allocator to use
     * - min The new minimum capacity
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool reserve(HgAllocator& mem, usize min) {
        if (min > capacity) {
            void* new_items = mem.realloc_fn(items, capacity * sizeof(T), min * sizeof(T), alignof(T));
            if (new_items == nullptr)
                return false;
            items = (T* )new_items;

            if (tail > head) {
                usize diff = min - capacity;
                std::memmove(items + tail, items + tail + diff, (capacity - tail) * sizeof(T));
            }

            capacity = min;
        }
        return true;
    }

    /**
     * Increases the capacity of the queue, or inits to 1
     *
     * Parameters
     * - mem The allocator to use
     * - factor The growth factor to increase by
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool grow(HgAllocator& mem, f32 factor = 2.0f) {
        hg_assert(factor > 1.0f);
        hg_assert(capacity <= (usize)((f32)SIZE_MAX / factor));
        return reserve(mem, capacity == 0 ? 1 : (usize)((f32)capacity * factor));
    }

    /**
     * Push an item to the end to the queue
     *
     * Note, space must be available
     *
     * Parameters
     * - args The arguments to use to construct the new item, if any
     */
    template<typename... Args>
    constexpr void push(Args&&... args) {
        hg_assert(has_space());
        new (&items[head]) T{std::forward<Args>(args)...};
        head = (head + 1) % capacity;
    }

    /**
     * Pops an item off the end of the queue
     *
     * Returns
     * - The popped item
     */
    constexpr T pop() {
        hg_assert(!is_empty());
        T ret = items[tail];
        tail = (tail + 1) % capacity;
        return ret;
    }
};

/**
 * A dynamic string container
 */
struct HgString {
    /**
     * The character buffer
     */
    HgSpan<char> chars;
    /**
     * The number of characters currently in the string
     */
    usize length;

    /**
     * Returns the size in bytes
     */
    constexpr usize size() {
        return length;
    }

    /**
     * Creates a new string with empty capacity
     *
     * Parameters
     * - mem The allocator to use
     * - capacity The capacity to begin with
     *
     * Returns
     * - The created empty string
     * - nullopt if allocation failed
     */
    static HgOption<HgString> create(HgAllocator& mem, usize capacity);

    /**
     * Creates a new string copied from an existing string
     *
     * Parameters
     * - mem The allocator to use
     * - init The initial string to copy from
     *
     * Returns
     * - The created copied string
     * - nullopt if allocation failed
     */
    static HgOption<HgString> create(HgAllocator& mem, HgStringView init);

    /**
     * Frees the string
     *
     * Parameters
     * - mem The allocator to use
     */
    void destroy(HgAllocator& mem);

    /**
     * Removes all characters
     */
    constexpr void reset() {
        length = 0;
    }

    /**
     * Access the character at index
     *
     * Parameters
     * - index The index to get from, must be < length
     *
     * Returns
     * - A reference to the gotten character
     */
    constexpr char& get(usize index) {
        hg_assert(index < length);
        return chars[index];
    }

    /**
     * Access the character at index in a const context
     *
     * Parameters
     * - index The index to get from, must be < count
     *
     * Returns
     * - A reference to the gotten character
     */
    constexpr const char& get(usize index) const {
        hg_assert(index < length);
        return chars[index];
    }

    /**
     * Access using the index operator
     */
    constexpr char& operator[](usize index) {
        return get(index);
    }

    /**
     * Access using the index operator in a const context
     */
    constexpr const char& operator[](usize index) const {
        return get(index);
    }

    /**
     * For c++ ranged based for loop
     */
    constexpr char* begin() {
        return chars.data;
    }

    /**
     * For c++ ranged based for loop in a const context
     */
    constexpr const char* begin() const {
        return chars.data;
    }

    /**
     * For c++ ranged based for loop
     */
    constexpr char* end() {
        return chars.data + length;
    }

    /**
     * For c++ ranged based for loop in a const context
     */
    constexpr const char* end() const {
        return chars.data + length;
    }

    /**
     * Increases the capacity to a minimum value
     *
     * Parameters
     * - mem The allocator to use
     * - min The new minimum capacity
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool reserve(HgAllocator& mem, usize min);

    /**
     * Increases the capacity of the string, or inits to 1
     *
     * Parameters
     * - mem The allocator to use
     * - factor The growth factor to increase by
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool grow(HgAllocator& mem, f32 factor = 2.0f);

    /**
     * Copies another string into this string at index
     * 
     * Parameters
     * - mem The allocator to use
     */
    void insert(HgAllocator& mem, usize index, HgStringView str);

    /**
     * Copies another string to the end of this string
     */
    void append(HgAllocator& mem, HgStringView str) {
        insert(mem, length, str);
    }

    /**
     * Copies another string to the beginning of this string
     */
    void prepend(HgAllocator& mem, HgStringView str) {
        insert(mem, 0, str);
    }

    /**
     * Implicit converts to string_view
     */
    constexpr operator HgStringView() {
        return {chars.data, length};
    }
};

inline bool operator==(HgString lhs, HgString rhs) {
    return lhs.length == rhs.length && memcmp(lhs.chars.data, rhs.chars.data, lhs.length) == 0;
}

inline bool operator!=(HgString lhs, HgString rhs) {
    return lhs.length != rhs.length || memcmp(lhs.chars.data, rhs.chars.data, lhs.length) != 0;
}

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
template<typename T>
constexpr usize hg_hash(f32 val) {
    union {
        f32 as_float;
        usize as_usize;
    } u;
    u.as_float = val;
    return u.as_usize;
}

/**
 * Hash map hashing for f64
 */
template<typename T>
constexpr usize hg_hash(f64 val) {
    union {
        f64 as_float;
        usize as_usize;
    } u;
    u.as_float = val;
    return u.as_usize;
}

/**
 * Hash map hashing for pointer types
 */
inline usize hg_hash(void* val) {
    return (uptr)val;
}

/**
 * Hash map hashing for strings
 */
constexpr usize hg_hash(HgStringView str) {
    constexpr u64 power = 257;
    u64 mult = 1;
    u64 res = 0;
    for (char c : str) {
        res += (u64)c * mult;
        mult *= power;
    }
    return res;
}

/**
 * Hash map hashing for HgString
 */
constexpr usize hg_hash(HgString str) {
    return hg_hash((HgStringView)str);
}

/**
 * Hash map hashing for C string
 */
constexpr usize hg_hash(const char* str) {
    return hg_hash(HgStringView{str});
}

/**
 * A key-value hash map
 *
 * Key type must have an overload of hg_hash
 */
template<typename Key, typename Value>
struct HgHashMap {
    static_assert(hg_is_memmove_safe<Key> && hg_is_memmove_safe<Value>);

    struct Pair {
        Key key;
        Value value;

        Pair(const Key& k, const Value& v) : key{k}, value{v} {}
    };

    using Slot = HgOption<Pair>;

    /**
     * Where the values are stored
     */
    HgSpan<Slot> slots;
    /**
     * The current number of values that are stored
     */
    usize load;

    /**
     * Creates a new hash map
     *
     * Parameters
     * - mem The allocator to use
     * - slot_count The max number of slots to store values in
     *
     * Returns
     * - The created empty hash map
     * - nullopt if allocation failed
     */
    static HgOption<HgHashMap> create(HgAllocator& mem, usize slot_count) {
        hg_assert(slot_count > 0);

        HgOption<HgHashMap> map{std::in_place};

        map->slots = mem.alloc<Slot>(slot_count);
        if (map->slots == nullptr)
            return std::nullopt;

        map->load = 0;

        return map;
    }

    /**
     * Destroys the hash map
     *
     * Parameters
     * - mem The allocator to use
     */
    void destroy(HgAllocator& mem) {
        mem.free(slots);
    }

    /**
     * Empties all slots
     */
    void reset() {
        for (Slot& slot : slots) {
            slot.reset();
        }
        load = 0;
    }

    /**
     * Resizes the slots and rehashes all entries
     *
     * Parameters
     * - mem The allocator to use
     * - new_size The new number of slots
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool resize(HgAllocator& mem, usize new_size) {
        HgSpan<Slot> old_slots = slots;
        hg_defer(mem.free(old_slots));
        slots = mem.alloc<Slot>(new_size);
        if (slots == nullptr)
            return false;
        for (Slot& slot : old_slots) {
            if (old_slots[index].has_value())
                insert(old_slots[index]->key, old_slots[index]->value);
        }
        return true;
    }

    /**
     * Grows the number of slots by a factor
     *
     * Parameters
     * - mem The allocator to use
     * - factor The factor to increase by
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool grow(HgAllocator& mem, f32 factor = 2) {
        return resize(mem, (usize)((f32)slots.count * factor));
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
    Value& insert(const Key& key, const Value& value) {
        hg_assert(load < slots.count - 1);

        Slot temp;
        temp.emplace(key, value);

        usize index = hg_hash(key) % slots.count;
        usize dist = 0;
        while (slots[index].has_value() && slots[index]->key != key) {
            usize other_hash = hg_hash(slots[index]->key);
            if (other_hash < index)
                other_hash += slots.count;

            usize other_dist = other_hash - index;
            if (dist > other_dist) {
                slots[index].swap(temp);
                dist = other_dist;
            }

            index = (index + 1) % slots.count;
            ++dist;
        }

        ++load;
        slots[index].swap(temp);
        return slots[index]->value;
    }

    /**
     * Removes a value from the hash map, and returns it
     *
     * Parameters
     * - key The key to remove from
     */
    HgOption<Value> get_remove(const Key& key) {
        hg_assert(load < slots.count);

        usize index = hg_hash(key) % slots.count;
        while (slots[index].has_value() && slots[index]->key != key) {
            index = (index + 1) % slots.count;
        }
        if (!slots[index].has_value())
            return std::nullopt;

        HgOption<Value> val = slots[index]->value;
        slots[index].reset();
        --load;

        index = (index + 1) % slots.count;
        while (slots[index].has_value()) {
            if (hg_hash(slots[index]->key) % slots.count != index) {
                Slot temp = slots[index];
                slots[index].reset();
                --load;
                insert(temp->key, temp->value);
            }
            index = (index + 1) % slots.count;
        }

        return val;
    }

    /**
     * Removes a value from the hash map
     *
     * Parameters
     * - key The key to remove from
     */
    void remove(const Key& key) {
        hg_assert(load < slots.count);

        usize index = hg_hash(key) % slots.count;
        while (slots[index].has_value() && slots[index]->key != key) {
            index = (index + 1) % slots.count;
        }
        if (!slots[index].has_value())
            return;

        slots[index].reset();
        --load;

        index = (index + 1) % slots.count;
        while (slots[index].has_value()) {
            if (hg_hash(slots[index]->key) % slots.count != index) {
                Slot temp = slots[index];
                slots[index].reset();
                --load;
                insert(temp->key, temp->value);
            }
            index = (index + 1) % slots.count;
        }
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
        hg_assert(load < slots.count);

        usize index = hg_hash(key) % slots.count;
        while (slots[index].has_value()) {
            if (slots[index]->key == key)
                return true;
            index = (index + 1) % slots.count;
        }
        return false;
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
    Value* try_get(const Key& key) {
        hg_assert(load < slots.count);

        usize index = hg_hash(key) % slots.count;
        while (slots[index].has_value()) {
            if (slots[index]->key == key)
                return &slots[index]->value;
            index = (index + 1) % slots.count;
        }
        return nullptr;
    }

    /**
     * Gets a reference to the value at key, asserting its existance
     *
     * Parameters
     * - key The key to get from
     *
     * Returns
     * - A reference to the value
     */
    Value& get(const Key& key) {
        hg_assert(load < slots.count);

        usize index = hg_hash(key) % slots.count;
        for (;;) {
            hg_assert(slots[index].has_value());
            if (slots[index]->key == key)
                return slots[index]->value;
            index = (index + 1) % slots.count;
        }
    }
};

/**
 * A hash set
 *
 * T must have an overload of hg_hash
 */
template<typename T>
struct HgHashSet {
    static_assert(hg_is_memmove_safe<T>);

    using Slot = HgOption<T>;

    /**
     * Where the values are stored
     */
    HgSpan<Slot> slots;
    /**
     * The current number of values that are stored
     */
    usize load;

    /**
     * Creates a new hash set
     *
     * Parameters
     * - mem The allocator to use
     * - slot_count The max number of slots to store values in
     *
     * Returns
     * - The created empty hash set
     * - nullopt if allocation failed
     */
    static HgOption<HgHashSet> create(HgAllocator& mem, usize slot_count) {
        hg_assert(slot_count > 0);

        HgOption<HgHashSet> set{std::in_place};

        set->slots = mem.alloc<Slot>(slot_count);
        if (set->slots == nullptr)
            return std::nullopt;

        set->load = 0;

        return set;
    }

    /**
     * Destroys the hash set
     *
     * Parameters
     * - mem The allocator to use
     */
    void destroy(HgAllocator& mem) {
        mem.free(slots);
    }

    /**
     * Empties all slots
     */
    void reset() {
        for (Slot& slot : slots) {
            slot.reset();
        }
        load = 0;
    }

    /**
     * Resizes the slots and rehashes all entries
     *
     * Parameters
     * - mem The allocator to use
     * - new_size The new number of slots
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool resize(HgAllocator& mem, usize new_size) {
        HgSpan<Slot> old_slots = slots;
        hg_defer(mem.free(old_slots));
        slots = mem.alloc<Slot>(new_size);
        if (slots == nullptr)
            return false;
        for (Slot& slot : old_slots) {
            if (old_slots[index].has_value())
                insert(old_slots[index].value());
        }
        return true;
    }

    /**
     * Grows the number of slots by a factor
     *
     * Parameters
     * - mem The allocator to use
     * - factor The factor to increase by
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool grow(HgAllocator& mem, f32 factor = 2) {
        return resize(mem, (usize)((f32)slots.count * factor));
    }

    /**
     * Inserts a value into the hash set
     *
     * Parameters
     * - value The value to store
     *
     * Returns
     * - A reference to the constructed object
     */
    void insert(const T& value) {
        hg_assert(load < slots.count - 1);

        Slot temp;
        temp.emplace(value);

        usize index = hg_hash(value) % slots.count;
        usize dist = 0;
        while (slots[index].has_value() && slots[index].value() != value) {
            usize other_hash = hg_hash(slots[index].value());
            if (other_hash < index)
                other_hash += slots.count;

            usize other_dist = index - other_hash;
            if (dist > other_dist) {
                slots[index].swap(temp);
                dist = other_dist;
            }

            index = (index + 1) % slots.count;
            ++dist;
        }

        ++load;
        slots[index].swap(temp);
    }

    /**
     * Removes a value from the hash set
     *
     * Parameters
     * - key The value to remove
     */
    void remove(const T& value) {
        hg_assert(load < slots.count);

        usize index = hg_hash(value) % slots.count;
        while (slots[index].has_value() && slots[index].value() != value) {
            index = (index + 1) % slots.count;
        }
        if (!slots[index].has_value())
            return;

        slots[index].reset();
        --load;

        index = (index + 1) % slots.count;
        while (slots[index].has_value()) {
            if (hg_hash(slots[index].value()) % slots.count != index) {
                Slot temp = slots[index];
                slots[index].reset();
                --load;
                insert(temp.value());
            }
            index = (index + 1) % slots.count;
        }
    }

    /**
     * Checks whether a value is contained in the set
     *
     * Parameters
     * - value The value to check
     *
     * Returns
     * - Whether the value is present
     */
    bool has(const T& value) {
        hg_assert(load < slots.count);

        usize index = hg_hash(value) % slots.count;
        while (slots[index].has_value()) {
            if (slots[index].value() == value)
                return true;
            index = (index + 1) % slots.count;
        }
        return false;
    }
};

template<typename>
struct HgFunction;

/**
 * A generic function object
 */
template<typename R, typename... Args>
struct HgFunction<R(Args...)> {
    /**
     * The function's capture
     */
    HgSpan<void> capture;
    /**
     * The function pointer
     */
    R (*fn)(void* data, Args...);

    /**
     * Calls the function
     */
    R operator()(Args... args) {
        if constexpr (std::is_same_v<R, void>) {
            fn(capture.data, args...);
        } else {
            return fn(capture.data, args...);
        }
    }

    /**
     * Creates a function object, which owns its capture and must be destroyed
     *
     * Parameters
     * - mem The allocator to use
     * - fn The function to use
     *
     * Returns
     * - The function object
     * - nullopt if allocation failed
     */
    template<typename F>
    static HgOption<HgFunction<R(Args...)>> create(HgAllocator& mem, F fn) {
        static_assert(std::is_trivially_destructible_v<F>);
        static_assert(std::is_invocable_r_v<R, F, Args...>);

        HgOption<HgFunction<R(Args...)>> func{std::in_place};

        func->capture = mem.alloc(sizeof(F), alignof(F));
        if (func->capture == nullptr)
            return std::nullopt;

        new ((F* )func->capture.data) F{std::move(fn)};

        func->fn = [](void* data, Args... args) -> R {
            if constexpr (std::is_same_v<R, void>) {
                (*(F* )data)(args...);
            } else {
                return (*(F* )data)(args...);
            }
        };

        return func;
    }

    /**
     * Destroys the function object
     *
     * Parameters
     * - mem The allocator to use
     */
    void destroy(HgAllocator& mem) {
        mem.free(capture);
    }
};

/**
 * Creates a function object, convenience over HgFunction<...>::create
 * 
 * Parameters
 * - mem The allocator to use
 * - fn The function to create from
 *
 * Returns
 * - The created function object
 * - nullopt if allocation failed
 */
template<typename Signature, typename F>
HgOption<HgFunction<Signature>> hg_function(HgAllocator& mem, F&& fn) {
    return HgFunction<Signature>::create(mem, std::forward<F>(fn));
}

/**
 * A fence for basic thread synchronization
 */
struct HgFence {
    std::atomic<usize> counter;

    HgFence() {
        counter.store(0);
    }

    /**
     * Returns whether all work has been completed
     */
    bool complete() {
        return counter.load() == 0;
    }

    /**
     * Waits for all work submissions to be completed
     *
     * Parameters
     * - timeout_seconds The time in seconds to wait before timing out
     *
     * Returns
     * - true if all work completed
     * - false if the timeout was reached
     */
    bool wait(f64 timeout_seconds);
};

/**
 * A continuous thread with signals
 */
struct HgThread {
    /**
     * The thread itself
     */
    std::thread thread;
    /**
     * Signal whether it is currently working
     */
    std::atomic_bool is_working;
    /**
     * Signal to close
     */
    std::atomic_bool should_close;
};

/**
 * A thread pool
 */
struct HgThreadPool {
    /**
     * The work for threads to execute
     */
    struct Work {
        /**
         * The fence to signal on completion
         */
        HgFence* fence;
        /**
         * The data passed to the function
         */
        void* data;
        /**
         * The function to execute
         */
        void (*fn)(void*);
    };

    /**
     * The threads in the pool
     */
    HgSpan<HgThread> threads;
    /**
     * Mutex to protect the work queue
     */
    std::mutex work_queue_mutex;
    /**
     * The queue of work to be executed
     */
    HgArray<Work> work_queue;

    /**
     * Creates a new thread pool
     *
     * Parameters
     * - mem The allocator to use
     * - thread_count The number of threads to spawn (recommended hardware - 1)
     *
     * Returns
     * - The created thread pool
     * - nullptr on failure
     */
    static HgThreadPool* create(HgAllocator& mem, usize thread_count, usize queue_size);

    /**
     * Destroys the thread pool
     */
    static void destroy(HgAllocator& mem, HgThreadPool* pool);

    /**
     * Waits for all thread submissions to be completed
     *
     * Parameters
     * - timeout_seconds The time in seconds to wait before timing out
     *
     * Returns
     * - true if all threads completed
     * - false if the timeout was reached
     */
    bool wait_idle(f64 timeout_seconds);

    /**
     * Pushes work to the queue to be executed
     *
     * Parameters
     * - fence The fence to signal upon completion, may be nullptr
     * - work The function to be executed
     */
    void call_par(HgFence* fence, void* data, void (*fn)(void*));

    /**
     * Iterates in parallel over a function n times in chunks
     *
     * Note, uses a fence internally to wait for all work to complete
     *
     * Parameters
     * - count The number of elements to iterate [0, count)
     * - chunk_size The number of elementes to iterate per parallel execution
     * - fn The function to use to iterate, takes begin and end indices
     */
    void for_par(usize count, usize chunk_size, void* data, void (*fn)(void*, usize begin, usize end));
};

/**
 * A global thread pool
 */
inline HgThreadPool* hg_threads = nullptr;

/**
 * The handle for an ECS entity
 */
struct HgEntity {
    u32 index = (u32)-1;

    operator u32() const {
        return index;
    }
};

/**
 * Creates a new id for each component
 *
 * Should only be used by hg_component_id
 */
u32 hg_create_component_id();

/**
 * The unique component id for a type, keeping const and non-const the same
 */
template<typename T>
inline const u32 hg_component_id = hg_create_component_id();

/**
 * An entity component system
 */
struct HgECS {
    /**
     * A component type in an entity component system
     */
    struct Component {
        /**
         * sparse[entity] is the index into dense and components for that entity
         */
        HgSpan<u32> sparse;
        /**
         * dense[index] is the entity for that component
         */
        HgArray<HgEntity> dense;
        /**
         * The data for the components
         */
        HgArrayAny components;
    };

    /**
     * The pool of entity ids
     */
    HgSpan<HgEntity> entity_pool;
    /**
     * The next entity in the pool
     */
    HgEntity next_entity;
    /**
     * The component systems
     */
    HgSpan<Component> systems;

    /**
     * Creates an entity component system
     *
     * Parameters
     * - allocator The allocator to use
     * - max_entities The max entities that can exist
     *
     * Returns
     * - The created entity component system
     * - nullopt if allocation failed
     */
    static HgOption<HgECS> create(HgAllocator& mem, u32 max_entities);

    /**
     * Destroys an entity component system
     */
    void destroy(HgAllocator& mem);

    /**
     * Resets an entity component system, removing all entities
     *
     * Note, does not unregister components, just clears storage
     */
    void reset();

    /**
     * Reallocates the entity pool, increasing the max number of entities
     *
     * Note, partial allocation failure, may leave an unstable state
     *
     * Parameters
     * - mem The allocator to use
     * - new_max The new max number of entities
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool resize_entities(HgAllocator& mem, u32 new_max);

    /**
     * Grows the entity pool to current * factor
     *
     * Note, partial allocation failure, may leave an unstable state
     *
     * Parameters
     * - mem The allocator to use
     * - factor The factor to increase by
     *
     * Returns
     * - Whether the allocation succeeded
     */
    bool grow_entities(HgAllocator& mem, f32 factor = 2.0f) {
        return resize_entities(mem, (u32)((f32)entity_pool.count * factor));
    }

    /**
     * Creates a new entity in an ECS
     *
     * Note, there cannot be more than entities than allocated at creation
     *
     * Returns
     * - The created entity
     */
    HgEntity spawn();

    /**
     * Destroys an entity in an ECS
     *
     * Note, this function will invalidate iterators
     *
     * Parameters
     * - entity The entity to destroy
     */
    void despawn(HgEntity entity);

    /**
     * Checks whether an entity id is alive and can be used
     *
     * Parameters
     * - entity The id of the entity to check
     *
     * Returns
     * - Whether the entity is alive and can be used
     */
    bool is_alive(HgEntity entity) {
        return entity < entity_pool.count && entity_pool[entity] == entity;
    }

    /**
     * Registers a component in this ECS
     *
     * Note, a component id must be unregistered before reregistering
     *
     * Parameters
     * - mem The allocator to get memory from
     * - max_component The max number of this component the ECS can hold
     * - component_size The size in bytes of the component struct
     * - component_alignment The alignment of the component struct
     * - component_id The id of the component
     *
     * Returns
     * - Whether allocation was successful
     */
    bool register_component_untyped(
        HgAllocator& mem,
        u32 max_components,
        u32 component_size,
        u32 component_alignment,
        u32 component_id);

    /**
     * Registers a component in this ECS
     *
     * Note, a component id must be unregistered before reregistering
     *
     * Parameters
     * - mem The allocator to get memory from
     * - max_component The max number of this component the ECS can hold
     */
    template<typename T>
    bool register_component(HgAllocator& mem, u32 max_components) {
        static_assert(hg_is_memmove_safe<T>);
        return register_component_untyped(mem, max_components, sizeof(T), alignof(T), hg_component_id<T>);
    }

    /**
     * Unregisters a component in this ECS
     *
     * Parameters
     * - mem The allocator to get memory from
     * - component_id The id of the component
     */
    void unregister_component_untyped(HgAllocator& mem, u32 component_id);

    /**
     * Unregisters a component in this ECS
     *
     * Parameters
     * - mem The allocator to get memory from
     */
    template<typename T>
    void unregister_component(HgAllocator& mem) {
        static_assert(hg_is_memmove_safe<T>);
        unregister_component_untyped(mem, hg_component_id<T>);
    }

    /**
     * Checks whether a component is registered in this ECS
     *
     * Parameters
     * - component_id The component id to check
     *
     * Returns
     * - Whether the component id is registered
     */
    bool is_registered(u32 component_id) {
        hg_assert(component_id < systems.count);
        return systems[component_id].components.items != nullptr;
    }

    /**
     * Checks whether a component is registered in this ECS
     *
     * Parameters
     * - component_id The component id to check
     *
     * Returns
     * - Whether the component id is registered
     */
    template<typename T>
    bool is_registered() {
        static_assert(hg_is_memmove_safe<T>);
        return is_registered(hg_component_id<T>);
    }

    /**
     * Gets the number of components that exist under the component id
     *
     * Parameters
     * - component_id The component id to count
     *
     * Returns
     * - The number of components under the id
     */
    u32 component_count(u32 component_id) {
        hg_assert(is_registered(component_id));
        return (u32)systems[component_id].components.count;
    }

    /**
     * Gets the number of components that exist under the component id
     *
     * Returns
     * - The number of components under the id
     */
    template<typename T>
    u32 component_count() {
        static_assert(hg_is_memmove_safe<T>);
        return component_count(hg_component_id<T>);
    }

    /**
     * Finds the index/id of the system with the fewest elements
     *
     * Parameters
     * - ids The indices to check, must be registered
     *
     * Returns
     * - The index/id of the smallest in the array
     */
    u32 smallest_system_untyped(HgSpan<u32> ids);

    /**
     * Finds the index/id of the system with the fewest elements
     *
     * Returns
     * - The index/id of the smallest in the array
     */
    template<typename... Ts>
    u32 smallest_system() {
        u32 ids[sizeof...(Ts)];

        u32 index = 0;
        ((ids[index++] = hg_component_id<Ts>), ...);

        return smallest_system_untyped({ids, hg_countof(ids)});
    }

    /**
     * Creates a component for an entity
     *
     * Note, the entity must not have a component of this type already
     *
     * Parameters
     * - entity The entity to add to, must be alive
     * - component_id The id of the component, must be registered
     *
     * Returns
     * - A pointer to the created component
     */
    void* add(HgEntity entity, u32 component_id) {
        hg_assert(is_alive(entity));
        hg_assert(is_registered(component_id));
        hg_assert(!has(entity, component_id));

        systems[component_id].sparse[entity] = (u32)systems[component_id].dense.count;
        systems[component_id].dense.push(entity);
        return systems[component_id].components.push();
    }

    /**
     * Creates a component for an entity
     *
     * Note, the entity must not have a component of this type already
     *
     * Parameters
     * - entity The entity to add to, must be alive
     *
     * Returns
     * - A pointer to the created component
     */
    template<typename T>
    T& add(HgEntity entity) {
        static_assert(hg_is_memmove_safe<T>);
        return* (T* )add(entity, hg_component_id<T>);
    }

    /**
     * Destroys a component from an entity
     *
     * Note, this function will invalidate iterators
     *
     * Parameters
     * - entity The id of the entity, must be alive
     * - component_id The id of the component, must be registered
     */
    void remove(HgEntity entity, u32 component_id) {
        hg_assert(is_alive(entity));
        hg_assert(is_registered(component_id));
        hg_assert(has(entity, component_id));

        u32 index = systems[component_id].sparse[entity];
        systems[component_id].sparse[entity] = (u32)-1;

        systems[component_id].dense.swap_remove(index);
        systems[component_id].components.swap_remove(index);
    }

    /**
     * Destroys a component from an entity
     *
     * The entity must have an associated component in the system
     *
     * Note, this function will invalidate iterators
     *
     * Parameters
     * - entity The id of the entity, must be alive
     */
    template<typename T>
    void remove(HgEntity entity) {
        static_assert(hg_is_memmove_safe<T>);
        remove(entity, hg_component_id<T>);
    }

    /**
     * Swaps the component data from the given indices
     * 
     * Parameters
     * - lhs The first component to swap
     * - rhs The second component to swap
     * - component_id The component id, must be registered
     */
    void swap_idx(u32 lhs, u32 rhs, u32 component_id);

    /**
     * Swaps the component data from the given indices
     * 
     * Parameters
     * - lhs The first component to swap
     * - rhs The second component to swap
     */
    template<typename T>
    void swap_idx(u32 lhs, u32 rhs) {
        static_assert(hg_is_memmove_safe<T>);
        swap_idx(lhs, rhs, hg_component_id<T>);
    }

    /**
     * Swaps the component data from each entity
     * 
     * Parameters
     * - lhs The first entity to swap, must be alive
     * - rhs The second entity to swap, must be alive
     * - component_id The component id, must be registered
     */
    void swap(HgEntity lhs, HgEntity rhs, u32 component_id) {
        hg_assert(is_registered(component_id));
        hg_assert(is_alive(lhs));
        hg_assert(is_alive(rhs));
        hg_assert(has(lhs, component_id));
        hg_assert(has(rhs, component_id));

        swap_idx(
            systems[component_id].sparse[lhs],
            systems[component_id].sparse[rhs],
            component_id);
    }

    /**
     * Swaps the component data from each entity
     * 
     * Parameters
     * - lhs The first entity to swap, must be alive
     * - rhs The second entity to swap, must be alive
     */
    template<typename T>
    void swap(HgEntity lhs, HgEntity rhs) {
        static_assert(hg_is_memmove_safe<T>);
        swap(lhs, rhs, hg_component_id<T>);
    }

    /**
     * Swaps the locations of components with their entities
     *
     * Parameters
     * - lhs The first entity to swap
     * - rhs The second entity to swap
     * - component_id The component id, must be registered
     */
    void swap_location_idx(u32 lhs, u32 rhs, u32 component_id);

    /**
     * Swaps the locations of components with their entities
     *
     * Parameters
     * - lhs The first entity to swap
     * - rhs The second entity to swap
     */
    template<typename T>
    void swap_location_idx(u32 lhs, u32 rhs) {
        static_assert(hg_is_memmove_safe<T>);
        swap_location_idx(lhs, rhs, hg_component_id<T>);
    }

    /**
     * Swaps where the entities' component are located
     *
     * Parameters
     * - lhs The first entity to swap
     * - rhs The second entity to swap
     * - component_id The component id, must be registered
     */
    void swap_location(HgEntity lhs, HgEntity rhs, u32 component_id);

    /**
     * Swaps where the entities' component are located
     *
     * Parameters
     * - lhs The first entity to swap
     * - rhs The second entity to swap
     */
    template<typename T>
    void swap_location(HgEntity lhs, HgEntity rhs) {
        static_assert(hg_is_memmove_safe<T>);
        swap_location(lhs, rhs, hg_component_id<T>);
    }

    /**
     * Checks whether an entity has a component or not
     *
     * Parameters
     * - entity The id of the entity, must be alive
     * - component_id The id of the component, must be registered
     *
     * Returns
     * - Whether the entity has a component in the system
     */
    bool has(HgEntity entity, u32 component_id) {
        hg_assert(is_alive(entity));
        hg_assert(is_registered(component_id));
        return systems[component_id].sparse[entity] < systems[component_id].dense.count;
    }

    /**
     * Checks whether an entity has a component or not
     *
     * Parameters
     * - entity The id of the entity, must be alive
     *
     * Returns
     * - Whether the entity has a component in the system
     */
    template<typename T>
    bool has(HgEntity entity) {
        static_assert(hg_is_memmove_safe<T>);
        return has(entity, hg_component_id<T>);
    }

    /**
     * Checks whether an entity has all given components
     *
     * Parameters
     * - entity The id of the entity, must be alive
     *
     * Returns
     * - Whether the entity has all given components in the system
     */
    template<typename... Ts>
    bool has_all(HgEntity entity) {
        return (has<Ts>(entity) && ...);
    }

    /**
     * Checks whether an entity has any of given components
     *
     * Parameters
     * - entity The id of the entity, must be alive
     *
     * Returns
     * - Whether the entity has any of given components in the system
     */
    template<typename... Ts>
    bool has_any(HgEntity entity) {
        return (has<Ts>(entity) || ...);
    }

    /**
     * Gets a pointer to the entity's component
     *
     * Note, the entity must have a component in the system
     *
     * Parameters
     * - entity The id of the entity, must be alive
     * - component_id The id of the component, must be registered
     *
     * Returns
     * - The entity's component, will never be 0
     */
    void* get(HgEntity entity, u32 component_id) {
        hg_assert(is_alive(entity));
        hg_assert(is_registered(component_id));
        hg_assert(has(entity, component_id));
        return systems[component_id].components[systems[component_id].sparse[entity]];
    }

    /**
     * Gets a pointer to the entity's component
     *
     * Note, the entity must have a component in the system
     *
     * Parameters
     * - entity The id of the entity, must be alive
     *
     * Returns
     * - The entity's component, will never be 0
     */
    template<typename T>
    T& get(HgEntity entity) {
        static_assert(hg_is_memmove_safe<T>);
        return* ((T* )get(entity, hg_component_id<T>));
    }

    /**
     * Gets the entity id from it's component
     *
     * Parameters
     * - component The component to lookup, must be a valid component
     * - component_id The id of the component, must be registered
     *
     * Returns
     * - The components's entity, will never be 0
     */
    HgEntity get_entity(const void* component, u32 component_id) {
        hg_assert(component != nullptr);
        hg_assert(is_registered(component_id));

        usize index = ((uptr)component - (uptr)systems[component_id].components.items)
                    / systems[component_id].components.width;
        return systems[component_id].dense[index];
    }

    /**
     * Gets the entity id from it's component
     *
     * Parameters
     * - component The component to lookup, must be a valid component
     *
     * Returns
     * - The components's entity, will never be 0
     */
    template<typename T>
    HgEntity get_entity(const T& component) {
        static_assert(hg_is_memmove_safe<T>);
        hg_assert(is_registered(hg_component_id<T>));

        u32 index = (u32)(&component - (T* )systems[hg_component_id<T>].components.items);
        return systems[hg_component_id<T>].dense[index];
    }

    /**
     * A view into component for c++ range based for loops
     */
    template<typename T>
    struct ComponentView {
        static_assert(hg_is_memmove_safe<T>);

        struct Iter {
            HgEntity* entity;
            T* component;

            Iter& operator++() {
                ++entity;
                ++component;
                return* this;
            }

            bool operator!=(const Iter& other) {
                return entity != other.entity;
            }

            struct Ref {
                HgEntity& entity;
                T& component;
            };

            Ref operator*() {
                return {*entity,* component};
            }
        };

        HgEntity* entity_begin;
        HgEntity* entity_end;
        T* component_begin;

        constexpr Iter begin() {
            return {entity_begin, component_begin};
        }

        constexpr Iter end() {
            return {entity_end, nullptr};
        }
    };

    /**
     * Creates a view to iterate using c++ range based for loops
     *
     * Example:
     * for (auto [entity, component] : ecs.component_iter<Component>()) {
     *     foo(component);
     * }
     *
     * Returns
     * - A view with iterators into the component array
     */
    template<typename T>
    ComponentView<T> component_iter() {
        static_assert(hg_is_memmove_safe<T>);

        u32 id = hg_component_id<T>;
        hg_assert(is_registered(id));

        ComponentView<T> view;
        view.entity_begin = systems[id].dense.begin();
        view.entity_end = systems[id].dense.end();
        view.component_begin = (T* )systems[id].components.items;
        return view;
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
        static_assert(hg_is_memmove_safe<T>);
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity&, T&>);

        for (auto [e, c] : component_iter<T>()) {
            fn(e, c);
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
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity&, Ts&...>);

        u32 id = smallest_system<Ts...>();
        for (HgEntity e : systems[id].dense) {
            if (has_all<Ts...>(e))
                fn(e, get<Ts>(e)...);
        }
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
    void for_each(Fn fn) {
        static_assert(sizeof...(Ts) != 0);
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity&, Ts&...>);

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
    void for_each_par_single(u32 chunk_size, Fn& fn) {
        static_assert(hg_is_memmove_safe<T>);
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity&, T&>);

        struct Capture {
            Component& system;
            Fn& function;
        } capture {
            systems[hg_component_id<T>], fn,
        };

        auto fn_it = [](void* pcapture, usize begin, usize end) {
            auto& [system, function] = *(Capture*)pcapture;

            HgEntity* e_begin = system.dense.items + begin;
            HgEntity* e_end = system.dense.items + end;
            T* c_begin = (T* )system.components[begin];
            for (; e_begin != e_end; ++e_begin, ++c_begin) {
                function(*e_begin,* c_begin);
            }
        };

        hg_threads->for_par(capture.system.dense.count, chunk_size, &capture, fn_it);
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
    void for_each_par_multi(u32 chunk_size, Fn& fn) {
        static_assert(std::is_invocable_r_v<void, Fn, HgEntity&, Ts&...>);

        struct Capture {
            HgECS& ecs;
            u32 id;
            Fn& function;
        } capture {
            *this, smallest_system<Ts...>(), fn,
        };


        auto fn_it = [](void *pcapture, usize begin, usize end) {
            auto& [ecs, id, function] = *(Capture*)pcapture;

            HgEntity* e_begin = ecs.systems[id].dense.items + begin;
            HgEntity* e_end = ecs.systems[id].dense.items + end;
            for (; e_begin != e_end; ++e_begin) {
                if (ecs.template has_all<Ts...>(*e_begin))
                    function(*e_begin, ecs.template get<Ts>(*e_begin)...);
            }
        };

        hg_threads->for_par(component_count(capture.id), chunk_size, &capture, fn_it);
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
    void for_each_par(u32 chunk_size, Fn fn) {
        static_assert(sizeof...(Ts) != 0);

        if constexpr (sizeof...(Ts) == 1) {
            for_each_par_single<Ts...>(chunk_size, fn);
        } else {
            for_each_par_multi<Ts...>(chunk_size, fn);
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
        bool (*compare)(void*, HgEntity lhs, HgEntity rhs));

    /**
     * Sorts components
     *
     * Parameters
     * - compare The comparison function
     */
    template<typename T>
    void sort(void* data, bool (*compare)(void*, HgEntity lhs, HgEntity rhs)) {
        static_assert(hg_is_memmove_safe<T>);
        sort_untyped(0, component_count<T>(), hg_component_id<T>, data, compare);
    }
};

/**
 * A global entity component system
 */
inline HgECS* hg_ecs;

/**
 * A resource manager to load, store, and unload resources
 */
struct HgResourceManager {
    struct Request {
        HgFence* fence;
        void* data;
        HgAllocator* mem;
        void* resource;
        HgStringView path;
        void (*fn)(void*, HgAllocator*, void*, HgStringView);
    };

    /**
     * The mutex protecting the request_queue
     */
    std::mutex request_queue_mutex;
    /**
     * The requests to be processed
     */
    HgQueue<Request> request_queue;
    /**
     * The thread which handles requests
     */
    HgThread request_thread;

    /**
     * Create a new resource manager
     *
     * Parameters
     * - mem The allocator to use
     * - max_requests The max requests that can be concurrently processed
     *
     * Returns
     * - A pointer to the created resource manager
     */
    static HgResourceManager* create(HgAllocator& mem, usize max_requests);

    /**
     * Destroys a resource manager
     *
     * Parameters
     * - mem The allocator to use
     * - rm The resource manager to destroy
     */
    static void destroy(HgAllocator& mem, HgResourceManager* rm);

    /**
     * Request to operate on a resource
     *
     * Parameters
     * - request The request to make
     */
    void request(const Request& request);
};

/**
 * A global resource manager
 */
inline HgResourceManager* hg_resources;

/**
 * A loaded binary file
 */
struct HgFileBinary {
    void* data;
    usize size;

    static constexpr usize alignment = alignof(std::max_align_t);
};

/**
 * Load a binary file from disc
 *
 * Parameters
 * - fence The fence to signal on completion
 * - mem The allocator to use
 * - file The file to load into, must be stable
 * - path The file path to the image
 */
void hg_load_file_binary(HgFence* fence, HgAllocator& mem, HgFileBinary& file, HgStringView path);

/**
 * Unload a binary file resource
 *
 * Parameters
 * - fence The fence to signal on completion
 * - mem The allocator to use
 * - file The file to unload, must be stable
 */
void hg_unload_file_binary(HgFence* fence, HgAllocator& mem, HgFileBinary& file);

/**
 * Store a binary file to disc
 *
 * Parameters
 * - fence The fence to signal on completion
 * - file The file to store, must be stable
 * - path The file path
 */
void hg_store_file_binary(HgFence* fence, HgFileBinary& file, HgStringView path);

// text files : TODO
// json files : TODO
// image files : TODO
// 3d model files : TODO
// audio files : TODO

/**
 * A loaded image file
 */
struct HgImage {
    /**
     * The pixel data
     */
    void* pixels;
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
 * Load an image from disc
 *
 * Parameters
 * - fence The fence to signal on completion
 * - mem The allocator to use
 * - image The image to load into, must be stable
 * - path The file path to the image
 */
void hg_load_image(HgFence* fence, HgAllocator& mem, HgImage& image, HgStringView path);

/**
 * Unload an image resource
 *
 * Parameters
 * - fence The fence to signal on completion
 * - mem The allocator to use
 * - image The image to unload, must be stable
 */
void hg_unload_image(HgFence* fence, HgAllocator& mem, HgImage& image);

/**
 * Store an image to disc
 *
 * Parameters
 * - fence The fence to signal on completion
 * - image The image to store
 * - path The file path to the image
 */
void hg_store_image(HgFence* fence, HgImage& image, HgStringView path);

/**
 * A high precision clock for timers and game deltas
 */
struct HgClock {
    std::chrono::time_point<std::chrono::high_resolution_clock> time = std::chrono::high_resolution_clock::now();

    /**
     * Resets the clock and returns the delta since the last tick in seconds
     *
     * Parameters
     * - clock The clock to tick, must not be nullptr
     *
     * Returns
     * - Seconds since last tick
     */
    f64 tick();
};

/**
 * Initializes the graphics subsystem
 */
void hg_graphics_init();

/**
 * Deinitializes the graphics subsystem
 */
void hg_graphics_deinit();

inline VkInstance hg_vk_instance = nullptr;
inline VkPhysicalDevice hg_vk_physical_device = nullptr;
inline VkDevice hg_vk_device = nullptr;
inline VmaAllocator hg_vk_vma = nullptr;

inline VkQueue hg_vk_queue = nullptr;
inline u32 hg_vk_queue_family = (u32)-1;

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
 * Returns
 * - The created VkInstance, will never be nullptr
 */
VkInstance hg_vk_create_instance();

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
 *
 * Returns
 * - The queue family if found
 * - nullopt if not found
 */
HgOption<u32> hg_vk_find_queue_family(VkPhysicalDevice gpu, VkQueueFlags queue_flags);

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
 * Creates a general purpose Vulkan memory allocator
 *
 * Returns
 * - The created Vulkan memory allocator, will never be nullptr;
 */
VmaAllocator hg_vk_create_vma_allocator();

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
 *
 * Returns
 * - The created Vulkan swapchain
 */
HgSwapchainData hg_vk_create_swapchain(
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
    VkCommandBuffer* cmds;
    VkFence* frame_finished;
    VkSemaphore* image_available;
    VkSemaphore* ready_to_present;
    u32 frame_count;
    u32 current_frame;
    u32 current_image;

    /**
     * Creates a swapchain command buffer system
     *
     * Parameters
     * - swapchain The Vulkan swapchain to create frames for, must not be nullptr
     * - cmd_pool The Vulkan command pool to allocate cmds from, must not be nullptr
     *
     * Returns
     * - The created swaphchain command buffer system
     */
    static HgSwapchainCommands create(VkSwapchainKHR swapchain, VkCommandPool cmd_pool);

    /**
     * Destroys a swaphchain command buffer system
     */
    void destroy();

    /**
     * Acquires the next swapchain image and begins its command buffer
     *
     * Returns
     * - The command buffer to record this frame
     * - nullptr if the swapchain is out of date
     */
    VkCommandBuffer acquire_and_record();

    /**
     * Finishes recording the command buffer and presents the swapchain image
     *
     * Parameters
     * - queue The Vulkan queue, must not be nullptr
     */
    void end_and_present(VkQueue queue);
};

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
 * - transfer_queue The Vulkan queue to transfer on, must not be nullptr
 * - cmd_pool The command pool for the queue, must not be nullptr
 * - dst The buffer to write to, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - src The data to write, must not be nullptr
 */
void hg_vk_buffer_staging_write(
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    VkBuffer dst,
    usize offset,
    HgSpan<const void> src);

/**
 * Reads from a Vulkan device local buffer through a staging buffer
 *
 * Parameters
 * - transfer_queue The Vulkan queue to transfer on, must not be nullptr
 * - cmd_pool The command pool for the queue, must not be nullptr
 * - dst The location to write to, must not be nullptr
 * - src The buffer to read from, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 */
void hg_vk_buffer_staging_read(
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
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
 * - transfer_queue The Vulkan queue to transfer on, must not be nullptr
 * - cmd_pool The command pool for the queue, must not be nullptr
 * - config The configuration for the write
 */
void hg_vk_image_staging_write(
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    const HgVkImageStagingWriteConfig& config);

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
 * - transfer_queue The Vulkan queue to transfer on, must not be nullptr
 * - cmd_pool The command pool for the queue, must not be nullptr
 * - config The configuration for the read, must not be nullptr
 */
void hg_vk_image_staging_read(
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    const HgVkImageStagingReadConfig& config);

// cubemap read/write : TODO

/**
 * Generates mipmaps from the base level
 *
 * Parameters
 * - transfer_queue The Vulkan queue to transfer on, must not be nullptr
 * - cmd_pool The command pool for the queue, must not be nullptr
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
    VkQueue transfer_queue,
    VkCommandPool cmd_pool,
    VkImage image,
    VkImageAspectFlags aspect_mask,
    VkImageLayout old_layout,
    VkImageLayout new_layout,
    u32 width,
    u32 height,
    u32 depth,
    u32 mip_count);

struct HgTexture {
    VmaAllocation allocation;
    VkImage image;
    VkImageView view;
    VkSampler sampler;
};

void hg_load_texture(VkCommandPool cmd_pool, HgTexture& texture, VkFilter filter, HgImage& src);

void hg_unload_texture(HgTexture& texture);

struct HgTransform {
    HgVec3 position = {0.0f, 0.0f, 0.0f};
    HgVec3 scale = {1.0f, 1.0f, 1.0f};
    HgQuat rotation = {1.0f, 0.0f, 0.0f, 0.0f};
};

struct HgSprite {
    HgTexture* texture;
    HgVec2 uv_pos;
    HgVec2 uv_size;
};

struct HgPipeline2D {

    struct VPUniform {
        HgMat4 proj;
        HgMat4 view;
    };

    struct Push {
        HgMat4 model;
        HgVec2 uv_pos;
        HgVec2 uv_size;
    };

    VkDescriptorSetLayout vp_layout;
    VkDescriptorSetLayout texture_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline;

    VkDescriptorPool descriptor_pool;
    VkDescriptorSet vp_set;

    VkBuffer vp_buffer;
    VmaAllocation vp_buffer_allocation;

    HgHashMap<HgTexture*, VkDescriptorSet> texture_sets;

    static HgOption<HgPipeline2D> create(
        HgAllocator& mem,
        usize max_textures,
        VkFormat color_format,
        VkFormat depth_format);

    void destroy(HgAllocator& mem);

    void add_texture(HgTexture* texture);
    void remove_texture(HgTexture* texture);

    void update_projection(HgMat4& projection);
    void update_view(HgMat4& view);

    void add_sprite(HgEntity entity, HgTexture& texture, HgVec2 uv_pos, HgVec2 uv_size);
    void remove_sprite(HgEntity entity);

    void draw(VkCommandBuffer cmd);
};

/**
 * Initializes global resources for windowing
 */
void hg_platform_init();

/**
 * Deinitializes global resources for windowing
 */
void hg_platform_deinit();

// audio system : TODO

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
    Internals* internals;

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
     *
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
    void set_cursor_image(u32* data, u32 width, u32 height);

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
    void get_size(u32* width, u32* height);

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
     *
     * Returns
     * - whether the key is being held down
     */
    bool is_key_down(HgKey key);

    /**
     * Checks if a key was pressed this frame
     *
     * Parameters
     * - key The key to check
     *
     * Returns
     * - whether the key was pressed this frame
     */
    bool was_key_pressed(HgKey key);

    /**
     * Checks if a key was released this frame
     *
     * Parameters
     * - key The key to check
     *
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
 * - window The window to create a surface for, must exist
 *
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
 * - windows All open windows, must not be nullptr
 */
void hg_process_window_events(HgSpan<const HgWindow> windows);

#endif // HURDYGURDY_HPP

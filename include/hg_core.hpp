/*
 * =============================================================================
 *
 * Copyright (c) 2025-2026 Cooper Herlihy
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

#ifndef HG_CORE_HPP
#define HG_CORE_HPP

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include <type_traits>

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
 * Formats and logs a message to stderr for debugging purposes
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hgLog(...) do { (void)fprintf(stderr, "HurdyGurdy Log: " __VA_ARGS__); } while(0)

/**
 * Formats and logs a message to stderr as a warning
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hgWarn(...) do { (void)fprintf(stderr, "HurdyGurdy Warning: " __VA_ARGS__); } while(0)

/**
 * Formats and logs a message to stderr and aborts the program
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hgPanic(...) do { (void)fprintf(stderr, "HurdyGurdy Panic: " __VA_ARGS__); abort(); } while(0)

#else

/**
 * Formats and logs a message to stderr for debugging puposes
 */
#define hgWarn(...) do {} while(0)

/**
 * Formats and logs a message to stderr as a warning
 */
#define hgWarn(...) do {} while(0)

/**
 * Formats and logs a message to stderr and aborts the program
 */
#define hgPanic(...) do { abort(); } while(0)

#endif

#ifdef HG_ASSERTIONS

/**
 * Asserts condition, or aborts the program
 */
#define hgAssert(cond) do { \
    if (!(cond)) \
    { \
        hgPanic("Assertion failed in " __FILE__ ":%d %s() " #cond "\n", __LINE__, __func__); \
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
 * A block of binary data
 */
struct HgBinary {
    /**
     * The data
     */
    const void* data;
    /**
     * The size of data in bytes
     */
    u64 size;
};

/**
 * A view into a string
 */
struct HgString {
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
    HgString() = default;

    /**
     * Create a string view from a pointer and length
     */
    constexpr HgString(const char* charsVal, u64 lengthVal)
        : chars{charsVal}, length{lengthVal} {}

    /**
     * Create a string view from begin and end pointers
     */
    constexpr HgString(const char* charsBegin, const char* charsEnd)
        : chars{charsBegin}, length{(uptr)(charsEnd - charsBegin)}
    {
        hgAssert(charsBegin <= charsEnd);
    }

    /**
     * Implicit constexpr conversion from c string
     *
     * Potentially dangerous, c string should be at most 4096 chars
     */
    constexpr HgString(const char* cStr) : chars{cStr}, length{0}
    {
        if (cStr != nullptr)
        {
            while (cStr[length] != '\0')
            {
                ++length;
                hgAssert(length <= 4096);
            }
        }
    }

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr const char& operator[](u64 idx) const
    {
        hgAssert(chars != nullptr);
        hgAssert(idx < length);
        return chars[idx];
    }
};

/**
 * Get this thread's most recent error message
 */
HgString hgErrorGet();

/**
 * Set this thread's current error message
 */
void hgErrorGet(HgString error);

/**
 * The Hurdy Gurdy subsystems
 */
enum HgSubsystem : u32 {
    HgSubsystem_memory = 0x1,
    HgSubsystem_concurrency = 0x2,
    HgSubsystem_gpu = 0x4,
    HgSubsystem_assets = 0x8,
    HgSubsystem_windowing = 0x10,
    HgSubsystem_audio = 0x20,
    HgSubsystem_all = (u32)-1,
};
typedef u32 HgSubsystemFlags;

/**
 * Initialize the Hurdy Gurdy library
 *
 * Parameters
 * - init Which subsystems to initialize, all are recommended
 */
void hgInit(HgSubsystemFlags init = HgSubsystem_all);

/**
 * Shut down the Hurdy Gurdy library
 */
void hgDeinit();

/**
 * Run Hurdy Gurdy tests, asserting success
 */
void hgTest();

#endif // HG_CORE_HPP

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

#pragma once

#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>

#include <algorithm>
#include <utility>
#include <type_traits>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace hg {

// ============================================================================
// Configuration Macros
// ============================================================================

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

#ifndef HG_LOGGING
#define HG_NO_LOGGING 1
#endif

#ifndef HG_ASSERTIONS
#define HG_NO_ASSERTIONS 1
#endif

#ifndef HG_VK_DEBUG_MESSENGER
#define HG_NO_VK_DEBUG_MESSENGER 1
#endif

#endif

// ============================================================================
// Core Types
// ============================================================================

/**
 * An 8 bit, 1 byte unsigned integer
 */
using u8 = uint8_t;
/**
 * A 16 bit, 2 byte unsigned integer
 */
using u16 = uint16_t;
/**
 * A 32 bit, 4 byte unsigned integer
 */
using u32 = uint32_t;
/**
 * A 64 bit, 8 byte unsigned integer
 */
using u64 = uint64_t;

/**
 * An 8 bit, 1 byte signed integer
 */
using i8 = int8_t;
/**
 * A 16 bit, 2 byte signed integer
 */
using i16 = int16_t;
/**
 * A 32 bit, 4 byte signed integer
 */
using i32 = int32_t;
/**
 * A 64 bit, 8 byte signed integer
 */
using i64 = int64_t;

/**
 * An unsigned integer representing a pointer
 */
using uptr = uintptr_t;
/**
 * A signed integer representing a pointer
 */
using iptr = intptr_t;

/**
 * A 32 bit, 4 byte floating point value
 */
using f32 = float;
/**
 * A 64 bit, 8 byte floating point value
 */
using f64 = double;

/**
 * A block of binary data
 */
struct BinaryView;

/**
 * A view into a string
 */
struct StringView;

/**
 * A view into memory of a type
 */
template<typename T>
struct Span;

/**
 * An object which may or may not exist
 */
template<typename T>
struct Maybe;

// ============================================================================
// Error Handling
// ============================================================================

/**
 * Get this thread's most recent error message
 */
StringView getError();

/**
 * Set this thread's current error message
 *
 * Note, truncates at 4096 bytes
 */
void setError(StringView error);

/**
 * Format and set this thread's error message
 */
template<typename... Ts>
void formatError(StringView errorFmt, Ts... args);

/**
 * Log this thread's current error message to stderr
 */
void logError();

// ============================================================================
// Utility Macros
// ============================================================================

#define HG_MACRO_CONCAT_INTERNAL(x, y) x##y

/**
 * Concatenate two macros
 */
#define HG_MACRO_CONCAT(x, y) HG_MACRO_CONCAT_INTERNAL(x, y)

/**
 * A template to defer code execution until end of scope
 */
template<typename F>
struct Defer {
    /**
     * The function to execute
     */
    F fn;

    /**
     * Prepare the function to defer
     */
    Defer(F fnVal) : fn(fnVal) {}

    /**
     * Execute the function
     */
    ~Defer()
    {
        fn();
    }
};

/**
 * Defers a piece of code until the end of the scope
 */
#define HG_DEFER(...) [[maybe_unused]] hg::Defer HG_MACRO_CONCAT(defer_, __LINE__){[&]{ __VA_ARGS__ ;}};

#ifdef HG_LOGGING

#define HG_LOG(...) do { std::fprintf(stderr, "HurdyGurdy Log: " __VA_ARGS__); } while(0)
#define HG_WARN(...) do { std::fprintf(stderr, "HurdyGurdy Warn: " __VA_ARGS__); } while(0)
#define HG_PANIC(...) do { logError(); std::fprintf(stderr, "HurdyGurdy Panic: " __VA_ARGS__); std::abort(); \
} while(0)

#else

#define HG_LOG(...) do {} while(0)
#define HG_WARN(...) do {} while(0)
#define HG_PANIC(...) do { abort(); } while(0)

#endif

#ifdef HG_ASSERTIONS

/**
 * Asserts condition, or aborts the program
 */
#define HG_ASSERT(cond) do { \
    if (!(cond)) \
        HG_PANIC("Assertion failed in " __FILE__ ":%d %s() \"" #cond "\"\n", __LINE__, __func__); \
} while(0)

#else

/**
 * Asserts condition, or aborts the program
 */
#define HG_ASSERT(cond) do {} while(0)

#endif

// ============================================================================
// Initialization
// ============================================================================

/**
 * The Hurdy Gurdy subsystems
 */
enum Subsystem : u32 {
    Subsystem_memory = 0x1,
    Subsystem_concurrency = 0x2,
    Subsystem_gpu = 0x4,
    Subsystem_assets = 0x8,
    Subsystem_windowing = 0x10,
    Subsystem_audio = 0x20,
    Subsystem_all = static_cast<u32>(-1),
};
using SubsystemFlags = u32;

/**
 * A scope guard for library initialization
 */
struct HurdyGurdy {
    /**
     * Whether this scope guard needs to deinitialize the library
     */
    bool alive = true;

    /**
     * Default construct
     */
    HurdyGurdy() noexcept = default;

    /**
     * Deinitialize the library
     */
    ~HurdyGurdy() noexcept;

    /**
     * Move the library scope guard
     */
    HurdyGurdy(HurdyGurdy&& other) noexcept
        : alive{other.alive}
    {
        if (this != &other)
            other.alive = false;
    }

    /**
     * Move the library scope guard
     */
    HurdyGurdy& operator=(HurdyGurdy&& other) noexcept
    {
        HG_ASSERT(!alive);
        if (this != &other)
        {
            alive = other.alive;
            other.alive = false;
        }
        return *this;
    }

    HurdyGurdy(const HurdyGurdy&) = delete;
    HurdyGurdy& operator=(const HurdyGurdy&) = delete;
};

/**
 * Initialize the Hurdy Gurdy library
 *
 * Parameters
 * - init Which subsystems to initialize, all are recommended
 *
 * Returns
 * - A HurdyGurdy scope guard, or nothing on failure
 */
Maybe<HurdyGurdy> init(SubsystemFlags init = Subsystem_all);

// ============================================================================
// Core Types (Implementations)
// ============================================================================

/**
 * A non-owning view into binary data
 */
struct BinaryView {
    /**
     * The data
     */
    const void* data = nullptr;
    /**
     * The size of data in bytes
     */
    u64 size = 0;
};

/**
 * Read data at index into a buffer
 *
 * Parameters
 * - idx The index into the file in bytes to read from
 * - dst A pointer to store the read data
 * - size The size in bytes to read
 */
void binaryRead(BinaryView bin, u64 idx, void* dst, u64 len);

/**
 * Read data of arbitrary type from the file
 *
 * Parameters
 * - idx The index into the file in bytes to read from
 */
template<typename T>
T binaryRead(BinaryView bin, u64 idx)
{
    T ret;
    binaryRead(bin, idx, &ret, sizeof(T));
    return ret;
}

/**
 * A non-owning view into a string
 */
struct StringView {
    /**
     * The characters
     */
    const char* chars = nullptr;
    /**
     * The number of characters;
     */
    u64 length = 0;

    /**
     * Construct uninitialized
     */
    StringView() = default;

    /**
     * Create a string view from a pointer and length
     */
    constexpr StringView(const char* charsVal, u64 lengthVal)
        : chars{charsVal}, length{lengthVal} {}

    /**
     * Create a string view from begin and end pointers
     */
    constexpr StringView(const char* charsBegin, const char* charsEnd)
        : chars{charsBegin}, length{static_cast<uptr>(charsEnd - charsBegin)}
    {
        HG_ASSERT(charsBegin <= charsEnd);
    }

    /**
     * Implicit constexpr conversion from c string
     *
     * Potentially dangerous, c string should be at most 4096 chars
     */
    constexpr StringView(const char* cStr) : chars{cStr}, length{0}
    {
        if (cStr != nullptr)
        {
            while (cStr[length] != '\0')
            {
                ++length;
                if (length > 4096)
                    HG_PANIC("C string greater than 4096 cannot be implicitly converted");
            }
        }
    }

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr const char& operator[](u64 idx) const
    {
        HG_ASSERT(chars != nullptr);
        HG_ASSERT(idx < length);
        return chars[idx];
    }

    /**
     * Use range for
     */
    const char* begin() const
    {
        return chars;
    }

    /**
     * Use range for
     */
    const char* end() const
    {
        return chars + length;
    }
};

/**
 * A view into memory of a type
 */
template<typename T>
struct Span {
    /**
     * The values viewed
     */
    T* values;
    /**
     * The number of values
     */
    u64 count;

    /**
     * Construct from pointer and count
     */
    Span(T* valuesVal, u64 countVal)
        : values{valuesVal}, count{countVal}
    {}

    /**
     * Construct from begin and end
     */
    Span(T* begin, T* end)
        : values{begin}, count{static_cast<uptr>(begin - end)}
    {}

    /**
     * Access by index
     */
    T& operator[](u32 idx) const
    {
        HG_ASSERT(idx < count);
        return values[idx];
    }

    /**
     * Use range for
     */
    T* begin()
    {
        return values;
    }

    /**
     * Use range for
     */
    T* end()
    {
        return values + count;
    }

    /**
     * Use range for
     */
    const T* begin() const
    {
        return values;
    }

    /**
     * Use range for
     */
    const T* end() const
    {
        return values + count;
    }
};

/**
 * An object which may or may not exist
 */
template<typename T>
struct Maybe
{
    /**
     * Whether the object exists
     */
    bool has = false;
    union {
        /**
         * The object, which may or may not exist
         */
        T val;
    };

    /**
     * Construct empty
     */
    Maybe() noexcept {};

    /**
     * Destroy the value, if it exists
     */
    ~Maybe() noexcept
    {
        if (has)
            val.~T();
    }

    /**
     * Copy construct
     */
    Maybe(const Maybe& other)
    {
        if (other.has)
            new (&val) T{other.val};
        has = other.has;
    }

    /**
     * Copy assign
     */
    Maybe& operator=(const Maybe& other)
    {
        if (this != &other)
        {
            if (has)
                val.~T();

            if (other.has)
                new (&val) T{other.val};
            has = other.has;
        }
        return *this;
    }

    /**
     * Move construct
     */
    Maybe(Maybe&& other) noexcept
    {
        if (other.has)
            new (&val) T{std::move(other.val)};
        has = other.has;

        if (other.has)
            other.val.~T();
        other.has = false;
    }

    /**
     * Move assign
     */
    Maybe& operator=(Maybe&& other) noexcept
    {
        if (this != &other)
        {
            if (has)
                val.~T();

            if (other.has)
                new (&val) T{std::move(other.val)};
            has = other.has;

            if (other.has)
                other.val.~T();
            other.has = false;
        }
        return *this;
    }

    /**
     * Return the value, or a default value if it does not exist
     */
    T orElse(T defaultVal)
    {
        if (has)
        {
            T tmp = std::move(val);
            val.~T();
            has = false;
            return tmp;
        }
        else
        {
            return std::move(defaultVal);
        }
    }

    /**
     * Expect there to be a value, or panic
     */
    T expect(StringView errMsg)
    {
        if (has)
        {
            T tmp = std::move(val);
            val.~T();
            has = false;
            return tmp;
        }
        else
        {
            HG_PANIC("%.*s", (int)errMsg.length, errMsg.chars);
        }
    }
};

/**
 * Create a filled Maybe
 */
template<typename T, typename... Args>
Maybe<T> some(Args&&... args)
{
    Maybe<T> maybe;
    new (&maybe.val) T{std::forward<Args>(args)...};
    maybe.has = true;
    return maybe;
}

/**
 * Create an empty Maybe
 */
template<typename T>
Maybe<T> none()
{
    Maybe<T> maybe;
    maybe.has = false;
    return maybe;
}

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Returns whether a value is a power of 2
 */
constexpr bool isPowerOf2(u64 val)
{
    return val > 0 && (val & (val - 1)) == 0;
}

/**
 * Aligns a pointer to an alignment
 *
 * Parameters
 * - val The value to align
 * - align The alignment, must be a power of two
 *
 * Returns
 * - The aligned value
 */
constexpr uptr align(uptr val, uptr align)
{
    HG_ASSERT(isPowerOf2(align));
    return (val + align - 1) & ~(align - 1);
}

/**
 * Reverse the endianness of a 16 bit value
 */
constexpr u16 endianReverse16(u16 val)
{
    return static_cast<u16>(val >> 8) | static_cast<u16>(val << 8);
}

/**
 * Reverse the endianness of a 32 bit value
 */
constexpr u32 endianReverse32(u32 val)
{
    return (val >> 24) | ((val >> 8) & 0xff00) | ((val & 0xff00) << 8) | (val << 24);
}

/**
 * Reverse the endianness of a 64 bit value
 */
constexpr u64 endianReverse64(u64 val)
{
    u64 swapped = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    swapped = ((swapped << 16) & 0xFFFF0000FFFF0000ULL) | ((swapped >> 16) & 0x0000FFFF0000FFFFULL);
    return (swapped << 32) | (swapped >> 32);
}

/**
 * Clear a section of memory
 */
void memClear(void* dst, u64 size, u8 val = 0);

/**
 * Copy memory from src to dst, may not overlap
 */
void memCopy(void* __restrict dst, const void* __restrict src, u64 size);

/**
 * Copy memory from src to dst, may overlap
 */
void memMove(void* dst, const void* src, u64 size);

/**
 * Check if two regions of memory are identical
 */
bool memEqual(const void* dst, const void* src, u64 size);

// ============================================================================
// Memory
// ============================================================================

/**
 * Allocates memory from a general purpose allocator
 *
 * Parameters
 * - size The size in bytes to allocate
 * - alignment The required alignment of the allocation in bytes
 *
 * Returns
 * - The allocation, never nullptr
 */
void* heapAlloc(u64 size, u64 alignment);

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
T* heapAlloc(u64 count)
{
    return static_cast<T*>(heapAlloc(count * sizeof(T), alignof(T)));
}

/**
 * Reallocates memory from a general purpose allocator
 *
 * Parameters
 * - allocation The allocation to grow
 * - oldSize The old size in bytes allocation
 * - newSize The new size in bytes to allocate
 * - alignment The required alignment of the allocation in bytes
 *
 * Returns
 * - The allocation, never nullptr
 */
void* heapRealloc(void* allocation, u64 oldSize, u64 newSize, u64 alignment);

/**
 * A convenience to reallocate an array of a type
 *
 * Note, objects are not initialized
 *
 * Parameters
 * - allocation The allocation to reallocate
 * - oldCount The old number of T allocated
 * - newCount The new number of T to allocate
 *
 * Returns
 * - The reallocated array, never nullptr
 */
template<typename T>
T* heapRealloc(T* allocation, u64 oldCount, u64 newCount)
{
    return static_cast<T*>(heapRealloc(allocation, oldCount * sizeof(T), newCount * sizeof(T), alignof(T)));
}

/**
 * A convenience to free an allocation from a general purpose allocator
 *
 * Parameters
 * - allocation The allocation to free
 * - count The number of T allocated
 */
template<typename T>
void heapFree(T* allocation, u64 count)
{
    heapFree(static_cast<void*>(allocation), count * sizeof(T));
}

/**
 * Free an allocation from a general purpose allocator
 *
 * Parameters
 * - allocation The allocation to free
 * - size The size of the allocation in bytes
 * - alignment The alignment of the allocation in bytes
 */
template<>
void heapFree(void* allocation, u64 size);

/**
 * An arena allocator
 */
struct Arena {
    /**
     * A pointer to the memory being allocated
     */
    void* memory = nullptr;
    /**
     * The capacity of the memory being allocated
     */
    u64 capacity = 0;
    /**
     * The next allocation to be given out
     */
    u64 head = 0;
};

/**
 * Create a guard which restores an arena's head at the end of the scope
 */
#define HG_ARENA_SCOPE(arena) \
    [[maybe_unused]] u64 arenaScopeHead = arena->head; \
    HG_DEFER(arena->head = arenaScopeHead);

/**
 * Allocates memory from an arena
 *
 * Parameters
 * - arena The arena to allocate from
 * - size The size in bytes to allocate
 * - alignment The required alignment of the allocation in bytes
 *
 * Returns
 * - The allocation, or nullptr if out of memory
 */
void* arenaAlloc(Arena* arena, u64 size, u64 alignment);

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
 * - The allocated array, or nullptr if out of memory
 */
template<typename T>
T* arenaAlloc(Arena* arena, u64 count)
{
    return static_cast<T*>(arenaAlloc(arena, count * sizeof(T), alignof(T)));
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
 * - The allocation, never or if out of memory
 */
void* arenaRealloc(Arena* arena, void* allocation, u64 oldSize, u64 newSize, u64 alignment);

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
 * - The reallocated array, or nullptr if out of memory
 */
template<typename T>
T* arenaRealloc(Arena* arena, T* allocation, u64 oldCount, u64 newCount)
{
    return static_cast<T*>(arenaRealloc(arena, allocation, oldCount * sizeof(T), newCount * sizeof(T), alignof(T)));
}

/**
 * Returns whether the allocation can be extended, or if it must be moved
 */
bool arenaCanExtend(Arena* arena, void* allocation, u64 size, u64 align);

/**
 * Returns whether the allocation can be extended, or if it must be moved
 */
template<typename T>
bool arenaCanExtend(Arena* arena, T* allocation, u64 count)
{
    return arenaCanExtend(arena, static_cast<void*>(allocation), count * sizeof(T), alignof(T));
}

/**
 * Initializes scratch arenas on this thread
 *
 * Parameters
 * - count The number of arenas to allocate
 * - size The size of each arena in bytes
 */
void initScratch(u32 count, u64 size);

/**
 * Deinitializes scratch arenas
 */
void deinitScratch();

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
Arena* getScratch(Arena const* const* conflicts = nullptr, u32 count = 0);

// ============================================================================
// Concurrency
// ============================================================================

/**
 * Initialize synchronization and threads
 */
void initConcurrency();

/**
 * Deinitialize synchronization and threads
 */
void deinitConcurrency();

/**
 * A spinlock mutex for basic thread synchronization
 */
struct Spinlock;

/**
 * Create a new mutex
 */
Spinlock* mutexCreate();

/**
 * Destroy a mutex
 */
void mutexDestroy(Spinlock* mtx);

/**
 * Wait until the mutex is acquired
 */
void mutexAcquire(Spinlock* mtx);

/**
 * Try to acquire the mutex
 *
 * Returns
 * - true if acquisition succeeded
 * - false if the mutex was already in use
 */
bool mutexTryAcquire(Spinlock* mtx);

/**
 * Release the mutex lock
 */
void mutexRelease(Spinlock* mtx);

/**
 * A spinlock fence for basic thread synchronization
 */
struct Fence;

/**
 * Create a new fence
 */
Fence* fenceCreate();

/**
 * Destroy a fence
 */
void fenceDestroy(Fence* fence);

/**
 * Add more events for the fence to wait on
 *
 * Parameters
 * - fence The fence to attach to
 * - count The number of added events
 */
void fenceAttach(Fence* fence, u32 count = 1);

/**
 * Signal that events have completed
 *
 * Parameters
 * - fence The fence to signal
 * - count The number of signaled events
 */
void fenceSignal(Fence* fence, u32 count = 1);

/**
 * Returns whether all work has been completed
 */
bool fenceIsComplete(Fence* fence);

/**
 * Spin waits for all work submissions to be completed
 *
 * Parameters
 * - fence The fence to wait on
 * - timeout The time in seconds to wait before timing out
 *
 * Returns
 * - true if the fence was completed, false if the timeout was triggered
 */
bool fenceWait(Fence* fence, f64 timeout);

/**
 * Spin waits for all work submissions to be completed
 */
void fenceWaitIndefinite(Fence* fence);

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
bool helpThreads(Fence* fence, f64 timeout);

/**
 * Pushes work to the thread pool queue to be executed
 *
 * Parameters
 * - fence The fences to signal upon completion
 * - data The data passed to the function
 * - work The function to be executed
 */
void callPar(Fence* fence, void* data, void (*fn)(void* data));

/**
 * Iterates in parallel over a function n times using the thread pool
 *
 * Note, uses a fence internally to wait for all work to complete
 *
 * Parameters
 * - begin The first index to iterate from
 * - end The end index to iterate to
 * - data The data pointer passed to fn
 * - fn The function to use to iterate, takes the index
 */
void forPar(u64 begin, u64 end, void* data, void (*fn)(void* data, u64 idx));

/**
 * Iterates in parallel over a function n times using the thread pool
 *
 * Note, uses a fence internally to wait for all work to complete
 *
 * Parameters
 * - begin The first index to iterate from
 * - end The end index to iterate to
 * - fn The function to use to iterate, takes the index
 */
template<typename F>
void forPar(u64 begin, u64 end, F fn);

// ============================================================================
// GPU
// ============================================================================

// ----------------------------------------------------------------------------
// GPU Init
// ----------------------------------------------------------------------------

/**
 * Initializes the graphics subsystem, loading all global Vulkan resources
 *
 * Returns
 * - Whether init succeeded
 */
bool initGpu();

/**
 * Deinitializes the graphics subsystem, unloading all global Vulkan resources
 */
void deinitGpu();

/**
 * Wait for the GPU to finish work
 */
void gpuWaitIdle();

// ----------------------------------------------------------------------------
// Pixel Formats
// ----------------------------------------------------------------------------

/**
 * Pixel formats
 */
enum Format : u32 {
    Format_undefined = 0,
    Format_r4g4_unorm_pack8 = 1,
    Format_r4g4b4a4_unorm_pack16 = 2,
    Format_b4g4r4a4_unorm_pack16 = 3,
    Format_r5g6b5_unorm_pack16 = 4,
    Format_b5g6r5_unorm_pack16 = 5,
    Format_r5g5b5a1_unorm_pack16 = 6,
    Format_b5g5r5a1_unorm_pack16 = 7,
    Format_a1r5g5b5_unorm_pack16 = 8,
    Format_r8_unorm = 9,
    Format_r8_snorm = 10,
    Format_r8_uscaled = 11,
    Format_r8_sscaled = 12,
    Format_r8_uint = 13,
    Format_r8_sint = 14,
    Format_r8_srgb = 15,
    Format_r8g8_unorm = 16,
    Format_r8g8_snorm = 17,
    Format_r8g8_uscaled = 18,
    Format_r8g8_sscaled = 19,
    Format_r8g8_uint = 20,
    Format_r8g8_sint = 21,
    Format_r8g8_srgb = 22,
    Format_r8g8b8_unorm = 23,
    Format_r8g8b8_snorm = 24,
    Format_r8g8b8_uscaled = 25,
    Format_r8g8b8_sscaled = 26,
    Format_r8g8b8_uint = 27,
    Format_r8g8b8_sint = 28,
    Format_r8g8b8_srgb = 29,
    Format_b8g8r8_unorm = 30,
    Format_b8g8r8_snorm = 31,
    Format_b8g8r8_uscaled = 32,
    Format_b8g8r8_sscaled = 33,
    Format_b8g8r8_uint = 34,
    Format_b8g8r8_sint = 35,
    Format_b8g8r8_srgb = 36,
    Format_r8g8b8a8_unorm = 37,
    Format_r8g8b8a8_snorm = 38,
    Format_r8g8b8a8_uscaled = 39,
    Format_r8g8b8a8_sscaled = 40,
    Format_r8g8b8a8_uint = 41,
    Format_r8g8b8a8_sint = 42,
    Format_r8g8b8a8_srgb = 43,
    Format_b8g8r8a8_unorm = 44,
    Format_b8g8r8a8_snorm = 45,
    Format_b8g8r8a8_uscaled = 46,
    Format_b8g8r8a8_sscaled = 47,
    Format_b8g8r8a8_uint = 48,
    Format_b8g8r8a8_sint = 49,
    Format_b8g8r8a8_srgb = 50,
    Format_a8b8g8r8_unorm_pack32 = 51,
    Format_a8b8g8r8_snorm_pack32 = 52,
    Format_a8b8g8r8_uscaled_pack32 = 53,
    Format_a8b8g8r8_sscaled_pack32 = 54,
    Format_a8b8g8r8_uint_pack32 = 55,
    Format_a8b8g8r8_sint_pack32 = 56,
    Format_a8b8g8r8_srgb_pack32 = 57,
    Format_a2r10g10b10_unorm_pack32 = 58,
    Format_a2r10g10b10_snorm_pack32 = 59,
    Format_a2r10g10b10_uscaled_pack32 = 60,
    Format_a2r10g10b10_sscaled_pack32 = 61,
    Format_a2r10g10b10_uint_pack32 = 62,
    Format_a2r10g10b10_sint_pack32 = 63,
    Format_a2b10g10r10_unorm_pack32 = 64,
    Format_a2b10g10r10_snorm_pack32 = 65,
    Format_a2b10g10r10_uscaled_pack32 = 66,
    Format_a2b10g10r10_sscaled_pack32 = 67,
    Format_a2b10g10r10_uint_pack32 = 68,
    Format_a2b10g10r10_sint_pack32 = 69,
    Format_r16_unorm = 70,
    Format_r16_snorm = 71,
    Format_r16_uscaled = 72,
    Format_r16_sscaled = 73,
    Format_r16_uint = 74,
    Format_r16_sint = 75,
    Format_r16_sfloat = 76,
    Format_r16g16_unorm = 77,
    Format_r16g16_snorm = 78,
    Format_r16g16_uscaled = 79,
    Format_r16g16_sscaled = 80,
    Format_r16g16_uint = 81,
    Format_r16g16_sint = 82,
    Format_r16g16_sfloat = 83,
    Format_r16g16b16_unorm = 84,
    Format_r16g16b16_snorm = 85,
    Format_r16g16b16_uscaled = 86,
    Format_r16g16b16_sscaled = 87,
    Format_r16g16b16_uint = 88,
    Format_r16g16b16_sint = 89,
    Format_r16g16b16_sfloat = 90,
    Format_r16g16b16a16_unorm = 91,
    Format_r16g16b16a16_snorm = 92,
    Format_r16g16b16a16_uscaled = 93,
    Format_r16g16b16a16_sscaled = 94,
    Format_r16g16b16a16_uint = 95,
    Format_r16g16b16a16_sint = 96,
    Format_r16g16b16a16_sfloat = 97,
    Format_r32_uint = 98,
    Format_r32_sint = 99,
    Format_r32_sfloat = 100,
    Format_r32g32_uint = 101,
    Format_r32g32_sint = 102,
    Format_r32g32_sfloat = 103,
    Format_r32g32b32_uint = 104,
    Format_r32g32b32_sint = 105,
    Format_r32g32b32_sfloat = 106,
    Format_r32g32b32a32_uint = 107,
    Format_r32g32b32a32_sint = 108,
    Format_r32g32b32a32_sfloat = 109,
    Format_r64_uint = 110,
    Format_r64_sint = 111,
    Format_r64_sfloat = 112,
    Format_r64g64_uint = 113,
    Format_r64g64_sint = 114,
    Format_r64g64_sfloat = 115,
    Format_r64g64b64_uint = 116,
    Format_r64g64b64_sint = 117,
    Format_r64g64b64_sfloat = 118,
    Format_r64g64b64a64_uint = 119,
    Format_r64g64b64a64_sint = 120,
    Format_r64g64b64a64_sfloat = 121,
    Format_b10g11r11_ufloat_pack32 = 122,
    Format_e5b9g9r9_ufloat_pack32 = 123,
    Format_d16_unorm = 124,
    Format_x8_d24_unorm_pack32 = 125,
    Format_d32_sfloat = 126,
    Format_s8_uint = 127,
    Format_d16_unorm_s8_uint = 128,
    Format_d24_unorm_s8_uint = 129,
    Format_d32_sfloat_s8_uint = 130,
    Format_bc1_rgb_unorm_block = 131,
    Format_bc1_rgb_srgb_block = 132,
    Format_bc1_rgba_unorm_block = 133,
    Format_bc1_rgba_srgb_block = 134,
    Format_bc2_unorm_block = 135,
    Format_bc2_srgb_block = 136,
    Format_bc3_unorm_block = 137,
    Format_bc3_srgb_block = 138,
    Format_bc4_unorm_block = 139,
    Format_bc4_snorm_block = 140,
    Format_bc5_unorm_block = 141,
    Format_bc5_snorm_block = 142,
    Format_bc6h_ufloat_block = 143,
    Format_bc6h_sfloat_block = 144,
    Format_bc7_unorm_block = 145,
    Format_bc7_srgb_block = 146,
    Format_etc2_r8g8b8_unorm_block = 147,
    Format_etc2_r8g8b8_srgb_block = 148,
    Format_etc2_r8g8b8a1_unorm_block = 149,
    Format_etc2_r8g8b8a1_srgb_block = 150,
    Format_etc2_r8g8b8a8_unorm_block = 151,
    Format_etc2_r8g8b8a8_srgb_block = 152,
    Format_eac_r11_unorm_block = 153,
    Format_eac_r11_snorm_block = 154,
    Format_eac_r11g11_unorm_block = 155,
    Format_eac_r11g11_snorm_block = 156,
    Format_astc_4x4_unorm_block = 157,
    Format_astc_4x4_srgb_block = 158,
    Format_astc_5x4_unorm_block = 159,
    Format_astc_5x4_srgb_block = 160,
    Format_astc_5x5_unorm_block = 161,
    Format_astc_5x5_srgb_block = 162,
    Format_astc_6x5_unorm_block = 163,
    Format_astc_6x5_srgb_block = 164,
    Format_astc_6x6_unorm_block = 165,
    Format_astc_6x6_srgb_block = 166,
    Format_astc_8x5_unorm_block = 167,
    Format_astc_8x5_srgb_block = 168,
    Format_astc_8x6_unorm_block = 169,
    Format_astc_8x6_srgb_block = 170,
    Format_astc_8x8_unorm_block = 171,
    Format_astc_8x8_srgb_block = 172,
    Format_astc_10x5_unorm_block = 173,
    Format_astc_10x5_srgb_block = 174,
    Format_astc_10x6_unorm_block = 175,
    Format_astc_10x6_srgb_block = 176,
    Format_astc_10x8_unorm_block = 177,
    Format_astc_10x8_srgb_block = 178,
    Format_astc_10x10_unorm_block = 179,
    Format_astc_10x10_srgb_block = 180,
    Format_astc_12x10_unorm_block = 181,
    Format_astc_12x10_srgb_block = 182,
    Format_astc_12x12_unorm_block = 183,
    Format_astc_12x12_srgb_block = 184,
    Format_g8b8g8r8_422_unorm = 1000156000,
    Format_b8g8r8g8_422_unorm = 1000156001,
    Format_g8_b8_r8_3plane_420_unorm = 1000156002,
    Format_g8_b8r8_2plane_420_unorm = 1000156003,
    Format_g8_b8_r8_3plane_422_unorm = 1000156004,
    Format_g8_b8r8_2plane_422_unorm = 1000156005,
    Format_g8_b8_r8_3plane_444_unorm = 1000156006,
    Format_r10x6_unorm_pack16 = 1000156007,
    Format_r10x6g10x6_unorm_2pack16 = 1000156008,
    Format_r10x6g10x6b10x6a10x6_unorm_4pack16 = 1000156009,
    Format_g10x6b10x6g10x6r10x6_422_unorm_4pack16 = 1000156010,
    Format_b10x6g10x6r10x6g10x6_422_unorm_4pack16 = 1000156011,
    Format_g10x6_b10x6_r10x6_3plane_420_unorm_3pack16 = 1000156012,
    Format_g10x6_b10x6r10x6_2plane_420_unorm_3pack16 = 1000156013,
    Format_g10x6_b10x6_r10x6_3plane_422_unorm_3pack16 = 1000156014,
    Format_g10x6_b10x6r10x6_2plane_422_unorm_3pack16 = 1000156015,
    Format_g10x6_b10x6_r10x6_3plane_444_unorm_3pack16 = 1000156016,
    Format_r12x4_unorm_pack16 = 1000156017,
    Format_r12x4g12x4_unorm_2pack16 = 1000156018,
    Format_r12x4g12x4b12x4a12x4_unorm_4pack16 = 1000156019,
    Format_g12x4b12x4g12x4r12x4_422_unorm_4pack16 = 1000156020,
    Format_b12x4g12x4r12x4g12x4_422_unorm_4pack16 = 1000156021,
    Format_g12x4_b12x4_r12x4_3plane_420_unorm_3pack16 = 1000156022,
    Format_g12x4_b12x4r12x4_2plane_420_unorm_3pack16 = 1000156023,
    Format_g12x4_b12x4_r12x4_3plane_422_unorm_3pack16 = 1000156024,
    Format_g12x4_b12x4r12x4_2plane_422_unorm_3pack16 = 1000156025,
    Format_g12x4_b12x4_r12x4_3plane_444_unorm_3pack16 = 1000156026,
    Format_g16b16g16r16_422_unorm = 1000156027,
    Format_b16g16r16g16_422_unorm = 1000156028,
    Format_g16_b16_r16_3plane_420_unorm = 1000156029,
    Format_g16_b16r16_2plane_420_unorm = 1000156030,
    Format_g16_b16_r16_3plane_422_unorm = 1000156031,
    Format_g16_b16r16_2plane_422_unorm = 1000156032,
    Format_g16_b16_r16_3plane_444_unorm = 1000156033,
    Format_g8_b8r8_2plane_444_unorm = 1000330000,
    Format_g10x6_b10x6r10x6_2plane_444_unorm_3pack16 = 1000330001,
    Format_g12x4_b12x4r12x4_2plane_444_unorm_3pack16 = 1000330002,
    Format_g16_b16r16_2plane_444_unorm = 1000330003,
    Format_a4r4g4b4_unorm_pack16 = 1000340000,
    Format_a4b4g4r4_unorm_pack16 = 1000340001,
    Format_astc_4x4_sfloat_block = 1000066000,
    Format_astc_5x4_sfloat_block = 1000066001,
    Format_astc_5x5_sfloat_block = 1000066002,
    Format_astc_6x5_sfloat_block = 1000066003,
    Format_astc_6x6_sfloat_block = 1000066004,
    Format_astc_8x5_sfloat_block = 1000066005,
    Format_astc_8x6_sfloat_block = 1000066006,
    Format_astc_8x8_sfloat_block = 1000066007,
    Format_astc_10x5_sfloat_block = 1000066008,
    Format_astc_10x6_sfloat_block = 1000066009,
    Format_astc_10x8_sfloat_block = 1000066010,
    Format_astc_10x10_sfloat_block = 1000066011,
    Format_astc_12x10_sfloat_block = 1000066012,
    Format_astc_12x12_sfloat_block = 1000066013,
    Format_a1b5g5r5_unorm_pack16 = 1000470000,
    Format_a8_unorm = 1000470001,
    Format_pvrtc1_2bpp_unorm_block_img = 1000054000,
    Format_pvrtc1_4bpp_unorm_block_img = 1000054001,
    Format_pvrtc2_2bpp_unorm_block_img = 1000054002,
    Format_pvrtc2_4bpp_unorm_block_img = 1000054003,
    Format_pvrtc1_2bpp_srgb_block_img = 1000054004,
    Format_pvrtc1_4bpp_srgb_block_img = 1000054005,
    Format_pvrtc2_2bpp_srgb_block_img = 1000054006,
    Format_pvrtc2_4bpp_srgb_block_img = 1000054007,
    Format_r8_bool_arm = 1000460000,
    Format_r16g16_sfixed5_nv = 1000464000,
    Format_r10x6_uint_pack16_arm = 1000609000,
    Format_r10x6g10x6_uint_2pack16_arm = 1000609001,
    Format_r10x6g10x6b10x6a10x6_uint_4pack16_arm = 1000609002,
    Format_r12x4_uint_pack16_arm = 1000609003,
    Format_r12x4g12x4_uint_2pack16_arm = 1000609004,
    Format_r12x4g12x4b12x4a12x4_uint_4pack16_arm = 1000609005,
    Format_r14x2_uint_pack16_arm = 1000609006,
    Format_r14x2g14x2_uint_2pack16_arm = 1000609007,
    Format_r14x2g14x2b14x2a14x2_uint_4pack16_arm = 1000609008,
    Format_r14x2_unorm_pack16_arm = 1000609009,
    Format_r14x2g14x2_unorm_2pack16_arm = 1000609010,
    Format_r14x2g14x2b14x2a14x2_unorm_4pack16_arm = 1000609011,
    Format_g14x2_b14x2r14x2_2plane_420_unorm_3pack16_arm = 1000609012,
    Format_g14x2_b14x2r14x2_2plane_422_unorm_3pack16_arm = 1000609013,
};

/**
 * Turns a Format into the size in bytes
 *
 * Parameters
 * - format The format to get the size of
 *
 * Returns
 * - The size of the format in bytes
 */
u32 formatToSize(Format format);

// Vulkan allocator : TODO?

// ----------------------------------------------------------------------------
// Pipeline Stages & Access
// ----------------------------------------------------------------------------

/**
 * Where in the pipeline a resource can be accessed
 */
enum GpuStage : u32 {
    GpuStage_none = 0,
    GpuStage_topOfPipe = 0x00000001,
    GpuStage_drawIndirect = 0x00000002,
    GpuStage_vertexInput = 0x00000004,
    GpuStage_vertexShader = 0x00000008,
    GpuStage_tessellationControlShader = 0x00000010,
    GpuStage_tessellationEvaluationShader = 0x00000020,
    GpuStage_geometryShader = 0x00000040,
    GpuStage_fragmentShader = 0x00000080,
    GpuStage_earlyFragmentTests = 0x00000100,
    GpuStage_lateFragmentTests = 0x00000200,
    GpuStage_colorAttachmentOutput = 0x00000400,
    GpuStage_computeShader = 0x00000800,
    GpuStage_transfer = 0x00001000,
    GpuStage_bottomOfPipe = 0x00002000,
    GpuStage_host = 0x00004000,
    GpuStage_allGraphics = 0x00008000,
    GpuStage_allCommands = 0x00010000,
};
using GpuStageFlags = u32;

/**
 * How a resource can be accessed
 */
enum GpuAccess : u32 {
    GpuAccess_none = 0,
    GpuAccess_indirectCommandRead = 0x00000001,
    GpuAccess_indexRead = 0x00000002,
    GpuAccess_vertexAttributeRead = 0x00000004,
    GpuAccess_uniformRead = 0x00000008,
    GpuAccess_inputAttachmentRead = 0x00000010,
    GpuAccess_shaderRead = 0x00000020,
    GpuAccess_shaderWrite = 0x00000040,
    GpuAccess_colorAttachmentRead = 0x00000080,
    GpuAccess_colorAttachmentWrite = 0x00000100,
    GpuAccess_depthStencilAttachmentRead = 0x00000200,
    GpuAccess_depthStencilAttachmentWrite = 0x00000400,
    GpuAccess_transferRead = 0x00000800,
    GpuAccess_transferWrite = 0x00001000,
    GpuAccess_hostRead = 0x00002000,
    GpuAccess_hostWrite = 0x00004000,
    GpuAccess_memoryRead = 0x00008000,
    GpuAccess_memoryWrite = 0x00010000,
};
using GpuAccessFlags = u32;

// ----------------------------------------------------------------------------
// Buffers
// ----------------------------------------------------------------------------

/**
 * A gpu buffer
 */
struct GpuBuffer;

/**
 * How a gpu buffer will be used
 */
enum GpuBufferUsage : u32 {
    GpuBufferUsage_transferSrc = 0x00000001,
    GpuBufferUsage_transferDst = 0x00000002,
    GpuBufferUsage_uniformTexelBuffer = 0x00000004,
    GpuBufferUsage_storageTexelBuffer = 0x00000008,
    GpuBufferUsage_uniformBuffer = 0x00000010,
    GpuBufferUsage_storageBuffer = 0x00000020,
    GpuBufferUsage_indirectBuffer = 0x00000100,
};
using GpuBufferUsageFlags = u32;

/**
 * How a gpu buffer will be accessed
 */
enum GpuMemoryUsage : u32 {
    /**
     * It will only be accessed from the device
     */
    GpuMemoryUsage_deviceOnly = 0,
    /**
     * It will be used as a staging buffer to transfer from host to device
     */
    GpuMemoryUsage_stagingWrite = 1,
    /**
     * It will be used as a staging buffer to transfer from device to host
     */
    GpuMemoryUsage_stagingRead = 2,
    /**
     * It will be frequently written from the host and read on the device
     */
    GpuMemoryUsage_frequentUpdate = 3,
};

/**
 * How a gpu buffer can be accessed
 */
enum GpuMemoryHostAccess : u32 {
    /**
     * The buffer cannot be accessed by the host
     */
    GpuMemoryHostAccess_none = 0x0,
    /**
     * The buffer can be written to by the host
     */
    GpuMemoryHostAccess_write = 0x1,
    /**
     * The buffer can be read from by the host
     */
    GpuMemoryHostAccess_read = 0x2,
};

/**
 * Create a gpu buffer
 *
 * Parameters
 * - size The size in bytes of the buffer
 * - usageFlags How the buffer will be used
 * - access How the buffer should be accessed
 */
GpuBuffer* gpuBufferCreate(
    u64 size,
    GpuBufferUsageFlags usageFlags,
    GpuMemoryUsage access = GpuMemoryUsage_deviceOnly);

/**
 * Destroy a gpu buffer
 */
void gpuBufferDestroy(GpuBuffer* buffer);

/**
 * Get the uniform buffer descriptor index from the buffer
 */
u32 gpuBufferUniformDescriptor(GpuBuffer* buffer);

/**
 * Get the storage buffer descriptor index from the buffer
 */
u32 gpuBufferStorageDescriptor(GpuBuffer* buffer);

/**
 * Writes to a gpu buffer
 *
 * Parameters
 * - dst The buffer to write to, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - src The data to write, must not be nullptr
 * - size The size in bytes to write
 */
void gpuBufferWrite(GpuBuffer* dst, u64 offset, const void* src, u64 size);

/**
 * Reads from a Vulkan device local buffer through a staging buffer
 *
 * Parameters
 * - dst The location to write to, must not be nullptr
 * - src The buffer to read from, must not be nullptr
 * - offset The offset in bytes into the dst buffer
 * - size The size in bytes to read
 */
void gpuBufferRead(void* dst, GpuBuffer* src, u64 offset, u64 size);

// ----------------------------------------------------------------------------
// Images & Views
// ----------------------------------------------------------------------------

/**
 * A gpu image
 */
struct GpuImage;

/**
 * How an image will be used
 */
enum GpuImageUsage : u32 {
    GpuImageUsage_transferSrc = 0x00000001,
    GpuImageUsage_transferDst = 0x00000002,
    GpuImageUsage_sampled = 0x00000004,
    GpuImageUsage_storage = 0x00000008,
    GpuImageUsage_colorAttachment = 0x00000010,
    GpuImageUsage_depthStencilAttachment = 0x00000020,
    GpuImageUsage_transientAttachment = 0x00000040,
    GpuImageUsage_inputAttachment = 0x00000080,
    GpuImageUsage_hostTransfer = 0x00400000,
};
using GpuImageUsageFlags = u32;

/**
 * The layout of an image
 */
enum GpuLayout : u32 {
    GpuLayout_undefined = 0,
    GpuLayout_general = 1,
    GpuLayout_colorAttachment = 2,
    GpuLayout_depthStencilAttachment = 3,
    GpuLayout_depthStencilReadOnly = 4,
    GpuLayout_shaderReadOnly = 5,
    GpuLayout_transferSrc = 6,
    GpuLayout_transferDst = 7,
    GpuLayout_preinitialized = 8,
    GpuLayout_presentSrc = 1000001002,
};

/**
 * Extra config flags for image creation
 */
enum GpuImageConfig : u32 {
    GpuImageConfig_cubeCompatible = 0x00000010,
};
using GpuImageConfigFlags = u32;

/**
 * Create a gpu image assuming most defaults
 */
GpuImage* gpuImageCreate(u32 width, u32 height, Format format, GpuImageUsageFlags usage);

/**
 * Config for gpuImageCreateEx
 */
struct GpuImageCreateEx {
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
    Format format = Format_undefined;
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
    GpuImageUsageFlags usage = 0;
    /**
     * Extra config flags
     */
    GpuImageConfigFlags flags = 0;
};

/**
 * Create a gpu image with extended options
 */
GpuImage* gpuImageCreateEx(const GpuImageCreateEx* create);

/**
 * Destroy a gpu image
 */
void gpuImageDestroy(GpuImage* image);

/**
 * Get the width of an image
 */
u32 gpuImageWidth(GpuImage* image);

/**
 * Get the height of an image
 */
u32 gpuImageHeight(GpuImage* image);

/**
 * A gpu view
 */
struct GpuView;

/**
 * The dimensionality of an image
 */
enum GpuViewType : u32 {
    GpuViewType_1D = 0,
    GpuViewType_2D = 1,
    GpuViewType_3D = 2,
    GpuViewType_cube = 3,
    GpuViewType_1DArray = 4,
    GpuViewType_2DArray = 5,
    GpuViewType_cubeArray = 6,
};

/**
 * The aspect the image will be accessed in
 */
enum GpuAspect : u32 {
    GpuAspect_none = 0,
    GpuAspect_color = 0x00000001,
    GpuAspect_depth = 0x00000002,
    GpuAspect_stencil = 0x00000004,
    GpuAspect_metadata = 0x00000008,
    GpuAspect_plane0 = 0x00000010,
    GpuAspect_plane1 = 0x00000020,
    GpuAspect_plane2 = 0x00000040,
};
using GpuAspectFlags = u32;

/**
 * How a sampler interpolates between pixels
 */
enum GpuFilter : u32 {
    GpuFilter_nearest = 0,
    GpuFilter_linear = 1,
    GpuFilter_count,
};

/**
 * How a sampler samples off the image's edge
 */
enum GpuSamplerEdgeMode : u32 {
    GpuSamplerEdgeMode_repeat = 0,
    GpuSamplerEdgeMode_mirroredRepeat = 1,
    GpuSamplerEdgeMode_clampToEdge = 2,
    GpuSamplerEdgeMode_clampToBorder = 3,
    GpuSamplerEdgeMode_mirrorClampToEdge = 4,
    GpuSamplerEdgeMode_count,
};

/**
 * The border color if the sampler edge mode has a border
 */
enum GpuSamplerBorder : u32 {
    GpuSamplerBorder_floatTransparentBlack = 0,
    GpuSamplerBorder_intTransparentBlack = 1,
    GpuSamplerBorder_floatOpaqueBlack = 2,
    GpuSamplerBorder_intOpaqueBlack = 3,
    GpuSamplerBorder_floatOpaqueWhite = 4,
    GpuSamplerBorder_intOpaqueWhite = 5,
    GpuSamplerBorder_count,
};

/**
 * Create a gpu image view
 */
GpuView* gpuViewCreate(
    GpuImage* image,
    GpuAspectFlags aspectFlags,
    GpuFilter filter = GpuFilter_nearest);

/**
 * Config for gpuViewCreateEx
 */
struct GpuViewCreateEx {
    GpuImage* image = nullptr;
    u32 baseMipLevel = 0;
    u32 levelCount = 0;
    u32 baseArrayLayer = 0;
    u32 layerCount = 0;
    GpuAspectFlags aspectFlags = 0;
    GpuViewType type = {};
    GpuFilter filter = {};
    GpuSamplerEdgeMode edgeMode = {};
    GpuSamplerBorder border = {};
};

/**
 * Create a gpu image view with extended config
 */
GpuView* gpuViewCreateEx(const GpuViewCreateEx* config);

/**
 * Destroy a gpu image view
 */
void gpuViewDestroy(GpuView* view);

/**
 * Get the width of an image
 */
u32 gpuViewWidth(GpuView* view);

/**
 * Get the height of an image
 */
u32 gpuViewHeight(GpuView* view);

/**
 * Get the image sampler descriptor index from the image view
 */
u32 gpuImageSamplerDescriptor(GpuView* view);

/**
 * Get the storage image descriptor index from the image view
 */
u32 gpuImageStorageDescriptor(GpuView* view);

/**
 * Write to a gpu image
 *
 * Note, only fills the base mip level
 *
 * Parameters
 * - dst The image to write to
 * - src The data to read from
 */
void gpuImageWrite(GpuView* dst, const void* src);

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
void gpuImageWriteCubemap(GpuView* dst, const void* src);

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
void gpuImageRead(void* dst, GpuView* src);

/**
 * Generates mipmaps from the base level
 *
 * Note, dst should only have 1 array layer
 *
 * Parameters
 * - image The image to generate mipmaps for
 */
void gpuImageGenMipmaps(GpuView* dst);

/**
 * Calculates the maximum number of mipmap levels that an image can have
 *
 * Parameters
 * - width The width of the image
 * - height The height of the image
 * - depth The depth of the image
 */
u32 getMaxMipmaps(u32 width, u32 height, u32 depth);

// ----------------------------------------------------------------------------
// Pipelines
// ----------------------------------------------------------------------------

/**
 * A gpu pipeline
 */
struct GpuPipeline;

/**
 * A push constant range in a pipeline
 */
struct GpuPushRange {
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
 * How the vertex list is interpreted
 */
enum GpuTopology : u32 {
    GpuTopology_pointList = 0,
    GpuTopology_lineList = 1,
    GpuTopology_lineStrip = 2,
    GpuTopology_triangleList = 3,
    GpuTopology_triangleStrip = 4,
    GpuTopology_triangleFan = 5,
    GpuTopology_lineListWithAdjacency = 6,
    GpuTopology_lineStripWithAdjacency = 7,
    GpuTopology_triangleListWithAdjacency = 8,
    GpuTopology_triangleStripWithAdjacency = 9,
    GpuTopology_patchList = 10,
};

/**
 * How to treat vertices
 */
enum GpuPolygonMode : u32 {
    GpuPolygonMode_fill = 0,
    GpuPolygonMode_line = 1,
    GpuPolygonMode_point = 2,
};

enum GpuCull : u32 {
    GpuCull_none = 0,
    GpuCull_front = 0x00000001,
    GpuCull_back = 0x00000002,
    GpuCull_both = 0x00000003,
};
using GpuCullFlags = u32;

/**
 * Config for createGraphicsPipeline
 */
struct CreateGpuGraphicsPipeline {
    /**
     * The vertex shader code
     */
    const void* vertexShader = nullptr;
    /**
     * The size in bytes of the vertex shader code
     */
    u64 vertexShaderSize = 0;
    /**
     * The fragment shader code
     */
    const void* fragmentShader = nullptr;
    /**
     * The size in bytes of the fragment shader code
     */
    u64 fragmentShaderSize = 0;
    /**
     * The size of the push constant
     */
    u32 pushConstantSize = 0;
    /**
     * The format of the color attachments, none can be UNDEFINED
     */
    const Format* colorAttachmentFormats = nullptr;
    /**
     * The number of color attachment formats
     */
    u32 colorAttachmentCount = 0;
    /**
     * The format of the depth attachment, no depth attachment if UNDEFINED
     */
    Format depthAttachmentFormat = Format_undefined;
    /**
     * The format of the stencil attachment, no stencil attachment if UNDEFINED
     */
    Format stencilAttachmentFormat = Format_undefined;
    /**
     * How to interpret vertices into topology
     */
    GpuTopology topology = GpuTopology_triangleList;
    /**
     * The number of patch control points in the tesselation stage
     */
    u32 tesselationPatchControlPoints = 0;
    /**
     * How polygons are drawn
     */
    GpuPolygonMode polygonMode = GpuPolygonMode_fill;
    /**
     * Enables back/front face culling
     */
    GpuCullFlags cullMode = GpuCull_none;
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
GpuPipeline* gpuPipelineCreateGraphics(const CreateGpuGraphicsPipeline* config);

/**
 * Create a compute pipeline
 *
 * Parameters
 * - pushSize The size of the push constant
 * - shaderCode The compute shader, must not be nullptr
 * - shaderCodeSize The size in bytes of shaderCode
 */
GpuPipeline* gpuPipelineCreateCompute(u32 pushSize, const u8* shaderCode, u64 shaderCodeSize);

/**
 * Destroy a graphics or compute pipeline
 */
void gpuPipelineDestroy(GpuPipeline* pipeline);

// ----------------------------------------------------------------------------
// Command Buffer & Draw
// ----------------------------------------------------------------------------

/**
 * A gpu command buffer
 */
struct GpuCmd;

/**
 * Begin a command buffer to be executed once
 *
 * Returns
 * - The command buffer to record, will never be nullptr
 */
GpuCmd* gpuCmdBegin();

/**
 * Execute the command buffer and wait for completion
 *
 * Parameters
 * - cmd The command buffer from beginGpuCommands, must not be nullptr
 */
void gpuCmdEnd(GpuCmd* cmd);

/**
 * Bind a graphics or compute pipeline
 */
void gpuBindPipeline(GpuCmd* cmd, GpuPipeline* pipeline);

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
void gpuPushConstants(GpuCmd* cmd, GpuPipeline* pipeline, void* push, u32 size);

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
void gpuDraw(GpuCmd* cmd, u32 vertexBegin, u32 vertexCount, u32 instanceBegin, u32 instanceCount);

/**
 * Dispatch a compute shader
 *
 * Parameters
 * - cmd The command buffer to record to
 * - groupCountX The number of workgroups in the x dimension
 * - groupCountY The number of workgroups in the y dimension
 * - groupCountZ The number of workgroups in the z dimension
 */
void gpuDispatch(GpuCmd* cmd, u32 groupCountX, u32 groupCountY, u32 groupCountZ);

// ----------------------------------------------------------------------------
// Barriers
// ----------------------------------------------------------------------------

/**
 * An image dependency barrier
 */
struct GpuImageBarrier {
    /**
     * The image to sychronize
     */
    GpuView* image = nullptr;
    /**
     * Where the image will be used next
     */
    GpuStageFlags nextStage = 0;
    /**
     * How the image will be accessed next
     */
    GpuAccessFlags nextAccess = 0;
    /**
     * The next layout the image needs to be in
     */
    GpuLayout nextLayout = GpuLayout_undefined;
};

/**
 * A buffer dependency barrier
 */
struct GpuBufferBarrier {
    /**
     * The buffer to sychronize
     */
    GpuBuffer* buffer = nullptr;
    /**
     * Where the image will be used next
     */
    GpuStageFlags nextStage = 0;
    /**
     * How the image will be accessed next
     */
    GpuAccessFlags nextAccess = 0;
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
void gpuMemoryBarrier(
    GpuCmd* cmd,
    const GpuBufferBarrier* bufferBarriers,
    u32 bufferBarrierCount,
    const GpuImageBarrier* imageBarriers,
    u32 imageBarrierCount);

// ----------------------------------------------------------------------------
// Compute Pass
// ----------------------------------------------------------------------------

/**
 * A compute pass description
 */
struct GpuComputePass {
    /**
     * The uniforms buffer dependencies
     */
    GpuBuffer* uniformBuffers = nullptr;
    /**
     * The number of uniform buffers
     */
    u32 uniformBufferCount = 0;
    /**
     * The storage buffer dependencies
     */
    GpuBuffer* storageBuffers = nullptr;
    /**
     * The number of storage buffers
     */
    u32 storageBufferCount = 0;
    /**
     * The sampled image dependencies
     */
    GpuView* sampledImages = nullptr;
    /**
     * The number of sampled images
     */
    u32 sampledImageCount = 0;
    /**
     * The storage image dependencies
     */
    GpuView* storageImages = nullptr;
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
void gpuComputePass(GpuCmd* cmd, const GpuComputePass* pass);

// ----------------------------------------------------------------------------
// Render Pass
// ----------------------------------------------------------------------------

/**
 * The operation to load a render attachment
 */
enum GpuLoadOp : u32 {
    GpuLoadOp_load = 0,
    GpuLoadOp_clear = 1,
    GpuLoadOp_dontCare = 2,
};

/**
 * The operation to store a render attachment
 */
enum GpuStoreOp : u32 {
    GpuStoreOp_store = 0,
    GpuStoreOp_dontCare = 1,
};

/**
 * The value to clear color attachments to
 */
union GpuClearValueColor {
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
struct GpuClearValueDepthStencil {
    /**
     * The depth value
     */
    f32 depth = 0.0f;
    /**
     * The stencil value
     */
    u32 stencil = 0;
};

/**
 * The value to clear a render attachment to
 */
union GpuClearValue {
    /**
     * The value for color attachments
     */
    GpuClearValueColor color;
    /**
     * The value for depth and stencil attachments
     */
    GpuClearValueDepthStencil depthStencil;
};

/**
 * A rendering attachment
 */
struct GpuRenderAttachment {
    /**
     * The image attached, must not be nullptr
     */
    GpuView* image = nullptr;
    /**
     * How the image will be loaded
     */
    GpuLoadOp loadOp = GpuLoadOp_clear;
    /**
     * How the image will be stored
     */
    GpuStoreOp storeOp = GpuStoreOp_store;
    /**
     * What to clear the image to if cleared
     */
    GpuClearValue clearValue = {};
};

/**
 * A render pass description
 */
struct GpuRenderPass {
    /**
     * The uniforms buffer dependencies
     */
    GpuBuffer* uniformBuffers = nullptr;
    /**
     * The number of uniform buffers
     */
    u32 uniformBufferCount = 0;
    /**
     * The storage buffer dependencies
     */
    GpuBuffer* storageBuffers = nullptr;
    /**
     * The number of storage buffers
     */
    u32 storageBufferCount = 0;
    /**
     * The sampled image dependencies
     */
    GpuView* sampledImages = nullptr;
    /**
     * The number of sampled images
     */
    u32 sampledImageCount = 0;
    /**
     * The storage image dependencies
     */
    GpuView* storageImages = nullptr;
    /**
     * The number of storage images
     */
    u32 storageImageCount = 0;
    /**
     * The color images to write to
     */
    const GpuRenderAttachment* colorAttachments = nullptr;
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
    const GpuRenderAttachment* depthAttachment = nullptr;
    /**
     * The stencil attachment, if any
     */
    const GpuRenderAttachment* stencilAttachment = nullptr;
};

/**
 * Performs render barrier and begins a render pass
 *
 * Parameters
 * - cmd The command buffer
 * - pass The render pass description
 */
void gpuRenderPassBegin(GpuCmd* cmd, const GpuRenderPass* pass);

/**
 * Ends the render pass
 *
 * Parameters
 * - cmd The command buffer
 */
void gpuRenderPassEnd(GpuCmd* cmd);

/**
 * Set the rendering viewport, should be called after gpuRenderPassBegin
 */
void gpuSetViewport(GpuCmd* cmd, f32 x, f32 y, f32 width, f32 height, f32 near = 0.0f, f32 far = 1.0f);

/**
 * Set the rendering scissor, should be called after gpuRenderPassBegin
 */
void gpuSetScissor(GpuCmd* cmd, i32 x, i32 y, u32 width, u32 height);

// ============================================================================
// Math
// ============================================================================

// ----------------------------------------------------------------------------
// Constants & Helpers
// ----------------------------------------------------------------------------

/**
 * The value of Pi
 */
#define HG_PI 3.1415926535897932
/**
 * The value of Euler's number
 */
#define HG_EULER 2.7182818284590452
/**
 * The value of square root 2
 */
#define HG_ROOT2 1.4142135623730951
/**
 * The value of square root 3
 */
#define HG_ROOT3 1.7320508075688772

/**
 * Returns base to the positive integer exp power
 */
constexpr f32 pow(f32 base, u32 exp)
{
    f32 ret = 1.0f;

    while (exp > 0)
    {
        if (exp & 1)
            ret *= base;

        base *= base;
        exp >>= 1;
    }

    return ret;
}

/**
 * Squares a number
 */
constexpr f32 square(f32 x)
{
    return x * x;
}

/**
 * Interpolates between two values
 */
constexpr f32 lerp(f32 a, f32 b, f32 t)
{
    return a + (b - a) * t;
}

/**
 * Smooth a t value for interpolation
 */
constexpr f32 smooth(f32 t)
{
    return t * t * (3.0f - 2.0f * t);
}

/**
 * Smooth a t value for interpolation using a quintic formula
 */
constexpr f32 smoothQuintic(f32 t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

// ----------------------------------------------------------------------------
// Vector Types
// ----------------------------------------------------------------------------

/**
 * A 2D vector
 */
struct Vec2 {
    /**
     * The vector components
     */
    f32 x, y;

    /**
     * Construct uninitialized
     */
    Vec2() = default;

    /**
     * Construct from a list of values
     */
    constexpr Vec2(f32 xVal, f32 yVal) : x{xVal}, y{yVal} {}

    /**
     * Construct from a single scalar
     */
    explicit constexpr Vec2(f32 scalar) : x{scalar}, y{scalar} {}

    /**
     * Add another vector in place
     */
    Vec2& operator+=(Vec2 other);

    /**
     * Subtract another vector in place
     */
    Vec2& operator-=(Vec2 other);

    /**
     * Multiply another vector in place
     */
    Vec2& operator*=(Vec2 other);

    /**
     * Divide another vector in place
     */
    Vec2& operator/=(Vec2 other);

    /**
     * Access by index
     */
    constexpr f32& operator[](u32 idx)
    {
        HG_ASSERT(idx < 2);
        return *(&x + idx);
    }
};

/**
 * A 3D vector
 */
struct Vec3 {
    /**
     * The vector components
     */
    f32 x, y, z;

    /**
     * Construct uninitialized
     */
    Vec3() = default;

    /**
     * Construct from a list of values
     */
    constexpr Vec3(f32 xVal, f32 yVal, f32 zVal) : x{xVal}, y{yVal}, z{zVal} {}

    /**
     * Construct from a single scalar
     */
    explicit constexpr Vec3(f32 scalar) : x{scalar}, y{scalar}, z{scalar} {}

    /**
     * Construct from a Vec2 and scalar
     */
    explicit constexpr Vec3(Vec2 other, f32 zVal) : x{other.x}, y{other.y}, z{zVal} {}

    /**
     * Downsize to Vec2
     */
    explicit constexpr operator Vec2() const
    {
        return {x, y};
    }

    /**
     * Add another vector in place
     */
    Vec3& operator+=(Vec3 other);

    /**
     * Subtract another vector in place
     */
    Vec3& operator-=(Vec3 other);

    /**
     * Multiply another vector in place
     */
    Vec3& operator*=(Vec3 other);

    /**
     * Divide another vector in place
     */
    Vec3& operator/=(Vec3 other);

    /**
     * Access by index
     */
    constexpr f32& operator[](u32 idx)
    {
        HG_ASSERT(idx < 3);
        return *(&x + idx);
    }
};

/**
 * A 4D vector
 */
struct Vec4 {
    /**
     * The vector components
     */
    f32 x, y, z, w;

    /**
     * Construct uninitialized
     */
    Vec4() = default;

    /**
     * Construct from a list of values
     */
    constexpr Vec4(f32 xVal, f32 yVal, f32 zVal, f32 wVal) : x{xVal}, y{yVal}, z{zVal}, w{wVal} {}

    /**
     * Construct from a single scalar
     */
    explicit constexpr Vec4(f32 scalar) : x{scalar}, y{scalar}, z{scalar}, w{scalar} {}

    /**
     * Construct from a Vec2 and scalars
     */
    explicit constexpr Vec4(Vec2 other, f32 zVal, f32 wVal) : x{other.x}, y{other.y}, z{zVal}, w{wVal} {}

    /**
     * Construct from a Vec3 and scalar
     */
    explicit constexpr Vec4(Vec3 other, f32 wVal) : x{other.x}, y{other.y}, z{other.z}, w{wVal} {}

    /**
     * Downsize to Vec2
     */
    explicit constexpr operator Vec2() const
    {
        return {x, y};
    }

    /**
     * Downsize to Vec3
     */
    explicit constexpr operator Vec3() const
    {
        return {x, y, z};
    }

    /**
     * Add another vector in place
     */
    Vec4& operator+=(Vec4 other);

    /**
     * Subtract another vector in place
     */
    Vec4& operator-=(Vec4 other);

    /**
     * Multiply another vector in place
     */
    Vec4& operator*=(Vec4 other);

    /**
     * Divide another vector in place
     */
    Vec4& operator/=(Vec4 other);

    /**
     * Access by index
     */
    constexpr f32& operator[](u32 idx)
    {
        HG_ASSERT(idx < 4);
        return *(&x + idx);
    }
};

// ----------------------------------------------------------------------------
// Matrix Types
// ----------------------------------------------------------------------------

/**
 * A 2x2 matrix
 */
struct Mat2 {
    /**
     * The matrix components
     */
    Vec2 x, y;

    /**
     * Construct uninitialized
     */
    Mat2() = default;

    /**
     * Construct from a list of vectors
     */
    constexpr Mat2(Vec2 xVal, Vec2 yVal) : x{xVal}, y{yVal} {}

    /**
     * Construct from a single scalar
     */
    explicit constexpr Mat2(f32 scalar) : x{scalar, 0}, y{0, scalar} {}

    /**
     * Construct from a list of values
     */
    explicit constexpr Mat2(f32 xx, f32 xy, f32 yx, f32 yy) : x{xx, xy}, y{yx, yy} {}

    /**
     * Add another matrix in place
     */
    Mat2& operator+=(const Mat2& other);

    /**
     * Subtract another matrix in place
     */
    Mat2& operator-=(const Mat2& other);

    /**
     * Access by index
     */
    constexpr Vec2& operator[](u32 idx)
    {
        HG_ASSERT(idx < 2);
        return *(&x + idx);
    }
};

/**
 * A 3x3 matrix
 */
struct Mat3 {
    /**
     * The matrix components
     */
    Vec3 x, y, z;

    /**
     * Construct uninitialized
     */
    Mat3() = default;

    /**
     * Construct from a list of vectors
     */
    constexpr Mat3(Vec3 xVal, Vec3 yVal, Vec3 zVal)
        : x{xVal}, y{yVal}, z{zVal} {}

    /**
     * Construct from a single scalar
     */
    explicit constexpr Mat3(f32 scalar)
        : x{scalar, 0, 0}, y{0, scalar, 0}, z{0, 0, scalar} {}

    /**
     * Construct from a Mat2
     */
    explicit constexpr Mat3(const Mat2& other)
        : x{other.x, 0}, y{other.y, 0}, z{0, 0, 1} {}

    /**
     * Downsize to Mat2
     */
    explicit constexpr operator Mat2() const
    {
        return Mat2{Vec2{x}, Vec2{y}};
    }

    /**
     * Add another matrix in place
     */
    Mat3& operator+=(const Mat3& other);

    /**
     * Subtract another matrix in place
     */
    Mat3& operator-=(const Mat3& other);

    /**
     * Access by index
     */
    constexpr Vec3& operator[](u32 idx)
    {
        HG_ASSERT(idx < 3);
        return *(&x + idx);
    }
};

/**
 * A 4x4 matrix
 */
struct Mat4 {
    /**
     * The matrix components
     */
    Vec4 x, y, z, w;

    /**
     * Construct uninitialized
     */
    Mat4() = default;

    /**
     * Construct from a list of vectors
     */
    constexpr Mat4(Vec4 xVal, Vec4 yVal, Vec4 zVal, Vec4 wVal)
        : x{xVal}, y{yVal}, z{zVal}, w{wVal} {}

    /**
     * Construct from a single scalar
     */
    explicit constexpr Mat4(f32 scalar)
        : x{scalar, 0, 0, 0}, y{0, scalar, 0, 0}, z{0, 0, scalar, 0}, w{0, 0, 0, scalar} {}

    /**
     * Construct from a Mat2
     */
    explicit constexpr Mat4(const Mat2& other)
        : x{other.x, 0, 0}, y{other.y, 0, 0}, z{0, 0, 1, 0}, w{0, 0, 0, 1} {}

    /**
     * Construct from a Mat3
     */
    explicit constexpr Mat4(const Mat3& other)
        : x{other.x, 0}, y{other.y, 0}, z{other.z, 0}, w{0, 0, 0, 1} {}

    /**
     * Downsize to Mat2
     */
    explicit constexpr operator Mat2() const
    {
        return Mat2{Vec2{x}, Vec2{y}};
    }

    /**
     * Downsize to Mat3
     */
    explicit constexpr operator Mat3() const
    {
        return Mat3{Vec3{x}, Vec3{y}, Vec3{z}};
    }

    /**
     * Add another matrix in place
     */
    Mat4& operator+=(const Mat4& other);

    /**
     * Subtract another matrix in place
     */
    Mat4& operator-=(const Mat4& other);

    /**
     * Access by index
     */
    constexpr Vec4& operator[](u32 idx)
    {
        HG_ASSERT(idx < 4);
        return *(&x + idx);
    }
};

// ----------------------------------------------------------------------------
// Complex & Quaternion Types
// ----------------------------------------------------------------------------

/**
 * A complex number
 */
struct Complex {
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
    Complex() = default;

    /**
     * Construct from just a real value
     */
    constexpr Complex(f32 rVal) : r{rVal}, i{0} {}

    /**
     * Construct from a list of values
     */
    constexpr Complex(f32 rVal, f32 iVal) : r{rVal}, i{iVal} {}

    /**
     * Add another complex number in place
     */
    Complex& operator+=(Complex other);
    /**
     * Subtract another complex number in place
     */
    Complex& operator-=(Complex other);
};

/**
 * A quaternion
 */
struct Quat {
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
    Quat() = default;

    /**
     * Construct from just a real value
     */
    constexpr Quat(f32 rVal) : r{rVal}, i{0}, j{0}, k{0} {}

    /**
     * Construct from a list of values
     */
    constexpr Quat(f32 rVal, f32 iVal, f32 jVal, f32 kVal) : r{rVal}, i{iVal}, j{jVal}, k{kVal} {}

    /**
     * Add another quaternion in place
     */
    Quat& operator+=(Quat other);

    /**
     * Subtract another quaternion in place
     */
    Quat& operator-=(Quat other);
};

// ----------------------------------------------------------------------------
// Comparison Operators
// ----------------------------------------------------------------------------

/**
 * Compare vectors
 */
constexpr bool operator==(Vec2 lhs, Vec2 rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

/**
 * Compare vectors
 */
constexpr bool operator!=(Vec2 lhs, Vec2 rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

/**
 * Compare vectors
 */
constexpr bool operator==(Vec3 lhs, Vec3 rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

/**
 * Compare vectors
 */
constexpr bool operator!=(Vec3 lhs, Vec3 rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}

/**
 * Compare vectors
 */
constexpr bool operator==(Vec4 lhs, Vec4 rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

/**
 * Compare vectors
 */
constexpr bool operator!=(Vec4 lhs, Vec4 rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
}

/**
 * Compare vectors, treating values within epsilon as the same
 */
constexpr bool vecEq2(Vec2 lhs, Vec2 rhs)
{
    return std::abs(lhs.x - rhs.x) < 1e-6 &&
           std::abs(lhs.y - rhs.y) < 1e-6;
}

/**
 * Compare vectors, treating values within epsilon as the same
 */
constexpr bool vecEq3(Vec3 lhs, Vec3 rhs)
{
    return std::abs(lhs.x - rhs.x) < 1e-6 &&
           std::abs(lhs.y - rhs.y) < 1e-6 &&
           std::abs(lhs.z - rhs.z) < 1e-6;
}

/**
 * Compare vectors, treating values within epsilon as the same
 */
constexpr bool vecEq4(Vec4 lhs, Vec4 rhs)
{
    return std::abs(lhs.x - rhs.x) < 1e-6 &&
           std::abs(lhs.y - rhs.y) < 1e-6 &&
           std::abs(lhs.z - rhs.z) < 1e-6 &&
           std::abs(lhs.w - rhs.w) < 1e-6;
}

/**
 * Compare matrices
 */
constexpr bool operator==(const Mat2& lhs, const Mat2& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

/**
 * Compare matrices
 */
constexpr bool operator!=(const Mat2& lhs, const Mat2& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y;
}

/**
 * Compare matrices
 */
constexpr bool operator==(const Mat3& lhs, const Mat3& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

/**
 * Compare matrices
 */
constexpr bool operator!=(const Mat3& lhs, const Mat3& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z;
}

/**
 * Compare matrices
 */
constexpr bool operator==(const Mat4& lhs, const Mat4& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z && lhs.w == rhs.w;
}

/**
 * Compare matrices
 */
constexpr bool operator!=(const Mat4& lhs, const Mat4& rhs)
{
    return lhs.x != rhs.x || lhs.y != rhs.y || lhs.z != rhs.z || lhs.w != rhs.w;
}

/**
 * Compare complex numbers
 */
constexpr bool operator==(Complex lhs, Complex rhs)
{
    return lhs.r == rhs.r && lhs.i == rhs.i;
}

/**
 * Compare complex numbers
 */
constexpr bool operator!=(Complex lhs, Complex rhs)
{
    return lhs.r != rhs.r || lhs.i != rhs.i;
}

/**
 * Compare quaternions
 */
constexpr bool operator==(Quat lhs, Quat rhs)
{
    return lhs.r == rhs.r && lhs.i == rhs.i && lhs.j == rhs.j && lhs.k == rhs.k;
}

/**
 * Compare quaternions
 */
constexpr bool operator!=(Quat lhs, Quat rhs)
{
    return lhs.r != rhs.r || lhs.i != rhs.i || lhs.j != rhs.j || lhs.k != rhs.k;
}

// ----------------------------------------------------------------------------
// Vector Functions
// ----------------------------------------------------------------------------

/**
 * Add 2D vectors
 */
constexpr Vec2 operator+(Vec2 lhs, Vec2 rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y};
}

/**
 * Add 3D vectors
 */
constexpr Vec3 operator+(Vec3 lhs, Vec3 rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z};
}

/**
 * Add 4D vectors
 */
constexpr Vec4 operator+(Vec4 lhs, Vec4 rhs)
{
    return {lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w};
}

/**
 * Subtract 2D vectors
 */
constexpr Vec2 operator-(Vec2 lhs, Vec2 rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y};
}

/**
 * Subtract 3D vectors
 */
constexpr Vec3 operator-(Vec3 lhs, Vec3 rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

/**
 * Subtract 4D vectors
 */
constexpr Vec4 operator-(Vec4 lhs, Vec4 rhs)
{
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w};
}

/**
 * Multiple a 2D vector by -1
 */
constexpr Vec2 operator-(Vec2 v)
{
    return {-v.x, -v.y};
}

/**
 * Multiple a 3D vector by -1
 */
constexpr Vec3 operator-(Vec3 v)
{
    return {-v.x, -v.y, -v.z};
}

/**
 * Multiple a 4D vector by -1
 */
constexpr Vec4 operator-(Vec4 v)
{
    return {-v.x, -v.y, -v.z, -v.w};
}

/**
 * Multiply pairwise 2D vectors
 */
constexpr Vec2 operator*(Vec2 lhs, Vec2 rhs)
{
    return {lhs.x * rhs.x, lhs.y * rhs.y};
}

/**
 * Multiply pairwise 3D vectors
 */
constexpr Vec3 operator*(Vec3 lhs, Vec3 rhs)
{
    return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z};
}

/**
 * Multiply pairwise 4D vectors
 */
constexpr Vec4 operator*(Vec4 lhs, Vec4 rhs)
{
    return {lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w};
}

/**
 * Multiply a scalar and a 2D vector
 */
constexpr Vec2 operator*(f32 scalar, Vec2 vec)
{
    return {scalar * vec.x, scalar * vec.y};
}

/**
 * Multiply a scalar and a 2D vector
 */
constexpr Vec2 operator*(Vec2 vec, f32 scalar)
{
    return {scalar * vec.x, scalar * vec.y};
}

/**
 * Multiply a scalar and a 3D vector
 */
constexpr Vec3 operator*(f32 scalar, Vec3 vec)
{
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z};
}

/**
 * Multiply a scalar and a 3D vector
 */
constexpr Vec3 operator*(Vec3 vec, f32 scalar)
{
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z};
}

/**
 * Multiply a scalar and a 4D vector
 */
constexpr Vec4 operator*(f32 scalar, Vec4 vec)
{
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
}

/**
 * Multiply a scalar and a 4D vector
 */
constexpr Vec4 operator*(Vec4 vec, f32 scalar)
{
    return {scalar * vec.x, scalar * vec.y, scalar * vec.z, scalar * vec.w};
}

/**
 * Divide pairwise 2D vectors
 *
 * Note, cannot divide by 0
 */
constexpr Vec2 operator/(Vec2 lhs, Vec2 rhs)
{
    HG_ASSERT(rhs.x != 0 && rhs.y != 0);
    return {lhs.x / rhs.x, lhs.y / rhs.y};
}

/**
 * Divide pairwise 3D vectors
 *
 * Note, cannot divide by 0
 */
constexpr Vec3 operator/(Vec3 lhs, Vec3 rhs)
{
    HG_ASSERT(rhs.x != 0 && rhs.y != 0 && rhs.z != 0);
    return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z};
}

/**
 * Divide pairwise 4D vectors
 *
 * Note, cannot divide by 0
 */
constexpr Vec4 operator/(Vec4 lhs, Vec4 rhs)
{
    HG_ASSERT(rhs.x != 0 && rhs.y != 0 && rhs.z != 0 && rhs.w != 0);
    return {lhs.x / rhs.x, lhs.y / rhs.y, lhs.z / rhs.z, lhs.w / rhs.w};
}

/**
 * Divide a 2D vector by a scalar
 *
 * Note, cannot divide by 0
 */
constexpr Vec2 operator/(Vec2 vec, f32 scalar)
{
    HG_ASSERT(scalar != 0);
    return {vec.x / scalar, vec.y / scalar};
}

/**
 * Divide a 3D vector by a scalar
 *
 * Note, cannot divide by 0
 */
constexpr Vec3 operator/(Vec3 vec, f32 scalar)
{
    HG_ASSERT(scalar != 0);
    return {vec.x / scalar, vec.y / scalar, vec.z / scalar};
}

/**
 * Divide a 4D vector by a scalar
 *
 * Note, cannot divide by 0
 */
constexpr Vec4 operator/(Vec4 vec, f32 scalar)
{
    HG_ASSERT(scalar != 0);
    return {vec.x / scalar, vec.y / scalar, vec.z / scalar, vec.w / scalar};
}

/**
 * Compute the dot product of 2D vectors
 */
constexpr f32 vecDot2(Vec2 lhs, Vec2 rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

/**
 * Compute the dot product of 3D vectors
 */
constexpr f32 vecDot3(Vec3 lhs, Vec3 rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

/**
 * Compute the dot product of 4D vectors
 */
constexpr f32 vecDot4(Vec4 lhs, Vec4 rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
}

/**
 * Compute the length squared of a 2D vector
 */
constexpr f32 vecLenSqr2(Vec2 vec)
{
    return vecDot2(vec, vec);
}

/**
 * Compute the length squared of a 3D vector
 */
constexpr f32 vecLenSqr3(Vec3 vec)
{
    return vecDot3(vec, vec);
}

/**
 * Compute the length squared of a 4D vector
 */
constexpr f32 vecLenSqr4(Vec4 vec)
{
    return vecDot4(vec, vec);
}

/**
 * Compute the length of a 2D vector
 */
f32 vecLen2(Vec2 vec);

/**
 * Compute the length of a 3D vector
 */
f32 vecLen3(Vec3 vec);

/**
 * Compute the length of a 4D vector
 */
f32 vecLen4(Vec4 vec);

/**
 * Normalize a 2D vector
 *
 * Note, cannot normalize 0
 */
Vec2 vecNorm2(Vec2 vec);

/**
 * Normalize a 3D vector
 *
 * Note, cannot normalize 0
 */
Vec3 vecNorm3(Vec3 vec);

/**
 * Normalize a 4D vector
 *
 * Note, cannot normalize 0
 */
Vec4 vecNorm4(Vec4 vec);

/**
 * Compute the cross product of 2D vectors
 */
f32 vecCross2(Vec2 lhs, Vec2 rhs);

/**
 * Compute the cross product of 3D vectors
 */
Vec3 vecCross3(Vec3 lhs, Vec3 rhs);

// ----------------------------------------------------------------------------
// Matrix Functions
// ----------------------------------------------------------------------------

/**
 * Add 2x2 matrices
 */
Mat2 operator+(const Mat2& lhs, const Mat2& rhs);

/**
 * Add 3x3 matrices
 */
Mat3 operator+(const Mat3& lhs, const Mat3& rhs);

/**
 * Add 4x4 matrices
 */
Mat4 operator+(const Mat4& lhs, const Mat4& rhs);

/**
 * Subtract 2x2 matrices
 */
Mat2 operator-(const Mat2& lhs, const Mat2& rhs);

/**
 * Subtract 3x3 matrices
 */
Mat3 operator-(const Mat3& lhs, const Mat3& rhs);

/**
 * Subtract 4x4 matrices
 */
Mat4 operator-(const Mat4& lhs, const Mat4& rhs);

/**
 * Multiply 2x2 matrices
 */
Mat2 operator*(const Mat2& lhs, const Mat2& rhs);

/**
 * Multiply 3x3 matrices
 */
Mat3 operator*(const Mat3& lhs, const Mat3& rhs);

/**
 * Multiply 4x4 matrices
 */
Mat4 operator*(const Mat4& lhs, const Mat4& rhs);

/**
 * Multiply a 2x2 matrix and a 2D vector
 */
Vec2 operator*(const Mat2& lhs, Vec2 rhs);

/**
 * Multiply a 3x3 matrix and a 3D vector
 */
Vec3 operator*(const Mat3& lhs, Vec3 rhs);

/**
 * Multiply a 4x4 matrix and a 4D vector
 */
Vec4 operator*(const Mat4& lhs, Vec4 rhs);

/**
 * Transpose the matrix
 */
void matTranspose(u32 width, u32 height, f32* dst, const f32* mat);

/**
 * Transpose the matrix
 */
Mat2 matTranspose2(const Mat2& mat);

/**
 * Transpose the matrix
 */
Mat3 matTranspose3(const Mat3& mat);

/**
 * Transpose the matrix
 */
Mat4 matTranspose4(const Mat4& mat);

// ----------------------------------------------------------------------------
// Complex Functions
// ----------------------------------------------------------------------------

/**
 * Add complex numbers
 */
constexpr Complex operator+(Complex lhs, Complex rhs)
{
    return Complex{lhs.r + rhs.r, lhs.i + rhs.i};
}

/**
 * Subtract complex numbers
 */
constexpr Complex operator-(Complex lhs, Complex rhs)
{
    return Complex{lhs.r - rhs.r, lhs.i - rhs.i};
}

/**
 * Multiply complex numbers
 */
constexpr Complex operator*(Complex lhs, Complex rhs)
{
    return Complex{lhs.r * rhs.r - lhs.i * rhs.i, lhs.r * rhs.i + lhs.i * rhs.r};
}

/**
 * Compute the conjugate of a complex number
 */
constexpr Complex complexConj(Complex comp)
{
    return Complex{comp.r, -comp.i};
}

/**
 * Compute the absolute value squared of a complex number
 */
f32 complexAbsSqr(Complex comp);

/**
 * Compute the absolute value of a complex number
 */
f32 complexAbs(Complex comp);

/**
 * Normalize a complex number
 */
Complex complexNorm(Complex comp);

/**
 * Rotate a 2D vector using a complex number
 */
Vec2 vecRot2(Complex lhs, Vec2 rhs);

// ----------------------------------------------------------------------------
// Quaternion Functions
// ----------------------------------------------------------------------------

/**
 * Add quaternions
 */
constexpr Quat operator+(Quat lhs, Quat rhs)
{
    return Quat{lhs.r + rhs.r, lhs.i + rhs.i, lhs.j + rhs.j, lhs.k + rhs.k};
}

/**
 * Subtract quaternions
 */
constexpr Quat operator-(Quat lhs, Quat rhs)
{
    return Quat{lhs.r - rhs.r, lhs.i - rhs.i, lhs.j - rhs.j, lhs.k - rhs.k};
}

/**
 * Multiply quaternions
 */
Quat operator*(Quat lhs, Quat rhs);

/**
 * Compute the conjugate of a quaternion
 */
constexpr Quat quatConj(Quat quat)
{
    return Quat{quat.r, -quat.i, -quat.j, -quat.k};
}

/**
 * Return the absolute value squared of a quaternion
 */
f32 quatAbsSqr(Quat quat);

/**
 * Return the absolute value of a quaternion
 */
f32 quatAbs(Quat quat);

/**
 * Return a normalized quaternion
 */
Quat quatNorm(Quat q);

/**
 * Create a rotation quaternion from an axis and angle
 */
Quat quatAxisAngle(Vec3 axis, f32 angle);

/**
 * Create a rotation quaternion between two directions
 */
Quat quatBetween(Vec3 from, Vec3 to);

/**
 * Rotate a 3D vector using a quaternion
 */
Vec3 vecRot3(Quat lhs, Vec3 rhs);

/**
 * Rotate a 3x3 matrix using a quaternion
 */
Mat3 matRot3(Quat lhs, Mat3 rhs);

// ----------------------------------------------------------------------------
// Transform Matrices
// ----------------------------------------------------------------------------

/**
 * Creates a model matrix for 2D graphics
 *
 * Parameters
 * - position The position of the model
 * - scale The scale of the model
 * - rotation The rotation of the model in radians
 *
 * Returns
 * - The model matrix
 */
Mat4 matModel2D(Vec3 position, Vec2 scale, f32 rotation);

/**
 * Creates a model matrix for 3D graphics
 *
 * Parameters
 * - position The position of the model
 * - scale The scale of the model
 * - rotation The rotation of the model
 */
Mat4 matModel3D(const Vec3& position, const Vec3& scale, const Quat& rotation);

/**
 * Creates a view matrix
 *
 * Parameters
 * - position The position of the camera
 * - zoom The zoom of the camera
 * - rotation The rotation of the camera
 */
Mat4 matView(const Vec3& position, const Vec3& zoom, const Quat& rotation);

/**
 * Creates a view matrix from a model matrix
 *
 * Note, shearing causes distortion
 */
Mat4 matModelToView(const Mat4& model);

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
Mat4 matOrthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far);

/**
 * Creates a perspective projection matrix
 *
 * Parameters
 * - fov The field of view of the projection in radians
 * - aspect The aspect ratio of the projection
 * - near The near plane of the projection, must be greater than 0.0f
 * - far The far plane of the projection, must be greater than near
 */
Mat4 matPerspective(f32 fov, f32 aspect, f32 near, f32 far);

// ----------------------------------------------------------------------------
// Geometry 2D
// ----------------------------------------------------------------------------

/**
 * A 2D circle
 */
struct Circle {
    /**
     * The center position
     */
    Vec2 pos;
    /**
     * The radius
     */
    f32 radius;
};

/**
 * Returns whether the circle contains the point
 */
bool containsPointCircle(Vec2 point, Circle circle);

/**
 * Returns the distance between the point and the circle
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 distPointCircle(Vec2 point, Circle circle);

/**
 * Returns the closest point to pos which lies on the circle
 */
Vec2 closestPointCircle(Vec2 pos, Circle circle);

/**
 * Returns whether two circles intersect or not (includes touching)
 */
bool intersectCircles(Circle a, Circle b);

/**
 * Returns the distance squared between the circles
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 distCircles(Circle a, Circle b);

/**
 * A 2D rectangle
 */
struct Rect {
    /**
     * The origin position
     */
    Vec2 begin;
    /**
     * The extent in each dimension
     */
    Vec2 end;
};

/**
 * Returns an empty rect
 */
Rect rectEmpty();

/**
 * Expands the rect to include the point
 */
Rect rectAddPoint(Rect rect, Vec2 point);

/**
 * Returns whether the rect contains the point
 */
bool containsPointRect(Vec2 point, Rect rect);

/**
 * Returns the closest point to pos which lies on the rect
 */
Vec2 closestPointRect(Vec2 pos, Rect rect);

/**
 * Returns whether two rects intersect or not (includes touching)
 */
bool intersectRects(Rect a, Rect b);

/**
 * Returns whether a rect and a circle intersect or not (includes touching)
 */
bool intersectRectCircle(Rect rect, Circle circle);

/**
 * 2D intersection info
 */
struct Hit2D {
    /**
     * The hit distance along the ray or line
     *
     * Ray hit pos: pos + dist * dir
     * Line hit pos: begin + dist * (end - begin)
     */
    f32 dist;
    /**
     * The normal at the hit position
     */
    Vec2 normal;
};

/**
 * A 2D ray
 */
struct Ray2D {
    /**
     * The origin position
     */
    Vec2 pos;
    /**
     * The direction
     */
    Vec2 dir;
};

/**
 * A 2D line
 */
struct Line2D {
    /**
     * The begin vertex
     */
    Vec2 begin;
    /**
     * The end vertex
     */
    Vec2 end;
};

/**
 * Intersect two rays
 *
 * Parameters
 * - ray The ray to cast
 * - other The other ray to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the rays intersect
 */
bool intersectRays2D(Ray2D ray, Ray2D other, Hit2D* hit);

/**
 * Intersect a ray and a line
 *
 * Parameters
 * - ray The ray to cast
 * - line The line to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the ray and line intersect
 */
bool intersectRayLine2D(Ray2D ray, Line2D line, Hit2D* hit);

/**
 * Intersect a ray and a circle
 *
 * Parameters
 * - ray The ray to cast
 * - circle The circle to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the ray and circle intersect
 */
bool intersectRayCircle(Ray2D ray, Circle circle, Hit2D* hit);

/**
 * Intersect a ray and a rect
 *
 * Parameters
 * - ray The ray to cast
 * - rect The rect to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the ray and rect intersect
 */
bool intersectRayRect(Ray2D ray, Rect rect, Hit2D* hit);

/**
 * Intersect two lines
 *
 * Parameters
 * - line The line to cast
 * - other The other line to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the lines intersect
 */
bool intersectLines2D(Line2D line, Line2D other, Hit2D* hit);

/**
 * Intersect a line and a ray
 *
 * Parameters
 * - line The line to cast
 * - ray The ray to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the line and ray intersect
 */
bool intersectLineRay2D(Line2D line, Ray2D ray, Hit2D* hit);

/**
 * Intersect a line and a circle
 *
 * Parameters
 * - line The line to cast
 * - circle The circle to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the line and circle intersect
 */
bool intersectLineCircle(Line2D line, Circle circle, Hit2D* hit);

/**
 * Intersect a line and a rect
 *
 * Parameters
 * - line The line to cast
 * - rect The rect to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the line and rect intersect
 */
bool intersectLineRect(Line2D line, Rect rect, Hit2D* hit);

// ----------------------------------------------------------------------------
// Geometry 3D
// ----------------------------------------------------------------------------

/**
 * A 3D sphere
 */
struct Sphere {
    /**
     * The center position
     */
    Vec3 pos;
    /**
     * The radius from the center
     */
    f32 radius;
};

/**
 * Returns whether the sphere contains the point
 */
bool containsPointSphere(Vec3 point, Sphere sphere);

/**
 * Returns the distance squared between the point and the sphere
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 distPointSphere(Vec3 point, Sphere sphere);

/**
 * Returns the closest point to pos which lies on the sphere
 */
Vec3 closestPointSphere(Vec3 pos, Sphere sphere);

/**
 * Returns whether two spheres intersect or not (includes touching)
 */
bool intersectSpheres(Sphere a, Sphere b);

/**
 * Returns the distance squared between the spheres
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 distSpheres(Sphere a, Sphere b);

/**
 * A 3D box
 */
struct Box {
    /**
     * The origin position
     */
    Vec3 begin;
    /**
     * The extent in each dimension
     */
    Vec3 end;
};

/**
 * Returns an empty box
 */
Box boxEmpty();

/**
 * Expands the box to include the point
 */
Box boxAddPoint(Box box, Vec3 point);

/**
 * Returns whether the box contains the point
 */
bool containsPointBox(Vec3 point, Box box);

/**
 * Returns the closest point to pos which lies on the box
 */
Vec3 closestPointBox(Vec3 pos, Box box);

/**
 * Returns whether two boxs intersect or not (includes touching)
 */
bool intersectBox(Box a, Box b);

/**
 * Returns whether a box and a sphere intersect or not (includes touching)
 */
bool intersectBoxSphere(Box box, Sphere sphere);

/**
 * 3D intersection info
 */
struct Hit3D {
    /**
     * The hit distance along the ray or line
     *
     * Ray hit pos: pos + dist * dir
     * Line hit pos: begin + dist * (end - begin)
     */
    f32 dist;
    /**
     * The normal at the hit position
     */
    Vec3 normal;
};

/**
 * A 3D ray
 */
struct Ray3D {
    /**
     * The origin position
     */
    Vec3 pos;
    /**
     * The direction
     */
    Vec3 dir;
};

/**
 * A 3D line
 */
struct Line3D {
    /**
     * The begin vertex
     */
    Vec3 begin;
    /**
     * The end vertex
     */
    Vec3 end;
};

/**
 * A 3D triangle
 */
struct Tri {
    /**
     * The first vertex
     */
    Vec3 a;
    /**
     * The second vertex
     */
    Vec3 b;
    /**
     * The third vertex
     */
    Vec3 c;
};

/**
 * A 3D plane
 */
struct Plane {
    /**
     * The plane's normal
     */
    Vec3 normal;
    /**
     * The distance in the direction of the normal
     */
    f32 dist;
};

/**
 * Create a plane at the point
 */
Plane planeFromPoint(Vec3 point, Vec3 normal);

/**
 * Create a plane from a triangle
 */
Plane planeFromTri(Tri tri);

/**
 * Intersect a ray and a sphere
 *
 * Parameters
 * - ray The ray to cast
 * - sphere The sphere to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the ray and sphere intersect
 */
bool intersectRaySphere(Ray3D ray, Sphere sphere, Hit3D* hit);

/**
 * Intersect a ray and a box
 *
 * Parameters
 * - ray The ray to cast
 * - box The box to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the ray and box intersect
 */
bool intersectRayBox(Ray3D ray, Box box, Hit3D* hit);

/**
 * Intersect a ray and a triangle
 *
 * Parameters
 * - ray The ray to cast
 * - triangle The triangle to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the ray and triangle intersect
 */
bool intersectRayTri(Ray3D ray, Tri tri, Hit3D* hit);

/**
 * Intersect a ray and a plane
 *
 * Parameters
 * - ray The ray to cast
 * - plane The plane to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the ray and plane intersect
 */
bool intersectRayPlane(Ray3D ray, Plane plane, Hit3D* hit);

/**
 * Intersect a line and a sphere
 *
 * Parameters
 * - line The line to cast
 * - sphere The sphere to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the line and sphere intersect
 */
bool intersectLineSphere(Line3D line, Sphere sphere, Hit3D* hit);

/**
 * Intersect a line and a box
 *
 * Parameters
 * - line The line to cast
 * - box The box to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the line and box intersect
 */
bool intersectLineBox(Line3D line, Box box, Hit3D* hit);

/**
 * Intersect a line and a triangle
 *
 * Parameters
 * - line The line to cast
 * - triangle The triangle to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the line and triangle intersect
 */
bool intersectLineTri(Line3D line, Tri tri, Hit3D* hit);

/**
 * Intersect a line and a plane
 *
 * Parameters
 * - line The line to cast
 * - plane The plane to intersect
 * - hit A pointer to store the hit data, or nullptr
 *
 * Returns
 * - Whether the line and plane intersect
 */
bool intersectLinePlane(Line3D line, Plane plane, Hit3D* hit);

// ----------------------------------------------------------------------------
// Noise & RNG
// ----------------------------------------------------------------------------

/**
 * Generate white noise
 */
u32 noise(u32 seed, u32 pos);

/**
 * Generate white noise
 */
u32 noise2D(u32 seed, u32 x, u32 y);

/**
 * Generate white noise
 */
u32 noise3D(u32 seed, u32 x, u32 y, u32 z);

/**
 * Generate white noise
 */
u32 noise4D(u32 seed, u32 x, u32 y, u32 z, u32 w);

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
f32 noiseNorm(u32 seed, f32 pos);

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
f32 noiseNorm2D(u32 seed, Vec2 pos);

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
f32 noiseNorm3D(u32 seed, Vec3 pos);

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
f32 noiseNorm4D(u32 seed, Vec4 pos);

/**
 * Generate white noise unit vector
 */
f32 noiseVec1D(u32 seed, f32 pos);

/**
 * Generate white noise unit vector
 */
Vec2 noiseVec2D(u32 seed, Vec2 pos);

// value and gradient noise : TODO

/**
 * Get a true random number from hardware
 */
u32 trueRandom();

/**
 * A pseudo random number generator
 */
struct Rng {
    u32 seed = 0;
    u32 pos = 0;
};

/**
 * Set the rng seed
 */
void rngSeed(Rng* rng, u32 seed);

/**
 * Get the next random value
 */
u32 rngNext(Rng* rng);

/**
 * Get the next 64 bit random value
 */
u64 rngNext64(Rng* rng);

// sort algorithm : TODO

// ============================================================================
// Binary Builder
// ============================================================================

/**
 * A binary builder
 */
struct BinaryBuilder {
    /**
     * The data
     */
    void* data = nullptr;
    /**
     * The size of data in bytes
     */
    u64 size = 0;

    /**
     * Implicitly convert to Binary
     */
    constexpr operator BinaryView()
    {
        return {data, size};
    }
};

/**
 * Resize the binary
 *
 * Parameters
 * - arena The arena to allocate from
 * - newSize The new size of the file in bytes
 */
void binaryResize(Arena* arena, BinaryBuilder* bin, u64 newSize);

/**
 * Overwrite data at the index
 *
 * Parameters
 * - idx The index into the file to overwrite
 * - src The data to write
 * - size The size of the data in bytes
 */
void binaryOverwrite(BinaryBuilder* bin, u64 idx, const void* src, u64 len);

/**
 * Overwrite data of arbitrary type at the index
 *
 * Parameters
 * - idx The index into the file to overwrite
 * - src The data to write
 */
template<typename T>
void binaryOverwrite(BinaryBuilder* bin, u64 idx, const T& src)
{
    binaryOverwrite(bin, idx, &src, sizeof(T));
}

// ============================================================================
// String Utilities
// ============================================================================

/**
 * Compare strings
 */
inline bool operator==(StringView lhs, StringView rhs)
{
    return lhs.length == rhs.length && memEqual(lhs.chars, rhs.chars, lhs.length);
}

/**
 * Compare strings
 */
inline bool operator!=(StringView lhs, StringView rhs)
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
char* cString(Arena* arena, StringView str);

/**
 * Create a new owning string
 */
StringView stringCreate(StringView data);

/**
 * Destroy an owning string
 */
void stringDestroy(StringView* str);

/**
 * A string builder using arenas
 */
struct StringBuilder {
    /**
     * The string data
     */
    char* chars = nullptr;
    /**
     * The number of characters currently in the string
     */
    u64 length = 0;

    /**
     * Access using the index operator
     */
    constexpr char& operator[](u64 index) const
    {
        HG_ASSERT(index < length);
        return chars[index];
    }

    /**
     * Implicit converts to a string view
     */
    constexpr operator StringView() const
    {
        return {chars, length};
    }
};

/**
 * Compare string builders
 */
inline bool operator==(const StringBuilder& lhs, const StringBuilder& rhs)
{
    return StringView{lhs} == StringView{rhs};
}

/**
 * Compare string builders
 */
inline bool operator!=(const StringBuilder& lhs, const StringBuilder& rhs)
{
    return !(lhs == rhs);
}

/**
 * Creates a new string copied from an existing string
 *
 * Parameters
 * - arena The arena to allocate from
 * - init The initial string to copy from
 */
StringBuilder stringCopy(Arena* arena, StringView str);

/**
 * Create a formatted string : TODO
 */
// template<typename... Ts>
// StringBuilder stringFormat(Arena* arena, String fmt, Ts... args);

/**
 * Copies another string into the string at index
 *
 * Parameters
 * - arena The arena to allocate from
 * - dst The string to insert into
 * - idx The index into dst
 * - src The string to copy from
 */
void stringInsert(Arena* arena, StringBuilder* dst, u64 idx, StringView src);

/**
 * Copies another string to the end of the string
 */
inline void stringAppend(Arena* arena, StringBuilder* dst, StringView src)
{
    stringInsert(arena, dst, dst->length, src);
}

/**
 * Copies another string to the beginning of the string
 */
inline void stringPrepend(Arena* arena, StringBuilder* dst, StringView src)
{
    stringInsert(arena, dst, 0, src);
}

/**
 * Copies a character into the string at index
 *
 * Parameters
 * - arena The arena to allocate from
 * - dst The string to insert into
 * - idx The index into dst
 * - c The character to insert
 */
inline void stringInsertC(Arena* arena, StringBuilder* dst, u64 idx, char c)
{
    stringInsert(arena, dst, idx, {&c, 1});
}

/**
 * Copies another string to the end of the string
 */
inline void stringAppendC(Arena* arena, StringBuilder* dst, char c)
{
    stringInsertC(arena, dst, dst->length, c);
}

/**
 * Copies another string to the beginning of the string
 */
inline void stringPrependC(Arena* arena, StringBuilder* dst, char c)
{
    stringInsertC(arena, dst, 0, c);
}

/**
 * Check whether a character is whitespace (space, tab, or newline)
 */
bool isWhitespace(char c);

/**
 * Check whether a character is a base 10 numeral (0-9)
 */
bool isNumeral(char c);

/**
 * Check whether a string is a base 10 integer
 */
bool isInteger(StringView str);

/**
 * Check whether a string is a base 10 floating point number
 */
bool isFloat(StringView str);

/**
 * Create an integer from a base 10 string
 */
i64 stringToInteger(StringView str);

/**
 * Create a float from a base 10 string
 */
f64 stringToFloat(StringView str);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 */
StringBuilder integerToString(Arena* arena, i64 num);

/**
 * Create a base 10 string from an integer
 *
 * Parameters
 * - arena The arena to allocate from
 * - num The integer number to create from
 * - decimalCount The number of trailing decimal digits
 */
StringBuilder floatToString(Arena* arena, f64 num, u32 decimalCount);

// base 2 and 16 string-int conversions : TODO
// arbitrary base string-int conversions : TODO?

// ============================================================================
// Serialization
// ============================================================================

/**
 * The primitive serializable types
 */
enum SerialType : u32 {
    /**
     * An object with fields
     */
    SerialType_object,
    /**
     * String data
     */
    SerialType_string,
    /**
     * An integer value
     */
    SerialType_integer,
    /**
     * A floating point value
     */
    SerialType_floating,
    /**
     * A boolean value
     */
    SerialType_boolean,
};

/**
 * Stringify SerialType
 */
const char* serialTypeToString(SerialType s);

/**
 * A serialized data node
 */
struct SerialNode {
    /**
     * The parent object or array
     */
    SerialNode* parent = nullptr;
    /**
     * The next node in the object or array
     */
    SerialNode* next = nullptr;
    /**
     * The type of data
     */
    SerialType type = {};
    /**
     * The number of fields, if an object
     */
    u32 count = 0;
    /**
     * The data
     */
    union {
        /**
         * The fields of an object
         */
        SerialNode* children;
        /**
         * String data
         */
        StringView string;
        /**
         * Integer value
         */
        i64 integer;
        /**
         * Floating point value
         */
        f64 floating;
        /**
         * Boolean value
         */
        bool boolean;
    };
};

/**
 * The data for serialization
 */
struct Serializer {
    /**
     * The arena to allocate from
     */
    Arena* arena = nullptr;
    /**
     * The root node
     */
    SerialNode* root = nullptr;
    /**
     * The current object
     */
    SerialNode* parent = nullptr;
    /**
     * The current data node
     */
    SerialNode* current = nullptr;
    /**
     * Whether the serializer is reading or writing
     */
    bool writing = false;
};

/**
 * Begin a serial writer
 */
Serializer serialWriter(Arena* arena);

/**
 * Begin a serial reader
 */
Serializer serialReader(Arena* arena, SerialNode* begin);

/**
 * The preamble to serializing a node, generally not needed
 */
void serializeNodeStart(Serializer* s);

/**
 * Begin serializing an object or array
 */
void serializeBegin(Serializer* s, u32* size = nullptr);

/**
 * Begin serializing an object or array
 */
void serializeEnd(Serializer* s);

/**
 * Serialize a value of unknown type
 */
void serializeVoid(Serializer* s, void* val, u32 size);

/**
 * Serialize a value, should be overridden
 */
template<typename T>
void serialize(Serializer* s, T* val);

/**
 * Serialize an object conveniently
 */
template<typename... Ts>
void serializeObject(Serializer* s, Ts*... vals);

/**
 * Serialize an array of values
 */
template<typename T, u64 N>
void serialize(Serializer* s, T (*arr)[N]);

/**
 * Binary serialization
 */
template<>
void serialize(Serializer* s, BinaryView* val);

/**
 * String serialization
 */
template<>
void serialize(Serializer* s, StringView* val);

/**
 * StringBuilder serialization
 */
template<>
void serialize(Serializer* s, StringBuilder* val);

/**
 * u8 serialization
 */
template<>
void serialize(Serializer* s, u8* val);

/**
 * u16 serialization
 */
template<>
void serialize(Serializer* s, u16* val);

/**
 * u32 serialization
 */
template<>
void serialize(Serializer* s, u32* val);

/**
 * u64 serialization
 */
template<>
void serialize(Serializer* s, u64* val);

/**
 * i8 serialization
 */
template<>
void serialize(Serializer* s, i8* val);

/**
 * i16 serialization
 */
template<>
void serialize(Serializer* s, i16* val);

/**
 * i32 serialization
 */
template<>
void serialize(Serializer* s, i32* val);

/**
 * i64 serialization
 */
template<>
void serialize(Serializer* s, i64* val);

/**
 * f32 serialization
 */
template<>
void serialize(Serializer* s, f32* val);

/**
 * f64 serialization
 */
template<>
void serialize(Serializer* s, f64* val);

/**
 * bool serialization
 */
template<>
void serialize(Serializer* s, bool* val);

/**
 * Vec2 serialization
 */
template<>
void serialize(Serializer* s, Vec2* val);

/**
 * Vec3 serialization
 */
template<>
void serialize(Serializer* s, Vec3* val);

/**
 * Vec4 serialization
 */
template<>
void serialize(Serializer* s, Vec4* val);

/**
 * Mat2 serialization
 */
template<>
void serialize(Serializer* s, Mat2* val);

/**
 * Mat3 serialization
 */
template<>
void serialize(Serializer* s, Mat3* val);

/**
 * Mat4 serialization
 */
template<>
void serialize(Serializer* s, Mat4* val);

/**
 * Complex serialization
 */
template<>
void serialize(Serializer* s, Complex* val);

/**
 * Quat serialization
 */
template<>
void serialize(Serializer* s, Quat* val);

/**
 * Write serialized data in a binary format
 */
BinaryView binaryWriteSerial(Arena* arena, Serializer* data);

/**
 * Read binary data to be deserialized
 */
Serializer binaryReadSerial(Arena* arena, BinaryView bin);

/**
 * Write serialized data as json
 */
StringView jsonWriteSerial(Arena* arena, Serializer* data);

// /**
//  * Read json data to be deserialized : TODO
//  */
// Serializer jsonReadSerial(Arena* arena, StringView json);

/**
 * An error contained in the json
 */
struct JsonError {
    /**
     * The next error
     */
    JsonError* next = nullptr;
    /**
     * The error message
     */
    StringView msg = {};
};

/**
 * A node in the json file
 */
struct JsonNode;

/**
 * The types contained in nodes
 */
enum JsonType : u32 {
    JsonType_none = 0,
    JsonType_struct,
    JsonType_field,
    JsonType_array,
    JsonType_string,
    JsonType_float,
    JsonType_integer,
    JsonType_bool,
};

/**
 * A field in a struct
 */
struct JsonField {
    /**
     * The next field
     */
    JsonField* next = nullptr;
    /**
     * The name of the field
     */
    StringView name = {};
    /**
     * The value stored in the field
     */
    JsonNode* value = nullptr;
};

/**
 * A struct contained in the json
 */
struct JsonStruct {
    /**
     * The first field
     */
    JsonField* fields = nullptr;
};

/**
 * An element in an array
 */
struct JsonElem {
    /**
     * The next element
     */
    JsonElem* next = nullptr;
    /**
     * The value stored in the element
     */
    JsonNode* value = nullptr;
};

/**
 * An array contained in the json
 */
struct JsonArray {
    /**
     * The first element
     */
    JsonElem* elems = nullptr;
};

/**
 * A node in the json file
 */
struct JsonNode {
    /**
     * The node's type
     */
    JsonType type = {};
    /**
     * The value in the node
     */
    union {
        JsonStruct jstruct;
        JsonField field;
        JsonArray array;
        StringView string;
        f64 floating;
        i64 integer;
        bool boolean;
    };
};

/**
 * A parsed Json file
 */
struct Json {
    /**
     * The successfully parsed nodes
     */
    JsonNode* file = nullptr;
    /**
     * The errors found
     */
    JsonError* errors = nullptr;
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
Json parseJson(Arena* arena, StringView text);

// ============================================================================
// Containers
// ============================================================================

/**
 * A dynamic array
 */
template<typename T>
struct Array {
    /**
     * The values stored
     */
    T* vals;
    /**
     * The number of vals
     */
    u32 count;
    /**
     * The current max number of vals
     */
    u32 capacity;

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr T& operator[](u64 idx) const
    {
        HG_ASSERT(vals != nullptr);
        HG_ASSERT(idx < count);
        return vals[idx];
    }
};

/**
 * Array serialization
 */
template<typename T>
void serialize(Serializer* s, Array<T>* arr);

/**
 * Create an array
 */
template<typename T>
Array<T> arrayCreate(u32 count = 0, u32 capacity = 1024);

/**
 * Destroy an array
 */
template<typename T>
void arrayDestroy(Array<T>* arr);

/**
 * Create a temporary array which need not be destroyed, but cannot be resized
 */
template<typename T>
Array<T> arrayTemp(Arena* arena, u32 count = 0, u32 capacity = 1024);

/**
 * Resize an array
 */
template<typename T>
void arrayResize(Array<T>* arr, u32 newCount);

/**
 * Resize an array using an arena
 */
template<typename T>
void arrayResizeTemp(Arena* arena, Array<T>* arr, u32 newCount);

/**
 * Push a value to the end of the array
 */
template<typename T>
T* arrayPush(Array<T>* arr);

/**
 * Push a value to the end of the array using an arena
 */
template<typename T>
T* arrayPushTemp(Arena* arena, Array<T>* arr);

/**
 * Remove the value at idx in the array, with stanle order
 */
template<typename T>
T arrayRemove(Array<T>* arr, u32 idx);

/**
 * Remove the value at idx in the array, without stable order
 */
template<typename T>
T arrayRemoveSwap(Array<T>* arr, u32 idx);

/**
 * Pop a value from the end of the array
 */
template<typename T>
T arrayPop(Array<T>* arr);

/**
 * An array of values of unknown type
 */
struct ArrayAny {
    /**
     * The values stored
     */
    void* vals = nullptr;
    /**
     * The number of vals
     */
    u32 count = 0;
    /**
     * The current max number of vals
     */
    u32 capacity = 0;
    /**
     * The width of each val in bytes
     */
    u32 width = 0;
    /**
     * The alignment of each val in bytes
     */
    u32 align = 0;

    /**
     * Convenience to index into the array with debug bounds checking
     */
    constexpr void* operator[](u64 idx) const
    {
        HG_ASSERT(vals != nullptr);
        HG_ASSERT(idx < count);
        return static_cast<u8*>(vals) + idx * width;
    }
};

/**
 * ArrayAny serialization
 */
template<>
void serialize(Serializer* s, ArrayAny* arr);

/**
 * Create an array of unknown type
 */
ArrayAny arrayAnyCreate(u32 width, u32 align, u32 count = 0, u32 capacity = 1024);

/**
 * Destroy an array of unknown type
 */
void arrayAnyDestroy(ArrayAny* arr);

/**
 * Create a temporary array which need not be destroyed, but cannot be resized
 */
ArrayAny arrayAnyTemp(Arena* arena, u32 width, u32 align, u32 count = 0, u32 capacity = 1024);

/**
 * Resize an array of unknown type
 */
void arrayAnyResize(ArrayAny* arr, u32 newCount);

/**
 * Resize an array of unkown type using an arena
 */
void arrayAnyResizeTemp(Arena* arean, ArrayAny* arr, u32 newCount);

/**
 * Push a value to the end of the array
 */
void* arrayAnyPush(ArrayAny* arr);

/**
 * Push a value to the end of the array using an arena
 */
void* arrayAnyPushTemp(Arena* arena, ArrayAny* arr);

/**
 * Remove a value from the array, with stable order
 */
void arrayAnyRemove(ArrayAny* arr, u32 idx, void* dst);

/**
 * Remove a value from the array, without stable order
 */
void arrayAnyRemoveSwap(ArrayAny* arr, u32 idx, void* dst);

/**
 * Pop a value from the end of the array
 */
void arrayAnyPop(ArrayAny* arr, void* dst);

/**
 * A double ended ring buffer queue
 */
template<typename T>
struct Queue {
    /**
     * The values in the queue
     */
    T* vals;
    /**
     * The index of the front
     */
    u32 front;
    /**
     * The index of the back
     */
    u32 back;
    /**
     * The number of vals in the queue
     */
    u32 count;
    /**
     * The max number of vals
     */
    u32 capacity;
};

/**
 * Create a new empty queue
 */
template<typename T>
Queue<T> queueCreate(u32 capacity = 1024);

/**
 * Destroy a queue
 */
template<typename T>
void queueDestroy(Queue<T>* queue);

/**
 * Push a value to the front of the queue
 */
template<typename T, typename U = T>
void queuePushFront(Queue<T>* queue, U val);

/**
 * Push a value to the back of the queue
 */
template<typename T, typename U = T>
void queuePushBack(Queue<T>* queue, U val);

/**
 * Pop a value from the front of the queue
 */
template<typename T>
T queuePopFront(Queue<T>* queue);

/**
 * Pop a value from the back of the queue
 */
template<typename T>
T queuePopBack(Queue<T>* queue);

/**
 * The hash template
 */
template<typename T>
constexpr u64 hash(T)
{
    static_assert(false, "hash must be implemented for each type");
    return 0;
}

/**
 * A hash set
 */
template<typename V>
struct Set {
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
 * Set serialization
 */
template<typename V>
void serialize(Serializer* s, Set<V>* set);

/**
 * Creates a new hash set
 *
 * Parameters
 * - slotCount The max number of slots to store values in
 */
template<typename V>
Set<V> setCreate(u32 slotCount);

/**
 * Destroy a hash set
 */
template<typename V>
void setDestroy(Set<V>* set);

/**
 * Create a new hash set which need not be destroyed, but cannot be resized
 *
 * Parameters
 * - arena The arena to allocate from
 * - slotCount The max number of slots to store values in
 */
template<typename V>
Set<V> setTemp(Arena* arena, u32 slotCount);

/**
 * Resize the set
 */
template<typename V>
void setResize(Set<V>* set, u32 newSize);

/**
 * Empty all slots
 */
template<typename V>
void setReset(Set<V>* set);

/**
 * Add a value to the set
 */
template<typename V, typename T = V>
void setAdd(Set<V>* set, const T& val);

/**
 * Remove a value from the set
 */
template<typename V, typename T = V>
void setRemove(Set<V>* set, const T& val);

/**
 * Check whether a value is contained in the set
 */
template<typename V, typename T = V>
bool setHas(const Set<V>* set, const T& val);

/**
 * Calls a function for each value in the hash map
 */
template<typename V, typename F>
void setForEach(Set<V>* set, F fn);

/**
 * A key-value hash map
 */
template<typename K, typename V>
struct Map {
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
 * Map serialization
 */
template<typename K, typename V>
void serialize(Serializer* s, Map<K, V>* set);

/**
 * Create a new hash map
 *
 * Parameters
 * - slotCount The max number of slots to store values in
 */
template<typename K, typename V>
Map<K, V> mapCreate(u32 slotCount = 1024);

/**
 * Destroy a hash map
 */
template<typename K, typename V>
void mapDestroy(Map<K, V>* map);

/**
 * Create a new hash map which need not be destroyed, but cannot be resized
 *
 * Parameters
 * - arena The arena to allocate from
 * - slotCount The max number of slots to store values in
 */
template<typename K, typename V>
Map<K, V> mapTemp(Arena* arena, u32 slotCount);

/**
 * Resize a hash map
 */
template<typename K, typename V>
void mapResize(Map<K, V>* map, u32 newSize);

/**
 * Empties all slots
 */
template<typename K, typename V>
void mapReset(Map<K, V>* map);

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
V* mapAdd(Map<K, V>* map, const T& key, const U& val);

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
bool mapRemove(Map<K, V>* map, const T& key, V* val = nullptr);

/**
 * Gets the value stored at a key
 *
 * Returns
 * - A pointer to the value, or nullptr if it does not exist
 */
template<typename K, typename V, typename T = K>
V* mapGet(const Map<K, V>* map, const T& key);

/**
 * Calls a function for each value in the hash map
 */
template<typename K, typename V, typename F>
void mapForEach(Map<K, V>* map, F fn);

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
    union {
        f32 asFloat;
        u32 asHash;
    } u{};
    u.asFloat = val;
    return static_cast<u64>(u.asHash);
}

/**
 * Hash map hashing for f64
 */
template<>
constexpr u64 hash(f64 val)
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
constexpr u64 hashPtr(T* val)
{
    union {
        T* asPtr;
        uptr asUptr;
    } u{};
    u.asPtr = val;
    return static_cast<u64>(u.asUptr);
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

/**
 * A pool of objects
 */
struct Pool {
    /**
     * The free list
     */
    Queue<void*> freeList = {};
    /**
     * The items in the pool
     */
    Array<void*> itemStores = {};
    /**
     * The size of each item in bytes
     */
    u32 width = 0;
    /**
     * The alignment of each item in bytes
     */
    u32 align = 0;
};

/**
 * Create an object pool
 */
Pool poolCreate(u32 width, u32 align);

/**
 * Create an object pool
 */
template<typename T>
Pool poolCreate()
{
    return poolCreate(sizeof(T), alignof(T));
}

/**
 * Destroy an object pool
 */
void poolDestroy(Pool* pool);

/**
 * Allocate an object from the pool
 */
void* poolAlloc(Pool* pool);

/**
 * Free an object from the pool
 */
void poolFree(Pool* pool, void* item);

/**
 * A generation counted handle
 */
struct Handle {
    /**
     * The handle id
     */
    u32 id = 0;
};

/**
 * The null handle
 */
static constexpr Handle handleNull = Handle{0};

/**
 * Compare handles
 */
constexpr bool operator==(Handle lhs, Handle rhs)
{
    return lhs.id == rhs.id;
}

/**
 * Compare handles
 */
constexpr bool operator!=(Handle lhs, Handle rhs)
{
    return lhs.id != rhs.id;
}

/**
 * Hash map hashing for Handle
 */
template<>
constexpr u64 hash(Handle val)
{
    return hash(val.id);
}

/**
 * The number of bits in a handle used for the index
 */
static constexpr u32 handleIdxBits = 24;

/**
 * Get the index from a handle
 */
constexpr u32 handleIdx(Handle handle)
{
    return handle.id & ((1 << handleIdxBits) - 1);
}

/**
 * Get the generation from a handle
 */
constexpr u32 handleGeneration(Handle handle)
{
    return handle.id & ~(((u32)1 << handleIdxBits) - (u32)1);
}

/**
 * Returns a new handle at the same index
 */
constexpr Handle handleNextGeneration(Handle handle)
{
    return {handle.id + (1 << handleIdxBits)};
}

/**
 * A handle pool
 */
struct HandlePool {
    /**
     * The currently active handles, or null in vacant slots
     */
    Array<Handle> handles = {};
    /**
     * The freed handles
     */
    Array<Handle> freed = {};
};

/**
 * Create a new object pool
 */
HandlePool handlePoolCreate();

/**
 * Destroy a handle pool
 */
void handlePoolDestroy(HandlePool* pool);

/**
 * Reset a handle pool
 */
void handlePoolReset(HandlePool* pool);

/**
 * Allocate an index from the pool
 */
Handle handlePoolAlloc(HandlePool* pool);

/**
 * Returns whether a handle is alive in the pool
 */
bool handlePoolAlive(HandlePool* pool, Handle handle);

/**
 * Free an index back into a pool
 *
 * Note, the object handle must be valid and alive
 */
void handlePoolFree(HandlePool* pool, Handle handle);

// ============================================================================
// Asset System
// ============================================================================

/**
 * Initialize all default asset types
 *
 * Asset types:
 * - Binary
 * - Texture
 * - GpuTexture
 * - Mesh
 * - GpuMesh
 * - Audio
 */
void assetInitDefaults();

/**
 * Deinitialize all default asset types
 */
void assetDeinitDefaults();

/**
 * The data associated with assets
 */
template<typename T>
struct Asset {
    /**
     * The asset data
     */
    T asset;
    /**
     * The reference count
     */
    u32 refCount;
    /**
     * The unique path for caching
     */
    StringView path;
};

/**
 * An asset manager
 */
template<typename T>
struct AssetManager {
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>);

    /**
     * The asset lookup
     */
    Map<StringView, Asset<T>*> map;
    /**
     * The asset pool
     */
    Pool pool;
};

/**
 * Global per type asset managers
 */
template<typename T>
inline AssetManager<T> assets{};

/**
 * Initialize a type's asset manager
 */
template<typename T>
void assetInit();

/**
 * Deinitialize an type's asset manager
 */
template<typename T>
void assetDeinit();

/**
 * Load an asset, implemented per asset type, should be blocking
 */
template<typename T>
void assetLoadImpl(Asset<T>* data);

/**
 * Unload an asset, implemented per asset type, should be blocking
 */
template<typename T>
void assetUnloadImpl(Asset<T>* data);

/**
 * Create a unique empty asset (does not load)
 */
template<typename T>
Asset<T>* assetCreate();

/**
 * Load an asset (or increment the ref count)
 */
template<typename T>
Asset<T>* assetLoad(StringView path);

/**
 * Destroy an asset and unload it (or decrement the ref count)
 */
template<typename T>
void assetUnload(Asset<T>* asset);

/**
 * Hot reload an asset
 */
template<typename T>
void assetReload(Asset<T>* asset);

/**
 * Increment the asset's reference count
 */
template<typename T>
Asset<T>* assetCopy(Asset<T>* asset);

/**
 * Asset serialization
 */
template<typename T>
void serialize(Serializer* s, Asset<T>** asset);

/**
 * A binary file asset handle
 */
using BinaryAsset = Asset<BinaryView>;

/**
 * Binary asset load implementation
 */
template<>
void assetLoadImpl(Asset<BinaryView>* data);

/**
 * Binary asset unload implementation
 */
template<>
void assetUnloadImpl(Asset<BinaryView>* data);

/**
 * Store a binary file to disc
 *
 * Returns
 * - Whether the write succeeded
 */
bool binaryStore(BinaryView bin, StringView path);

// ============================================================================
// Timing
// ============================================================================

/**
 * A high precision clock for timers and game deltas
 */
struct Clock {
    /**
     * The begin time
     */
    f64 time = 0.0;
};

/**
 * Resets the clock
 *
 * Returns
 * - The time in seconds since the last tick
 */
f64 clockTick(Clock* clock);

/**
 * Put this thread to sleep
 *
 * Parameters
 * - time The time in seconds to sleep for
 */
void sleep(f64 time);

/**
 * A simple performance measurement tool
 */
struct Perf {
    /**
     * The clock to keep track of each time
     */
    Clock clock = {};
    /**
     * The measured time for each iteration
     */
    f64* times = nullptr;
    /**
     * The max number of measurements
     */
    u32 count = 0;
    /**
     * The current measurement
     */
    u32 current = 0;
};

/**
 * Create a performance measurer
 */
Perf perfCreate(Arena* arena, u32 count);

/**
 * Begin the timer for a measurement
 */
void perfBegin(Perf* perf);

/**
 * End the timer for a measurement
 *
 * Returns
 * - The time this measurement took
 */
f64 perfEnd(Perf* perf);

/**
 * A set of statistics from performance measurements
 */
struct PerfStats {
    /**
     * The average time of all measurements
     */
    f64 avg = 0.0;
    /**
     * The best case (the shortest time)
     */
    f64 best = 0.0;
    /**
     * The worst case (the longest time)
     */
    f64 worst = 0.0;
};

/**
 * Analyzes the performance measurements for statistics
 */
PerfStats perfAnalyze(const Perf* perf);

/**
 * The scale to log performance metrics at
 */
enum PerfScale : u32 {
    PerfScale_seconds,
    PerfScale_milli,
    PerfScale_micro,
    PerfScale_nano,
};

/**
 * Logs performance statistics to stdout
 */
void perfLog(StringView title, const PerfStats* stats, PerfScale scale);

// ============================================================================
// Dynamic Library
// ============================================================================

/**
 * A dynamically loaded library
 */
struct Library;

/**
 * Load a dynamic library
 *
 * Returns
 * - The loaded library, or nullptr on failure
 */
Library* libraryLoad(StringView path);

/**
 * Unload a dynamic library
 */
void libraryUnload(Library* lib);

/**
 * Find a function from a dynamic library
 *
 * Parameters
 * - lib The dynamic library to load from
 * - path The symbol of the function to load
 *
 * Returns
 * - A function pointer to the found symbol, or nullptr not found
 */
void* libraryFindFunction(Library* lib, StringView symbol);

// ============================================================================
// Windowing
// ============================================================================

/**
 * Initialize the windowing subsystem
 */
void initWindowing();

/**
 * Deinitialize the windowing subsystem
 */
void deinitWindowing();

/**
 * The present mode for the swapchain
 */
enum GpuPresentMode : u32 {
    GpuPresentMode_immediate = 0,
    GpuPresentMode_mailbox = 1,
    GpuPresentMode_fifo = 2,
    GpuPresentMode_fifoRelaxed = 3,
};

/**
 * Configuration for a window
 */
struct WindowConfig {
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
    GpuPresentMode preferredPresentMode = GpuPresentMode_fifo;
    /**
     * How the swapchain images will be used
     */
    GpuImageUsageFlags imageUsage = GpuImageUsage_colorAttachment;
};

/**
 * A window
 */
struct Window;

/**
 * Create a new window
 *
 * Note, width and height are ignored if fullscreen is enabled
 *
 * Returns
 * - The created window, or nullptr on failure
 */
Window* windowCreate(StringView title, u32 width, u32 height, const WindowConfig* config);

/**
 * Destroy a window
 */
void windowDestroy(Window* window);

/**
 * Acquire an image from each swapchain and begin a command buffer
 *
 * Returns
 * - The command buffer to record this frame
 */
GpuCmd* gpuFrameBegin(Window** windows, u32 windowCount);

/**
 * Finishes recording the command buffer and presents the window images
 *
 * Parameters
 * - cmd The command buffer given from beginFrame
 */
void gpuFrameEnd(GpuCmd* cmd);

/**
 * Returns the window's current image, or nullptr if unavailable this frame
 */
GpuView* windowImageView(Window* window);

/**
 * Returns the window's width in pixels
 */
Format windowImageFormat(Window* window);

// ============================================================================
// Input
// ============================================================================

/**
 * Processes all events since startup or the last call to process events
 */
void processEvents();

/**
 * The types of events
 */
enum WindowEventType : u32 {
    WindowEventType_none = 0,
    WindowEventType_buttonPress,
    WindowEventType_buttonRelease,
    WindowEventType_count,
};

/**
 * The button inputs
 */
enum Button : u32 {
    Button_none = 0,
    Button_k0,
    Button_k1,
    Button_k2,
    Button_k3,
    Button_k4,
    Button_k5,
    Button_k6,
    Button_k7,
    Button_k8,
    Button_k9,
    Button_q,
    Button_w,
    Button_e,
    Button_r,
    Button_t,
    Button_y,
    Button_u,
    Button_i,
    Button_o,
    Button_p,
    Button_a,
    Button_s,
    Button_d,
    Button_f,
    Button_g,
    Button_h,
    Button_j,
    Button_k,
    Button_l,
    Button_z,
    Button_x,
    Button_c,
    Button_v,
    Button_b,
    Button_n,
    Button_m,
    Button_semicolon,
    Button_colon,
    Button_apostrophe,
    Button_quotation,
    Button_comma,
    Button_period,
    Button_question,
    Button_grave,
    Button_tilde,
    Button_exclamation,
    Button_at,
    Button_hash,
    Button_dollar,
    Button_percent,
    Button_carot,
    Button_ampersand,
    Button_asterisk,
    Button_lparen,
    Button_rparen,
    Button_lbracket,
    Button_rbracket,
    Button_lbrace,
    Button_rbrace,
    Button_equal,
    Button_less,
    Button_greater,
    Button_plus,
    Button_minus,
    Button_slash,
    Button_backslash,
    Button_underscore,
    Button_bar,
    Button_up,
    Button_down,
    Button_left,
    Button_right,
    Button_mouse1,
    Button_mouse2,
    Button_mouse3,
    Button_mouse4,
    Button_mouse5,
    Button_lmouse = Button_mouse1,
    Button_rmouse = Button_mouse2,
    Button_mmouse = Button_mouse3,
    Button_escape,
    Button_space,
    Button_enter,
    Button_backspace,
    Button_kdelete,
    Button_insert,
    Button_tab,
    Button_home,
    Button_end,
    Button_f1,
    Button_f2,
    Button_f3,
    Button_f4,
    Button_f5,
    Button_f6,
    Button_f7,
    Button_f8,
    Button_f9,
    Button_f10,
    Button_f11,
    Button_f12,
    Button_lshift,
    Button_rshift,
    Button_lctrl,
    Button_rctrl,
    Button_lmeta,
    Button_rmeta,
    Button_lalt,
    Button_ralt,
    Button_lsuper,
    Button_rsuper,
    Button_capslock,
    Button_count,
};

/**
 * A button input event
 */
struct WindowButtonEvent {
    /**
     * The type of event
     */
    WindowEventType type = {};
    /**
     * The button which was pressed or released
     */
    Button button = {};
};

/**
 * Input event data
 */
union WindowEvent {
    /**
     * The type of event
     */
    WindowEventType type;
    /**
     * The button press or release event
     */
    WindowButtonEvent button;
};

/**
 * Returns whether the application was quit
 */
bool wasQuit();

/**
 * Returns whether the window was closed
 */
bool windowWasClosed(Window* window);

/**
 * Returns whether the mouse is focused on the window
 */
bool windowIsFocused(Window* window);

/**
 * Get the window's width in pixels
 */
u32 windowWidth(Window* window);

/**
 * Get the window's width in pixels
 */
u32 windowHeight(Window* window);

/**
 * Returns the current x position of the mouse relative to the window
 */
f32 mouseX(Window* window);

/**
 * Returns the current y position of the mouse relative to the window
 */
f32 mouseY(Window* window);

/**
 * Returns the change in x position of the mouse relative to the window height
 */
f32 mouseDeltaX(Window* window);

/**
 * Returns the change in y position of the mouse relative to the window height
 */
f32 mouseDeltaY(Window* window);

/**
 * Returns whether the key is currently down
 */
bool isButtonDown(Window* window, Button key);

/**
 * Get the key events since last event processing
 */
WindowEvent* windowEvents(Window* window, u32* count);

// ============================================================================
// Audio
// ============================================================================

/**
 * Initialize the audio subsystem
 *
 * Returns
 * - Whether init succeeded
 */
bool audioInit();

/**
 * Deinitialize the audio subsystem
 */
void audioDeinit();

/**
 * An audio stream
 */
struct AudioStream;

/**
 * Create a new audio stream
 *
 * Parameters
 * - frequency The segments per second to play
 * - channels The number of channels (mono, stereo, etc.)
 *
 * Returns
 * - The created audio stream, or nullptr on failure
 */
AudioStream* audioStreamCreate(u32 frequency, u32 channels);

/**
 * Destroy an audio stream
 */
void audioStreamDestroy(AudioStream* player);

/**
 * Push data to the audio stream
 *
 * Parameters
 * - stream The stream to push to
 * - data The raw audio data in 32 bit float format
 * - size The size of the pushed data in bytes
 */
void audioStreamPush(AudioStream* player, const f32* data, u64 size);

/**
 * Returns the amount of audio still queued in bytes
 */
u32 audioStreamQueuedSize(AudioStream* player);

/**
 * Clear data from the audio player
 */
void audioStreamClear(AudioStream* player);

/**
 * The the gain for the stream
 */
void audioStreamSetGain(AudioStream* stream, f32 gain);

/**
 * Audio data asset
 */
struct Sound {
    /**
     * The sound data
     */
    f32* data = nullptr;
    /**
     * The size of the data in bytes
     */
    u64 size = 0;
    /**
     * The floats per second
     */
    u32 frequency = 0;
    /**
     * The number of channels (mono, stereo, etc.)
     */
    u32 channels = 0;
};

/**
 * A handle to an audio asset
 */
using SoundAsset = Asset<Sound>;

/**
 * AudioData asset load implementation
 */
template<>
void assetLoadImpl(Asset<Sound>* data);

/**
 * AudioData asset unload implementation
 */
template<>
void assetUnloadImpl(Asset<Sound>* data);

/**
 * A music track in the audio player
 */
struct AudioPlayerMusic {
    /**
     * The music's stream
     */
    AudioStream* stream = nullptr;
    /**
     * The music sound to play
     */
    SoundAsset* sound = nullptr;
    /**
     * The current position in the sound
     */
    u32 pos = 0;
    /**
     * Whether the music is currently playing or paused
     */
    bool playing = false;
};

/**
 * An audio player system
 */
struct AudioPlayer {
    /**
     * The repeating music
     */
    Array<AudioPlayerMusic> music = {};
    /**
     * The temporary sounds
     */
    Array<AudioStream*> sounds = {};
};

/**
 * Create a new audio player
 */
AudioPlayer audioPlayerCreate();

/**
 * Destroy an audio player
 */
void audioPlayerDestroy(AudioPlayer* player);

/**
 * Update the audio player
 */
void audioPlayerUpdate(AudioPlayer* player);

/**
 * Start a new music track, or resume an existing one
 */
void audioPlayerMusic(AudioPlayer* player, SoundAsset* music);

/**
 * Remove a music track from the player
 */
void audioPlayerMusicKill(AudioPlayer* player, SoundAsset* music);

/**
 * Pause a music track
 */
void audioPlayerMusicPause(AudioPlayer* player, SoundAsset* music);

/**
 * Set the volume for a music track
 */
void audioPlayerSetMusicGain(AudioPlayer* player, SoundAsset* music, f32 gain = 1.0f);

/**
 * Play a sound once
 */
void audioPlayerSound(AudioPlayer* player, SoundAsset* sound, f32 gain);

// ============================================================================
// Camera
// ============================================================================

/**
 * The types of camera projections
 */
enum CameraType : u32 {
    CameraType_perspective,
    CameraType_orthographic,
};

/**
 * A perspective camera
 */
struct CameraPerspective {
    /**
     * The aspect ratio
     */
    f32 aspect = 0.0f;
    /**
     * The field of view
     */
    f32 fov = 0.0f;
    /**
     * The near clipping plane
     */
    f32 near = 0.0f;
    /**
     * The far clipping plane
     */
    f32 far = 0.0f;
};

/**
 * An orthographic camera
 */
struct CameraOrthographic {
    /**
     * The clipping planes in each direction
     */
    f32 left = 0, right = 0, top = 0, bottom = 0, near = 0, far = 0;
};

/**
 * A camera component
 */
struct Camera {
    /**
     * The gpu view projection data
     */
    GpuBuffer* vpBuffer = nullptr;
    /**
     * The current rotation
     */
    Quat rotation = {};
    /**
     * The current position
     */
    Vec3 position = {};
    /**
     * The type of projection
     */
    CameraType type = {};
    /**
     * The projection data
     */
    union {
        /**
         * An orthographic camera
         */
        CameraOrthographic orthographic;
        /**
         * A perspective camera
         */
        CameraPerspective perspective;
    };
};

/**
 * Camera serialization
 */
template<>
void serialize(Serializer* s, Camera* camera);

/**
 * Create a camera
 */
Camera cameraCreate();

/**
 * Destroy a camera
 */
void cameraDestroy(Camera* camera);

/**
 * The the camera to a perspective projection
 */
void cameraSetPerspective(
    Camera* camera,
    f32 aspect,
    f32 fov = static_cast<f32>(HG_PI) / 2.0f,
    f32 near = 0.01f,
    f32 far = 1000.0f);

/**
 * The the camera to an orthographic projection
 *
 * Parameters
 * - camera The camera to set
 * - width The desired width of the render space
 * - height The desired height of the render space
 * - actualAspect The actual aspect, so margins can be added, or 0 to ignore
 */
void cameraSetOrthographic(Camera* camera, f32 width, f32 height, f32 actualAspect = 0.0f);

/**
 * Update the camera's gpu side data
 */
void cameraUpdate(Camera* camera);

// ============================================================================
// Texture Assets
// ============================================================================

/**
 * A texture asset
 */
struct TextureData {
    /**
     * The width of the texture in pixels
     */
    u32 width = 0;
    /**
     * The height of the texture in pixels
     */
    u32 height = 0;
    /**
     * The depth of the texture in pixels
     */
    u32 depth = 0;
    /**
     * The format of each pixel
     */
    Format format = {};
    /**
     * The pixel data, aligned to 16 bytes
     */
    void* pixels = nullptr;
};

/**
 * A handle to a texture
 */
using TextureDataAsset = Asset<TextureData>;

/**
 * Texture asset load implementation
 */
template<>
void assetLoadImpl(Asset<TextureData>* data);

/**
 * Texture asset unload implementation
 */
template<>
void assetUnloadImpl(Asset<TextureData>* data);

/**
 * Store an image to disc in the png format
 *
 * Returns
 * - Whether the write succeeded
 */
bool textureStorePng(TextureData* texture, StringView path);

/**
 * A texture asset stored on the gpu
 */
struct Texture {
    /**
     * The image
     */
    GpuImage* image = nullptr;
    /**
     * The image view
     */
    GpuView* view = nullptr;
};

/**
 * A handle to a texture asset
 */
using TextureAsset = Asset<Texture>;

/**
 * GpuTexture asset load implementation
 */
template<>
void assetLoadImpl(Asset<Texture>* data);

/**
 * GpuTexture asset unload implementation
 */
template<>
void assetUnloadImpl(Asset<Texture>* data);

// ============================================================================
// 2D Renderer
// ============================================================================

/**
 * Initialize the 2D renderer
 */
void rendererInit2D(Format colorFormat);

/**
 * Deinitialize the 2D renderer
 */
void rendererDeinit2D();

/**
 * The 2D instance types
 */
enum Render2DInstanceType : u32 {
    /**
     * A instance with a color value
     */
    Render2DInstanceType_color = 0,
    /**
     * A instance with a sprite
     */
    Render2DInstanceType_sprite = 1,
};

/**
 * A rectangle instance
 */
struct Rect2DInstance {
    /**
     * The instance position
     */
    Vec2 pos = {};
    /**
     * The instance size
     */
    Vec2 size = {};
    /**
     * The instance type
     */
    u32 type = 0;
    /**
     * Padding for 16 byte alignment
     */
    u32 pad[3] = {};
    /**
     * The rectangle fill color
     */
    Vec4 color = {};
};

/**
 * A sprite instance
 */
struct Sprite2DInstance {
    /**
     * The instance position
     */
    Vec2 pos = {};
    /**
     * The instance size
     */
    Vec2 size = {};
    /**
     * The instance type
     */
    u32 type = 0;
    /**
     * Padding for 16 byte alignment
     */
    u32 pad[2] = {};
    /**
     * The texture index
     */
    u32 tex = 0;
    /**
     * The texture uv coordinates
     */
    Vec2 uvPos = {};
    /**
     * The texture uv coordinates
     */
    Vec2 uvSize = {};
};

/**
 * An instance in a 2D layer
 */
union Render2DInstance {
    /**
     * The rectangle data
     */
    Rect2DInstance rect;
    /**
     * The sprite data
     */
    Sprite2DInstance sprite;
};

/**
 * A 2D render layer
 */
struct Layer2D {
    /**
     * The transform, does not affect changed
     */
    Mat4 transform = {};
    /**
     * The instance data
     */
    Array<Render2DInstance> instances = {};
    /**
     * The gpu side instance buffer
     */
    GpuBuffer* instanceBuffer = nullptr;
    /**
     * The capacity of the instance buffer
     */
    u32 instanceCapacity = 0;
    /**
     * Whether the gpu data needs to be updated
     */
    bool changed = false;
};

/**
 * Create a 2D render layer
 */
Layer2D layerCreate2D();

/**
 * Destroy a 2D render layer
 */
void layerDestroy2D(Layer2D* layer);

/**
 * Remove all drawings from the layer
 */
void layerClear2D(Layer2D* layer);

/**
 * Issue draw commands for a 2D layer
 */
void renderLayer2D(GpuCmd* cmd, Camera* camera, Layer2D* layer);

/**
 * Issue draw commands for a 2D layer using debug lines
 */
void renderDebug2D(GpuCmd* cmd, Camera* camera, Layer2D* layer);

/**
 * Draw a rectangle on the layer
 */
void drawRect2D(Layer2D* layer, Vec4 color, Rect dst);

/**
 * A 2D sprite which can be drawn
 */
struct Sprite2D {
    /**
     * The sprite's texture
     */
    TextureAsset* texture = nullptr;
    /**
     * The uv coords in the texture
     */
    Rect uv = {};
};

/**
 *  Draw the sprite on the layer
 */
void drawSprite2D(Layer2D* layer, Sprite2D* sprite, Rect dst);

/**
 * A texture atlas
 */
struct Atlas2D {
    /**
     * The texture
     */
    TextureAsset* texture = nullptr;
    /**
     * The sprites
     */
    Array<Rect> sprites = {};
};

/**
 * Create a new texture atlas
 */
Atlas2D atlasCreate2D(TextureAsset* texture);

/**
 * Destroy a texture atlas
 */
void atlasDestroy2D(Atlas2D* atlas);

/**
 * Add a sprite to the atlas
 */
u32 atlasAdd2D(Atlas2D* atlas, Rect sprite);

/**
 * Add a grid of sprites to the atlas
 *
 * Parameters
 * - atlas The atlas to add to
 * - grid The uv coords of the grid
 * - width The number of horizontal subdivisions
 * - height The number of vertical subdivisions
 *
 * Returns
 * - The first sprite index
 */
u32 atlasAddGrid2D(Atlas2D* atlas, Rect grid, u32 width, u32 height);

/**
 * Get a sprite from the atlas
 */
Sprite2D atlasGet2D(Atlas2D* atlas, u32 idx);

/**
 * A world map of tiles
 */
struct Tilemap2D {
    /**
     * The tilemap data
     */
    u32* tiles = nullptr;
    /**
     * The width of the tilemap in tiles
     */
    u32 width = 0;
    /**
     * The height of the tilemap in tiles
     */
    u32 height = 0;
};

/**
 * Create an empty tilemap
 */
Tilemap2D tilemapCreate2D(u32 width, u32 height);

/**
 * Destroy a tilemap
 */
void tilemapDestroy2D(Tilemap2D* tilemap);

/**
 * Get the value of a tile in a tilemap
 */
u32 tilemapGet2D(Tilemap2D* tilemap, u32 x, u32 y);

/**
 * Set the value of a tile in a tilemap
 */
void tilemapSet2D(Tilemap2D* tilemap, u32 x, u32 y, u32 tile);

/**
 * Draw a tilemap to the layer
 */
void drawTilemap2D(Layer2D* layer, Atlas2D* atlas, Tilemap2D* tilemap, Rect dst);

// ============================================================================
// 3D Mesh
// ============================================================================

/**
 * A vertex in a mesh
 */
struct MeshVertex {
    /**
     * The vertex position
     */
    Vec3 pos;
    /**
     * The u part of the vertex uv coordinate
     */
    f32 uvU;
    /**
     * The vertex normal
     */
    Vec3 norm;
    /**
     * The v part of the vertex uv coordinate
     */
    f32 uvV;
    /**
     * The vertex tangent
     */
    Vec4 tan;

    /**
     * Construct the vertex
     */
    MeshVertex(Vec3 pos, Vec3 norm, Vec4 tan, Vec2 uv)
        : pos{pos}, uvU{uv.x}, norm{norm}, uvV{uv.y}, tan{tan} {}
};

/**
 * A 3d mesh asset
 */
struct MeshData {
    /**
     * The file index of the first vertex
     */
    MeshVertex* vertices = nullptr;
    /**
     * The file index of the first geometry index
     */
    u32* indices = nullptr;
    /**
     * The number of vertices
     */
    u32 vertexCount = 0;
    /**
     * The size of each vertex in bytes
     */
    u32 vertexWidth = 0;
    /**
     * The number of indices (4 bytes each)
     */
    u32 indexCount = 0;
    /**
     * How the vertices should be interpreted in sequence
     */
    GpuTopology topology = {};
};

/**
 * A handle to a 3d mesh asset
 */
using MeshDataAsset = Asset<MeshData>;

/**
 * Mesh asset load implementation
 */
template<>
void assetLoadImpl(Asset<MeshData>* data);

/**
 * Mesh asset unload implementation
 */
template<>
void assetUnloadImpl(Asset<MeshData>* data);

/**
 * Store the model data to disc in gltf format : TODO
 */
void meshStoreGltf(MeshData* data, StringView path, Fence* fence);

/**
 * A 3d mesh asset stored on the gpu
 */
struct Mesh {
    /**
     * The vertex buffer
     */
    GpuBuffer* vertexBuffer = nullptr;
    /**
     * The index buffer
     **/
    GpuBuffer* indexBuffer = nullptr;
    /**
     * The number of vertices
     */
    u32 vertexCount = 0;
    /**
     * The size of each vertex in bytes
     */
    u32 vertexWidth = 0;
    /**
     * The number of indices (4 bytes each)
     */
    u32 indexCount = 0;
};

/**
 * A gpu mesh asset handle
 */
using MeshAsset = Asset<Mesh>;

/**
 * GpuMesh asset load implementation
 */
template<>
void assetLoadImpl(Asset<Mesh>* data);

/**
 * GpuMesh asset unload implementation
 */
template<>
void assetUnloadImpl(Asset<Mesh>* data);

// ============================================================================
// ImGui Integration
// ============================================================================

/**
 * Initialize ImGui platform backend
 *
 * Parameters
 * - window The window for ImGui to use
 * - colorFormat The format the color target will be in
 * - depthFormat The format the depth buffer will be in, if used
 * - stencilFormat The format the stencil will be in, if used
 */
void initImGui(
    Window* window,
    Format colorFormat,
    Format depthFormat = Format_undefined,
    Format stencilFormat = Format_undefined);

/**
 * Deinitializes ImGui platform backend
 */
void deinitImGui();

/**
 * Create an ImGui texture
 */
void* createImGuiTexture(GpuView* view, GpuLayout layout);

/**
 * Create an ImGui texture
 */
void destroyImGuiTexture(void* texture);

/**
 * Create a new ImGui frame for the platform backend
 */
void beginImGuiFrame();

/**
 * Draw the ImGui frame
 *
 * Parameters
 * - cmd The command buffer to record to
 */
void renderImGui(GpuCmd* cmd);

// ============================================================================
// ECS
// ============================================================================

/**
 * An entity in the ecs
 */
struct Entity {
    /**
     * The entity handle
     */
    Handle handle = {};
};

/**
 */
static constexpr Entity entityNull = Entity{};

/**
 * Compare entities
 */
constexpr bool operator==(Entity lhs, Entity rhs)
{
    return lhs.handle.id == rhs.handle.id;
}

/**
 * Compare entities
 */
constexpr bool operator!=(Entity lhs, Entity rhs)
{
    return lhs.handle.id != rhs.handle.id;
}

/**
 * Hashing for entities
 */
template<>
constexpr u64 hash(Entity e)
{
    return hash(e.handle.id);
}

/**
 * The unique component id for a type
 */
template<typename T>
inline u64 componentId = (u64)-1;

/**
 * The function called on removing the component, may be overridden
 */
template<typename T>
void ecsDtor(T* component)
{
    static_cast<void>(component);
}

/**
 * The serializer for an ecs
 */
union EntitySerializer {
    /**
     * The indices of entities
     */
    u32* entityToIdx;
    /**
     * The entities by index
     */
    Entity* idxToEntity;
};

/**
 * The default serialization for a component, should be overridden
 */
template<typename T>
void ecsSerialize(Serializer* s, T* val, EntitySerializer* entities)
{
    serialize(s, val);
    static_cast<void>(entities);
}

/**
 * Entity ecs serialization
 */
void entitySerialize(Serializer* s, Entity* val, EntitySerializer* ecs);

/**
 * A system of components
 */
struct Component {
    /**
     * The name of the component type
     */
    StringView name = {};
    /**
     * The component lookup from entity index
     */
    Array<u32> indices = {};
    /**
     * The entity lookup from component index
     */
    Array<Entity> entities = {};
    /**
     * The component data
     */
    ArrayAny components = {};
    /**
     * The function called on removing the component
     */
    void (*dtor)(void* component) = nullptr;
    /**
     * The function called on serializing the component
     *
     * Parameters
     * - s The serializer
     * - The value to serialize
     * - ecs The ecs serializer data, if needed
     */
    void (*serialize)(Serializer* s, void* val, EntitySerializer* ecs) = nullptr;
};

/**
 * An entity component system
 */
struct Ecs {
    /**
     * The entity pool
     */
    HandlePool entities = {};
    /**
     * The component systems
     */
    Map<u64, Component> components = {};
};

/**
 * Create a new entity component system
 */
Ecs ecsCreate();

/**
 * Destroy an entity component system
 */
void ecsDestroy(Ecs* ecs);

/**
 * Destroy all entities, leave components registered
 */
void ecsReset(Ecs* ecs);

/**
 * Ecs serialization
 */
template<>
void serialize(Serializer* s, Ecs* ecs);

/**
 * The config to register a component
 */
struct EcsRegisterComponent {
    /**
     * The name of the component to create
     *
     * Note, the componentId is derived from this name
     */
    StringView name = {};
    /**
     * The width of the component data in bytes
     */
    u32 width = 0;
    /**
     * The alignment of the component data in bytes
     */
    u32 align = 0;
    /**
     * The function called on removing the component
     */
    void (*dtor)(void* component) = nullptr;
    /**
     * The function called on serializing the component
     */
    void (*serialize)(Serializer* s, void* val, EntitySerializer* ecs) = nullptr;
};

/**
 * Register a new component type
 */
void ecsRegisterComponent(Ecs* ecs, EcsRegisterComponent* config);

/**
 * Register a new component type, creating the name from the type
 */
#define HG_ECS_REGISTER_TYPE(ecs, T) \
    do { \
        componentId<T> = hash(#T); \
        EcsRegisterComponent registerComponent_##T{}; \
        registerComponent_##T.name = #T; \
        registerComponent_##T.width = sizeof(T); \
        registerComponent_##T.align = alignof(T); \
        registerComponent_##T.dtor = [](void* component) \
        { \
            ecsDtor<T>(static_cast<T*>(component)); \
        }; \
        registerComponent_##T.serialize = []( \
            Serializer* s, \
            void* val, \
            EntitySerializer* entities) \
        { \
            ecsSerialize<T>(s, static_cast<T*>(val), entities); \
        }; \
        ecsRegisterComponent(ecs, &registerComponent_##T); \
    } while (0)

/**
 * Unregister a component
 */
void ecsUnregisterComponent(Ecs* ecs, u64 componentId);

/**
 * Unregister a component
 */
template<typename T>
void ecsUnregisterComponent(Ecs* ecs)
{
    ecsUnregisterComponent(ecs, componentId<T>);
}

/**
 * Returns the name of the component type
 */
StringView ecsComponentName(Ecs* ecs, u64 componentId);

/**
 * Return a new entity
 */
Entity ecsSpawn(Ecs* ecs);

/**
 * Destroy an entity
 *
 * Note, this function will invalidate iterators
 */
void ecsDespawn(Ecs* ecs, Entity e);

/**
 * Return whether an entity is alive
 */
bool ecsAlive(Ecs* ecs, Entity e);

/**
 * Add a component to an entity
 *
 * Note, the entity must not have a component of this type already
 *
 * Returns
 * - A pointer to the created component
 */
void* ecsAdd(Ecs* ecs, Entity e, u64 componentId);

/**
 * Add a component to an entity
 *
 * Note, the entity must not have a component of this type already
 *
 * Returns
 * - A pointer to the created component
 */
template<typename T>
T* ecsAdd(Ecs* ecs, Entity e)
{
    return static_cast<T*>(ecsAdd(ecs, e, componentId<T>));
}

/**
 * Remove a component from an entity
 *
 * Note, this function will invalidate iterators
 */
void ecsRemove(Ecs* ecs, Entity e, u64 componentId);

/**
 * Remove a component from an entity
 *
 * Note, this function will invalidate iterators
 */
template<typename T>
void ecsRemove(Ecs* ecs, Entity e)
{
    ecsRemove(ecs, e, componentId<T>);
}

/**
 * Check whether an entity has a component or not
 */
bool ecsHas(Ecs* ecs, Entity e, u64 componentId);

/**
 * Check whether an entity has a component or not
 */
template<typename T>
bool ecsHas(Ecs* ecs, Entity e)
{
    return ecsHas(ecs, e, componentId<T>);
}

/**
 * Return whether the entity has all given components
 */
template<typename... Ts>
bool ecsHasAll(Ecs* ecs, Entity e)
{
    return (ecsHas<Ts>(ecs, e) && ...);
}

/**
 * Return whether the entity has any of the given components
 */
template<typename... Ts>
bool ecsHasAny(Ecs* ecs, Entity e)
{
    return (ecsHas<Ts>(ecs, e) || ...);
}

/**
 * Get a pointer to the entity's component
 *
 * Note, the entity must have the component
 *
 * Returns
 * - A pointer to the entity's component, will never be nullptr
 */
void* ecsGet(Ecs* ecs, Entity e, u64 componentId);

/**
 * Get a reference to the entity's component
 *
 * Note, the entity must have the component
 *
 * Returns
 * - A reference to the entity's component
 */
template<typename T>
T* ecsGet(Ecs* ecs, Entity e)
{
    return static_cast<T*>(ecsGet(ecs, e, componentId<T>));
}

/**
 * Get the entity from it's component
 *
 * Parameters
 * - c The component to lookup, must be a valid component
 * - componentId The id of the component
 */
Entity ecsGetEntity(Ecs* ecs, const void* c, u64 componentId);

/**
 * Get the entity from it's component
 *
 * Parameters
 * - c The component to lookup, must be a valid component
 */
template<typename T>
Entity ecsGetEntity(Ecs* ecs, const T* c)
{
    return ecsGetEntity(ecs, static_cast<const void*>(c), componentId<T>);
}

/**
 * Return a pointer to all entities of type
 */
Entity* ecsEntities(Ecs* ecs, u64 componentId);

/**
 * Return a pointer to all entities of type
 */
template<typename T>
Entity* ecsEntities(Ecs* ecs)
{
    return ecsEntities(ecs, componentId<T>);
}

/**
 * Return a pointer to all components of type
 */
void* ecsComponents(Ecs* ecs, u64 componentId);

/**
 * Return a pointer to all components of type
 */
template<typename T>
T* ecsComponents(Ecs* ecs)
{
    return static_cast<T*>(ecsComponents(ecs, componentId<T>));
}

/**
 * Return the number of active components of a type
 */
u32 ecsCount(Ecs* ecs, u64 componentId);

/**
 * Return the number of active components of a type
 */
template<typename T>
u32 ecsCount(Ecs* ecs)
{
    return ecsCount(ecs, componentId<T>);
}

/**
 * Find the id of the system with the fewest elements
 */
u64 ecsFindSmallest(Ecs* ecs, u64* ids, u32 idCount);

/**
 * Find the id of the system with the fewest elements
 */
template<typename... Ts>
u64 ecsFindSmallest(Ecs* ecs)
{
    u32 index = 0;
    u64 ids[sizeof...(Ts)];
    ((ids[index++] = componentId<Ts>), ...);
    return ecsFindSmallest(ecs, ids, sizeof...(Ts));
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
void ecsForEach(Ecs* ecs, Fn fn);

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
void ecsForPar(Ecs* ecs, Fn fn);

/**
 * Sort components
 *
 * Parameters
 * - componentId The component system to sort
 * - data The data passed to compare
 * - compare The comparison function
 */
void ecsSort(Ecs* ecs, u64 componentId, void* data, bool (*compare)(void*, Ecs* ecs, Entity lhs, Entity rhs));

/**
 * Sort components
 *
 * Parameters
 * - data The data passed to compare
 * - compare The comparison function
 */
template<typename T>
void ecsSort(Ecs* ecs, void* data, bool (*compare)(void*, Ecs* ecs, Entity lhs, Entity rhs))
{
    ecsSort(ecs, componentId<T>, data, compare);
}

/**
 * A node component for entities in a hierarchy
 */
struct Node {
    /**
     * The entity's parent, if any
     */
    Entity parent{};
    /**
     * The next child of this entity's parent
     */
    Entity nextSibling{};
    /**
     * The previous child of this entity's parent
     */
    Entity prevSibling{};
    /**
     * The first of this entity's children, forming a linked list
     */
    Entity firstChild{};
};

/**
 * Node serialization implementation
 */
template<>
void ecsSerialize(Serializer* s, Node* node, EntitySerializer* ecs);

/**
 * Add an empty node to an entity
 */
Node* nodeAdd(Ecs* ecs, Entity e);

/**
 * Remove the entity from its hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to detach, must be alive
 */
void nodeDetach(Ecs* ecs, Entity e);

/**
 * Destroy the entity and all its children in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to destroy to, must be alive
 */
void nodeDestroy(Ecs* ecs, Entity e);

/**
 * Add a new child to an entity in a hierarchy
 *
 * Parameters
 * - ecs The ecs
 * - parent The parent to add to, must be alive
 * - child The child to add, must be alive
 */
void nodeAddChild(Ecs* ecs, Entity parent, Entity child);

/**
 * The transform component for entities in absolute space
 */
struct Transform {
    /**
     * The entity's transform model matrix in world space
     */
    Mat4 mat{1.0f};
    /**
     * The entity's position relative to its parent
     * - x: -left, +right
     * - y: -up, +down
     * - z: -backward, +forward
     */
    Vec3 position{0.0f, 0.0f, 0.0f};
    /**
     * The entity's scaling relative to its parent
     * - x: horizonatal
     * - y: vertical
     * - z: depth
     */
    Vec3 scale{1.0f, 1.0f, 1.0f};
    /**
     * The entity's rotation relative to its parent
     */
    Quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
};

/**
 * Transform serialization impl
 */
template<>
void serialize(Serializer* s, Transform* node);

/**
 * Add an identity transform to an entity
 */
Transform* transformAdd(
    Ecs* ecs,
    Entity e,
    Vec3 position = Vec3{0.0f},
    Vec3 scale = Vec3{1.0f},
    Quat rotation = Quat{1.0f});

/**
 * Get the position from Transform mat
 */
constexpr Vec3 transformWorldPos(Transform& tf)
{
    return Vec3{tf.mat.w};
}

/**
 * Update Transform for the entity and its children to the TransformLocal
 */
void transformUpdate(Ecs* ecs, Entity e);

/**
 * An audio source component
 */
struct AudioSource {
    /**
     * The audio player for this source, should not be modified
     */
    AudioStream* player = nullptr;
    /**
     * The audio to play from
     */
    SoundAsset* audio = nullptr;
    /**
     * The current position in the audio data
     */
    u64 position = 0;
    /**
     * Whether the source should repeat playing
     */
    bool repeat = false;
};

/**
 * AudioSource serialization
 */
template<>
void serialize(Serializer* s, AudioSource* src);

/**
 * Add an audio source to an entity
 */
AudioSource* audioSourceAdd(Ecs* ecs, Entity e, SoundAsset* audio, bool repeat);

/**
 * AudioSource ecs destructor
 */
template<>
void ecsDtor(AudioSource* src);

/**
 * Update all audio sources, playing sound if needed
 */
void audioUpdate(Ecs* ecs, Entity listener);

/**
 * Camera ecs add implementation
 */
Camera* cameraAdd(Ecs* ecs, Entity e);

/**
 * Camera ecs destructor
 */
template<>
void ecsDtor(Camera* camera);

/**
 * Update the camera's gpu side data, must have a camera and transform
 */
void cameraUpdateEcs(Ecs* ecs, Entity e);

/**
 * Initialize the sprite pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be undefined
 * - depthFormat The format of the depth attachment, must not be undefined
 */
void spritesInit(Format colorFormat, Format depthFormat);

/**
 * Deinitialize the sprite pipeline
 */
void spritesDeinit();

/**
 * A sprite component
 */
struct Sprite {
    /**
     * The texture to draw from
     */
    TextureAsset* texture = nullptr;
    /**
     * The beginning coordinate to read from texture, [0.0, 1.0]
     */
    Vec2 uvPos{0.0f, 0.0f};
    /**
     * The size of the region to read from texture, [0.0, 1.0]
     */
    Vec2 uvSize{1.0f, 1.0f};
};

/**
 * Sprite serialization
 */
template<>
void serialize(Serializer* s, Sprite* sprite);

/**
 * Add a sprite to an entity
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to add to
 * - texture A copy of the sprite's texture
 */
Sprite* spriteAdd(
    Ecs* ecs,
    Entity e,
    TextureAsset* texture,
    Vec2 uvPos = Vec2{0.0f},
    Vec2 uvSize = Vec2{1.0f});

/**
 * Sprite ecs destructor
 */
template<>
void ecsDtor(Sprite* sprite);

/**
 * Issue draw commands for all Sprite2D components in the ecs
 *
 * Parameters
 * - ecs The ecs to draw
 * - camera The camera entity to use
 * - cmd The command buffer to record to, must not be nullptr
 */
void spritesDraw(Ecs* ecs, Entity camera, GpuCmd* cmd);

/**
 * Initialize the skybox pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be undefined
 * - depthFormat The format of the depth attachment, if used
 */
void skyboxInit(Format colorFormat, Format depthFormat);

/**
 * Deinitialize the skybox pipeline
 */
void skyboxDeinit();

/**
 * A skybox component
 */
struct Skybox {
    /**
     * The cubemap texture
     */
    TextureAsset* texture = nullptr;
};

/**
 * Skybox serialization
 */
template<>
void serialize(Serializer* s, Skybox* skybox);

/**
 * Add a skybox to an entity
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to add to
 * - texture A copy of the skybox's texture
 */
Skybox* skyboxAdd(Ecs* ecs, Entity e, TextureAsset* texture);

/**
 * Skybox ecs destructor
 */
template<>
void ecsDtor(Skybox* skybox);

/**
 * Draw a skybox from a texture
 *
 * Parameters
 * - ecs The ecs with the camera
 * - camera The camera entity to use
 * - cmd The command buffer to record to, must not be nullptr
 */
void skyboxDraw(Ecs* ecs, Entity camera, GpuCmd* cmd);

/**
 * A directional light component
 */
struct DirLight {
    /**
     * The direction of the light
     */
    Vec3 dir = {};
    /**
     * The color of the light
     */
    Vec4 color = {};
};

/**
 * DirLight serialization
 */
template<>
void serialize(Serializer* s, DirLight* light);

/**
 * Add a directional light to an entity
 */
DirLight* dirLightAdd(Ecs* ecs, Entity e, Vec3 dir, Vec4 color);

/**
 * A point light component, should have a transform
 */
struct PointLight {
    /**
     * The color of the light
     */
    Vec4 color = {};
};

/**
 * PointLight serialization
 */
template<>
void serialize(Serializer* s, PointLight* light);

/**
 * Add a point light to an entity
 */
PointLight* pointLightAdd(Ecs* ecs, Entity e, Vec4 color);

/**
 * Initialize the 3D model pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be undefined
 * - depthFormat The format of the depth attachment, must not be undefined
 */
void modelsInit(Format colorFormat, Format depthFormat);

/**
 * Deinitialize the 3D model pipeline
 */
void modelsDeinit();

/**
 * A 3D model component
 */
struct Model {
    /**
     * The model to render
     */
    MeshAsset* mesh = nullptr;
    /**
     * The model's color map
     */
    TextureAsset* colorMap = nullptr;
    /**
     * The model's normal map
     */
    TextureAsset* normalMap = nullptr;
};

/**
 * Model serialization
 */
template<>
void serialize(Serializer* s, Model* model);

/**
 * Add a model to an entity
 */
Model* modelAdd(
    Ecs* ecs,
    Entity e,
    MeshAsset* mesh,
    TextureAsset* colorMap,
    TextureAsset* normalMap);

/**
 * Model ecs destructor
 */
template<>
void ecsDtor(Model* model);

/**
 * Issue draw commands for all Model3D components in the ecs
 *
 * Parameters
 * - ecs The ecs to draw
 * - camera The camera entity to use
 * - cmd The command buffer to record to, must not be nullptr
 */
void modelsDraw(Ecs* ecs, Entity camera, GpuCmd* cmd);

// ============================================================================
// Template Implementations
// ============================================================================

template<typename... Ts>
void formatError(StringView errorFmt, Ts... args)
{
    char fmt[4096];
    u64 fmtLen = errorFmt.length < sizeof(fmt) - 1
        ? errorFmt.length
        : sizeof(fmt) - 1;
    memCopy(fmt, errorFmt.chars, fmtLen);
    fmt[fmtLen] = 0;

    char buf[4096];
    snprintf(buf, sizeof(buf), fmt, args...);

    setError(buf);
}

template<typename F>
void forPar(u64 begin, u64 end, F fn)
{
    static_assert(std::is_invocable_r_v<void, F, u64>);

    forPar(begin, end, &fn, [](void* pfn, u64 idx)
    {
        (*static_cast<F*>(pfn))(idx);
    });
}

template<typename T>
void serialize(Serializer* s, T* val)
{
    serializeVoid(s, val, sizeof(*val));
}

template<typename... Ts>
void serializeObject(Serializer* s, Ts*... vals)
{
    serializeBegin(s);
    (serialize(s, vals), ...);
    serializeEnd(s);
}

template<typename T, u64 N>
void serialize(Serializer* s, T (*arr)[N])
{
    serializeBegin(s);
    for (u64 i = 0; i < N; ++i)
    {
        serialize(s, &(*arr)[i]);
    }
    serializeEnd(s);
}

template<typename T>
void serialize(Serializer* s, Array<T>* arr)
{
    serializeBegin(s);
    serialize(s, &arr->count);
    serialize(s, &arr->capacity);
    if (!s->writing)
        arr->vals = heapAlloc<T>(arr->capacity);
    for (u32 i = 0; i < arr->count; ++i)
    {
        serialize(s, &(*arr)[i]);
    }
    serializeEnd(s);
}

template<typename T>
Array<T> arrayCreate(u32 count, u32 capacity)
{
    if (capacity < count)
        capacity = count;

    Array<T> arr{};
    arr.vals = heapAlloc<T>(capacity);
    arr.count = count;
    arr.capacity = capacity;

    return arr;
}

template<typename T>
void arrayDestroy(Array<T>* arr)
{
    HG_ASSERT(arr != nullptr);
    heapFree(arr->vals, arr->capacity);
}

template<typename T>
Array<T> arrayTemp(Arena* arena, u32 count, u32 capacity)
{
    HG_ASSERT(arena != nullptr);
    HG_ASSERT(count <= capacity);

    Array<T> arr{};
    arr.vals = arenaAlloc<T>(arena, capacity);
    arr.count = count;
    arr.capacity = capacity;

    return arr;
}

template<typename T>
void arrayResize(Array<T>* arr, u32 newCount)
{
    if (newCount > arr->capacity)
    {
        arr->vals = heapRealloc(arr->vals, arr->capacity, newCount * 2);
        arr->capacity = newCount * 2;
    }
    arr->count = newCount;
}

template<typename T>
void arrayResizeTemp(Arena* arena, Array<T>* arr, u32 newCount)
{
    if (newCount > arr->capacity)
    {
        arr->vals = arenaRealloc(arena, arr->vals, arr->capacity, newCount * 2);
        arr->capacity = newCount * 2;
    }
    arr->count = newCount;
}

template<typename T>
T* arrayPush(Array<T>* arr)
{
    if (arr->count == arr->capacity)
    {
        u32 newCapacity = arr->capacity == 0 ? 16 : arr->capacity * 2;
        arr->vals = heapRealloc(arr->vals, arr->capacity, newCapacity);
        arr->capacity = newCapacity;
    }
    return &arr->vals[arr->count++];
}

template<typename T>
T* arrayPushTemp(Arena* arena, Array<T>* arr)
{
    if (arr->count == arr->capacity)
    {
        u32 newCapacity = arr->capacity == 0 ? 16 : arr->capacity * 2;
        arr->vals = arenaRealloc(arena, arr->vals, arr->capacity, newCapacity);
        arr->capacity = newCapacity;
    }
    return &arr->vals[arr->count++];
}

template<typename T>
T arrayRemove(Array<T>* arr, u32 idx)
{
    HG_ASSERT(idx < arr->count);

    T val = (*arr)[idx];
    if (idx + 1 < arr->count)
    {
        memCopy(
            &(*arr)[idx],
            &(*arr)[idx + 1],
            (arr->count - (idx + 1)) * sizeof(T));
    }
    --arr->count;
    return val;
}

template<typename T>
T arrayRemoveSwap(Array<T>* arr, u32 idx)
{
    T val = (*arr)[idx];
    if (idx + 1 < arr->count)
    {
        memCopy(
            &(*arr)[idx],
            &(*arr)[arr->count - 1],
            sizeof(T));
    }
    --arr->count;
    return val;
}

template<typename T>
T arrayPop(Array<T>* arr)
{
    HG_ASSERT(arr->count > 0);
    return arr->vals[--arr->count];
}

template<typename T>
Queue<T> queueCreate(u32 capacity)
{
    Queue<T> queue{};
    queue.vals = heapAlloc<T>(capacity);
    queue.front = 0;
    queue.back = 0;
    queue.count = 0;
    queue.capacity = capacity;
    return queue;
}

template<typename T>
void queueDestroy(Queue<T>* queue)
{
    if (queue != nullptr)
    {
        heapFree(queue->vals, queue->capacity);
    }
}

template<typename T, typename U>
void queuePushFront(Queue<T>* queue, U val)
{
    HG_ASSERT(queue != nullptr);
    static_assert(std::is_convertible_v<U, T>);

    ++queue->count;
    if (queue->count == queue->capacity)
    {
        u32 newCapacity = queue->capacity * 2;
        queue->vals = heapRealloc(queue->vals, queue->capacity, newCapacity);

        if (queue->back < queue->front)
        {
            memCopy(queue->vals + queue->capacity, queue->vals, queue->back * sizeof(T));
            queue->back += queue->capacity;
        }

        queue->capacity = newCapacity;
    }

    queue->front = (queue->front == 0 ? queue->capacity : queue->front) - 1;
    queue->vals[queue->front] = static_cast<T>(val);
}

template<typename T, typename U>
void queuePushBack(Queue<T>* queue, U val)
{
    HG_ASSERT(queue != nullptr);
    static_assert(std::is_convertible_v<U, T>);

    ++queue->count;
    if (queue->count == queue->capacity)
    {
        u32 newCapacity = queue->capacity * 2;
        queue->vals = heapRealloc(queue->vals, queue->capacity, newCapacity);

        if (queue->back < queue->front)
        {
            memCopy(queue->vals + queue->capacity, queue->vals, queue->back * sizeof(T));
            queue->back += queue->capacity;
        }

        queue->capacity = newCapacity;
    }

    queue->vals[queue->back] = static_cast<T>(val);
    queue->back = (queue->back + 1) % queue->capacity;
}

template<typename T>
T queuePopFront(Queue<T>* queue)
{
    HG_ASSERT(queue->count > 0);
    --queue->count;

    T ret = queue->vals[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    return ret;
}

template<typename T>
T queuePopBack(Queue<T>* queue)
{
    HG_ASSERT(queue->count > 0);
    --queue->count;

    queue->back = (queue->back == 0 ? queue->capacity : queue->back) - 1;
    return queue->vals[queue->back];
}

template<typename V>
void serialize(Serializer* s, Set<V>* set)
{
    serializeBegin(s);

    serialize(s, &set->count);
    serialize(s, &set->capacity);

    if (s->writing)
    {
        setForEach(set, [&](V* val)
        {
            serialize(s, val);
        });
    }
    else
    {
        u32 count = set->count;
        setDestroy(set);
        *set = setCreate(set->capacity);
        for (u32 i = 0; i < count; ++i)
        {
            V val;
            serialize(s, &val);
            setAdd(set, val);
        }
    }

    serializeEnd(s);
}

template<typename V>
Set<V> setCreate(u32 slotCount)
{
    HG_ASSERT(slotCount > 0);

    Set<V> set;
    set.hasVal = heapAlloc<bool>(slotCount);
    set.vals = heapAlloc<V>(slotCount);
    set.capacity = slotCount;
    setReset(&set);
    return set;
}

template<typename V>
void setDestroy(Set<V>* set)
{
    HG_ASSERT(set != nullptr);

    heapFree(set->hasVal);
    heapFree(set->vals);
}

template<typename V>
Set<V> setTemp(Arena* arena, u32 slotCount)
{
    HG_ASSERT(slotCount > 0);

    Set<V> set;
    set.hasVal = arenaAlloc<bool>(arena, slotCount);
    set.vals = arenaAlloc<V>(arena, slotCount);
    set.capacity = slotCount;
    setReset(&set);
    return set;
}

template<typename V>
void setResize(Set<V>* set, u32 newSize)
{
    HG_ASSERT(set != nullptr);

    if (newSize > set->capacity)
    {
        Set<V> newSet = setCreate<V>(newSize);

        for (u32 i = 0; i < set->capacity; ++i)
        {
            if (set->hasVal[i])
                setAdd(&newSet, set->vals[i]);
        }

        setDestroy(set);
        *set = newSet;
    }
}

template<typename V>
void setReset(Set<V>* set)
{
    HG_ASSERT(set != nullptr);

    for (u32 i = 0; i < set->capacity; ++i)
    {
        set->hasVal[i] = false;
    }
    set->count = 0;
}

template<typename V, typename T>
void setAdd(Set<V>* set, const T& val)
{
    HG_ASSERT(set != nullptr);
    HG_ASSERT(set->count < set->capacity - 1);

    static_assert(std::is_convertible_v<T, V>);
    V v = static_cast<V>(val);

    u32 idx = static_cast<u32>(hash(v) % set->capacity);
    for (u32 dist = 0; set->hasVal[idx] && !(set->vals[idx] == v); ++dist)
    {
        u32 otherDist = static_cast<u32>(hash(set->vals[idx]) % set->capacity) - idx;
        if (otherDist > set->capacity)
            otherDist += set->capacity;

        if (otherDist < dist)
        {
            std::swap(v, set->vals[idx]);
            dist = otherDist;
        }

        idx = (idx + 1) % set->capacity;
    }

    set->hasVal[idx] = true;
    set->vals[idx] = v;
    ++set->count;
}

template<typename V, typename T>
void setRemove(Set<V>* set, const T& val)
{
    HG_ASSERT(set != nullptr);

    static_assert(std::is_convertible_v<T, V>);
    V v = static_cast<V>(val);

    u32 idx = static_cast<u32>(hash(v) % set->capacity);
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
        if (hash(set->vals[next]) % set->capacity != next)
        {
            set->vals[idx] = set->vals[next];
            idx = next;
        }
        next = (next + 1) % set->capacity;
    }
    set->hasVal[idx] = false;
    --set->count;
}

template<typename V, typename T>
bool setHas(const Set<V>* set, const T& val)
{
    HG_ASSERT(set != nullptr);

    static_assert(std::is_convertible_v<T, V>);
    V v = static_cast<V>(val);

    for (u32 idx = static_cast<u32>(hash(v) % set->capacity); set->hasVal[idx]; idx = (idx + 1) % set->capacity)
    {
        if (set->vals[idx] == v)
            return true;
    }
    return false;
}

template<typename V, typename F>
void setForEach(Set<V>* set, F fn)
{
    HG_ASSERT(set != nullptr);
    static_assert(std::is_invocable_r_v<void, F, V*>);

    for (u32 i = 0; i < set->capacity; ++i)
    {
        if (set->hasVal[i])
            fn(&set->vals[i]);
    }
}

template<typename K, typename V>
void serialize(Serializer* s, Map<K, V>* map)
{
    serializeBegin(s);

    serialize(s, &map->count);
    serialize(s, &map->capacity);

    if (s->writing)
    {
        mapForEach(map, [&](K* key, V* val)
        {
            serializeObject(s, key, val);
        });
    }
    else
    {
        u32 count = map->count;
        mapDestroy(map);
        *map = mapCreate(map->capacity);
        for (u32 i = 0; i < count; ++i)
        {
            K key;
            V val;
            serializeObject(s, &key, &val);
            mapAdd(map, key, val);
        }
    }

    serializeEnd(s);
}

template<typename K, typename V>
Map<K, V> mapCreate(u32 slotCount)
{
    HG_ASSERT(slotCount > 0);

    Map<K, V> map;
    map.hasVal = heapAlloc<bool>(slotCount);
    map.keys = heapAlloc<K>(slotCount);
    map.vals = heapAlloc<V>(slotCount);
    map.capacity = slotCount;
    mapReset(&map);

    return map;
}

template<typename K, typename V>
void mapDestroy(Map<K, V>* map)
{
    HG_ASSERT(map != nullptr);

    heapFree(map->hasVal, map->capacity);
    heapFree(map->keys, map->capacity);
    heapFree(map->vals, map->capacity);
}

template<typename K, typename V>
Map<K, V> mapTemp(Arena* arena, u32 slotCount)
{
    HG_ASSERT(arena != nullptr);
    HG_ASSERT(slotCount > 0);

    Map<K, V> map;
    map.hasVal = arenaAlloc<bool>(arena, slotCount);
    map.keys = arenaAlloc<K>(arena, slotCount);
    map.vals = arenaAlloc<V>(arena, slotCount);
    map.capacity = slotCount;
    mapReset(&map);

    return map;
}

template<typename K, typename V>
void mapResize(Map<K, V>* map, u32 newSize)
{
    HG_ASSERT(map != nullptr);

    if (newSize > map->capacity)
    {
        Map<K, V> newMap = mapCreate<K, V>(newSize);

        for (u32 i = 0; i < map->capacity; ++i)
        {
            if (map->hasVal[i])
                mapAdd(&newMap, map->keys[i], map->vals[i]);
        }

        mapDestroy(map);
        *map = newMap;
    }
}

template<typename K, typename V>
void mapReset(Map<K, V>* map)
{
    HG_ASSERT(map != nullptr);

    for (u32 i = 0; i < map->capacity; ++i)
    {
        map->hasVal[i] = false;
    }
    map->count = 0;
}

template<typename K, typename V, typename T, typename U>
V* mapAdd(Map<K, V>* map, const T& key, const U& val)
{
    HG_ASSERT(map != nullptr);
    HG_ASSERT(map->count < map->capacity - 1);

    static_assert(std::is_convertible_v<T, K> && std::is_convertible_v<U, V>);
    K k = static_cast<K>(key);
    V v = static_cast<V>(val);

    u32 idx = static_cast<u32>(hash(k) % map->capacity);
    for (u32 dist = 0; map->hasVal[idx] && !(map->keys[idx] == k); ++dist)
    {
        u32 otherDist = static_cast<u32>(hash(map->keys[idx]) % map->capacity) - idx;
        if (otherDist > map->capacity)
            otherDist += map->capacity;

        if (otherDist < dist)
        {
            std::swap(k, map->keys[idx]);
            std::swap(v, map->vals[idx]);
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

template<typename K, typename V, typename T>
bool mapRemove(Map<K, V>* map, const T& key, V* val)
{
    HG_ASSERT(map != nullptr);

    static_assert(std::is_convertible_v<T, K>);
    K k = static_cast<K>(key);

    u32 idx = static_cast<u32>(hash(k) % map->capacity);
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
        if (hash(map->keys[next]) % map->capacity != next)
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

template<typename K, typename V, typename T>
V* mapGet(const Map<K, V>* map, const T& key)
{
    HG_ASSERT(map != nullptr);

    static_assert(std::is_convertible_v<T, K>);
    K k = static_cast<K>(key);

    for (u32 idx = static_cast<u32>(hash(k) % map->capacity); map->hasVal[idx]; idx = (idx + 1) % map->capacity)
    {
        if (map->keys[idx] == k)
            return map->vals + idx;
    }
    return nullptr;
}

template<typename K, typename V, typename F>
void mapForEach(Map<K, V>* map, F fn)
{
    HG_ASSERT(map != nullptr);
    static_assert(std::is_invocable_r_v<void, F, K*, V*>);

    for (u32 i = 0; i < map->capacity; ++i)
    {
        if (map->hasVal[i])
            fn(&map->keys[i], &map->vals[i]);
    }
}

template<typename T>
void assetInit()
{
    assets<T>.map = mapCreate<StringView, Asset<T>*>();
    assets<T>.pool = poolCreate<Asset<T>>();
}

template<typename T>
void assetDeinit()
{
    poolDestroy(&assets<T>.pool);
    mapDestroy(&assets<T>.map);
}

template<typename T>
void assetLoadImpl(Asset<T>* data)
{
    static_cast<void>(data);
    static_assert(false, "Asset type cannot be loaded without implementation");
}

template<typename T>
void assetUnloadImpl(Asset<T>* data)
{
    static_cast<void>(data);
    static_assert(false, "Asset type cannot be unloaded without implementation");
}

template<typename T>
Asset<T>* assetCreate()
{
    Asset<T>* data = static_cast<Asset<T>*>(poolAlloc(&assets<T>.pool));
    data->asset = {};
    data->refCount = 1;
    data->path = {};

    return data;
}

template<typename T>
Asset<T>* assetLoad(StringView path)
{
    if (Asset<T>** asset = mapGet(&assets<T>.map, path);
        asset != nullptr)
    {
        ++(*asset)->refCount;
        return *asset;
    }

    Asset<T>* data = static_cast<Asset<T>*>(poolAlloc(&assets<T>.pool));
    data->asset = {};
    data->refCount = 1;
    data->path = stringCreate(path);

    if (assets<T>.map.count * 2 > assets<T>.map.capacity)
        mapResize(&assets<T>.map, assets<T>.map.capacity * 2);
    mapAdd(&assets<T>.map, data->path, data);

    assetLoadImpl(data);
    return data;
}

template<typename T>
void assetUnload(Asset<T>* asset)
{
    if (asset != nullptr && --asset->refCount == 0)
    {
        assetUnloadImpl(asset);

        if (asset->path != nullptr)
        {
            mapRemove(&assets<T>.map, asset->path);
            stringDestroy(&asset->path);
        }
        poolFree(&assets<T>.pool, asset);
    }
}

template<typename T>
void assetReload(Asset<T>* asset)
{
    HG_ASSERT(asset != nullptr);

    assetUnloadImpl(asset);
    assetLoadImpl(asset);
}

template<typename T>
Asset<T>* assetCopy(Asset<T>* asset)
{
    HG_ASSERT(asset != nullptr);
    ++asset->refCount;
    return asset;
}

template<typename T>
void serialize(Serializer* s, Asset<T>** asset)
{
    if (s->writing)
    {
        StringView path = (*asset)->path;
        serialize(s, &path);
    }
    else
    {
        HG_ARENA_SCOPE(s->arena);
        StringBuilder path;
        serialize(s, &path);
        if (path != "")
            *asset = assetLoad<T>(path);
        else
            *asset = nullptr;
    }
}

template<typename T, typename Fn>
void ecsForEachSingle(Ecs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, Entity, T*>);

    Entity* e = ecsEntities<T>(ecs);
    Entity* end = e + ecsCount<T>(ecs);
    T* c = ecsComponents<T>(ecs);
    for (; e != end; ++e, ++c)
    {
        fn(*e, c);
    }
}

template<typename... Ts, typename Fn>
void ecsForEachMulti(Ecs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, Entity, Ts*...>);

    u64 id = ecsFindSmallest<Ts...>(ecs);
    Component* system = mapGet(&ecs->components, id);
    HG_ASSERT(system != nullptr);

    Entity* e = system->entities.vals + 1;
    Entity* end = e + system->entities.count - 1;
    for (; e != end; ++e)
    {
        if (ecsHasAll<Ts...>(ecs, *e))
            fn(*e, ecsGet<Ts>(ecs, *e)...);
    }
}

template<typename... Ts, typename Fn>
void ecsForEach(Ecs* ecs, Fn fn)
{
    static_assert(sizeof...(Ts) != 0);

    if constexpr (sizeof...(Ts) == 1)
    {
        ecsForEachSingle<Ts...>(ecs, fn);
    } else {
        ecsForEachMulti<Ts...>(ecs, fn);
    }
}

template<typename T, typename Fn>
void ecsForParSingle(Ecs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, Entity, T*>);

    struct Capture {
        Ecs* ecs;
        Fn* fn;
    };
    Capture capture{ecs, &fn};

    forPar(0, ecsCount<T>(ecs), &capture, [](void* pcapture, u64 idx)
    {
        Capture* capture = static_cast<Capture*>(pcapture);
        (*capture->fn)(
            ecsEntities<T>(capture->ecs)[idx],
            &ecsComponents<T>(capture->ecs)[idx]);
    });
}

template<typename... Ts, typename Fn>
void ecsForParMulti(Ecs* ecs, Fn& fn)
{
    static_assert(std::is_invocable_r_v<void, Fn, Entity, Ts*...>);

    Component* system = mapGet(&ecs->components, ecsFindSmallest<Ts...>(ecs));
    HG_ASSERT(system != nullptr);

    struct Capture {
        Ecs* ecs;
        Component* system;
        Fn* fn;
    };
    Capture capture{ecs, system, &fn};

    forPar(1, system->entities.count, &capture, [](void* pcapture, u64 idx)
    {
        Capture* capture = static_cast<Capture*>(pcapture);
        Entity e = capture->system->entities[idx];
        if (ecsHasAll<Ts...>(capture->ecs, e))
            (*capture->fn)(e, ecsGet<Ts>(capture->ecs, e)...);
    });
}

template<typename... Ts, typename Fn>
void ecsForPar(Ecs* ecs, Fn fn)
{
    static_assert(sizeof...(Ts) != 0);

    if constexpr (sizeof...(Ts) == 1)
    {
        ecsForParSingle<Ts...>(ecs, fn);
    } else {
        ecsForParMulti<Ts...>(ecs, fn);
    }
}

} // namespace hg

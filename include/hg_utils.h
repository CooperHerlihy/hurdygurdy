#ifndef HG_UTILS_H
#define HG_UTILS_H

#include <float.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef float f32;
typedef double f64;

typedef uint8_t byte;
typedef size_t usize;
typedef intptr_t iptr;

#define HG_ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define HG_MAX(a, b) ((a) > (b) ? (a) : (b))
#define HG_MIN(a, b) ((a) < (b) ? (a) : (b))

#if defined(NDEBUG)
#define HG_DEBUG(message) ((void)0)
#define HG_DEBUGF(message, ...) ((void)0)
#else
#define HG_DEBUG(message) { \
    fprintf(stderr, __FILE__ ":%d Debug: " message "\n", __LINE__); \
}
#define HG_DEBUGF(message, ...) { \
    fprintf(stderr, __FILE__ ":%d Debug: " message "\n", __LINE__, __VA_ARGS__); \
}
#endif

#if defined(NDEBUG)
#define HG_LOG(message) { fprintf(stdout, "Log: " message "\n"); }
#define HG_LOGF(message, ...) { fprintf(stdout, "Log: " message "\n", __VA_ARGS__); }
#else
#define HG_LOG(message) { \
    fprintf(stdout, "Log: %s() " __FILE__ ":%d " message "\n", __func__, __LINE__); \
}
#define HG_LOGF(message, ...) { \
    fprintf(stdout, "Log: %s() " __FILE__ ":%d " message "\n", __func__, __LINE__, __VA_ARGS__); \
}
#endif

#if defined(NDEBUG)
#define HG_WARN(message) { fprintf(stderr, "Warning: " message "\n"); }
#define HG_WARNF(message, ...) { fprintf(stderr, "Warning: " message "\n", __VA_ARGS__); }
#else
#define HG_WARN(message) { \
    fprintf(stderr, "Warning: %s() " __FILE__ ":%d " message "\n", __func__, __LINE__); \
}
#define HG_WARNF(message, ...) { \
    fprintf(stderr, "Warning: %s() " __FILE__ ":%d " message "\n", __func__, __LINE__, __VA_ARGS__); \
}
#endif

#if defined(NDEBUG)
#define HG_ERROR(message) { fprintf(stderr, "Error: " message "\n"); abort(); }
#define HG_ERRORF(message, ...) { fprintf(stderr, "Error: " message "\n", __VA_ARGS__); abort(); }
#else
#define HG_ERROR(message) { \
    fprintf(stderr, "Error: %s() " __FILE__ ":%d " message "\n", __func__, __LINE__); \
    abort(); \
}
#define HG_ERRORF(message, ...) { \
    fprintf(stderr, "Error: %s() " __FILE__ ":%d " message "\n", __func__, __LINE__, __VA_ARGS__); \
    abort(); \
}
#endif

#if defined(NDEBUG)
#define HG_ASSERT(condition) ((void)0)
#else
#define HG_ASSERT(condition) { if (!(condition)) { HG_ERROR("ASSERT failed: " #condition); } }
#endif

/**
 * Hurdy Gurdy error codes
 */
typedef enum HgError {
    HG_SUCCESS = 0,
    HG_ERROR_UNKNOWN,
    HG_ERROR_FILE_NOT_FOUND,
    HG_ERROR_FILE_READ_FAILURE,
    HG_ERROR_FILE_WRITE_FAILURE,
    HG_ERROR_WINDOW_INVALID,
} HgError;

/**
 * Allocates memory from the heap
 *
 * Just a wrapper around malloc()
 * Does not return NULL on failure, but instead crashes the program
 *
 * \param size The size of the memory to allocate
 * \return The allocated memory, or NULL if size is 0
 */
void* hg_heap_alloc(usize size);

/**
 * Reallocates memory from the heap
 *
 * Just a wrapper around realloc()
 * Does not return NULL on failure, but instead crashes the program
 *
 * \param ptr The memory to reallocate, or NULL to allocate new
 * \param size The size of the memory to reallocate
 * \return The reallocated memory, or NULL if size is 0
 */
void* hg_heap_realloc(void* ptr, usize size);

/**
 * Frees memory from the heap
 *
 * Just a wrapper around free()
 *
 * \param ptr The memory to free, or NULL to do nothing
 * \param size The size of the memory to free, must be 0 if ptr is NULL
 */
void hg_heap_free(void* ptr, usize size);

/**
 * Loads a binary file
 *
 * \param path The null-terminated path to the file to load
 * \param data A pointer to store the loaded data, must not be NULL
 * \param size A pointer to store the size of the loaded data, must not be NULL
 * \return HG_SUCCESS if the file was loaded successfully
 * \return HG_ERROR_FILE_NOT_FOUND if the file was not found
 * \return HG_ERROR_FILE_READ_FAILURE if the file could not be read
 */
HgError hg_file_load_binary(const char* path, byte** data, usize* size);

/**
 * Saves a binary file
 *
 * \param path The null-terminated path to the file to save
 * \param data The data to save
 * \param size The size of the data to save
 * \return HG_SUCCESS if the file was saved successfully
 * \return HG_ERROR_FILE_WRITE_FAILURE if the file could not be written
 */
HgError hg_file_save_binary(const char* path, const byte* data, usize size);

/**
 * Unloads a binary file
 *
 * Just a wrapper around hg_heap_free()
 *
 * \param data The data to unload, or NULL to do nothing
 * \param size The size of the data to unload, must be 0 if data is NULL
 */
void hg_file_unload_binary(byte* data, usize size);

/**
 * Loads an image file
 *
 * \param path The null-terminated path to the file to load
 * \param data A pointer to store the loaded data, must not be NULL
 * \param width A pointer to store the width of the loaded data, must not be NULL
 * \param height A pointer to store the height of the loaded data, must not be NULL
 * \return HG_SUCCESS if the file was loaded successfully
 * \return HG_ERROR_FILE_NOT_FOUND if the file was not found
 * \return HG_ERROR_FILE_READ_FAILURE if the file could not be read
 */
HgError hg_file_load_image(const char* path, u32** data, u32* width, u32* height);

/**
 * Saves an image file
 *
 * \param path The null-terminated path to the file to save
 * \param data The data to save
 * \param width The width of the data to save
 * \param height The height of the data to save
 * \return HG_SUCCESS if the file was saved successfully
 * \return HG_ERROR_FILE_WRITE_FAILURE if the file could not be written
 */
HgError hg_file_save_image(const char* path, const u32* data, u32 width, u32 height);

/**
 * Unloads an image file
 *
 * Just a wrapper around hg_heap_free()
 *
 * \param data The data to unload, or NULL to do nothing
 * \param width The width of the data to unload, must be 0 if data is NULL
 * \param height The height of the data to unload, must be 0 if data is NULL
 */
void hg_file_unload_image(u32* data, u32 width, u32 height);

/**
 * A clock and/or timer
 */
typedef struct HgClock {
    struct timespec time;
} HgClock;

/**
 * Gets the time since the last call to hg_clock_tick()
 *
 * Call this function also to initialize the clock
 *
 * \param clock The clock to get the time from
 * \return The time since the last call to hg_clock_tick()
 */
f64 hg_clock_tick(HgClock* clock);

#endif // HG_UTILS_H

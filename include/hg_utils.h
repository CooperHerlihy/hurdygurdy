#ifndef HG_UTILS_H
#define HG_UTILS_H

#include <stdalign.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#include <vulkan/vulkan.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

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

typedef enum HgError {
    HG_SUCCESS = 0,
    HG_ERROR_UNKNOWN,
    HG_FILE_NOT_FOUND,
    HG_FILE_NOT_READABLE,
} HgError;

void* hg_heap_alloc(usize size);
void hg_heap_free(void* ptr, usize size);

HgError hg_file_load(const char* path, byte** data, usize* size);
void hg_file_unload(byte* data, usize size);

typedef struct HgClock {
    struct timespec time;
} HgClock;

f64 hg_clock_tick(HgClock* clock);

#define HG_MAX(a, b) ((a) > (b) ? (a) : (b))
#define HG_MIN(a, b) ((a) < (b) ? (a) : (b))

#endif

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

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <time.h>

#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t usize;
typedef intptr_t isize;

typedef float f32;
typedef double f64;

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
 * Takes the minimum of two values
 *
 * Note, if the values are expressions, they will be evaluated twice
 *
 * Parameters
 * - a The first value to compare
 * - b The second value to compare
 * Returns
 * - The lower value
 */
#define hg_min(a, b) ((a) < (b) ? (a) : (b))

/**
 * Takes the maximum of two values
 *
 * Note, if the values are expressions, they will be evaluated twice
 *
 * Parameters
 * - a The first value to compare
 * - b The second value to compare
 * Returns
 * - The higher value
 */
#define hg_max(a, b) ((a) > (b) ? (a) : (b))

/**
 * Clamps a value between two bounds
 *
 * Note, if the values are expressions, they will be multiple times
 *
 * Parameters
 * - x The value to clamp
 * - a The lower bound
 * - b The upper bound
 * Returns
 * - The clamped value
 */
#define hg_clamp(x, a, b) hg_max((a), hg_min((b), (x)))

/**
 * Formats and logs a debug message to stderr in debug mode only
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_debug(...) do { hg_debug_mode({ (void)fprintf(stderr, "HurdyGurdy Debug: " __VA_ARGS__); }) } while(0)

/**
 * Formats and logs an info message to stderr
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_info(...) do { (void)fprintf(stderr, "HurdyGurdy Info: " __VA_ARGS__); } while(0)

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

/**
 * Aligns a pointer to an alignment
 *
 * Parameters
 * - value The value to align
 * - alignment The alignment, must be a multiple of 2
 * Returns
 * - The aligned size
 */
usize hg_align(usize value, usize alignment);

/**
 * A high precision clock for timers and game deltas
 */
typedef struct timespec HgClock;

/**
 * Resets the clock and gets the delta since the last tick in seconds
 *
 * Parameters
 * - clock The clock to tick, must not be NULL
 * Returns
 * - Seconds since last tick
 */
f64 hg_clock_tick(HgClock *hclock);

#define HG_PI      3.1415926535897932
#define HG_TAU     6.2831853071795864
#define HG_E       2.7182818284590452
#define HG_ROOT2   1.4142135623730951
#define HG_ROOT3   1.7320508075688772

/**
 * A 2D vector
 */
typedef struct HgVec2 {
    f32 x, y;
} HgVec2;

/**
 * A 3D vector
 */
typedef struct HgVec3 {
    f32 x, y, z;
} HgVec3;

/**
 * A 4D vector
 */
typedef struct HgVec4 {
    f32 x, y, z, w;
} HgVec4;

/**
 * A 2x2 matrix
 */
typedef struct HgMat2 {
    HgVec2 x, y;
} HgMat2;

/**
 * A 3x3 matrix
 */
typedef struct HgMat3 {
    HgVec3 x, y, z;
} HgMat3;

/**
 * A 4x4 matrix
 */
typedef struct HgMat4 {
    HgVec4 x, y, z, w;
} HgMat4;

/**
 * A complex number
 */
typedef struct HgComplex {
    f32 r, i;
} HgComplex;

/**
 * A quaternion
 *
 * r is the real part
 */
typedef struct HgQuat {
    f32 r, i, j, k;
} HgQuat;

/**
 * Creates a 2D vector with the given scalar
 *
 * Parameters
 * - scalar The scalar to create the vector with
 * Returns
 * - The created vector
 */
HgVec2 hg_svec2(f32 scalar);

/**
 * Creates a 3D vector with the given scalar
 *
 * Parameters
 * - scalar The scalar to create the vector with
 * Returns
 * - The created vector
 */
HgVec3 hg_svec3(f32 scalar);

/**
 * Creates a 4D vector with the given scalar
 *
 * Parameters
 * - scalar The scalar to create the vector with
 * Returns
 * - The created vector
 */
HgVec4 hg_svec4(f32 scalar);

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
HgMat2 hg_smat2(f32 scalar);

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
HgMat3 hg_smat3(f32 scalar);

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
HgMat4 hg_smat4(f32 scalar);

/**
 * Creates a 3D vector from a 2D vector and 0 for the z component
 *
 * Parameters
 * - lhs The vector to convert
 * Returns
 * - The converted vector
 */
HgVec3 hg_vec2to3(HgVec2 lhs);

/**
 * Creates a 4D vector from a 2D vector and 0 for the z and w components
 *
 * Parameters
 * - lhs The vector to convert
 * Returns
 * - The converted vector
 */
HgVec4 hg_vec2to4(HgVec2 lhs);

/**
 * Creates a 3D vector from a 3D vector and 0 for the w component
 *
 * Parameters
 * - lhs The vector to convert
 * Returns
 * - The converted vector
 */
HgVec4 hg_vec3to4(HgVec3 lhs);

/**
 * Scales a 2x2 matrix to a 3x3 matrix with 1 on the diagonal
 *
 * Parameters
 * - lhs The matrix to convert
 * Returns
 * - The converted matrix
 */
HgMat3 hg_mat2to3(HgMat2 lhs);

/**
 * Scales a 2x2 matrix to a 4x4 matrix with 1 on the diagonal
 *
 * Parameters
 * - lhs The matrix to convert
 * Returns
 * - The converted matrix
 */
HgMat4 hg_mat2to4(HgMat2 lhs);

/**
 * Scales a 3x3 matrix to a 4x4 matrix with 1 on the diagonal
 *
 * Parameters
 * - lhs The matrix to convert
 * Returns
 * - The converted matrix
 */
HgMat4 hg_mat3to4(HgMat3 lhs);

/**
 * Adds two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be NULL
 * - lhs The left-hand side vector, must not be NULL
 * - rhs The right-hand side vector, must not be NULL
 */
void hg_vadd(u32 size, f32* dst, f32* lhs, f32* rhs);

/**
 * Adds two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The added vector
 */
HgVec2 hg_vadd2(HgVec2 lhs, HgVec2 rhs);

/**
 * Adds two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The added vector
 */
HgVec3 hg_vadd3(HgVec3 lhs, HgVec3 rhs);

/**
 * Adds two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The added vector
 */
HgVec4 hg_vadd4(HgVec4 lhs, HgVec4 rhs);

/**
 * Subtracts two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be NULL
 * - lhs The left-hand side vector, must not be NULL
 * - rhs The right-hand side vector, must not be NULL
 */
void hg_vsub(u32 size, f32* dst, f32* lhs, f32* rhs);

/**
 * Subtracts two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The subtracted vector
 */
HgVec2 hg_vsub2(HgVec2 lhs, HgVec2 rhs);

/**
 * Subtracts two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The subtracted vector
 */
HgVec3 hg_vsub3(HgVec3 lhs, HgVec3 rhs);

/**
 * Subtracts two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The subtracted vector
 */
HgVec4 hg_vsub4(HgVec4 lhs, HgVec4 rhs);

/**
 * Multiplies pairwise two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be NULL
 * - lhs The left-hand side vector, must not be NULL
 * - rhs The right-hand side vector, must not be NULL
 */
void hg_vmul(u32 size, f32* dst, f32* lhs, f32* rhs);

/**
 * Multiplies pairwise two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The multiplied vector
 */
HgVec2 hg_vmul2(HgVec2 lhs, HgVec2 rhs);

/**
 * Multiplies pairwise two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The multiplied vector
 */
HgVec3 hg_vmul3(HgVec3 lhs, HgVec3 rhs);

/**
 * Multiplies pairwise two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The multiplied vector
 */
HgVec4 hg_vmul4(HgVec4 lhs, HgVec4 rhs);

/**
 * Multiplies a scalar and a vector
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be NULL
 * - scalar The scalar to multiply with
 * - vec The vector to multiply with, must not be NULL
 */
void hg_svmul(u32 size, f32* dst, f32 scalar, f32* vec);

/**
 * Multiplies a scalar and a 2D vector
 *
 * Parameters
 * - scalar The scalar to multiply with
 * - vec The vector to multiply with
 * Returns
 * - The multiplied vector
 */
HgVec2 hg_svmul2(f32 scalar, HgVec2 vec);

/**
 * Multiplies a scalar and a 3D vector
 *
 * Parameters
 * - scalar The scalar to multiply with
 * - vec The vector to multiply with
 * Returns
 * - The multiplied vector
 */
HgVec3 hg_svmul3(f32 scalar, HgVec3 vec);

/**
 * Multiplies a scalar and a 4D vector
 *
 * Parameters
 * - scalar The scalar to multiply with
 * - vec The vector to multiply with
 * Returns
 * - The multiplied vector
 */
HgVec4 hg_svmul4(f32 scalar, HgVec4 vec);

/**
 * Divides pairwise two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be NULL
 * - lhs The left-hand side vector, must not be NULL
 * - rhs The right-hand side vector, must not be NULL
 */
void hg_vdiv(u32 size, f32* dst, f32* lhs, f32* rhs);

/**
 * Divides pairwise two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The divided vector
 */
HgVec2 hg_vdiv2(HgVec2 lhs, HgVec2 rhs);

/**
 * Divides pairwise two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The divided vector
 */
HgVec3 hg_vdiv3(HgVec3 lhs, HgVec3 rhs);

/**
 * Divides pairwise two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The divided vector
 */
HgVec4 hg_vdiv4(HgVec4 lhs, HgVec4 rhs);

/**
 * Divides a vector by a scalar
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be NULL
 * - scalar The scalar to divide by
 * - vec The vector to divide, must not be NULL
 */
void hg_svdiv(u32 size, f32* dst, f32 scalar, f32* vec);

/**
 * Divides a 2D vector by a scalar
 *
 * Parameters
 * - scalar The scalar to divide by
 * - vec The vector to divide
 * Returns
 * - The divided vector
 */
HgVec2 hg_svdiv2(f32 scalar, HgVec2 vec);

/**
 * Divides a 3D vector by a scalar
 *
 * Parameters
 * - scalar The scalar to divide by
 * - vec The vector to divide
 * Returns
 * - The divided vector
 */
HgVec3 hg_svdiv3(f32 scalar, HgVec3 vec);

/**
 * Divides a 4D vector by a scalar
 *
 * Parameters
 * - scalar The scalar to divide by
 * - vec The vector to divide
 * Returns
 * - The divided vector
 */
HgVec4 hg_svdiv4(f32 scalar, HgVec4 vec);

/**
 * Computes the dot product of two arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be NULL
 * - lhs The left-hand side vector, must not be NULL
 * - rhs The right-hand side vector, must not be NULL
 */
void hg_vdot(u32 size, f32* dst, f32* lhs, f32* rhs);

/**
 * Computes the dot product of two 2D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The dot product
 */
f32 hg_vdot2(HgVec2 lhs, HgVec2 rhs);

/**
 * Computes the dot product of two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The dot product
 */
f32 hg_vdot3(HgVec3 lhs, HgVec3 rhs);

/**
 * Computes the dot product of two 4D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The dot product
 */
f32 hg_vdot4(HgVec4 lhs, HgVec4 rhs);

/**
 * Computes the length of a vector
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be NULL
 * - vec The vector to compute the length of, must not be NULL
 */
void hg_vlen(u32 size, f32* dst, f32* vec);

/**
 * Computes the length of a 2D vector
 *
 * Parameters
 * - lhs The vector to compute the length of
 * Returns
 * - The length of the vector
 */
f32 hg_vlen2(HgVec2 vec);

/**
 * Computes the length of a 3D vector
 *
 * Parameters
 * - lhs The vector to compute the length of
 * Returns
 * - The length of the vector
 */
f32 hg_vlen3(HgVec3 vec);

/**
 * Computes the length of a 4D vector
 *
 * Parameters
 * - lhs The vector to compute the length of
 * Returns
 * - The length of the vector
 */
f32 hg_vlen4(HgVec4 vec);

/**
 * Normalizes a vector
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be NULL
 * - vec The vector to normalize, must not be NULL
 */
void hg_vnorm(u32 size, f32* dst, f32* vec);


/**
 * Normalizes a 2D vector
 *
 * Parameters
 * - vec The vector to normalize
 * Returns
 * - The normalized vector
 */
HgVec2 hg_vnorm2(HgVec2 vec);

/**
 * Normalizes a 3D vector
 *
 * Parameters
 * - vec The vector to normalize
 * Returns
 * - The normalized vector
 */
HgVec3 hg_vnorm3(HgVec3 vec);

/**
 * Normalizes a 4D vector
 *
 * Parameters
 * - vec The vector to normalize
 * Returns
 * - The normalized vector
 */
HgVec4 hg_vnorm4(HgVec4 vec);

/**
 * Computes the cross product of two 3D vectors
 *
 * Parameters
 * - dst The destination vector, must not be NULL
 * - lhs The left-hand side vector, must not be NULL
 * - rhs The right-hand side vector, must not be NULL
 */
void hg_vcross(f32* dst, f32* lhs, f32* rhs);

/**
 * Computes the cross product of two 3D vectors
 *
 * Parameters
 * - lhs The left-hand side vector
 * - rhs The right-hand side vector
 * Returns
 * - The cross product
 */
HgVec3 hg_vcross3(HgVec3 lhs, HgVec3 rhs);

/**
 * Adds two arbitrary size matrices
 *
 * Parameters
 * - width The width of the matrices
 * - height The height of the matrices
 * - dst The destination matrix, must not be NULL
 * - lhs The left-hand side matrix, must not be NULL
 * - rhs The right-hand side matrix, must not be NULL
 */
void hg_madd(u32 width, u32 height, f32* dst, f32* lhs, f32* rhs);

/**
 * Adds two 2x2 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The added matrix
 */
HgMat2 hg_madd2(HgMat2 lhs, HgMat2 rhs);

/**
 * Adds two 3x3 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The added matrix
 */
HgMat3 hg_madd3(HgMat3 lhs, HgMat3 rhs);

/**
 * Adds two 4x4 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The added matrix
 */
HgMat4 hg_madd4(HgMat4 lhs, HgMat4 rhs);

/**
 * Subtracts two arbitrary size matrices
 *
 * Parameters
 * - width The width of the matrices
 * - height The height of the matrices
 * - dst The destination matrix, must not be NULL
 * - lhs The left-hand side matrix, must not be NULL
 * - rhs The right-hand side matrix, must not be NULL
 */
void hg_msub(u32 width, u32 height, f32* dst, f32* lhs, f32* rhs);

/**
 * Subtracts two 2x2 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The subtracted matrix
 */
HgMat2 hg_msub2(HgMat2 lhs, HgMat2 rhs);

/**
 * Subtracts two 3x3 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The subtracted matrix
 */
HgMat3 hg_msub3(HgMat3 lhs, HgMat3 rhs);


/**
 * Subtracts two 4x4 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The subtracted matrix
 */
HgMat4 hg_msub4(HgMat4 lhs, HgMat4 rhs);

/**
 * Multiplies two arbitrary size matrices
 *
 * Parameters
 * - dst The destination matrix, must not be NULL
 * - wl The width of the left-hand side matrix
 * - hl The height of the left-hand side matrix
 * - lhs The left-hand side matrix, must not be NULL
 * - wr The width of the right-hand side matrix
 * - hr The height of the right-hand side matrix
 * - rhs The right-hand side matrix, must not be NULL
 */
void hg_mmul(f32* dst, u32 wl, u32 hl, f32* lhs, u32 wr, u32 hr, f32* rhs);

/**
 * Multiplies two 2x2 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The multiplied matrix
 */
HgMat2 hg_mmul2(HgMat2 lhs, HgMat2 rhs);

/**
 * Multiplies two 3x3 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The multiplied matrix
 */
HgMat3 hg_mmul3(HgMat3 lhs, HgMat3 rhs);

/**
 * Multiplies two 4x4 matrices
 *
 * Parameters
 * - lhs The left-hand side matrix
 * - rhs The right-hand side matrix
 * Returns
 * - The multiplied matrix
 */
HgMat4 hg_mmul4(HgMat4 lhs, HgMat4 rhs);

/**
 * Multiplies a matrix and a vector
 *
 * Parameters
 * - width The width of the matrix
 * - height The height of the matrix
 * - dst The destination vector, must not be NULL
 * - mat The matrix to multiply with, must not be NULL
 * - vec The vector to multiply with, must not be NULL
 */
void hg_mvmul(u32 width, u32 height, f32* dst, f32* mat, f32* vec);

/**
 * Multiplies a 2x2 matrix and a 2D vector
 *
 * Parameters
 * - lhs The matrix to multiply with
 * - rhs The vector to multiply with
 * Returns
 * - The multiplied vector
 */
HgVec2 hg_mvmul2(HgMat2 lhs, HgVec2 rhs);

/**
 * Multiplies a 3x3 matrix and a 3D vector
 *
 * Parameters
 * - lhs The matrix to multiply with
 * - rhs The vector to multiply with
 * Returns
 * - The multiplied vector
 */
HgVec3 hg_mvmul3(HgMat3 lhs, HgVec3 rhs);

/**
 * Multiplies a 4x4 matrix and a 4D vector
 *
 * Parameters
 * - lhs The matrix to multiply with
 * - rhs The vector to multiply with
 * Returns
 * - The multiplied vector
 */
HgVec4 hg_mvmul4(HgMat4 lhs, HgVec4 rhs);

/**
 * Adds two complex numbers
 *
 * Parameters
 * - lhs The left-hand side complex number
 * - rhs The right-hand side complex number
 * Returns
 * - The added complex number
 */
HgComplex hg_cadd(HgComplex lhs, HgComplex rhs);

/**
 * Subtracts two complex numbers
 *
 * Parameters
 * - lhs The left-hand side complex number
 * - rhs The right-hand side complex number
 * Returns
 * - The subtracted complex number
 */
HgComplex hg_csub(HgComplex lhs, HgComplex rhs);

/**
 * Multiplies two complex numbers
 *
 * Parameters
 * - lhs The left-hand side complex number
 * - rhs The right-hand side complex number
 * Returns
 * - The multiplied complex number
 */
HgComplex hg_cmul(HgComplex lhs, HgComplex rhs);

/**
 * Adds two quaternions
 *
 * Parameters
 * - lhs The left-hand side quaternion
 * - rhs The right-hand side quaternion
 * Returns
 * - The added quaternion
 */
HgQuat hg_qadd(HgQuat lhs, HgQuat rhs);

/**
 * Subtracts two quaternions
 *
 * Parameters
 * - lhs The left-hand side quaternion
 * - rhs The right-hand side quaternion
 * Returns
 * - The subtracted quaternion
 */
HgQuat hg_qsub(HgQuat lhs, HgQuat rhs);

/**
 * Multiplies two quaternions
 *
 * Parameters
 * - lhs The left-hand side quaternion
 * - rhs The right-hand side quaternion
 * Returns
 * - The multiplied quaternion
 */
HgQuat hg_qmul(HgQuat lhs, HgQuat rhs);

/**
 * Computes the conjugate of a quaternion
 *
 * Parameters
 * - quat The quaternion to compute the conjugate of
 * Returns
 * - The conjugate of the quaternion
 */
HgQuat hg_qconj(HgQuat quat);

/**
 * Creates a rotation quaternion from an axis and angle
 *
 * Parameters
 * - axis The axis of the rotation
 * - angle The angle of the rotation
 * Returns
 * - The created quaternion
 */
HgQuat hg_axis_angle(HgVec3 axis, f32 angle);

/**
 * Rotates a 3D vector using a quaternion
 *
 * Parameters
 * - lhs The quaternion to rotate with
 * - rhs The vector to rotate
 * Returns
 * - The rotated vector
 */
HgVec3 hg_rotate_vec3(HgQuat lhs, HgVec3 rhs);

/**
 * Rotates a 3x3 matrix using a quaternion
 *
 * Parameters
 * - lhs The quaternion to rotate with
 * - rhs The matrix to rotate
 * Returns
 * - The rotated matrix
 */
HgMat3 hg_rotate_mat3(HgQuat lhs, HgMat3 rhs);

// random number generators : TODO

// hash functions : TODO

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
HgMat4 hg_model_matrix_2d(HgVec3 position, HgVec2 scale, f32 rotation);

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
HgMat4 hg_model_matrix_3d(HgVec3 position, HgVec3 scale, HgQuat rotation);

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
HgMat4 hg_view_matrix(HgVec3 position, f32 zoom, HgQuat rotation);

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
u32 hg_max_mipmaps(u32 width, u32 height, u32 depth);

/**
 * An arena allocator
 *
 * Allocations are made very quickly, and are not freed individually, instead
 * the whole block is freed at once
 */
typedef struct HgArena{
    /*
     * A pointer to the memory being allocated
     */
    void *data;
    /*
     * The total capacity of the data in bytes
     */
    usize capacity;
    /*
     * The next allocation to be given out
     */
    usize head;
} HgArena;

/**
 * Allocates an arena with capacity
 *
 * Parameters
 * - hg The hg context to allocate from
 * - capacity The size of the block to allocate and use
 * Returns
 * - The allocated arena
 */
HgArena hg_arena_create(usize capacity);

/**
 * Frees an arena's memory
 *
 * Parameters
 * - hg The hg context to free to
 * - arena The arena to destroy
 */
void hg_arena_destroy(HgArena *arena);

/**
 * Frees all allocations from an arena
 *
 * Parameters
 * - arena The arena to reset, must not be NULL
 */
void hg_arena_reset(HgArena *arena);

/**
 * Allocates memory from an arena
 *
 * Allocations are not individually freed, hg_arena() is
 * called instead to free all allocations at once
 *
 * Parameters
 * - arena The arena to allocate from, must not be NULL
 * - size The size in bytes of the allocation
 * Returns
 * - The allocation if successful
 * - NULL if the allocation exceeds capacity, or size is 0
 */
void *hg_arena_alloc(HgArena *arena, usize size);

/**
 * Reallocates memory from a arena
 *
 * Simply increases the size if allocation is the most recent allocation
 *
 * Parameters
 * - arena The to allocate from, must not be NULL
 * - allocation The allocation to grow, must be the last allocation made
 * - old_size The original size in bytes of the allocation
 * - new_size The new size in bytes of the allocation
 * Returns
 * - The allocation if successful
 * - NULL if the allocation exceeds capacity
 */
void *hg_arena_realloc(HgArena *arena, void *allocation, usize old_size, usize new_size);

/**
 * Frees an allocation from a arena
 *
 * Can only deallocate the most recent allocation, otherwise does nothing
 *
 * Parameters
 * - arena The to free from, must not be NULL
 * - allocation The allocation to free, must be the last allocation made
 * - size The size of the allocation
 */
void hg_arena_free(HgArena *arena, void *allocation, usize size);

/**
 * Creates a dynamic array type
 *
 * Parameters
 * - type The type of the elements in the dynamic array
 * Returns
 * - The created struct type of the dynamic array
 */
#define HgArray(type) struct { \
    type *items; \
    usize count; \
    usize capacity; \
}

/**
 * Allocates a new dynamic array
 *
 * Parameters
 * - type The type of the elements in the dynamic array
 * - count The number of elements to already be in the array
 * - capacity The number of possible elements before reallocating
 * Returns
 * - The created dynamic array
 */
#define hg_array_create(type, count, capacity) {malloc(capacity * sizeof(type)), count, capacity}

/**
 * Frees a dynamic array
 *
 * Parameters
 * - array The array to free, must not be NULL
 */
#define hg_array_destroy(array) do { \
    free((array)->items); \
} while(0)

/**
 * Pushes an item onto the end of a dynamic array, resizing if needed
 *
 * Parameters
 * - array The array to push to, must not be NULL
 * - item The value of the item to push
 */
#define hg_array_push(array, item) do { \
    if ((array)->count == (array)->capacity) { \
        (array)->capacity = ((array)->capacity + 1) * 2; \
        (array)->items = realloc((array)->items, (array)->capacity * sizeof(*(array)->items)); \
    } \
    (array)->items[(array)->count] = item; \
    ++(array)->count; \
} while(0)

/**
 * Removes the end item of a dynamic array
 *
 * Parameters
 * - array The array to remove from, must not be NULL
 */
#define hg_array_pop(array) do { \
    --(array)->count; \
} while(0)

/**
 * Inserts an item to the index in a dynamic array, moving elements forward and
 * resizing if needed
 *
 * Parameters
 * - array The array to push to, must not be NULL
 * - index The index to insert into
 * - item The value of the item to push
 */
#define hg_array_insert(array, index, item) do { \
    assert(index >= 0 && index < (array)->count); \
    if ((array)->count == (array)->capacity) { \
        (array)->capacity = ((array)->capacity + 1) * 2; \
        (array)->items = realloc((array)->items, (array)->capacity * sizeof(*(array)->items)); \
    } \
    memmove((array)->items + index + 1, (array)->items + index, ((array)->count - index) * sizeof(*(array)->items)); \
    ++(array)->count; \
    (array)->items[index] = item; \
} while(0)

/**
 * Deletes an item at the index in a dynamic array
 *
 * Parameters
 * - array The array to delete from, must not be NULL
 * - index The index in the array to delete
 */
#define hg_array_delete(array, index) do { \
    assert(index >= 0 && index < (array)->count); \
    --(array)->count; \
    memmove((array)->items + index, (array)->items + index + 1, ((array)->count - index) * sizeof(*(array)->items)); \
} while (0)

/**
 * Loads a binary file
 *
 * Parameters
 * - data A pointer to store the loaded data, must not be NULL
 * - size A pointer to store the size of the loaded data, must not be NULL
 * - path The null terminated path to the file to load, must not be NULL
 * Returns
 * - true if the file was loaded successfully
 * - false if the file was not found or could not be read
 */
bool hg_file_load_binary(u8** data, usize* size, const char *path);

/**
 * Unloads a binary file
 *
 * Parameters
 * - data The data to unload, noop if NULL
 * - size The size of the data to unload
 */
void hg_file_unload_binary(u8* data, usize size);

/**
 * Saves a binary file
 *
 * Parameters
 * - path The path to the file to save, must not be empty
 * - data The data to save, may be NULL if size is 0
 * - size The size of the data to save, may be 0 to write an empty file
 * Returns
 * - hg_SUCCESS if the file was saved successfully
 * - hg_ERROR_FILE_WRITE_FAILURE if the file could not be written
 */
bool hg_file_save_binary(const u8* data, usize size, const char *path);

// text files : TODO
// json files : TODO
// image files : TODO
// 3d model files : TODO
// audio files : TODO

/**
 * Platform specific internal resources
 */
typedef struct HgPlatform HgPlatform;

/**
 * Creates platform specific internal resources for audio, window, etc.
 *
 * Returns
 * - An opaque pointer to the platform's resources, will never be NULL
 */
HgPlatform *hg_platform_create(void);

/**
 * Destroys the platform specific internal resources
 *
 * Parameters
 * - platform The platform resources
 */
void hg_platform_destroy(HgPlatform *platform);

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

/**
 * Platform specific resources for a window
 */
typedef struct HgWindow HgWindow;

/**
 * Configuration for a window
 */
typedef struct HgWindowConfig {
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
} HgWindowConfig;

/**
 * Creates a window
 *
 * Parameters
 * - platform The platform resources, must not be NULL
 * - config The window configuration, must not be NULL
 * Returns
 * - The created window, will never be NULL
 */
HgWindow *hg_window_create(const HgPlatform *platform, const HgWindowConfig *config);

/**
 * Destroys a window
 *
 * Parameters
 * - platform The platform resources, must not be NULL
 * - window The window to destroy, noop if NULL
 */
void hg_window_destroy(const HgPlatform *platform, HgWindow *window);

/**
 * Sets the windows icons : TODO
 *
 * Parameters
 * - platform The platform resources, must not be NULL
 * - window The window to modify, must not be NULL
 * - icon_data The pixels of the image to set the icon to, must not be NULL
 * - width The width in pixels of the icon
 * - height The height in pixels of the icon
 */
void hg_window_set_icon(const HgPlatform *platform, HgWindow *window, u32 *icon_data, u32 width, u32 height);

/**
 * Gets whether the window is fullscreen or not : TODO
 *
 * Parameters
 * - platform The platform resources, must not be NULL
 * - window The window to check, must not be NULL
 * Returns
 * - Whether the window is fullscreen
 */
bool hg_window_get_fullscreen(const HgPlatform *platform, HgWindow *window);

/**
 * Sets the window to fullscreen of windowed mode : TODO
 *
 * Parameters
 * - platform The platform resources, must not be NULL
 * - window The window to set, must not be NULL
 * - fullscreen Whether to set fullscreen, or set windowed
 */
void hg_window_set_fullscreen(const HgPlatform *platform, HgWindow *window, bool fullscreen);

/**
 * The builtin cursor images
 */
typedef enum HgCursor {
    HG_CURSOR_NONE = 0,
    HG_CURSOR_ARROW,
    HG_CURSOR_TEXT,
    HG_CURSOR_WAIT,
    HG_CURSOR_CROSS,
    HG_CURSOR_HAND,
} HgCursor;

/**
 * Sets the window's cursor to a platform defined icon : TODO
 */
void hg_window_set_cursor(const HgPlatform *platform, HgWindow *window, HgCursor cursor);

/**
 * Sets the window's cursor to a custom image : TODO
 */
void hg_window_set_cursor_image(const HgPlatform *platform, HgWindow *window, u32 *data, u32 width, u32 height);

/**
 * Create a Vulkan surface for the window, according to the platform
 *
 * Parameters
 * - instance The Vulkan instance, must not be NULL
 * - platform The hg platform context, must not be NULL
 * - window The window to create a surface for, must not be NULL
 * Returns
 * - The created Vulkan surface, will never be NULL
 */
VkSurfaceKHR hg_vk_create_surface(
    VkInstance instance,
    const HgPlatform *platform,
    const HgWindow *window);

/**
 * Processes all events since the last call to process events or startup
 *
 * Must be called every frame before querying input
 * Processes all events, so all windows must be given
 * Updates the each window's input state
 *
 * Parameters
 * - hg The hg context, must not be NULL
 * - windows All open windows
 * - window_count The number of windows
 */
void hg_window_process_events(const HgPlatform *platform, HgWindow **windows, u32 window_count);

/**
 * Checks if the window was closed via close button or window manager
 *
 * hg_window_close() is not automatically called when this function returns
 * true, and may be called manually
 *
 * Parameters
 * - window The window, must not be NULL
 * Returns
 * - whether the window was closed
 */
bool hg_window_was_closed(const HgWindow *window);

/**
 * Checks if the window was resized
 *
 * Parameters
 * - window The window, must not be NULL
 * Returns
 * - whether the window was resized
 */
bool hg_window_was_resized(const HgWindow *window);

/**
 * Gets the size of the window in pixels
 *
 * Parameters
 * - window The window, must not be NULL
 * - width A pointer to store the width, must not be NULL
 * - height A pointer to store the height, must not be NULL
 */
void hg_window_get_size(const HgWindow *window, u32 *width, u32 *height);

/**
 * Gets the most recent mouse position
 *
 * Parameters
 * - window The window, must not be NULL
 * - x A pointer to store the x position, must not be NULL
 * - y A pointer to store the y position, must not be NULL
 */
void hg_window_get_mouse_pos(const HgWindow *window, f64 *x, f64 *y);

/**
 * Gets the most recent mouse delta
 *
 * Parameters
 * - window The window, must not be NULL
 * - x A pointer to store the x delta, must not be NULL
 * - y A pointer to store the y delta, must not be NULL
 */
void hg_window_get_mouse_delta(const HgWindow *window, f64 *x, f64 *y);

/**
 * Checks if a key is being held down
 *
 * Parameters
 * - window The window, must not be NULL
 * - key The key to check
 * Returns
 * - whether the key is being held down
 */
bool hg_window_is_key_down(const HgWindow *window, HgKey key);

/**
 * Checks if a key was pressed this frame
 *
 * Parameters
 * - window The window, must not be NULL
 * - key The key to check
 * Returns
 * - whether the key was pressed this frame
 */
bool hg_window_was_key_pressed(const HgWindow *window, HgKey key);

/**
 * Checks if a key was released this frame
 *
 * Parameters
 * - window The window, must not be NULL
 * - key The key to check
 * Returns
 * - whether the key was released this frame
 */
bool hg_window_was_key_released(const HgWindow *window, HgKey key);

// audio system : TODO

/**
 * Loads the Vulkan library and the functions required to create an instance
 */
void hg_vk_load(void);

/**
 * Loads the Vulkan functions which use the instance
 *
 * Note, this function is automatically called from hg_vk_create_instance
 *
 * Parameters
 * - instance The Vulkan instance, must not be NULL
 */
void hg_vk_load_instance(VkInstance instance);

/**
 * Loads the Vulkan functions which use the Device
 *
 * Note, this function is automatically called from
 * hg_vk_create_single_queue_device
 *
 * Parameters
 * - device The Vulkan device, must not be NULL
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
 * Creates a Vulkan instance with sensible defaults
 * 
 * In debug mode, enables debug messaging
 *
 * Note, loads Vulkan function pointers automatically
 *
 * Parameters
 * - app_name The name of the application, may be NULL
 * Returns
 * - The created VkInstance, will never be NULL
 */
VkInstance hg_vk_create_instance(const char *app_name);

/**
 * Creates a Vulkan debug messenger
 *
 * Parameters
 * - instance The Vulkan instance, must not be NULL
 * Returns
 * - The created debug messenger, will never be NULL
 */
VkDebugUtilsMessengerEXT hg_vk_create_debug_messenger(VkInstance instance);

/**
 * Find the first queue family index which supports the the queue flags
 *
 * Parameters
 * - gpu The physical device, must not be NULL
 * - queue_family A pointer to store the found queue family
 * - queue_flags The flags required of the queue family
 * Returns
 * - Whether a family was found
 */
bool hg_vk_find_queue_family(VkPhysicalDevice gpu, u32 *queue_family, VkQueueFlags queue_flags);

// find gpu with multiple potential queues : TODO
// create device with multiple potential queues : TODO

/**
 * A Vulkan device with a single general-purpose queue
 */
typedef struct HgSingleQueueDeviceData {
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
} HgSingleQueueDeviceData;

/**
 * Creates a Vulkan device with a single general-purpose queue
 *
 * Enables the swapchain extension, and the synchronization 2 and dynamic
 * rendering features
 *
 * Note, loads Vulkan function pointers automatically
 *
 * Parameters
 * - gpu The physical device, must not be NULL
 * - queue_family Which family to create the queue in
 * Returns
 * - The created Vulkan device, will never be NULL
 */
HgSingleQueueDeviceData hg_vk_create_single_queue_device(VkInstance instance);

/**
 * Configuration for Vulkan pipelines
 */
typedef struct HgVkPipelineConfig {
    /**
     * The format of the color attachments, none can be UNDEFINED
     */
    VkFormat *color_attachment_formats;
    /**
     * The number of color attachments to use
     */
    u32 color_attachment_count;
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
    const VkPipelineShaderStageCreateInfo *shader_stages;
    /**
     * The number of shaders and their stages
     */
    u32 shader_count;
    /**
     * The pipeline layout
     */
    VkPipelineLayout layout;
    /**
     * Descriptions of the vertex bindings, may be NULL
     */
    const VkVertexInputBindingDescription* vertex_bindings;
    /**
     * The number of vertex bindings, may be 0
     */
    u32 vertex_binding_count;
    /**
     * Descriptions of the vertex attributes, may be NULL
     */
    const VkVertexInputAttributeDescription* vertex_attributes;
    /**
     * The number of vertex attributes, may be 0
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
} HgVkPipelineConfig;

/**
 * Creates a graphics pipeline
 *
 * Parameters
 * - device The Vulkan device, must not be NULL
 * - config The pipeline configuration, must not be NULL
 * Returns
 * - The created graphics pipeline, will never be NULL
 */
VkPipeline hg_vk_create_graphics_pipeline(VkDevice device, const HgVkPipelineConfig *config);

/**
 * Creates a compute pipeline
 *
 * There cannot be any attachments
 * There can only be one shader stage, COMPUTE
 * There cannot be any vertex inputs
 *
 * Parameters
 * - device The Vulkan device, must not be NULL
 * - config The pipeline configuration, must not be NULL
 * Returns
 * - The created compute pipeline, will never be NULL
 */
VkPipeline hg_vk_create_compute_pipeline(VkDevice device, const HgVkPipelineConfig *config);

/**
 * Attempts to find the index of a memory type which has the desired flags and
 * does not have the undesired flags
 *
 * Note, if no such memory type exists, the next best thing will be found
 *
 * Parameters
 * - gpu The Vulkan physical device, must not be NULL
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
typedef struct HgSwapchainData {
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
} HgSwapchainData;

/**
 * Creates a Vulkan swapchain
 *
 * Parameters
 * - device The Vulkan device, must not be NULL
 * - gpu The physical device to query capabilities, must not be NULL
 * - old_swapchain The old swapchain, may be NULL
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
typedef struct HgFrameSync {
    VkCommandPool cmd_pool;
    VkSwapchainKHR swapchain;
    void *allocation;
    VkCommandBuffer *cmds;
    VkFence *frame_finished;
    VkSemaphore *image_available;
    VkSemaphore *ready_to_present;
    u32 frame_count;
    u32 current_frame;
    u32 current_image;
} HgFrameSync;

/**
 * Creates a frame sync system
 *
 * Parameters
 * - device The Vulkan device, must not be NULL
 * - cmd_pool The Vulkan command pool to allocate cmds from, must not be NULL
 * - swapchain The Vulkan swapchain to create frames for, must not be NULL
 * Returns
 * - The created frame sync system
 */
HgFrameSync hg_frame_sync_create(VkDevice device, VkCommandPool cmd_pool, VkSwapchainKHR swapchain);

/**
 * Destroys a frame sync system
 *
 * Note, it is safe to call begin_frame with a destroyed frame sync, it will
 * simply return NULL, but it is not safe to call end_frame_and_present
 *
 * Parameters
 * - device The Vulkan device, must not be NULL
 * - sync The frame sync system to destroy, must not be NULL
 */
void hg_frame_sync_destroy(VkDevice device, HgFrameSync *sync);

/**
 * Acquires the next swapchain image and begins its command buffer
 *
 * Parameters
 * - device The Vulkan device, must not be NULL
 * - sync The frame sync system, must not be NULL
 * Returns
 * - The command buffer to record this frame
 * - NULL if the swapchain is out of date
 */
VkCommandBuffer hg_frame_sync_begin_frame(VkDevice device, HgFrameSync *sync);

/**
 * Finishes recording the command buffer and presents the swapchain image
 *
 * Parameters
 * - queue The Vulkan queue, must not be NULL
 * - sync The frame sync system, must not be NULL
 */
void hg_frame_sync_end_frame_and_present(VkQueue queue, HgFrameSync *sync);

// Vulkan resource utilities : TODO

// staging buffer read/write : TOD
// staging image read/write : TOD
// cubemap read/write : TODO
// mipmap generation : TODO

/**
 * A pipeline to render 2D sprites
 */
typedef struct HgPipelineSprite {
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
} HgPipelineSprite;

/**
 * Creates a pipeline abstraction to render 2D sprites
 * 
 * Parameters
 * - device The Vulkan device, must not be NULL
 * - allocator The VMA allocator, must not be NULL,
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
 * - pipeline The pipeline to update, must not be NULL
 * - projection The value to update to, must not be NULL
 */
void hg_pipeline_sprite_update_projection(HgPipelineSprite *pipeline, HgMat4 *projection);

/**
 * Updates the sprite pipeline's view matrix
 *
 * Parameters
 * - pipeline The pipeline to update, must not be NULL
 * - view The value to update to, must not be NULL
 */
void hg_pipeline_sprite_update_view(HgPipelineSprite *pipeline, HgMat4 *view);

/**
 * The texture resources used by HgPipelineSprite
 */
typedef struct HgPipelineSpriteTexture {
    VmaAllocation allocation;
    VkImage image;
    VkImageView view;
    VkSampler sampler;
    VkDescriptorSet set;
} HgPipelineSpriteTexture;

/**
 * Configuration for a hgSpritePipelineTexture
 */
typedef struct HgPipelineSpriteTextureConfig {
    /**
     * The pixel data to use, must not be NULL
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
     * The size in bytes of each pixel, must be greater than 0
     */
    u32 pixel_width;
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
} HgPipelineSpriteTextureConfig;

/**
 * Creates a texture for HgPipelineSprite
 *
 * Note, if for some reason there are multiple HgPipelineSprite objects,
 * textures are compatible between them, so need not be duplicated
 *
 * Parameters
 * - pipeline The pipeline to create for, must not be NULL
 * - cmd_pool The command pool to get a command buffer from, must not be NULL
 * - transfer_queue The queue to transfer the data on, must not be NULL
 * - config The configuration for the texture, must not be NULL
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
 * - pipeline The pipeline, must not be NULL
 * - texture The texture to destroy, must not be NULL
 */
void hg_pipeline_sprite_destroy_texture(HgPipelineSprite *pipeline, HgPipelineSpriteTexture *texture);

/**
 * Binds the pipeline in a command buffer
 *
 * Note, the command buffer should be in a render pass, and the dynamic viewport
 * and scissor must be set
 *
 * Parameters
 * - pipeline The pipeline to bind, must not be NULL
 * - cmd The command buffer, must not be NULL
 */
void hg_pipeline_sprite_bind(HgPipelineSprite *pipeline, VkCommandBuffer cmd);

/**
 * The data pushed to each sprite draw call
 */
typedef struct HgPipelineSpritePush {
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
} HgPipelineSpritePush;

/**
 * Draws a sprite using the sprite pipeline
 *
 * The HpPipelineSprite must already be bound
 *
 * Parameters
 * - pipeline The pipeline, must not be NULL
 * - cmd The command buffer, must not be NULL
 * - texture The texture to read from, must not be NULL
 * - push_data The data to push to the draw call, must not be NULL
 */
void hg_pipeline_sprite_draw(
    HgPipelineSprite *pipeline,
    VkCommandBuffer cmd,
    HgPipelineSpriteTexture *texture,
    HgPipelineSpritePush *push_data);

#ifdef __cplusplus
}
#endif

#endif // HURDYGURDY_H

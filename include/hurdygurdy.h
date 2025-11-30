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
#define hg_debug(...) hg_debug_mode({ (void)fprintf(stderr, "Hurdygurdy Debug: " __VA_ARGS__); })

/**
 * Formats and logs an info message to stderr
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_info(...) { (void)fprintf(stderr, "Hurdygurdy Info: " __VA_ARGS__); }

/**
 * Formats and logs a warning message to stderr
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_warn(...) { (void)fprintf(stderr, "Hurdygurdy Warning: " __VA_ARGS__); }

/**
 * Formats and logs an error message to stderr and aborts the program
 *
 * Parameters
 * - ... The message to print and its format parameters
 */
#define hg_error(...) { (void)fprintf(stderr, "Hurdygurdy Error: " __VA_ARGS__); abort(); }

/**
 * Crashes on failure with an error message, in both debug and release builds
 *
 * Parameters
 * - cond The condition to check
 * - ... The message to print and its format parameters
 */
#define hg_assert(cond) hg_debug_mode({ if (!(cond)) { \
    (void)fprintf(stderr, "Hurdygurdy Assertion Error in %s(): " #cond "\n", __func__); \
} })

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
HgMat4 hg_orthographic_projection(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far);

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
HgMat4 hg_perspective_projection(f32 fov, f32 aspect, f32 near, f32 far);

/**
 * An arena allocator
 *
 * Allocations are made very quickly, and are not freed individually, instead
 * the whole block is freed at once
 *
 * Note, is not thread safe
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
 * A high precision clock for timers and game deltas
 */
typedef struct HgClock {
    struct timespec time;
} HgClock;

/**
 * Resets the clock and gets the delta since the last tick in seconds
 *
 * Parameters
 * - clock The clock to tick, must not be NULL
 * Returns
 * - Seconds since last tick
 */
f64 hg_clock_tick(HgClock *hclock);

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
 * Loads the Vulkan library and the functions required to create an instance
 */
void hg_vk_load(void);

/**
 * Loads the Vulkan functions which use the instance
 *
 * Parameters
 * - instance The Vulkan instance, must not be VK_NULL_HANDLE
 */
void hg_vk_load_instance(VkInstance instance);

/**
 * Loads the Vulkan functions which use the Device
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 */
void hg_vk_load_device(VkDevice device);

/**
 * Checks a VkResult enum, and terminates on errors
 *
 * Parameters
 * - result The VkResult to check
 */
void hg_vk_check(VkResult result);

/**
 * Creates a Vulkan instance with sensible defaults
 * 
 * In debug mode, enables debug messaging
 *
 * Parameters
 * - app_name The name of the application, may be NULL
 * Returns
 * - The created VkInstance, will never be VK_NULL_HANDLE
 */
VkInstance hg_vk_create_instance(const char *app_name);

/**
 * Destroys a Vulkan instance
 * 
 * Parameters
 * - instance The VkInstance to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_instance(VkInstance instance);

/**
 * Creates a Vulkan debug messenger
 *
 * Parameters
 * - instance The Vulkan instance, must not be VK_NULL_HANDLE
 * Returns
 * - The created debug messenger, will never be VK_NULL_HANDLE
 */
VkDebugUtilsMessengerEXT hg_vk_create_debug_messenger(VkInstance instance);

/**
 * Destroys a Vulkan debug messenger
 *
 * Parameters
 * - instance The Vulkan instance, must not be VK_NULL_HANDLE
 * - messenger The debug messenger to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_debug_messenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger);

// find gpu with multiple potential queues : TODO

/**
 * Finds a suitable Vulkan physical device
 *
 * Searches for a physical device which supports the swapchain extension, and
 * has at least one queue family which supports both graphics and compute
 *
 * Parameters
 * - instance The Vulkan instance, must not be VK_NULL_HANDLE
 * - queue_family A pointer to store the queue family, should not be NULL
 * Returns
 * - The found gpu, may be VK_NULL_HANDLE if no suitable gpu can be found
 */
VkPhysicalDevice hg_vk_find_single_queue_gpu(VkInstance instance, u32 *queue_family);

// create device with multiple potential queues : TODO

/**
 * Creates a Vulkan logical device with sensible defaults and only one queue
 *
 * Enables the swapchain extension, and the synchronization 2 and dynamic
 * rendering features
 * Creates 1 queue on the given queue_family
 *
 * Note, Vulkan functions will be loaded using this device
 *
 * Parameters
 * - gpu The physical device, must not be VK_NULL_HANDLE
 * - queue_family Which family to create the queue in
 * Returns
 * - The created Vulkan device, will never be VK_NULL_HANDLE
 */
VkDevice hg_vk_create_single_queue_device(VkPhysicalDevice gpu, u32 queue_family);

/**
 * Destroy a Vulkan logical device
 *
 * Parameters
 * - device The Vulkan device to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_device(VkDevice device);

/**
 * Waits for the device to idle
 *
 * Paramaters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 */
void hg_vk_wait_for_device(VkDevice device);

/**
 * Creates a Vulkan swapchain
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - gpu The physical device to query capabilities, must not be VK_NULL_HANDLE
 * - old_swapchain The old swapchain, may be VK_NULL_HANDLE
 * - surface The surface to create from
 * - image_usage How the swapchain's images will be used
 * - desired_mode The preferred present mode (fallback to FIFO)
 * - width A pointer to store the width of the swapchain, must not be NULL
 * - height A pointer to store the height of the swapchain, must not be NULL
 * - format A pointer to store the format of the swapchain, must not be NULL
 * - image_count A pointer to store the number of images, must not be NULL
 * Returns
 * - The created Vulkan swapchain
 */
VkSwapchainKHR hg_vk_create_swapchain(
    VkDevice device,
    VkPhysicalDevice gpu,
    VkSwapchainKHR old_swapchain,
    VkSurfaceKHR surface,
    VkImageUsageFlags image_usage,
    VkPresentModeKHR desired_mode,
    u32 *width,
    u32 *height,
    VkFormat *format,
    u32 *image_count);

/**
 * Destroys a Vulkan swapchain
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - swapchain The swapchain to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_swapchain(VkDevice device, VkSwapchainKHR swapchain);

/**
 * Gets the images from a swapchain
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - swapchain The swapchain to get the images from, must not be VK_NULL_HANDLE
 * - images A buffer to store the images in, must not be NULL
 * - count The number of images, must be equal to the number the swapchain has
 */
void hg_vk_get_swapchain_images(VkDevice device, VkSwapchainKHR swapchain, VkImage *images, u32 count);

/**
 * Gets the index of the next image in the Swapchain, signaling a semaphore
 * and/or fence when it becomes available
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - swapchain The swapchain to get from, must not be VK_NULL_HANDLE
 * - image_index A pointer to store the index, must not be NULL
 * - signal_semaphore The semaphore to signal, may be VK_NULL_HANDLE
 * - signal_fence The fence to signal, may be VK_NULL_HANDLE
 * Returns
 * - Whether the image was acquired successfully
 */
bool hg_vk_acquire_next_image(
    VkDevice device,
    VkSwapchainKHR swapchain,
    u32 *image_index,
    VkSemaphore signal_semaphore,
    VkFence signal_fence);

/**
 * Presents the swapchain to the display
 *
 * Parameters
 * - queue The queue to use to present, must not be VK_NULL_HANDLE
 * - swapchain The swapchain to present, must not be VK_NULL_HANDLE
 * - image_index The image in the swapchain
 * - wait_semaphores The semaphores to wait for, may be NULL
 * - semaphore_count The number of semaphores in wait_semaphores, may be 0
 */
void hg_vk_present(
    VkQueue queue,
    VkSwapchainKHR swapchain,
    u32 image_index,
    VkSemaphore *wait_semaphores,
    u32 semaphore_count);

/**
 * Creates a Vulkan semaphore
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - flags The optional flags to use
 * Returns
 * - The created Vulkan semaphore, will never be VK_NULL_HANDLE
 */
VkSemaphore hg_vk_create_semaphore(VkDevice device, VkSemaphoreCreateFlags flags);

/**
 * Destroys a Vulkan semaphore
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - semaphore The Vulkan semaphore to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_semaphore(VkDevice device, VkSemaphore semaphore);

/**
 * Creates a Vulkan fence
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - flags The optional flags to use
 * Returns
 * - The created Vulkan fence, will never be VK_NULL_HANDLE
 */
VkFence hg_vk_create_fence(VkDevice device, VkFenceCreateFlags flags);

/**
 * Destroys a Vulkan fence
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - semaphore The Vulkan fence to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_fence(VkDevice device, VkFence fence);

/**
 * Waits for Vulkan fences to be signaled
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - fences The fences to wait for, may be NULL if count is 0
 * - count The number of fences, noop if 0
 */
void hg_vk_wait_for_fences(VkDevice device, VkFence *fences, u32 count);

/**
 * Resets (unsignals) fences
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - fences The fences to reset, may be NULL if count is 0
 * - count The number of fences, noop if 0
 */
void hg_vk_reset_fences(VkDevice device, VkFence *fences, u32 count);

/**
 * Find the first queue family index which supports the the queue flags
 *
 * Parameters
 * - gpu The physical device, if 0 returns false
 * - queue_family A pointer to store the found queue family, must not be NULL
 * - queue_flags The flags required of the queue family
 * Returns
 * - Whether a family was found
 */
bool hg_vk_find_queue_family(VkPhysicalDevice gpu, u32 *queue_family, VkQueueFlags queue_flags);

/**
 * Gets a created queue from the device
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - queue_family The queue_family to get from
 * - queue_index The index within the family
 * Returns
 * - The queue, may be VK_NULL_HANDLE if queue_famiy or queue_index are out of
 *   range for the device
 */
VkQueue hg_vk_get_queue(VkDevice device, u32 queue_family, u32 queue_index);

/**
 * Wait for a queue to finish commands
 *
 * Can be used to ensure resources are free before destruction
 *
 * Parameters
 * - queue The Vulkan queue to wait for, noop if VK_NULL_HANDLE
 */
void hg_vk_queue_wait(VkQueue queue);

/**
 * Submits a command buffer for execution
 *
 * Parameters
 * - queue The queue to submit to, must not be VK_NULL_HANDLE
 * - submits The submissions, may be NULL if count is 0
 * - submit_count The number of submissions, noop if 0
 * - fence The fence to signal upon completion, may be VK_NULL_HANDLE
 */
void hg_vk_submit_commands(VkQueue queue, const VkSubmitInfo *submits, u32 submit_count, VkFence fence);

/**
 * Creates a Vulkan command pool
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - queue_family The index of the queue family to allocate in
 * - flags The optional flags to use
 * Returns
 * - The created Vulkan command pool, will never be VK_NULL_HANDLE
 */
VkCommandPool hg_vk_create_command_pool(VkDevice device, u32 queue_family, VkCommandPoolCreateFlags flags);

/**
 * Destroys a Vulkan command pool
 *
 * Parameters
 * - vk The hg Vulkan context, must not be VK_NULL_HANDLE
 * - pool The Vulkan command pool to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_command_pool(VkDevice device, VkCommandPool pool);

/**
 * Creates Vulkan commands buffers, filling a buffer
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - pool The command pool to allocate from, must not be VK_NULL_HANDLE
 * - cmds A pointer to store the command buffers, may be NULL if count is 0
 * - count The number of command buffers to create, noop if 0
 */
void hg_vk_allocate_command_buffers(
    VkDevice device,
    VkCommandPool pool,
    VkCommandBuffer *cmds,
    u32 count,
    VkCommandBufferLevel level);

/**
 * Frees Vulkan commands buffers
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - pool The command pool to free to, must not be VK_NULL_HANDLE
 * - cmds The command buffers to free, must not be NULL
 * - count The size of cmds and the number of command buffers to free
 */
void hg_vk_free_command_buffers(VkDevice device, VkCommandPool pool, VkCommandBuffer *cmds, u32 count);

/**
 * Resets a Vulkan command buffer
 *
 * Parameters
 * - cmd The command buffer to reset
 * - flags The optional flags to use
 */
void hg_vk_reset_command_buffer(VkCommandBuffer cmd, VkCommandBufferResetFlags flags);

/**
 * Creates a Vulkan descriptor pool
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - max_sets The maximum number of sets which can be allocated
 * - sizes The numbers of each descriptor type to allocate, must not be NULL
 * - count The count of sizes, must be greater than 0
 * Returns
 * - The created Vulkan descriptor pool, will never be VK_NULL_HANDLE
 */
VkDescriptorPool hg_vk_create_descriptor_pool(
    VkDevice device,
    u32 max_sets,
    VkDescriptorPoolSize *sizes,
    u32 count);

/**
 * Destroys a Vulkan descriptor pool
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - pool The Vulkan descriptor pool to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_descriptor_pool(VkDevice device, VkDescriptorPool pool);

/**
 * Resets a Vulkan descriptor pool, freeing all sets
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - pool The descriptor pool to reset, must not be VK_NULL_HANDLE
 */
void hg_vk_reset_descriptor_pool(VkDevice device, VkDescriptorPool pool);

/**
 * Allocates Vulkan descriptor sets from a descriptor pool
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - pool The descriptor pool to allocate from, must not be VK_NULL_HANDLE
 * - layouts The layouts of the sets to allocate, must not be NULL
 * - sets A pointer to store the allocated sets, must not be NULL
 * - count The number of sets to allocate, must be greater than 0
 * Returns
 * - Whether the allocation succeeded, or the pool is empty or fragmented
 */
bool hg_vk_allocate_descriptor_sets(
    VkDevice device,
    VkDescriptorPool pool,
    VkDescriptorSetLayout *layouts,
    VkDescriptorSet *sets,
    u32 count);

/**
 * Updates Vulkan descriptor sets by writing and copying
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - writes The writes to make
 * - write_count The number of writes
 * - copies The copies to make
 * - copy_count The number of copies
 */
void hg_vk_update_descriptor_sets(
    VkDevice device,
    const VkWriteDescriptorSet* writes,
    u32 write_count,
    const VkCopyDescriptorSet* copies,
    u32 copy_count);

/**
 * Writes to a single descriptor set
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - set The descriptor set to write to, must not be VK_NULL_HANDLE
 * - binding The binding in the descriptor set
 * - first_array_elem The index of the first array element to write to
 * - descriptor_type The type of descriptor
 * - image_info The image info if the type is an image type
 * - buffer_info The buffer info if the type is an buffer type
 * - texel_buffer_view The texel buffer view if the type is a texel buffer
 */
void hg_vk_write_descriptor_set(
    VkDevice device,
    VkDescriptorSet set,
    u32 binding,
    u32 first_array_elem,
    u32 descriptor_count,
    VkDescriptorType descriptor_type,
    const VkDescriptorImageInfo *image_info,
    const VkDescriptorBufferInfo *buffer_info,
    const VkBufferView *texel_buffer_view);

/**
 * Creates a Vulkan descriptor set layout
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - bindings Descriptions of each binding in the layout, must not be NULL
 * - count The number of bindings, must be greater than 0
 */
VkDescriptorSetLayout hg_vk_create_descriptor_set_layout(
    VkDevice device,
    const VkDescriptorSetLayoutBinding *bindings,
    u32 count);

/**
 * Destroys a Vulkan descriptor set layout
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - layout The layout to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_descriptor_set_layout(VkDevice device, VkDescriptorSetLayout layout);

/**
 * Creates a Vulkan pipeline layout
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - layouts The descriptor set layouts to include, may be NULL if none
 * - layout_count The number of descriptor set layouts, may be 0
 * - push_constants The push constant ranges to include, may be NULL
 * - push_constant_count The number of push constant ranges, may be 0
 * Returns
 * - The created Vulkan pipeline layout, will never be VK_NULL_HANDLE
 */
VkPipelineLayout hg_vk_create_pipeline_layout(
    VkDevice device,
    const VkDescriptorSetLayout* layouts,
    u32 layout_count,
    const VkPushConstantRange* push_constants,
    u32 push_constant_count
);

/**
 * Destroys a Vulkan pipeline layout
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - layout The layout to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_pipeline_layout(VkDevice device, VkPipelineLayout layout);

/**
 * Creates a Vulkan shader module
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - code The SPIR-V bytecode of the shader, must not be NULL
 * - size The size of the bytecode in bytes, must be greater than 0
 * Returns
 * - The created Vulkan shader module, will never be VK_NULL_HANDLE
 */
VkShaderModule hg_vk_create_shader_module(VkDevice device, const u8 *code, usize size);

/**
 * Destroys a Vulkan shader module
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - shader_module The shader module to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_shader_module(VkDevice device, VkShaderModule shader_module);

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
     * The pipeline layout
     */
    VkPipelineLayout layout;
    /**
     * The shaders
     */
    const VkPipelineShaderStageCreateInfo *shader_stages;
    /**
     * The number of shaders and their stages
     */
    u32 shader_count;
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
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - config The pipeline configuration, must not be NULL
 * Returns
 * - The created graphics pipeline, will never be VK_NULL_HANDLE
 */
VkPipeline hg_vk_create_graphics_pipeline(VkDevice device, const HgVkPipelineConfig *config);

/**
 * Creates a compute pipeline
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - config The pipeline configuration, must not be NULL
 * Returns
 * - The created compute pipeline, will never be VK_NULL_HANDLE
 */
VkPipeline hg_vk_create_compute_pipeline(VkDevice device, const HgVkPipelineConfig *config);

/**
 * Destroys a Vulkan pipeline
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - pipeline The pipeline to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_pipeline(VkDevice device, VkPipeline pipeline);

/**
 * Creates a Vulkan buffer
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - config The configuration for the buffer, must not be VK_NULL_HANDLE
 * Returns
 * - The created Vulkan buffer, will never be VK_NULL_HANDLE
 */
VkBuffer hg_vk_create_buffer(VkDevice device, const VkBufferCreateInfo *config);

/**
 * Destroys a Vulkan buffer
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - buffer The buffer to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_buffer(VkDevice device, VkBuffer buffer);

/**
 * Creates a Vulkan image
 *
 * Default configs:
 * - extent.width 0 defaults to 1
 * - extent.height 0 defaults to 1
 * - extent.depth 0 defaults to 1
 * - mipLevels 0 defaults to 1
 * - mipLevels UINT32_MAX defaults to max mips for the extent
 * - arrayLayers 0 defaults to 1
 * - sampler 0 default to 1
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - create_info The configuration for the image, must not be VK_NULL_HANDLE
 * Returns
 * - The created Vulkan image, will never be VK_NULL_HANDLE
 */
VkImage hg_vk_create_image(VkDevice device, const VkImageCreateInfo *config);

/**
 * Destroys a Vulkan image
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - image The Vulkan image to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_image(VkDevice device, VkImage image);

/**
 * Creates an image view for a Vulkan image
 *
 * Note, subresource.levelCount 0 defaults to REMAINING_MIP_LEVELS, and
 * subresource.layerCount 0 defaults to REMAINING_ARRAY_LAYERS
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - image The image to view, must not be VK_NULL_HANDLE
 * - format The pixel format of the image, must not be UNDEFINED
 * - type The type of image
 * - subresource The range of subresources in the image to view
 * Returns
 * - The created image view, will never be VK_NULL_HANDLE
 */
VkImageView hg_vk_create_image_view(
    VkDevice device,
    VkImage image,
    VkFormat format,
    VkImageViewType type,
    VkImageSubresourceRange subresource);

/**
 * Destroys a Vulkan image view
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - image_view The image view to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_image_view(VkDevice device, VkImageView image_view);

/**
 * Creates a Vulkan sampler
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - filter The filtering method to use
 * - edge_mode The addressing mode for texture edges
 * Returns
 * - The created sampler, will never be VK_NULL_HANDLE
 */
VkSampler hg_vk_create_sampler(VkDevice device, VkFilter filter, VkSamplerAddressMode edge_mode);

/**
 * Destroys a Vulkan sampler
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - sampler The Vulkan sampler to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_sampler(VkDevice device, VkSampler sampler);

/**
 * Gets memory requirements for a buffer
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - buffer The buffer to query, must not be VK_NULL_HANDLE
 * Returns
 * - The memory requirements for the buffer
 */
VkMemoryRequirements hg_vk_get_buffer_mem_reqs(VkDevice device, VkBuffer buffer);

/**
 * Gets memory requirements for an image
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - image The image to query, must not be VK_NULL_HANDLE
 * Returns
 * - The memory requirements for the image
 */
VkMemoryRequirements hg_vk_get_image_mem_reqs(VkDevice device, VkImage image);

/**
 * Allocates device memory matching the requirements
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - gpu The physical device to query memory types
 * - mem_reqs The required memory properties, must not be NULL
 * - desired_flags The flags the memory should have
 * - undesired_flags Flags that the memory should avoid
 * Returns
 * - The allocated memory, will never be VK_NULL_HANDLE
 */
VkDeviceMemory hg_vk_allocate_memory(
    VkDevice device,
    VkPhysicalDevice gpu,
    VkMemoryRequirements *mem_reqs,
    VkMemoryPropertyFlags desired_flags,
    VkMemoryPropertyFlags undesired_flags);

/**
 * Frees device memory
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - memory The memory to free, noop if VK_NULL_HANDLE
 */
void hg_vk_free_memory(VkDevice device, VkDeviceMemory memory);

/**
 * Binds device memory to a buffer
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - buffer The buffer to bind to, must not be VK_NULL_HANDLE
 * - memory The memory to bind, must not be VK_NULL_HANDLE
 * - offset Offset into memory
 */
void hg_vk_bind_buffer_memory(VkDevice device, VkBuffer buffer, VkDeviceMemory memory, usize offset);

/**
 * Binds device memory to an image
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - image The image to bind to, must not be VK_NULL_HANDLE
 * - memory The memory to bind, must not be VK_NULL_HANDLE
 * - offset Offset into memory
 */
void hg_vk_bind_image_memory(VkDevice device, VkImage image, VkDeviceMemory memory, usize offset);

/**
 * Maps device memory for CPU access
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - memory The memory to map, must not be VK_NULL_HANDLE
 * - offset The offset into memory
 * - size The region to map, must be greater than 0
 * Returns
 * - A pointer to mapped memory, never NULL unless mapping failed
 */
void *hg_vk_map_memory(VkDevice device, VkDeviceMemory memory, usize offset, usize size);

/**
 * Unmaps device memory
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - memory The memory to unmap, must not be VK_NULL_HANDLE
 */
void hg_vk_unmap_memory(VkDevice device, VkDeviceMemory memory);

/**
 * Flushes a range of device memory to make host writes visible to the GPU
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - memory The memory to flush, must not be VK_NULL_HANDLE
 * - offset Offset into the memory
 * - size Size of the memory region to flush, must be greater than 0
 */
void hg_vk_flush_memory(VkDevice device, VkDeviceMemory memory, usize offset, usize size);

/**
 * Invalidates a range of device memory to make GPU writes visible to the CPU
 *
 * Parameters
 * - device The Vulkan device, must not be VK_NULL_HANDLE
 * - memory The memory to invalidate, must not be VK_NULL_HANDLE
 * - offset Offset into the memory
 * - size Size of the memory region to invalidate, must be greater than 0
 */
void hg_vk_invalidate_memory(VkDevice device, VkDeviceMemory memory, usize offset, usize size);

/**
 * Begins recording a command buffer
 *
 * Parameters
 * - cmd The command buffer to begin recording, must not be VK_NULL_HANDLE
 * - flags Optional usage flags
 */
void hg_vk_begin_cmd(VkCommandBuffer cmd, VkCommandBufferUsageFlags flags);

/**
 * Ends recording of a command buffer
 *
 * Parameters
 * - cmd The command buffer to finish, must not be VK_NULL_HANDLE
 */
void hg_vk_end_cmd(VkCommandBuffer cmd);

/**
 * Copies data from one buffer to another
 *
 * Parameters
 * - cmd The command buffer to write the command to, must not be VK_NULL_HANDLE
 * - dst The destination buffer, must not be VK_NULL_HANDLE
 * - src The source buffer, must not be VK_NULL_HANDLE
 * - regions Copy regions, must not be NULL
 * - region_count Number of regions, must be greater than 0
 */
void hg_vk_copy_buffer(
    VkCommandBuffer cmd,
    VkBuffer dst,
    VkBuffer src,
    const VkBufferCopy *regions,
    u32 region_count);

/**
 * Copies data from one image to another
 *
 * Parameters
 * - cmd The command buffer to write to, must not be VK_NULL_HANDLE
 * - dst The destination image, must not be VK_NULL_HANDLE
 * - src The source image, must not be VK_NULL_HANDLE
 * - regions Copy regions, must not be NULL
 * - region_count Number of copy regions, must be greater than 0
 */
void hg_vk_copy_image(
    VkCommandBuffer cmd,
    VkImage dst,
    VkImage src,
    const VkImageCopy *regions,
    u32 region_count);

/**
 * Performs a blit between images
 *
 * Parameters
 * - cmd The command buffer to write to, must not be VK_NULL_HANDLE
 * - dst The destination image, must not be VK_NULL_HANDLE
 * - src The source image, must not be VK_NULL_HANDLE
 * - regions The blit regions, must not be NULL
 * - region_count Number of blit regions, must be greater than 0
 * - filter The filtering method to use
 */
void hg_vk_blit_image(
    VkCommandBuffer cmd,
    VkImage dst,
    VkImage src,
    const VkImageBlit *regions,
    u32 region_count,
    VkFilter filter);

/**
 * Copies data from a buffer to an image
 *
 * Parameters
 * - cmd The command buffer, must not be VK_NULL_HANDLE
 * - dst The destination image, must not be VK_NULL_HANDLE
 * - src The source buffer, must not be VK_NULL_HANDLE
 * - regions Copy regions, must not be NULL
 * - region_count Count of regions, must be greater than 0
 */
void hg_vk_copy_buffer_to_image(
    VkCommandBuffer cmd,
    VkImage dst,
    VkBuffer src,
    VkBufferImageCopy *regions,
    u32 region_count);

/**
 * Copies data from an image to a buffer
 *
 * Parameters
 * - cmd The command buffer, must not be VK_NULL_HANDLE
 * - dst The destination buffer, must not be VK_NULL_HANDLE
 * - src The source image, must not be VK_NULL_HANDLE
 * - regions Copy regions, must not be NULL
 * - region_count Count of regions, must be greater than 0
 */
void hg_vk_copy_image_to_buffer(
    VkCommandBuffer cmd,
    VkBuffer dst,
    VkImage src,
    VkBufferImageCopy *regions,
    u32 region_count);

/**
 * Inserts a pipeline barrier using Vulkan synchronization2
 *
 * Parameters
 * - cmd The command buffer to write to, must not be VK_NULL_HANDLE
 * - dependencies The dependency info, must not be NULL
 */
void hg_vk_pipeline_barrier(VkCommandBuffer cmd, const VkDependencyInfo *dependencies);

/**
 * Begins a dynamic rendering pass
 *
 * Parameters
 * - cmd The command buffer, must not be VK_NULL_HANDLE
 * - info The rendering info, must not be NULL
 */
void hg_vk_begin_rendering(VkCommandBuffer cmd, const VkRenderingInfo *info);

/**
 * Ends a dynamic rendering pass
 *
 * Parameters
 * - cmd The command buffer, must not be VK_NULL_HANDLE
 */
void hg_vk_end_rendering(VkCommandBuffer cmd);

/**
 * Sets the viewport dynamically
 *
 * Parameters
 * - cmd The command buffer to write the command to, must not be VK_NULL_HANDLE
 * - x Left coordinate of viewport
 * - y Top coordinate of viewport
 * - width Width of viewport
 * - height Height of viewport
 * - near Minimum depth value
 * - far Maximum depth value
 */
void hg_vk_set_viewport(
    VkCommandBuffer cmd,
    f32 x,
    f32 y,
    f32 width,
    f32 height,
    f32 near,
    f32 far);

/**
 * Sets the scissor rectangle dynamically
 *
 * Parameters
 * - cmd The command buffer to write to, must not be VK_NULL_HANDLE
 * - x Left coordinate of scissor
 * - y Top coordinate of scissor
 * - width Width of scissor
 * - height Height of scissor
 */
void hg_vk_set_scissor(VkCommandBuffer cmd, i32 x, i32 y, u32 width, u32 height);

/**
 * Binds a pipeline to a command buffer
 *
 * Parameters
 * - cmd The command buffer to bind to, must not be VK_NULL_HANDLE
 * - pipeline The Vulkan pipeline to bind, must not be VK_NULL_HANDLE
 * - bind_point Graphics or compute bind point
 */
void hg_vk_bind_pipeline(
    VkCommandBuffer cmd,
    VkPipeline pipeline,
    VkPipelineBindPoint bind_point
);

/**
 * Binds descriptor sets to a pipeline
 *
 * Parameters
 * - cmd The command buffer, must not be VK_NULL_HANDLE
 * - layout The pipeline layout, must not be VK_NULL_HANDLE
 * - bind_point Graphics or compute bind point
 * - begin_index First set index
 * - set_count Number of sets
 * - descriptor_sets The descriptor sets to bind, must not be NULL
 */
void hg_vk_bind_descriptor_sets(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    VkPipelineBindPoint bind_point,
    u32 begin_index,
    u32 set_count,
    const VkDescriptorSet *descriptor_sets);

/**
 * Pushes constant values into the command buffer
 *
 * Parameters
 * - cmd The command buffer, must not be VK_NULL_HANDLE
 * - layout The pipeline layout, must not be VK_NULL_HANDLE
 * - stages Shader stages to update
 * - offset Byte offset into push constant range
 * - size Size of data, must be greater than 0
 * - data Pointer to constant data, must not be NULL
 */
void hg_vk_push_constants(
    VkCommandBuffer cmd,
    VkPipelineLayout layout,
    VkShaderStageFlags stages,
    u32 offset,
    u32 size,
    const void *data);

/**
 * Binds multiple vertex buffers
 *
 * Parameters
 * - cmd The command buffer, must not be VK_NULL_HANDLE
 * - begin_index First binding index
 * - count Number of vertex buffers, must be greater than 0
 * - vertex_buffers Buffers to bind, must not be NULL
 * - offsets Byte offsets for each buffer, must not be NULL
 */
void hg_vk_bind_vertex_buffers(
    VkCommandBuffer cmd,
    u32 begin_index,
    u32 count,
    VkBuffer *vertex_buffers,
    usize *offsets);

/**
 * Binds a single vertex buffer
 *
 * Parameters
 * - cmd The command buffer, must not be VK_NULL_HANDLE
 * - vertex_buffer The vertex buffer to bind, must not be VK_NULL_HANDLE
 */
void hg_vk_bind_vertex_buffer(VkCommandBuffer cmd, VkBuffer vertex_buffer);

/**
 * Binds an index buffer for indexed drawing
 *
 * Parameters
 * - cmd The command buffer, must not be VK_NULL_HANDLE
 * - index_buffer The buffer containing indices, must not be VK_NULL_HANDLE
 * - offset Byte offset into the index buffer
 * - type The index type
 */
void hg_vk_bind_index_buffer(
    VkCommandBuffer cmd,
    VkBuffer index_buffer,
    usize offset,
    VkIndexType type);

/**
 * Draws the vertices to the framebuffer
 *
 * Parameters
 * - cmd The command buffer to write, must not be VK_NULL_HANDLE
 * - first_vertex The beginning vertex in the vertex buffer to use
 * - vertex_count The number of vertices to read, must be greater than 0
 * - first_instance The beginning instance in to use
 * - instance_count The number of instances to draw, must be greater than 0
 */
void hg_vk_draw(
    VkCommandBuffer cmd,
    u32 first_vertex,
    u32 vertex_count,
    u32 first_instance,
    u32 instance_count);

/**
 * Draws the vertices to the framebuffer, using an index buffer
 *
 * Parameters
 * - cmd The command buffer to write, must not be VK_NULL_HANDLE
 * - vertex_offset The offset into the vertices
 * - first_index The beginning index in the index buffer to use
 * - index_count The number of indices to read, must be greater than 0
 * - first_instance The beginning instance in to use
 * - instance_count The number of instances to draw, must be greater than 0
 */
void hg_vk_draw_indexed(
    VkCommandBuffer cmd,
    u32 vertex_offset,
    u32 first_index,
    u32 index_count,
    u32 first_instance,
    u32 instance_count);

/**
 * Dispatches the bound compute shader
 *
 * Parameters
 * - cmd The command buffer to write, must not be VK_NULL_HANDLE
 * - x The number of local workgroups in the x direction
 * - y The number of local workgroups in the x direction
 * - z The number of local workgroups in the x direction
 */
void hg_vk_dispatch(VkCommandBuffer cmd, u32 x, u32 y, u32 z);

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
 * A window
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
 * - window The window to destroy, noop if VK_NULL_HANDLE
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
 * - instance The Vulkan instance, must not be VK_NULL_HANDLE
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
 * Destroys a Vulkan surface
 *
 * Parameters
 * - instance The Vulkan instance, must not be VK_NULL_HANDLE
 * - surface The surface to destroy, noop if VK_NULL_HANDLE
 */
void hg_vk_destroy_surface(VkInstance instance, VkSurfaceKHR surface);

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

#ifdef __cplusplus
}
#endif

#endif // HURDYGURDY_H

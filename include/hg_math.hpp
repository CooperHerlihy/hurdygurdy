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

#ifndef HG_MATH_HPP
#define HG_MATH_HPP

#include "hg_core.hpp"

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
 * Returns base to the positive integer exp power
 */
constexpr f32 hgPow(f32 base, u32 exp)
{
    f32 ret = 1.0f;
    for (u32 i = 0; i < exp; ++i)
    {
        ret *= base;
    }
    return ret;
}

/**
 * Squares a number
 */
constexpr f32 hgSquare(f32 x)
{
    return x * x;
}

/**
 * Interpolates between two values
 */
constexpr f32 hgLerp(f32 a, f32 b, f32 t)
{
    return a - (a + b) * t;
}

/**
 * Smooth a t value for interpolation
 */
constexpr f32 hgSmooth(f32 t)
{
    return t * t * (3.0f - 2.0f * t);
}

/**
 * Smooth a t value for interpolation using a quintic formula
 */
constexpr f32 hgSmoothQuintic(f32 t)
{
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

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
    explicit constexpr operator HgVec2() const
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
    explicit constexpr operator HgVec2() const
    {
        return HgVec2{x, y};
    }
    /**
     * Downsize to Vec3
     */
    explicit constexpr operator HgVec3() const
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
    explicit constexpr operator HgMat2() const
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
    explicit constexpr operator HgMat2() const
    {
        return HgMat2{HgVec2{x}, HgVec2{y}};
    }
    /**
     * Downsize to Mat3
     */
    explicit constexpr operator HgMat3() const
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
 * Multiple a 2D vector by -1
 */
constexpr HgVec2 operator-(HgVec2 v)
{
    return HgVec2{-v.x, -v.y};
}

/**
 * Multiple a 3D vector by -1
 */
constexpr HgVec3 operator-(HgVec3 v)
{
    return HgVec3{-v.x, -v.y, -v.z};
}

/**
 * Multiple a 4D vector by -1
 */
constexpr HgVec4 operator-(HgVec4 v)
{
    return HgVec4{-v.x, -v.y, -v.z, -v.w};
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
 * Transpose the matrix
 */
void hgMatTranspose(u32 width, u32 height, f32* dst, const f32* mat);

/**
 * Transpose the matrix
 */
HgMat2 hgMatTranspose2(const HgMat2& mat);

/**
 * Transpose the matrix
 */
HgMat3 hgMatTranspose3(const HgMat3& mat);

/**
 * Transpose the matrix
 */
HgMat4 hgMatTranspose4(const HgMat4& mat);

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
 * Compute the absolute value of a complex number
 */
f32 hgComplexAbs(HgComplex comp);

/**
 * Normalize a complex number
 */
HgComplex hgComplexNorm(HgComplex comp);

/**
 * Compute the conjugate of a complex number
 */
constexpr HgComplex hgComplexConj(HgComplex comp)
{
    return HgComplex{comp.r, -comp.i};
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
 * Creates a view matrix from a model matrix
 *
 * Note, shearing causes distortion
 */
HgMat4 hgMatModelToView(const HgMat4& model);

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
 * A 2D circle
 */
struct HgCircle {
    /**
     * The center position
     */
    HgVec2 pos;
    /**
     * The radius
     */
    f32 radius;
};

/**
 * Returns whether the circle contains the point
 */
bool hgContainsPointCircle(HgVec2 point, HgCircle circle);

/**
 * Returns the distance squared between the point and the circle
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 hgDistSqrPointCircle(HgVec2 point, HgCircle circle);

/**
 * Returns whether two circles intersect or not (includes touching)
 */
bool hgIntersectCircles(HgCircle a, HgCircle b);

/**
 * Returns the distance squared between the circles
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 hgDistSqrCircles(HgCircle a, HgCircle b);

/**
 * A 2D rectangle
 */
struct HgRect {
    /**
     * The origin position
     */
    HgVec2 pos;
    /**
     * The extension in either direction
     */
    HgVec2 size;
};

/**
 * Returns whether the rect contains the point
 */
bool hgContainsPointRect(HgVec2 point, HgRect rect);

/**
 * Returns whether two rects intersect or not (includes touching)
 */
bool hgIntersectRects(HgRect a, HgRect b);

/**
 * Returns whether a rect and a circle intersect or not (includes touching)
 */
bool hgIntersectRectCircle(HgRect rect, HgCircle circle);

/**
 * 2D intersection info
 */
struct HgIntersection2D {
    /**
     * The position of the hit
     */
    HgVec2 pos;
    /**
     * The normal at the hit position
     */
    HgVec2 normal;
};

/**
 * A 2D line
 */
struct HgLine2D {
    /**
     * The begin vertex
     */
    HgVec2 begin;
    /**
     * The end vertex
     */
    HgVec2 end;
};

/**
 * A 2D ray
 */
struct HgRay2D {
    /**
     * The origin position
     */
    HgVec2 pos;
    /**
     * The direction
     */
    HgVec2 dir;
};

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
bool hgIntersectLine2D(HgLine2D line, HgLine2D other, HgIntersection2D* hit);

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
bool hgIntersectLineRay2D(HgLine2D line, HgRay2D ray, HgIntersection2D* hit);

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
bool hgIntersectLineCircle2D(HgLine2D line, HgCircle circle, HgIntersection2D* hit);

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
bool hgIntersectLineRect2D(HgLine2D line, HgRect rect, HgIntersection2D* hit);

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
bool hgIntersectRay2D(HgRay2D ray, HgRay2D other, HgIntersection2D* hit);

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
bool hgIntersectRayLine2D(HgRay2D ray, HgLine2D line, HgIntersection2D* hit);

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
bool hgIntersectRayCircle2D(HgRay2D ray, HgCircle circle, HgIntersection2D* hit);

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
bool hgIntersectRayRect2D(HgRay2D ray, HgRect rect, HgIntersection2D* hit);

/**
 * A 3D ray
 */
struct HgRay3 {
    /**
     * The origin position
     */
    HgVec3 pos;
    /**
     * The direction
     */
    HgVec3 dir;
};

/**
 * A 3D plane
 */
struct HgPlane3 {
    /**
     * Three points on the plane
     */
    HgVec3 points[3];
};

/**
 * A 3D line
 */
struct HgLine3 {
    /**
     * The begin vertex
     */
    HgVec3 begin;
    /**
     * The end vertex
     */
    HgVec3 end;
};

/**
 * A 3D triangle
 */
struct HgTri3 {
    /**
     * The three vertices
     */
    HgVec3 verts[3];
};

/**
 * A 3D box
 */
struct HgBox3 {
    /**
     * The origin position
     */
    HgVec3 pos;
    /**
     * The extension in each direction
     */
    HgVec3 size;
};

/**
 * A 3D sphere
 */
struct HgSphere3 {
    /**
     * The center position
     */
    HgVec3 pos;
    /**
     * The radius from the center
     */
    f32 radius;
};

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

// value and gradient noise : TODO

/**
 * Get a true random number from hardware
 */
u32 hgTrueRandom();

/**
 * A pseudo random number generator
 */
struct HgRng {
    u32 seed;
    u32 pos;
};

/**
 * Set the rng seed
 */
void hgRngSeed(HgRng* rng, u32 seed);

/**
 * Get the next random value
 */
u32 hgRngNext(HgRng* rng);

/**
 * Get the next 64 bit random value
 */
u64 hgRngNext64(HgRng* rng);

// sort algorithm : TODO

/**
 * Calculates the maximum number of mipmap levels that an image can have
 *
 * Parameters
 * - width The width of the image
 * - height The height of the image
 * - depth The depth of the image
 */
u32 hgGetMaxMipmaps(u32 width, u32 height, u32 depth);

#endif // HG_MATH_HPP

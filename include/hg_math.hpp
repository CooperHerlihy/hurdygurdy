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

namespace hg {

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
    for (u32 i = 0; i < exp; ++i)
    {
        ret *= base;
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
 * Compare vectors, treating values within FLT_EPSILON as the same
 */
constexpr bool vecEq2(Vec2 lhs, Vec2 rhs)
{
    return std::abs(lhs.x - rhs.x) < 1e-6 &&
           std::abs(lhs.y - rhs.y) < 1e-6;
}

/**
 * Compare vectors, treating values within FLT_EPSILON as the same
 */
constexpr bool vecEq3(Vec3 lhs, Vec3 rhs)
{
    return std::abs(lhs.x - rhs.x) < 1e-6 &&
           std::abs(lhs.y - rhs.y) < 1e-6 &&
           std::abs(lhs.z - rhs.z) < 1e-6;
}

/**
 * Compare vectors, treating values within FLT_EPSILON as the same
 */
constexpr bool vecEq4(Vec4 lhs, Vec4 rhs)
{
    return std::abs(lhs.x - rhs.x) < 1e-6 &&
           std::abs(lhs.y - rhs.y) < 1e-6 &&
           std::abs(lhs.z - rhs.z) < 1e-6 &&
           std::abs(lhs.w - rhs.w) < 1e-6;
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

/**
 * Add arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void vecAdd(u32 size, f32* dst, const f32* lhs, const f32* rhs);

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
 * Subtract arbitrary size vectors
 *
 * parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void vecSub(u32 size, f32* dst, const f32* lhs, const f32* rhs);

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
 * Multiply pairwise arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void vecMulPairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs);

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
 * Multiply a scalar and a vector
 */
void vecMulScalar(u32 size, f32* dst, f32 scalar, const f32* vec);

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
void vecDivPairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs);

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
 * Divide a vector by a scalar
 *
 * Note, cannot divide by 0
 */
void vecDivScalar(u32 size, f32* dst, const f32* vec, f32 scalar);

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
 * Compute the dot product of arbitrary size vectors
 *
 * Parameters
 * - size The size of the vectors
 * - dst The destination vector, must not be nullptr
 * - lhs The left-hand side vector, must not be nullptr
 * - rhs The right-hand side vector, must not be nullptr
 */
void vecDot(u32 size, f32* dst, const f32* lhs, const f32* rhs);

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
 * Compute the length squared of a vector
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be nullptr
 * - vec The vector to compute the length of, must not be nullptr
 */
void vecLenSqr(u32 size, f32* dst, const f32* vec);

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
 * Compute the length of a vector
 *
 * Parameters
 * - size The size of the vector
 * - dst The destination vector, must not be nullptr
 * - vec The vector to compute the length of, must not be nullptr
 */
void vecLen(u32 size, f32* dst, const f32* vec);

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
 * Normalize a vector
 *
 * Note, cannot normalize 0
 */
void vecNorm(u32 size, f32* dst, const f32* vec);

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
void matAdd(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs);

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
 * Subtract arbitrary size matrices
 *
 * Parameters
 * - width The width of the matrices
 * - height The height of the matrices
 * - dst The destination matrix, must not be nullptr
 * - lhs The left-hand side matrix, must not be nullptr
 * - rhs The right-hand side matrix, must not be nullptr
 */
void matSub(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs);

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
void matMul(f32* dst, u32 wl, u32 hl, const f32* lhs, u32 wr, u32 hr, const f32* rhs);

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
 * Multiply a matrix and a vector
 *
 * Parameters
 * - width The width of the matrix
 * - height The height of the matrix
 * - dst The destination vector, must not be nullptr
 * - mat The matrix to multiply with, must not be nullptr
 * - vec The vector to multiply with, must not be nullptr
 */
void matMulVec(u32 width, u32 height, f32* dst, const f32* mat, const f32* vec);

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

/**
 * Creates a model matrix for 2D graphics
 *
 * Parameters
 * - position The position of the model
 * - scale The scale of the model
 * - rotation The rotation of the model
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
    Vec2 pos;
    /**
     * The extent in each dimension
     */
    Vec2 size;
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
    Vec3 pos;
    /**
     * The extent in each dimension
     */
    Vec3 size;
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
    u32 seed;
    u32 pos;
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

/**
 * Calculates the maximum number of mipmap levels that an image can have
 *
 * Parameters
 * - width The width of the image
 * - height The height of the image
 * - depth The depth of the image
 */
u32 getMaxMipmaps(u32 width, u32 height, u32 depth);

} // namespace hg

#endif // HG_MATH_HPP

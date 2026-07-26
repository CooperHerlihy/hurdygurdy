#pragma once

#include "hg_types.hpp"

namespace hg {

/**
 * The value of Pi
 */
static constexpr f64 pi = 3.1415926535897932;

/**
 * The value of Pi
 */
static constexpr f32 pif = static_cast<f32>(pi);

/**
 * The value of Euler's number
 */
static constexpr f64 euler = 2.7182818284590452;

/**
 * The value of Pi
 */
static constexpr f32 eulerf = static_cast<f32>(euler);

/**
 * The value of square root 2
 */
static constexpr f64 root2 = 1.4142135623730951;

/**
 * The value of Pi
 */
static constexpr f32 root2f = static_cast<f32>(root2);

/**
 * The value of square root 3
 */
static constexpr f64 root3 = 1.7320508075688772;

/**
 * The value of Pi
 */
static constexpr f32 root3f = static_cast<f32>(root3);

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
inline bool vecEq2(Vec2 lhs, Vec2 rhs)
{
    return std::abs(lhs.x - rhs.x) < 1e-6 &&
           std::abs(lhs.y - rhs.y) < 1e-6;
}

/**
 * Compare vectors, treating values within epsilon as the same
 */
inline bool vecEq3(Vec3 lhs, Vec3 rhs)
{
    return std::abs(lhs.x - rhs.x) < 1e-6 &&
           std::abs(lhs.y - rhs.y) < 1e-6 &&
           std::abs(lhs.z - rhs.z) < 1e-6;
}

/**
 * Compare vectors, treating values within epsilon as the same
 */
inline bool vecEq4(Vec4 lhs, Vec4 rhs)
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

} // namespace hg


#pragma once

#include "hg/inttypes.hpp"
#include "hg/utility.hpp"

#include <cmath>

namespace hg {

/**
 * The value of Pi
 */
static constexpr f64 pi = 3.1415926535897932;

/**
 * The value of Pi
 */
static constexpr f32 pif = assertCast<f32>(pi);

/**
 * The value of Euler's number
 */
static constexpr f64 euler = 2.7182818284590452;

/**
 * The value of Euler's number
 */
static constexpr f32 eulerf = assertCast<f32>(euler);

/**
 * The value of square root 2
 */
static constexpr f64 root2 = 1.4142135623730951;

/**
 * The value of square root 2
 */
static constexpr f32 root2f = assertCast<f32>(root2);

/**
 * The value of square root 3
 */
static constexpr f64 root3 = 1.7320508075688772;

/**
 * The value of square root 3
 */
static constexpr f32 root3f = assertCast<f32>(root3);

/**
 * The value of square root 3
 */
static constexpr f32 epsilonf = std::numeric_limits<f32>::epsilon();

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
 * An N-dimensional vector
 */
template<usize N> requires (N > 1)
struct Vec {
    /**
     * A tag to check in templates
     */
    static constexpr bool isVec = true;

    /**
     * The dimensionality
     */
    static constexpr usize n = N;

    /**
     * The components
     */
    f32 vals[N];

    /**
     * Default construct
     */
    constexpr Vec() noexcept = default;

    /**
     * Copy assign
     */
    constexpr Vec(const Vec& other) noexcept = default;

    /**
     * Copy construct
     */
    constexpr Vec& operator=(const Vec& other) noexcept = default;

    /**
     * Construct with scalar
     */
    explicit constexpr Vec(f32 val)
    {
        for (f32& v : vals)
        {
            v = val;
        }
    }

    /**
     * Construct from list
     */
    template<typename... Vs> requires (sizeof...(Vs) == N && (std::convertible_to<f32, Vs> && ...))
    constexpr Vec(Vs... vs)
    {
        f32* v = vals;
        ((*(v++) = assertCast<f32>(vs)), ...);
    }

    /**
     * Construct from other
     */
    template<usize M>
    explicit constexpr Vec(const Vec<M>& other)
    {
        constexpr usize count = N < M ? N : M;
        for (usize i = 0; i < count; ++i)
        {
            vals[i] = other.vals[i];
        }
        for (usize i = count; i < N; ++i)
        {
            vals[i] = 0.0f;
        }
    }

    /**
     * Construct from other and scalars
     */
    template<usize M, typename... Vs> requires (M + sizeof...(Vs) == N && (std::convertible_to<f32, Vs> && ...))
    constexpr Vec(const Vec<M>& other, Vs... vs)
    {
        f32* v = vals;
        for (const f32& o : other)
        {
            *(v++) = o;
        }
        ((*(v++) = assertCast<f32>(vs)), ...);
    }

    /**
     * Access by index
     */
    constexpr f32& operator[](usize idx)
    {
        HG_ASSERT(idx < N);
        return vals[idx];
    }

    /**
     * Access by index
     */
    constexpr const f32& operator[](usize idx) const
    {
        HG_ASSERT(idx < N);
        return vals[idx];
    }

    /**
     * The x component, if it exists
     */
    constexpr f32& x()
    {
        static_assert(N >= 1);
        return vals[0];
    }

    /**
     * The x component, if it exists
     */
    constexpr const f32& x() const
    {
        static_assert(N >= 1);
        return vals[0];
    }

    /**
     * The y component, if it exists
     */
    constexpr f32& y()
    {
        static_assert(N >= 2);
        return vals[1];
    }

    /**
     * The y component, if it exists
     */
    constexpr const f32& y() const
    {
        static_assert(N >= 2);
        return vals[1];
    }

    /**
     * The z component, if it exists
     */
    constexpr f32& z()
    {
        static_assert(N >= 3);
        return vals[2];
    }

    /**
     * The z component, if it exists
     */
    constexpr const f32& z() const
    {
        static_assert(N >= 3);
        return vals[2];
    }

    /**
     * The w component, if it exists
     */
    constexpr f32& w()
    {
        static_assert(N >= 4);
        return vals[3];
    }

    /**
     * The w component, if it exists
     */
    constexpr const f32& w() const
    {
        static_assert(N >= 4);
        return vals[3];
    }

    /**
     * C++ style loops
     */
    constexpr f32* begin()
    {
        return vals;
    }

    /**
     * C++ style loops
     */
    constexpr const f32* begin() const
    {
        return vals;
    }

    /**
     * C++ style loops
     */
    constexpr f32* end()
    {
        return vals + N;
    }

    /**
     * C++ style loops
     */
    constexpr const f32* end() const
    {
        return vals + N;
    }

    /**
     * Add another vector in place
     */
    template<usize M> requires (M <= N)
    constexpr Vec& operator+=(const Vec<M>& other)
    {
        f32* l = vals;
        for (const f32& r : other)
        {
            *(l++) += r;
        }
        return *this;
    }

    /**
     * Subtract another vector in place
     */
    template<usize M> requires (M <= N)
    constexpr Vec& operator-=(const Vec<M>& other)
    {
        f32* l = vals;
        for (const f32& r : other)
        {
            *(l++) -= r;
        }
        return *this;
    }

    /**
     * Multiply another vector in place pairwise
     */
    template<usize M> requires (M <= N)
    constexpr Vec& operator*=(const Vec<M>& other)
    {
        f32* l = vals;
        for (const f32& r : other)
        {
            *(l++) *= r;
        }
        return *this;
    }

    /**
     * Multiply by a scalar in place
     */
    constexpr Vec& operator*=(f32 scalar)
    {
        for (f32& v : vals)
        {
            v *= scalar;
        }
        return *this;
    }

    /**
     * Divide another vector in place pairwise
     */
    template<usize M> requires (M <= N)
    constexpr Vec& operator/=(const Vec<M>& other)
    {
        f32* l = vals;
        for (const f32& r : other)
        {
            HG_ASSERT(r != 0.0f);
            *(l++) /= r;
        }
        return *this;
    }

    /**
     * Divide by a scalar in place
     */
    constexpr Vec& operator/=(f32 scalar)
    {
        HG_ASSERT(scalar != 0.0f);
        for (f32& v : vals)
        {
            v /= scalar;
        }
        return *this;
    }
};

/**
 * A 2D vector
 */
using Vec2 = Vec<2>;

/**
 * A 3D vector
 */
using Vec3 = Vec<3>;

/**
 * A 4D vector
 */
using Vec4 = Vec<4>;

/**
 * A column major matrix
 */
template<usize W, usize H = W>
struct Mat {
    /**
     * A tag to check in templates
     */
    static constexpr bool isMat = true;

    /**
     * The width
     */
    static constexpr usize width = W;

    /**
     * The height
     */
    static constexpr usize height = H;

    /**
     * The columns
     */
    Vec<H> cols[W];

    /**
     * Construct empty
     */
    constexpr Mat() noexcept = default;

    /**
     * Copy construct
     */
    constexpr Mat(const Mat& other) noexcept = default;

    /**
     * Copy assign
     */
    constexpr Mat& operator=(const Mat& other) noexcept = default;

    /**
     * Construct from a scalar
     */
    explicit constexpr Mat(f32 scalar)
    {
        static_assert(W == H);
        usize i = 0;
        for (Vec<H>& c : cols)
        {
            c = {};
            c[i++] = scalar;
        }
    }

    /**
     * Construct from a list of values
     */
    template<typename... Vs> requires (sizeof...(Vs) == W * H && (std::convertible_to<f32, Vs> && ...))
    constexpr Mat(Vs... vs)
    {
        usize i = 0;
        ((cols[i / H][i % H] = assertCast<f32>(vs), ++i), ...);
    }

    /**
     * Construct from a list of vectors
     */
    template<typename... Vs> requires (sizeof...(Vs) == W && ((Vs::isVec && Vs::n == H) && ...))
    constexpr Mat(Vs... vs)
    {
        Vec<H>* v = cols;
        ((*(v++) = Vec<H>{vs}), ...);
    }

    /**
     * Construct from other
     */
    template<usize WO, usize HO>
    explicit constexpr Mat(const Mat<WO, HO>& other)
    {
        constexpr usize count = W < WO ? W : WO;
        for (usize c = 0; c < count; ++c)
        {
            cols[c] = Vec<H>{other.cols[c]};
        }
        for (usize c = count; c < W; ++c)
        {
            cols[c] = {};
            if constexpr (W == H && WO == HO)
                cols[c][c] = 1.0f;
        }
    }

    /**
     * Access by index
     */
    constexpr Vec<H>& operator[](usize idx)
    {
        HG_ASSERT(idx < W);
        return cols[idx];
    }

    constexpr const Vec<H>& operator[](usize idx) const
    {
        HG_ASSERT(idx < W);
        return cols[idx];
    }

    /**
     * The x component, if it exists
     */
    constexpr Vec<H>& x()
    {
        static_assert(W >= 1);
        return cols[0];
    }

    /**
     * The x component, if it exists
     */
    constexpr const Vec<H>& x() const
    {
        static_assert(W >= 1);
        return cols[0];
    }

    /**
     * The y component, if it exists
     */
    constexpr Vec<H>& y()
    {
        static_assert(W >= 2);
        return cols[1];
    }

    /**
     * The y component, if it exists
     */
    constexpr const Vec<H>& y() const
    {
        static_assert(W >= 2);
        return cols[1];
    }

    /**
     * The z component, if it exists
     */
    constexpr Vec<H>& z()
    {
        static_assert(W >= 3);
        return cols[2];
    }

    /**
     * The z component, if it exists
     */
    constexpr const Vec<H>& z() const
    {
        static_assert(W >= 3);
        return cols[2];
    }

    /**
     * The w component, if it exists
     */
    constexpr Vec<H>& w()
    {
        static_assert(W >= 4);
        return cols[3];
    }

    /**
     * The w component, if it exists
     */
    constexpr const Vec<H>& w() const
    {
        static_assert(W >= 4);
        return cols[3];
    }

    /**
     * C++ style loops
     */
    constexpr Vec<H>* begin()
    {
        return cols;
    }

    /**
     * C++ style loops
     */
    constexpr const Vec<H>* begin() const
    {
        return cols;
    }

    /**
     * C++ style loops
     */
    constexpr Vec<H>* end()
    {
        return cols + W;
    }

    /**
     * C++ style loops
     */
    constexpr const Vec<H>* end() const
    {
        return cols + W;
    }

    /**
     * Add another matrix in place
     */
    template<usize WO, usize HO> requires (WO <= W && HO <= H)
    constexpr Mat& operator+=(const Mat<WO, HO>& other)
    {
        for (u32 c = 0; c < WO; ++c)
        {
            for (u32 r = 0; r < HO; ++r)
            {
                cols[c][r] += other.cols[c][r];
            }
        }
        return *this;
    }

    /**
     * Subtract another matrix in place
     */
    template<usize WO, usize HO> requires (WO <= W && HO <= H)
    constexpr Mat& operator-=(const Mat<WO, HO>& other)
    {
        for (u32 c = 0; c < WO; ++c)
        {
            for (u32 r = 0; r < HO; ++r)
            {
                cols[c][r] -= other.cols[c][r];
            }
        }
        return *this;
    }

    /**
     * Multiply by a scalar in place
     */
    constexpr Mat& operator*=(f32 scalar)
    {
        for (Vec<H>& c : cols)
        {
            c *= scalar;
        }
        return *this;
    }

    /**
     * Divide by a scalar in place
     */
    constexpr Mat& operator/=(f32 scalar)
    {
        for (Vec<H>& c : cols)
        {
            c /= scalar;
        }
        return *this;
    }
};

/**
 * A 2D matrix
 */
using Mat2 = Mat<2>;

/**
 * A 3D matrix
 */
using Mat3 = Mat<3>;

/**
 * A 4D matrix
 */
using Mat4 = Mat<4>;

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
    constexpr Complex() = default;

    /**
     * Construct from just a real value
     */
    constexpr Complex(f32 rVal)
        : r{rVal}, i{0}
    {}

    /**
     * Construct from a list of values
     */
    constexpr Complex(f32 rVal, f32 iVal)
        : r{rVal}, i{iVal}
    {}

    /**
     * Constexpr from a vector
     */
    explicit constexpr Complex(Vec2 vec)
        : r{vec.x()}, i{vec.y()}
    {}

    /**
     * Convert to a vector
     */
    explicit constexpr operator Vec2() const
    {
        return {r, i};
    }

    /**
     * Add another complex number in place
     */
    constexpr Complex& operator+=(Complex other)
    {
        r += other.r;
        i += other.i;
        return* this;
    }

    /**
     * Subtract another complex number in place
     */
    constexpr Complex& operator-=(Complex other)
    {
        r -= other.r;
        i -= other.i;
        return* this;
    }

    /**
     * Multiply by a scalar in place
     */
    constexpr Complex& operator*=(f32 scalar)
    {
        r *= scalar;
        i *= scalar;
        return *this;
    }

    /**
     * Divide by a scalar in place
     */
    constexpr Complex& operator/=(f32 scalar)
    {
        r /= scalar;
        i /= scalar;
        return *this;
    }
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
    constexpr Quat() = default;

    /**
     * Construct from just a real value
     */
    constexpr Quat(f32 rVal)
        : r{rVal}, i{0}, j{0}, k{0}
    {}

    /**
     * Construct from a list of values
     */
    constexpr Quat(f32 rVal, f32 iVal, f32 jVal, f32 kVal)
        : r{rVal}, i{iVal}, j{jVal}, k{kVal}
    {}

    /**
     * Constexpr from a real part 3d vector imaginary part
     */
    explicit constexpr Quat(f32 rVal, Vec3 vec)
        : r{rVal}, i{vec.x()}, j{vec.y()}, k{vec.z()}
    {}

    /**
     * Constexpr from a 4d vector
     */
    explicit constexpr Quat(Vec4 vec)
        : r{vec.x()}, i{vec.y()}, j{vec.z()}, k{vec.w()}
    {}

    /**
     * Convert to a 3d vector from imaginary parts
     */
    explicit constexpr operator Vec3() const
    {
        return {i, j, k};
    }

    /**
     * Convert to a 4d vector
     */
    explicit constexpr operator Vec4() const
    {
        return {r, i, j, k};
    }

    /**
     * Add another quaternion in place
     */
    constexpr Quat& operator+=(Quat other)
    {
        r += other.r;
        i += other.i;
        j += other.j;
        k += other.k;
        return* this;
    }

    /**
     * Subtract another quaternion in place
     */
    constexpr Quat& operator-=(Quat other)
    {
        r -= other.r;
        i -= other.i;
        j -= other.j;
        k -= other.k;
        return* this;
    }

    /**
     * Multiply by a scalar in place
     */
    constexpr Quat& operator*=(f32 scalar)
    {
        r *= scalar;
        i *= scalar;
        j *= scalar;
        k *= scalar;
        return* this;
    }

    /**
     * Divide by a scalar in place
     */
    constexpr Quat& operator/=(f32 scalar)
    {
        r /= scalar;
        i /= scalar;
        j /= scalar;
        k /= scalar;
        return* this;
    }
};

/**
 * Compare vectors
 */
template<usize N>
constexpr bool operator==(const Vec<N>& lhs, const Vec<N>& rhs)
{
    const f32* r = rhs.vals;
    for (const f32& l : lhs)
    {
        if (l != *(r++))
            return false;
    }
    return true;
}

/**
 * Compare matrices
 */
template<usize W, usize H>
constexpr bool operator==(const Mat<W, H>& lhs, const Mat<W, H>& rhs)
{
    const Vec<H>* r = rhs.cols;
    for (const Vec<H>& l : lhs)
    {
        if (l != *(r++))
            return false;
    }
    return true;
}

/**
 * Compare complex numbers
 */
constexpr bool operator==(Complex lhs, Complex rhs)
{
    return lhs.r == rhs.r && lhs.i == rhs.i;
}

/**
 * Compare quaternions
 */
constexpr bool operator==(const Quat& lhs, const Quat& rhs)
{
    return lhs.r == rhs.r && lhs.i == rhs.i && lhs.j == rhs.j && lhs.k == rhs.k;
}

/**
 * Add vectors
 */
template<usize N, usize M> requires (N >= M)
constexpr Vec<N> operator+(const Vec<N>& lhs, const Vec<M>& rhs)
{
    Vec<N> ret = lhs;
    ret += rhs;
    return ret;
}

/**
 * Add vectors
 */
template<usize N, usize M> requires (N < M)
constexpr Vec<M> operator+(const Vec<N>& lhs, const Vec<M>& rhs)
{
    Vec<M> ret = rhs;
    ret += lhs;
    return ret;
}

/**
 * Add matrices
 */
template<usize W, usize H>
constexpr Mat<W, H> operator+(const Mat<W, H>& lhs, const Mat<W, H>& rhs)
{
    Mat<W, H> ret = lhs;
    ret += rhs;
    return ret;
}

/**
 * Add complex numbers
 */
constexpr Complex operator+(Complex lhs, Complex rhs)
{
    return Complex{lhs.r + rhs.r, lhs.i + rhs.i};
}

/**
 * Add quaternions
 */
constexpr Quat operator+(Quat lhs, Quat rhs)
{
    return Quat{lhs.r + rhs.r, lhs.i + rhs.i, lhs.j + rhs.j, lhs.k + rhs.k};
}

/**
 * Subtract vectors
 */
template<usize N, usize M> requires (N >= M)
constexpr Vec<N> operator-(const Vec<N>& lhs, const Vec<M>& rhs)
{
    Vec<N> ret = lhs;
    ret -= rhs;
    return ret;
}

/**
 * Subtract vectors
 */
template<usize N, usize M> requires (N < M)
constexpr Vec<M> operator-(const Vec<N>& lhs, const Vec<M>& rhs)
{
    Vec<M> ret = rhs;
    ret -= lhs;
    return ret;
}

/**
 * Subtract matrices
 */
template<usize W, usize H>
constexpr Mat<W, H> operator-(const Mat<W, H>& lhs, const Mat<W, H>& rhs)
{
    Mat<W, H> ret = lhs;
    ret -= rhs;
    return ret;
}

/**
 * Subtract complex numbers
 */
constexpr Complex operator-(Complex lhs, Complex rhs)
{
    return Complex{lhs.r - rhs.r, lhs.i - rhs.i};
}

/**
 * Subtract quaternions
 */
constexpr Quat operator-(Quat lhs, Quat rhs)
{
    return Quat{lhs.r - rhs.r, lhs.i - rhs.i, lhs.j - rhs.j, lhs.k - rhs.k};
}

/**
 * Negate a vector
 */
template<usize N>
constexpr Vec<N> operator-(const Vec<N>& vec)
{
    Vec<N> ret = vec;
    ret *= -1.0f;
    return ret;
}

/**
 * Negate a matrix
 */
template<usize W, usize H>
constexpr Mat<W, H> operator-(const Mat<W, H>& mat)
{
    Mat<W, H> ret = mat;
    ret *= -1.0f;
    return ret;
}

/**
 * Negate a complex number
 */
constexpr Complex operator-(Complex c)
{
    return {-c.r, -c.i};
}

/**
 * Negate a quaternion
 */
constexpr Quat operator-(Quat q)
{
    return {-q.r, -q.i, -q.j, -q.k};
}

/**
 * Multiply vectors pairwise
 */
template<usize N, usize M> requires (N >= M)
constexpr Vec<N> operator*(const Vec<N>& lhs, const Vec<M>& rhs)
{
    Vec<N> ret = lhs;
    ret *= rhs;
    return ret;
}

/**
 * Multiply vectors pairwise
 */
template<usize N, usize M> requires (N < M)
constexpr Vec<M> operator*(const Vec<N>& lhs, const Vec<M>& rhs)
{
    Vec<M> ret = rhs;
    ret *= lhs;
    return ret;
}

/**
 * Multiply a scalar and a vector
 */
template<usize N>
constexpr Vec<N> operator*(f32 s, const Vec<N>& vec)
{
    Vec<N> ret = vec;
    ret *= s;
    return ret;
}

/**
 * Multiply a vector and a scalar
 */
template<usize N>
constexpr Vec<N> operator*(const Vec<N>& vec, f32 s)
{
    Vec<N> ret = vec;
    ret *= s;
    return ret;
}

/**
 * Multiply matrices
 */
template<usize WL, usize HL, usize WR, usize HR> requires (WL == HR)
constexpr Mat<WR, HL> operator*(const Mat<WL, HL>& lhs, const Mat<WR, HR>& rhs)
{
    Mat<WR, HL> ret;
    for (u32 col = 0; col < WR; ++col)
    {
        for (u32 row = 0; row < HL; ++row)
        {
            ret[col][row] = 0.0f;
            for (u32 k = 0; k < WL; ++k)
            {
                ret[col][row] += lhs[k][row] * rhs[col][k];
            }
        }
    }
    return ret;
}

/**
 * Multiply a matrix and a vector
 */
template<usize W, usize H>
constexpr Vec<H> operator*(const Mat<W, H>& mat, const Vec<W>& vec)
{
    Vec<H> ret;
    for (u32 i = 0; i < H; ++i)
    {
        ret[i] = 0.0f;
        for (u32 j = 0; j < W; ++j)
        {
            ret[i] += mat[j][i] * vec[j];
        }
    }
    return ret;
}

/**
 * Multiply a matrix and a scalar
 */
template<usize W, usize H>
constexpr Mat<W, H> operator*(const Mat<W, H>& mat, f32 scalar)
{
    Mat<W, H> ret = mat;
    ret *= scalar;
    return ret;
}

/**
 * Multiply a scalar and a matrix
 */
template<usize W, usize H>
constexpr Mat<W, H> operator*(f32 scalar, const Mat<W, H>& mat)
{
    Mat<W, H> ret = mat;
    ret *= scalar;
    return ret;
}

/**
 * Divide a matrix and a scalar
 */
template<usize W, usize H>
constexpr Mat<W, H> operator/(const Mat<W, H>& mat, f32 scalar)
{
    Mat<W, H> ret = mat;
    ret /= scalar;
    return ret;
}

/**
 * Multiply complex numbers
 */
constexpr Complex operator*(Complex lhs, Complex rhs)
{
    return {
        lhs.r * rhs.r - lhs.i * rhs.i,
        lhs.r * rhs.i + lhs.i * rhs.r
    };
}

/**
 * Multiply quaternions
 */
constexpr Quat operator*(const Quat& lhs, const Quat& rhs)
{
    return {
        lhs.r * rhs.r - lhs.i * rhs.i - lhs.j * rhs.j - lhs.k * rhs.k,
        lhs.r * rhs.i + lhs.i * rhs.r + lhs.j * rhs.k - lhs.k * rhs.j,
        lhs.r * rhs.j - lhs.i * rhs.k + lhs.j * rhs.r + lhs.k * rhs.i,
        lhs.r * rhs.k + lhs.i * rhs.j - lhs.j * rhs.i + lhs.k * rhs.r,
    };
}

/**
 * Multiply a quaternion and a scalar
 */
constexpr Quat operator*(Quat lhs, f32 rhs)
{
    lhs *= rhs;
    return lhs;
}

/**
 * Divide vectors pairwise
 */
template<usize N, usize M> requires (N >= M)
constexpr Vec<N> operator/(const Vec<N>& lhs, const Vec<M>& rhs)
{
    Vec<N> ret = lhs;
    ret /= rhs;
    return ret;
}

/**
 * Divide vectors pairwise
 */
template<usize N, usize M> requires (N < M)
constexpr Vec<M> operator/(const Vec<N>& lhs, const Vec<M>& rhs)
{
    Vec<M> ret = rhs;
    ret /= lhs;
    return ret;
}

/**
 * Divide a vector and a scalar
 */
template<usize N>
constexpr Vec<N> operator/(const Vec<N>& vec, f32 s)
{
    Vec<N> ret = vec;
    ret /= s;
    return ret;
}

/**
 * Divide a complex number and a scalar
 */
constexpr Complex operator/(Complex lhs, f32 rhs)
{
    lhs /= rhs;
    return lhs;
}

/**
 * Divide a quaternion and a scalar
 */
constexpr Quat operator/(Quat lhs, f32 rhs)
{
    lhs /= rhs;
    return lhs;
}

/**
 * Compute the vector dot product
 */
template<usize N>
constexpr f32 dot(Vec<N> lhs, Vec<N> rhs)
{
    f32 ret = 0.0f;
    f32* l = lhs.vals;
    for (f32 r : rhs)
    {
        ret += *(l++) * r;
    }
    return ret;
}

/**
 * Compute the length squared of a vector
 */
template<usize N>
constexpr f32 lengthSquared(Vec<N> vec)
{
    return dot(vec, vec);
}

/**
 * Compute the length of a vector
 */
template<usize N>
f32 length(Vec<N> vec)
{
    return std::sqrt(lengthSquared(vec));
}

/**
 * Normalize a 2D vector
 *
 * Note, cannot normalize 0
 */
template<usize N>
Vec<N> normalize(const Vec<N>& vec)
{
    f32 len = length(vec);
    HG_ASSERT(len != 0.0f);
    return vec / len;
}

/**
 * Compute the cross product of 2D vectors
 */
constexpr f32 cross(Vec<2> lhs, Vec<2> rhs)
{
    return lhs.x() * rhs.y() - lhs.y() * rhs.x();
}

/**
 * Compute the cross product of 3D vectors
 */
constexpr Vec<3> cross(Vec<3> lhs, Vec<3> rhs)
{
    return {
        lhs[1] * rhs[2] - lhs[2] * rhs[1],
        lhs[2] * rhs[0] - lhs[0] * rhs[2],
        lhs[0] * rhs[1] - lhs[1] * rhs[0],
    };
}

/**
 * Transpose the matrix
 */
template<usize W, usize H>
constexpr Mat<H, W> transpose(const Mat<W, H>& mat)
{
    Mat<H, W> ret;
    for (u32 i = 0; i < W; ++i)
    {
        for (u32 j = 0; j < H; ++j)
        {
            ret[j][i] = mat[i][j];
        }
    }
    return ret;
}

/**
 * Compute the conjugate of a complex number
 */
constexpr Complex conjugate(Complex comp)
{
    return Complex{comp.r, -comp.i};
}

/**
 * Compute the absolute value squared of a complex number
 */
constexpr f32 absSqr(Complex c)
{
    return c.r * c.r + c.i * c.i;
}

/**
 * Compute the absolute value squared of a complex number
 */
inline f32 abs(Complex c)
{
    return std::sqrt(absSqr(c));
}

/**
 * Normalize a complex number
 */
inline Complex normalize(Complex c)
{
    f32 len = abs(c);
    HG_ASSERT(len != 0.0f);
    return c / len;
}

/**
 * Rotate a 2D vector using a complex number
 */
constexpr Vec<2> rotate(Complex lhs, Vec<2> rhs)
{
    return Vec<2>{lhs * Complex{rhs}};
}

/**
 * Compute the conjugate of a quaternion
 */
constexpr Quat conjugate(Quat quat)
{
    return Quat{quat.r, -quat.i, -quat.j, -quat.k};
}

/**
 * Return the absolute value squared of a quaternion
 */
constexpr f32 absSqr(Quat quat)
{
    return quat.r * quat.r + quat.i * quat.i + quat.j * quat.j + quat.k * quat.k;
}

/**
 * Return the absolute value of a quaternion
 */
inline f32 abs(Quat quat)
{
    return std::sqrt(absSqr(quat));
}

/**
 * Return a normalized quaternion
 */
inline Quat normalize(Quat q)
{
    f32 len = abs(q);
    HG_ASSERT(len != 0.0f);
    return q / len;
}

/**
 * Create a rotation quaternion from an axis and angle
 */
inline Quat quatAxisAngle(Vec<3> axis, f32 angle)
{
    f32 halfAngle = angle * 0.5f;
    f32 sinHalfAngle = std::sin(halfAngle);
    return Quat{std::cos(halfAngle), axis * sinHalfAngle};
}


/**
 * Create a rotation quaternion between two directions
 */
inline Quat quatBetween(Vec<3> from, Vec<3> to)
{
    HG_ASSERT(from != Vec<3>{0.0f});
    HG_ASSERT(to != Vec<3>{0.0f});

    from = normalize(from);
    to = normalize(to);

    f32 dot = hg::dot(from, to);

    if (dot > 1 - epsilonf)
    {
        return Quat{1};
    }

    if (dot < -1 + epsilonf)
    {
        Vec<3> axis = std::abs(from.x()) >= 1 - epsilonf
            ? cross(from, {0, 1, 0})
            : cross(from, {1, 0, 0});
        return normalize(Quat(0, axis));
    }

    Vec<3> axis = cross(from, to);
    return normalize(Quat{dot + 1, axis});
}

/**
 * Rotate a 3D vector using a quaternion
 */
constexpr Vec<3> rotate(Quat quat, Vec<3> vec)
{
    return Vec<3>{quat * Quat{0, vec} * conjugate(quat)};
}

/**
 * Rotate a 3x3 matrix using a quaternion
 */
constexpr Mat<3> rotate(Quat quat, Mat<3> mat)
{
    return Mat<3>{
        rotate(quat, mat.x()),
        rotate(quat, mat.y()),
        rotate(quat, mat.z()),
    };
}

/**
 * Coordinate conventions for the rendering math in this header
 *
 * Model and world space follow glTF: right handed, y up, +z out of the
 * screen toward the viewer, meters, counter clockwise winding seen from
 * outside. View space is the camera's local frame; the camera looks along
 * its local +z (glTF cameras look along -z, so rotate imported camera
 * nodes 180 degrees around y to fit). Vulkan NDC is y down with depth in
 * [0, 1]: the perspective projection applies the single y negation needed
 * to connect y up view space to the y down NDC, and depth maps each view
 * z in [near, far] to [0, 1]. The orthographic projection instead accepts
 * y down screen style coordinates directly and maps them to the NDC
 * unmirrored.
 */

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
 * Note, the result is the inverse of the rotation and translation only,
 * scale and shear of the model matrix are not reflected in the result
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

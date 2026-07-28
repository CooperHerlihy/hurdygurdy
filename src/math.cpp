#include "hg/math.hpp"

#include <cmath>
#include <cfloat>

namespace hg {

Vec2& Vec2::operator+=(Vec2 other)
{
    x += other.x;
    y += other.y;
    return* this;
}

Vec2& Vec2::operator-=(Vec2 other)
{
    x -= other.x;
    y -= other.y;
    return* this;
}

Vec2& Vec2::operator*=(Vec2 other)
{
    x *= other.x;
    y *= other.y;
    return* this;
}

Vec2& Vec2::operator/=(Vec2 other)
{
    x /= other.x;
    y /= other.y;
    return* this;
}

Vec3& Vec3::operator+=(Vec3 other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return* this;
}

Vec3& Vec3::operator-=(Vec3 other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return* this;
}

Vec3& Vec3::operator*=(Vec3 other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return* this;
}

Vec3& Vec3::operator/=(Vec3 other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return* this;
}

Vec4& Vec4::operator+=(Vec4 other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return* this;
}

Vec4& Vec4::operator-=(Vec4 other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return* this;
}

Vec4& Vec4::operator*=(Vec4 other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
    return* this;
}

Vec4& Vec4::operator/=(Vec4 other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return* this;
}

Mat2& Mat2::operator+=(const Mat2& other)
{
    x += other.x;
    y += other.y;
    return* this;
}

Mat2& Mat2::operator-=(const Mat2& other)
{
    x -= other.x;
    y -= other.y;
    return* this;
}

Mat3& Mat3::operator+=(const Mat3& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return* this;
}

Mat3& Mat3::operator-=(const Mat3& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return* this;
}

Mat4& Mat4::operator+=(const Mat4& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return* this;
}

Mat4& Mat4::operator-=(const Mat4& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return* this;
}

void matTranspose(u32 width, u32 height, f32* dst, const f32* mat)
{
    for (u32 i = 0; i < width; ++i)
    {
        for (u32 j = 0; j < height; ++j)
        {
            dst[j * width + i] = mat[i * height + j];
        }
    }
}

Mat2 matTranspose2(const Mat2& mat)
{
    Mat2 ret;
    matTranspose(2, 2, &ret.x.x, &mat.x.x);
    return ret;
}

Mat3 matTranspose3(const Mat3& mat)
{
    Mat3 ret;
    matTranspose(3, 3, &ret.x.x, &mat.x.x);
    return ret;
}

Mat4 matTranspose4(const Mat4& mat)
{
    Mat4 ret;
    matTranspose(4, 4, &ret.x.x, &mat.x.x);
    return ret;
}

Complex& Complex::operator+=(Complex other)
{
    r += other.r;
    i += other.i;
    return* this;
}

Complex& Complex::operator-=(Complex other)
{
    r -= other.r;
    i -= other.i;
    return* this;
}

Quat& Quat::operator+=(Quat other)
{
    r += other.r;
    i += other.i;
    j += other.j;
    k += other.k;
    return* this;
}

Quat& Quat::operator-=(Quat other)
{
    r -= other.r;
    i -= other.i;
    j -= other.j;
    k -= other.k;
    return* this;
}

f32 vecLen2(Vec2 vec)
{
    return std::sqrt(vecLenSqr2(vec));
}

f32 vecLen3(Vec3 vec)
{
    return std::sqrt(vecLenSqr3(vec));
}

f32 vecLen4(Vec4 vec)
{
    return std::sqrt(vecLenSqr4(vec));
}

Vec2 vecNorm2(Vec2 vec)
{
    f32 len = vecLen2(vec);
    HG_ASSERT(len != 0);
    return {vec.x / len, vec.y / len};
}

Vec3 vecNorm3(Vec3 vec)
{
    f32 len = vecLen3(vec);
    HG_ASSERT(len != 0);
    return {vec.x / len, vec.y / len, vec.z / len};
}

Vec4 vecNorm4(Vec4 vec)
{
    f32 len = vecLen4(vec);
    HG_ASSERT(len != 0);
    return {vec.x / len, vec.y / len, vec.z / len, vec.w / len};
}

void vecCross3(f32* dst, const f32* lhs, const f32* rhs)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    dst[0] = lhs[1] * rhs[2] - lhs[2] * rhs[1];
    dst[1] = lhs[2] * rhs[0] - lhs[0] * rhs[2];
    dst[2] = lhs[0] * rhs[1] - lhs[1] * rhs[0];
}

f32 vecCross2(Vec2 lhs, Vec2 rhs)
{
    return lhs.x * rhs.y - lhs.y * rhs.x;
}

Vec3 vecCross3(Vec3 lhs, Vec3 rhs)
{
    return {
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x
    };
}

static void matAdd(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    for (u32 i = 0; i < width; ++i)
    {
        for (u32 j = 0; j < height; ++j)
        {
            dst[i * width + j] = lhs[i * width + j] + rhs[i * width + j];
        }
    }
}

Mat2 operator+(const Mat2& lhs, const Mat2& rhs)
{
    Mat2 result{};
    matAdd(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

Mat3 operator+(const Mat3& lhs, const Mat3& rhs)
{
    Mat3 result{};
    matAdd(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

Mat4 operator+(const Mat4& lhs, const Mat4& rhs)
{
    Mat4 result{};
    matAdd(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

static void matSub(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    for (u32 i = 0; i < width; ++i)
    {
        for (u32 j = 0; j < height; ++j)
        {
            dst[i * width + j] = lhs[i * width + j] - rhs[i * width + j];
        }
    }
}

Mat2 operator-(const Mat2& lhs, const Mat2& rhs)
{
    Mat2 result{};
    matSub(2, 2, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

Mat3 operator-(const Mat3& lhs, const Mat3& rhs)
{
    Mat3 result{};
    matSub(3, 3, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

Mat4 operator-(const Mat4& lhs, const Mat4& rhs)
{
    Mat4 result{};
    matSub(4, 4, &result.x.x, &lhs.x.x, &rhs.x.x);
    return result;
}

static void matMul(f32* dst, u32 wl, u32 hl, const f32* lhs, u32 wr, u32 hr, const f32* rhs)
{
    HG_ASSERT(hr == wl);
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    static_cast<void>(hr);
    for (u32 i = 0; i < wl; ++i)
    {
        for (u32 j = 0; j < wr; ++j)
        {
            dst[i * wl + j] = 0.0f;
            for (u32 k = 0; k < hl; ++k)
            {
                dst[i * wl + j] += lhs[k * wl + j] * rhs[i * wr + k];
            }
        }
    }
}

Mat2 operator*(const Mat2& lhs, const Mat2& rhs)
{
    Mat2 result{};
    matMul(&result.x.x, 2, 2, &lhs.x.x, 2, 2, &rhs.x.x);
    return result;
}

Mat3 operator*(const Mat3& lhs, const Mat3& rhs)
{
    Mat3 result{};
    matMul(&result.x.x, 3, 3, &lhs.x.x, 3, 3, &rhs.x.x);
    return result;
}

Mat4 operator*(const Mat4& lhs, const Mat4& rhs)
{
    Mat4 result{};
    matMul(&result.x.x, 4, 4, &lhs.x.x, 4, 4, &rhs.x.x);
    return result;
}

static void matMulVec(u32 width, u32 height, f32* dst, const f32* mat, const f32* vec)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(mat != nullptr);
    HG_ASSERT(vec != nullptr);
    for (u32 i = 0; i < height; ++i)
    {
        dst[i] = 0.0f;
        for (u32 j = 0; j < width; ++j)
        {
            dst[i] += mat[j * width + i] * vec[j];
        }
    }
}

Vec2 operator*(const Mat2& lhs, Vec2 rhs)
{
    Vec2 result{};
    matMulVec(2, 2, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

Vec3 operator*(const Mat3& lhs, Vec3 rhs)
{
    Vec3 result{};
    matMulVec(3, 3, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

Vec4 operator*(const Mat4& lhs, Vec4 rhs)
{
    Vec4 result{};
    matMulVec(4, 4, &result.x, &lhs.x.x, &rhs.x);
    return result;
}

f32 complexAbsSqr(Complex comp)
{
    return comp.r * comp.r + comp.i * comp.i;
}

f32 complexAbs(Complex comp)
{
    return sqrtf(complexAbsSqr(comp));
}

Complex complexNorm(Complex comp)
{
    f32 len = complexAbs(comp);
    HG_ASSERT(len != 0);
    return Complex{comp.r / len, comp.i / len};
}

Vec2 vecRot2(Complex lhs, Vec2 rhs)
{
    Complex c = lhs * Complex{rhs.x, rhs.y};
    return {c.r, c.i};
}

Quat operator*(Quat lhs, Quat rhs)
{
    return Quat{
        lhs.r * rhs.r - lhs.i * rhs.i - lhs.j * rhs.j - lhs.k * rhs.k,
        lhs.r * rhs.i + lhs.i * rhs.r + lhs.j * rhs.k - lhs.k * rhs.j,
        lhs.r * rhs.j - lhs.i * rhs.k + lhs.j * rhs.r + lhs.k * rhs.i,
        lhs.r * rhs.k + lhs.i * rhs.j - lhs.j * rhs.i + lhs.k * rhs.r,
    };
}

f32 quatAbsSqr(Quat quat)
{
    return quat.r * quat.r + quat.i * quat.i + quat.j * quat.j + quat.k * quat.k;
}

f32 quatAbs(Quat quat)
{
    return sqrtf(quatAbsSqr(quat));
}

Quat quatNorm(Quat quat)
{
    f32 len = quatAbs(quat);
    return Quat{quat.r / len, quat.i / len, quat.j / len, quat.k / len};
}

Quat quatAxisAngle(Vec3 axis, f32 angle)
{
    f32 halfAngle = angle * (f32)0.5;
    f32 sinHalfAngle = sinf(halfAngle);
    return Quat{
        cosf(halfAngle),
        axis.x * sinHalfAngle,
        axis.y * sinHalfAngle,
        axis.z * sinHalfAngle,
    };
}

Quat quatBetween(Vec3 from, Vec3 to)
{
    HG_ASSERT(from != Vec3{0});
    HG_ASSERT(to != Vec3{0});

    from = vecNorm3(from);
    to = vecNorm3(to);

    f32 dot = vecDot3(from, to);

    if (dot > 1 - FLT_EPSILON)
    {
        return Quat{1};
    }

    if (dot < -1 + FLT_EPSILON)
    {
        Vec3 axis = std::abs(from.x) >= 1 - FLT_EPSILON
            ? vecCross3(from, {0, 1, 0})
            : vecCross3(from, {1, 0, 0});
        return Quat(0, axis.x, axis.y, axis.z);
    }

    Vec3 axis = vecCross3(from, to);
    return quatNorm(Quat{dot + 1, axis.x, axis.y, axis.z});
}

Vec3 vecRot3(Quat lhs, Vec3 rhs)
{
    Quat q = lhs * Quat{0, rhs.x, rhs.y, rhs.z} * quatConj(lhs);
    return {q.i, q.j, q.k};
}

Mat3 matRot3(Quat lhs, Mat3 rhs)
{
    return Mat3{
        vecRot3(lhs, rhs.x),
        vecRot3(lhs, rhs.y),
        vecRot3(lhs, rhs.z),
    };
}

Mat4 matModel2D(Vec3 position, Vec2 scale, f32 rotation)
{
    Mat2 m2{{scale.x, 0.0f}, {0.0f, scale.y}};
    f32 rotSin = sinf(rotation);
    f32 rotCos = cosf(rotation);
    Mat2 rot{{rotCos, rotSin}, {-rotSin, rotCos}};
    Mat4 m4 = Mat4{rot * m2};
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

Mat4 matModel3D(const Vec3& position, const Vec3& scale, const Quat& rotation)
{
    Mat3 m3{1.0f};
    m3.x.x = scale.x;
    m3.y.y = scale.y;
    m3.z.z = scale.z;
    m3 = matRot3(rotation, m3);
    Mat4 m4 = Mat4{m3};
    m4.w.x = position.x;
    m4.w.y = position.y;
    m4.w.z = position.z;
    return m4;
}

Mat4 matView(const Vec3& position, const Vec3& zoom, const Quat& rotation)
{
    Mat4 rot{matRot3(quatConj(rotation), Mat3{1.0f})};
    Mat4 pos{1.0f};
    pos.x.x = zoom.x;
    pos.y.y = zoom.y;
    pos.z.z = zoom.z;
    pos.w.x = -position.x;
    pos.w.y = -position.y;
    pos.w.z = -position.z;
    return rot * pos;
}

Mat4 matModelToView(const Mat4& model)
{
    if (Vec3{model.x} == Vec3{0} || Vec3{model.y} == Vec3{0} || Vec3{model.z} == Vec3{0})
        return Mat4{Mat3{0}};

    Mat3 inv3 = matTranspose3(Mat3{
        vecNorm3(Vec3{model.x}),
        vecNorm3(Vec3{model.y}),
        vecNorm3(Vec3{model.z}),
    });
    Mat4 inv4{inv3};
    inv4.w = Vec4{inv3 * Vec3{model.w} * -1, 1};
    return inv4;
}

Mat4 matOrthographic(f32 left, f32 right, f32 top, f32 bottom, f32 near, f32 far)
{
    return Mat4{
        {2.0f / (right - left), 0.0f, 0.0f, 0.0f},
        {0.0f, 2.0f / (bottom - top), 0.0f, 0.0f},
        {0.0f, 0.0f, 1.0f / (far - near), 0.0f},
        {-(right + left) / (right - left), -(bottom + top) / (bottom - top), -(near) / (far - near), 1.0f},
    };
}

Mat4 matPerspective(f32 fov, f32 aspect, f32 near, f32 far)
{
    HG_ASSERT(near > 0.0f);
    HG_ASSERT(far > near);
    f32 scale = 1.0f / static_cast<f32>(tan(fov * 0.5f));
    return Mat4{
        {scale / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, scale, 0.0f, 0.0f},
        {0.0f, 0.0f, far / (far - near), 1.0f},
        {0.0f, 0.0f, -(far * near) / (far - near), 0.0f},
    };
}

} // namespace hg

#include "hg_core.hpp"
#include "hg_math.hpp"
#include "hg_utils.hpp"

#include <random>

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

void vecAdd(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = lhs[i] + rhs[i];
    }
}

void vecSub(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = lhs[i] - rhs[i];
    }
}

void vecMulPairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = lhs[i] * rhs[i];
    }
}

void vecMulScalar(u32 size, f32* dst, f32 scalar, const f32* vec)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(vec != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = scalar * vec[i];
    }
}

void vecDivPairwise(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    for (u32 i = 0; i < size; ++i)
    {
        HG_ASSERT(rhs[i] != 0);
        dst[i] = lhs[i] / rhs[i];
    }
}

void vecDivScalar(u32 size, f32* dst, const f32* vec, f32 scalar)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(vec != nullptr);
    HG_ASSERT(scalar != 0);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = vec[i] / scalar;
    }
}

void vecDot(u32 size, f32* dst, const f32* lhs, const f32* rhs)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    *dst = 0;
    for (u32 i = 0; i < size; ++i)
    {
        *dst += lhs[i] * rhs[i];
    }
}

void vecLenSqr(u32 size, f32* dst, const f32* vec)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(vec != nullptr);
    vecDot(size, dst, vec, vec);
}

void vecLen(u32 size, f32* dst, const f32* vec)
{
    vecLenSqr(size, dst, vec);
    *dst = std::sqrt(*dst);
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

void vecNorm(u32 size, f32* dst, const f32* vec)
{
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(vec != nullptr);
    f32 len;
    vecLen(size, &len, vec);
    HG_ASSERT(len != 0);
    for (u32 i = 0; i < size; ++i)
    {
        dst[i] = vec[i] / len;
    }
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

void matAdd(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs)
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

void matSub(u32 width, u32 height, f32* dst, const f32* lhs, const f32* rhs)
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

void matMul(f32* dst, u32 wl, u32 hl, const f32* lhs, u32 wr, u32 hr, const f32* rhs)
{
    HG_ASSERT(hr == wl);
    HG_ASSERT(dst != nullptr);
    HG_ASSERT(lhs != nullptr);
    HG_ASSERT(rhs != nullptr);
    (void)hr;
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

void matMulVec(u32 width, u32 height, f32* dst, const f32* mat, const f32* vec)
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
    f32 scale = 1.0f / (f32)tan(fov * 0.5f);
    return Mat4{
        {scale / aspect, 0.0f, 0.0f, 0.0f},
        {0.0f, scale, 0.0f, 0.0f},
        {0.0f, 0.0f, far / (far - near), 1.0f},
        {0.0f, 0.0f, -(far * near) / (far - near), 0.0f},
    };
}

bool containsPointCircle(Vec2 point, Circle circle)
{
    return distPointCircle(point, circle) <= FLT_EPSILON;
}

f32 distPointCircle(Vec2 point, Circle circle)
{
    return vecLen2(point - circle.pos) - circle.radius;
}

Vec2 closestPointCircle(Vec2 pos, Circle circle)
{
    return circle.pos + circle.radius * vecNorm2(pos - circle.pos);
}

bool intersectCircles(Circle a, Circle b)
{
    return distCircles(a, b) <= FLT_EPSILON;
}

f32 distCircles(Circle a, Circle b)
{
    return vecLen2(a.pos - b.pos) - a.radius - b.radius;
}

Rect rectEmpty()
{
    return {
        Vec2{INFINITY},
        Vec2{-INFINITY},
    };
}

Rect rectAddPoint(Rect rect, Vec2 point)
{
    Rect newRect;
    newRect.pos.x = min(rect.pos.x, point.x - FLT_EPSILON);
    newRect.pos.y = min(rect.pos.y, point.y - FLT_EPSILON);
    newRect.size.x = max(rect.size.x + rect.pos.x, point.x + FLT_EPSILON) - newRect.pos.x;
    newRect.size.y = max(rect.size.y + rect.pos.y, point.y + FLT_EPSILON) - newRect.pos.y;
    return newRect;
}

bool containsPointRect(Vec2 point, Rect rect)
{
    return point.x >= rect.pos.x - FLT_EPSILON && point.x <= rect.pos.x + rect.size.x + FLT_EPSILON
        && point.y >= rect.pos.y - FLT_EPSILON && point.y <= rect.pos.y + rect.size.y + FLT_EPSILON;
}

Vec2 closestPointRect(Vec2 pos, Rect rect)
{
    return {
        clamp(pos.x, rect.pos.x, rect.pos.x + rect.size.x),
        clamp(pos.y, rect.pos.y, rect.pos.y + rect.size.y),
    };
}

bool intersectRects(Rect a, Rect b)
{
    return a.pos.x + a.size.x >= b.pos.x && a.pos.x <= b.pos.x + b.size.x
        && a.pos.y + a.size.y >= b.pos.y && a.pos.y <= b.pos.y + b.size.y;
}

bool intersectRectCircle(Rect rect, Circle circle)
{
    return containsPointCircle(closestPointRect(circle.pos, rect), circle);
}

bool intersectRays2D(Ray2D ray, Ray2D other, Hit2D* hit)
{
    HG_ASSERT(ray.dir != Vec2{0});
    HG_ASSERT(other.dir != Vec2{0});

    f32 denom = vecCross2(ray.dir, other.dir);
    if (std::abs(denom) < FLT_EPSILON)
        return false;

    Vec2 diff = other.pos - ray.pos;

    f32 t = vecCross2(diff, other.dir) / denom;
    if (t < -FLT_EPSILON)
        return false;

    f32 tOther = vecCross2(diff, ray.dir) / denom;
    if (tOther < -FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = denom < 0
            ? vecNorm2({other.dir.y, -other.dir.x})
            : vecNorm2({-other.dir.y, other.dir.x});
    }
    return true;
}

bool intersectRayLine2D(Ray2D ray, Line2D line, Hit2D* hit)
{
    HG_ASSERT(ray.dir != Vec2{0});
    if (vecEq2(line.begin, line.end))
        return false;

    Vec2 lineDir = line.end - line.begin;

    f32 denom = vecCross2(ray.dir, lineDir);
    if (std::abs(denom) < FLT_EPSILON)
        return false;

    Vec2 diff = line.begin - ray.pos;

    f32 t = vecCross2(diff, lineDir) / denom;
    if (t < -FLT_EPSILON)
        return false;

    f32 tOther = vecCross2(diff, ray.dir) / denom;
    if (tOther < -FLT_EPSILON || tOther > 1 + FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = denom < 0
            ? vecNorm2({lineDir.y, -lineDir.x})
            : vecNorm2({-lineDir.y, lineDir.x});
    }
    return true;
}

bool intersectRayCircle(Ray2D ray, Circle circle, Hit2D* hit)
{
    HG_ASSERT(ray.dir != Vec2{0});

    Vec2 rel = ray.pos - circle.pos;
    f32 a = vecDot2(ray.dir, ray.dir);
    f32 b = 2 * vecDot2(ray.dir, rel);
    f32 c = vecDot2(rel, rel) - square(circle.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return false;
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = (ray.pos + t * ray.dir - circle.pos) / circle.radius;
    }
    return true;
}

bool intersectRayRect(Ray2D ray, Rect rect, Hit2D* hit)
{
    HG_ASSERT(ray.dir != Vec2{0});
    if (vecEq2(rect.size, Vec2{0}))
        return false;

    if (containsPointRect(ray.pos, rect))
    {
        if (hit != nullptr)
        {
            hit->dist = 0;
            hit->normal = -ray.dir;
        }
        return true;
    }

    f32 hits[4] = {
        (rect.pos.x - ray.pos.x) / ray.dir.x,
        (rect.pos.y - ray.pos.y) / ray.dir.y,
        (rect.pos.x + rect.size.x - ray.pos.x) / ray.dir.x,
        (rect.pos.y + rect.size.y - ray.pos.y) / ray.dir.y,
    };

    constexpr Vec2 norms[4] = {
        {-1, 0},
        {0, -1},
        {1, 0},
        {0, 1},
    };

    f32 t = INFINITY;
    Vec2 norm;
    for (u32 i = 0; i < arrayCount(hits); ++i)
    {
        if (hits[i] < -FLT_EPSILON)
            continue;

        if (!containsPointRect(ray.pos + hits[i] * ray.dir, rect))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = norm;
    }
    return true;
}

bool intersectLines2D(Line2D line, Line2D other, Hit2D* hit)
{
    if (vecEq2(line.begin, line.end) || vecEq2(other.begin, other.end))
        return false;

    Vec2 lineDir = line.end - line.begin;
    Vec2 otherDir = other.end - other.begin;

    f32 denom = vecCross2(lineDir, otherDir);
    if (std::abs(denom) < FLT_EPSILON)
        return false;

    Vec2 diff = other.begin - line.begin;

    f32 t = vecCross2(diff, otherDir) / denom;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return false;

    f32 tOther = vecCross2(diff, lineDir) / denom;
    if (tOther < -FLT_EPSILON || tOther > 1 + FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = denom < 0
            ? vecNorm2({otherDir.y, -otherDir.x})
            : vecNorm2({-otherDir.y, otherDir.x});
    }
    return true;
}

bool intersectLineRay2D(Line2D line, Ray2D ray, Hit2D* hit)
{
    if (vecEq2(line.begin, line.end))
        return false;
    HG_ASSERT(ray.dir != Vec2{0});

    Vec2 lineDir = line.end - line.begin;

    f32 denom = vecCross2(lineDir, ray.dir);
    if (std::abs(denom) < FLT_EPSILON)
        return false;

    Vec2 diff = ray.pos - line.begin;

    f32 t = vecCross2(diff, ray.dir) / denom;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return false;

    f32 tRay = vecCross2(diff, lineDir) / denom;
    if (tRay < -FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = denom < 0
            ? vecNorm2({ray.dir.y, -ray.dir.x})
            : vecNorm2({-ray.dir.y, ray.dir.x});
    }
    return true;
}

bool intersectLineCircle(Line2D line, Circle circle, Hit2D* hit)
{
    Vec2 dir = line.end - line.begin;

    Vec2 rel = line.begin - circle.pos;
    f32 a = vecDot2(dir, dir);
    f32 b = 2 * vecDot2(dir, rel);
    f32 c = vecDot2(rel, rel) - square(circle.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return false;
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t > 1 + FLT_EPSILON)
        return false;
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = (line.begin + t * dir - circle.pos) / circle.radius;
    }
    return true;
}

bool intersectLineRect(Line2D line, Rect rect, Hit2D* hit)
{
    if (vecEq2(line.begin, line.end) || vecEq2(rect.size, Vec2{0}))
        return false;

    f32 hits[4] = {
        (rect.pos.x - line.begin.x) / (line.end.x - line.begin.x),
        (rect.pos.y - line.begin.y) / (line.end.y - line.begin.y),
        (rect.pos.x + rect.size.x - line.begin.x) / (line.end.x - line.begin.x),
        (rect.pos.y + rect.size.y - line.begin.y) / (line.end.y - line.begin.y),
    };

    constexpr Vec2 norms[4] = {
        {-1, 0},
        {0, -1},
        {1, 0},
        {0, 1},
    };

    f32 t = INFINITY;
    Vec2 norm;
    for (u32 i = 0; i < arrayCount(hits); ++i)
    {
        if (hits[i] < -FLT_EPSILON || hits[i] > 1 + FLT_EPSILON)
            continue;

        if (!containsPointRect(line.begin + hits[i] * (line.end - line.begin), rect))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = norm;
    }
    return true;
}

bool containsPointSphere(Vec3 point, Sphere sphere)
{
    return distPointSphere(point, sphere) <= 0;
}

f32 distPointSphere(Vec3 point, Sphere sphere)
{
    return vecLen3(point - sphere.pos) - sphere.radius;
}

Vec3 closestPointSphere(Vec3 pos, Sphere sphere)
{
    Vec3 rel = pos - sphere.pos;
    if (vecLenSqr3(rel) <= sphere.radius + FLT_EPSILON)
        return pos;
    return sphere.pos + sphere.radius * vecNorm3(rel);
}

bool intersectSpheres(Sphere a, Sphere b)
{
    return distSpheres(a, b) <= 0;
}

f32 distSpheres(Sphere a, Sphere b)
{
    return vecLen3(a.pos - b.pos) - a.radius - b.radius;
}

Box boxEmpty()
{
    return {
        Vec3{INFINITY},
        Vec3{-INFINITY},
    };
}

Box boxAddPoint(Box box, Vec3 point)
{
    Box newBox;
    newBox.pos.x = min(box.pos.x, point.x - FLT_EPSILON);
    newBox.pos.y = min(box.pos.y, point.y - FLT_EPSILON);
    newBox.pos.z = min(box.pos.z, point.z - FLT_EPSILON);
    newBox.size.x = max(box.size.x + box.pos.x, point.x + FLT_EPSILON) - newBox.pos.x;
    newBox.size.y = max(box.size.y + box.pos.y, point.y + FLT_EPSILON) - newBox.pos.y;
    newBox.size.z = max(box.size.z + box.pos.z, point.z + FLT_EPSILON) - newBox.pos.z;
    return newBox;
}

bool containsPointBox(Vec3 point, Box box)
{
    return point.x >= box.pos.x && point.x <= box.pos.x + box.size.x
        && point.y >= box.pos.y && point.y <= box.pos.y + box.size.y
        && point.z >= box.pos.z && point.z <= box.pos.z + box.size.z;
}

Vec3 closestPointBox(Vec3 pos, Box box)
{
    return {
        clamp(pos.x, box.pos.x, box.pos.x + box.size.x),
        clamp(pos.y, box.pos.y, box.pos.y + box.size.y),
        clamp(pos.z, box.pos.z, box.pos.z + box.size.z),
    };
}

bool intersectBox(Box a, Box b)
{
    return a.pos.x + a.size.x >= b.pos.x && a.pos.x <= b.pos.x + b.size.x
        && a.pos.y + a.size.y >= b.pos.y && a.pos.y <= b.pos.y + b.size.y
        && a.pos.z + a.size.z >= b.pos.z && a.pos.z <= b.pos.z + b.size.z;
}

bool intersectBoxSphere(Box box, Sphere sphere)
{
    return containsPointSphere(closestPointBox(sphere.pos, box), sphere);
}

Plane planeFromPoint(Vec3 point, Vec3 normal)
{
    Plane plane;
    plane.normal = vecNorm3(normal);
    plane.dist = vecDot3(plane.normal, point);
    return plane;
}

Plane planeFromTri(Tri tri)
{
    Plane plane;
    plane.normal = vecNorm3(vecCross3(tri.b - tri.a, tri.c - tri.a));
    plane.dist = vecDot3(plane.normal, tri.a);
    return plane;
}

bool intersectRaySphere(Ray3D ray, Sphere sphere, Hit3D* hit)
{
    HG_ASSERT(ray.dir != Vec3{0});

    Vec3 rel = ray.pos - sphere.pos;
    f32 a = vecDot3(ray.dir, ray.dir);
    f32 b = 2 * vecDot3(ray.dir, rel);
    f32 c = vecDot3(rel, rel) - square(sphere.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return false;
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = (ray.pos + t * ray.dir - sphere.pos) / sphere.radius;
    }
    return true;
}

bool intersectRayBox(Ray3D ray, Box box, Hit3D* hit)
{
    HG_ASSERT(ray.dir != Vec3{0});
    if (vecEq3(box.size, Vec3{0}))
        return false;

    if (containsPointBox(ray.pos, box))
    {
        if (hit != nullptr)
        {
            hit->dist = 0;
            hit->normal = -ray.dir;
        }
        return true;
    }

    f32 hits[6] = {
        (box.pos.x - ray.pos.x) / ray.dir.x,
        (box.pos.y - ray.pos.y) / ray.dir.y,
        (box.pos.z - ray.pos.z) / ray.dir.z,
        (box.pos.x + box.size.x - ray.pos.x) / ray.dir.x,
        (box.pos.y + box.size.y - ray.pos.y) / ray.dir.y,
        (box.pos.z + box.size.z - ray.pos.y) / ray.dir.z,
    };

    constexpr Vec3 norms[6] = {
        {-1, 0, 0},
        {0, -1, 0},
        {0, 0, -1},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
    };

    f32 t = INFINITY;
    Vec3 norm;
    for (u32 i = 0; i < arrayCount(hits); ++i)
    {
        if (hits[i] < -FLT_EPSILON)
            continue;

        if (!containsPointBox(ray.pos + hits[i] * ray.dir, box))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = norm;
    }
    return true;
}

// Moller-Trumbore, Real Time Rendering 4th Edition
bool intersectRayTri(Ray3D ray, Tri tri, Hit3D* hit)
{
    HG_ASSERT(ray.dir != Vec3{0});

    if (tri.a == tri.b || tri.a == tri.c || tri.b == tri.c)
        return false;

    Vec3 e1 = tri.b - tri.a;
    Vec3 e2 = tri.c - tri.a;
    Vec3 q = vecCross3(ray.dir, e2);

    f32 a = vecDot3(e1, q);
    if (std::abs(a) < FLT_EPSILON)
        return false;

    Vec3 s = ray.pos - tri.a;

    f32 u = vecDot3(s, q) / a;
    if (u < -FLT_EPSILON)
        return false;

    Vec3 r = vecCross3(s, e1);

    f32 v = vecDot3(ray.dir, r) / a;
    if (v < -FLT_EPSILON || u + v > 1 + FLT_EPSILON)
        return false;

    f32 t = vecDot3(e2, r) / a;
    if (t < -FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = a < 0
            ? vecNorm3(vecCross3(e2, e1))
            : vecNorm3(vecCross3(e1, e2));
    }
    return true;
}

bool intersectRayPlane(Ray3D ray, Plane plane, Hit3D* hit)
{
    HG_ASSERT(ray.dir != Vec3{0});
    HG_ASSERT(plane.normal != Vec3{0});

    f32 denom = vecDot3(ray.dir, plane.normal);
    if (std::abs(denom) < FLT_EPSILON)
        return false;

    f32 t = (plane.dist - vecDot3(ray.pos, plane.normal)) / denom;
    if (t < -FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = denom < 0
            ? plane.normal
            : -plane.normal;
    }
    return true;
}

bool intersectLineSphere(Line3D line, Sphere sphere, Hit3D* hit)
{
    Vec3 dir = line.end - line.begin;

    Vec3 rel = line.begin - sphere.pos;
    f32 a = vecDot3(dir, dir);
    f32 b = 2 * vecDot3(dir, rel);
    f32 c = vecDot3(rel, rel) - square(sphere.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return false;
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t > 1 + FLT_EPSILON)
        return false;
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = (line.begin + t * dir - sphere.pos) / sphere.radius;
    }
    return true;
}

bool intersectLineBox(Line3D line, Box box, Hit3D* hit)
{
    if (vecEq3(line.begin, line.end) || vecEq3(box.size, Vec3{0}))
        return false;

    f32 hits[6] = {
        (box.pos.x - line.begin.x) / (line.end.x - line.begin.x),
        (box.pos.y - line.begin.y) / (line.end.y - line.begin.y),
        (box.pos.z - line.begin.z) / (line.end.z - line.begin.z),
        (box.pos.x + box.size.x - line.begin.x) / (line.end.x - line.begin.x),
        (box.pos.y + box.size.y - line.begin.y) / (line.end.y - line.begin.y),
        (box.pos.z + box.size.z - line.begin.z) / (line.end.z - line.begin.z),
    };

    constexpr Vec3 norms[6] = {
        {-1, 0, 0},
        {0, -1, 0},
        {0, 0, -1},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
    };

    f32 t = INFINITY;
    Vec3 norm;
    for (u32 i = 0; i < arrayCount(hits); ++i)
    {
        if (hits[i] < -FLT_EPSILON || hits[i] > 1 + FLT_EPSILON)
            continue;

        if (!containsPointBox(line.begin + hits[i] * (line.end - line.begin), box))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = norm;
    }
    return true;
}

// Moller-Trumbore, Real Time Rendering 4th Edition
bool intersectLineTri(Line3D line, Tri tri, Hit3D* hit)
{
    if (vecEq3(line.begin, line.end))
        return false;

    if (tri.a == tri.b || tri.a == tri.c || tri.b == tri.c)
        return false;

    Vec3 lineDir = line.end - line.begin;

    Vec3 e1 = tri.b - tri.a;
    Vec3 e2 = tri.c - tri.a;
    Vec3 q = vecCross3(lineDir, e2);

    f32 a = vecDot3(e1, q);
    if (std::abs(a) < FLT_EPSILON)
        return false;

    Vec3 s = line.begin - tri.a;

    f32 u = vecDot3(s, q) / a;
    if (u < -FLT_EPSILON)
        return false;

    Vec3 r = vecCross3(s, e1);

    f32 v = vecDot3(lineDir, r) / a;
    if (v < -FLT_EPSILON || u + v > 1 + FLT_EPSILON)
        return false;

    f32 t = vecDot3(e2, r) / a;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = a < 0
            ? vecNorm3(vecCross3(e2, e1))
            : vecNorm3(vecCross3(e1, e2));
    }
    return true;
}

bool intersectLinePlane(Line3D line, Plane plane, Hit3D* hit)
{
    if (line.begin == line.end)
        return false;

    HG_ASSERT(plane.normal != Vec3{0});

    Vec3 lineDir = line.end - line.begin;

    f32 denom = vecDot3(lineDir, plane.normal);
    if (std::abs(denom) < FLT_EPSILON)
        return false;

    f32 t = (plane.dist - vecDot3(line.begin, plane.normal)) / denom;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return false;

    if (hit != nullptr)
    {
        hit->dist = t;
        hit->normal = denom < 0
            ? plane.normal
            : -plane.normal;
    }
    return true;
}

u32 noise(u32 seed, u32 pos)
{
    u32 ret = (pos + 384521713u) * 955740521u;
    ret ^= ret >> 13;
    ret *= seed * 725937977u;
    ret ^= ret >> 7;
    ret *= 358166231u;
    ret ^= ret >> 11;
    return ret;
}

u32 noise2D(u32 seed, u32 x, u32 y)
{
    return noise(seed, x + (y * 425537443u));
}

u32 noise3D(u32 seed, u32 x, u32 y, u32 z)
{
    return noise(seed, x + y * 425537443u + z * 682607u);
}

u32 noise4D(u32 seed, u32 x, u32 y, u32 z, u32 w)
{
    return noise(seed, x + y * 425537443u + z * 682607u + w * 9067u);
}

f32 noiseNorm(u32 seed, f32 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)noise(seed, Convert{pos}.asU32) / (f32)UINT32_MAX;
}

f32 noiseNorm2D(u32 seed, Vec2 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)noise2D(seed, Convert{pos.x}.asU32, Convert{pos.y}.asU32) / (f32)UINT32_MAX;
}

f32 noiseNorm3D(u32 seed, Vec3 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)noise3D(seed, Convert{pos.x}.asU32, Convert{pos.y}.asU32, Convert{pos.z}.asU32) / (f32)UINT32_MAX;
}

f32 noiseNorm4D(u32 seed, Vec4 pos)
{
    union Convert {
        f32 asF32;
        u32 asU32;
    };
    return (f32)noise4D(
        seed,
        Convert{pos.x}.asU32,
        Convert{pos.y}.asU32,
        Convert{pos.z}.asU32,
        Convert{pos.w}.asU32) / (f32)UINT32_MAX;
}

f32 noiseVec1D(u32 seed, f32 pos)
{
    return noiseNorm(seed, pos) * 2.0f - 1.0f;
}

Vec2 noiseVec2D(u32 seed, Vec2 pos)
{
    f32 rot = 2.0f * (f32)HG_PI * noiseNorm2D(seed, pos);
    return Vec2(cosf(rot), sinf(rot));
}

u32 trueRandom()
{
    static std::random_device trueRandom{};
    return trueRandom();
}

void rngSeed(Rng* rng, u32 seed)
{
    rng->seed = seed;
}

u32 rngNext(Rng* rng)
{
    return rng->pos = noise(rng->seed, rng->pos);
}

u64 rngNext64(Rng* rng)
{
    return ((u64)rngNext(rng) << 32) | (u64)rngNext(rng);
}

u32 getMaxMipmaps(u32 width, u32 height, u32 depth)
{
    u32 max = width > height ? width : height;
    max = max > depth ? max : depth;
    return max == 0 ? 0 : (u32)log2((f32)max) + 1;
}

} // namespace hg

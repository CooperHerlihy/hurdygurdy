#include "tests.hpp"
#include "hg/math.hpp"

#include <cfloat>
#include <cmath>
#include <type_traits>

using namespace hg;

TEST(testScalarPow)
{
    static_assert(pow(2.0f, 0) == 1.0f);
    static_assert(pow(2.0f, 1) == 2.0f);
    static_assert(pow(3.0f, 2) == 9.0f);
    static_assert(pow(0.0f, 5) == 0.0f);
    static_assert(pow(1.0f, 100) == 1.0f);
    static_assert(pow(-2.0f, 3) == -8.0f);
    static_assert(pow(-2.0f, 2) == 4.0f);
    static_assert(pow(2.0f, 10) == 1024.0f);
    static_assert(pow(0.5f, 4) == 0.0625f);
    static_assert(pow(1.5f, 5) == 7.59375f);
    static_assert(pow(2.0f, 20) == 1048576.0f);
    static_assert(pow(0.0f, 0) == 1.0f);
    static_assert(pow(1.0f, 0) == 1.0f);
    static_assert(pow(5.0f, 0) == 1.0f);
    static_assert(pow(-3.0f, 0) == 1.0f);
    static_assert(pow(0.001f, 0) == 1.0f);
}

TEST(testScalarSquare)
{
    static_assert(square(0.0f) == 0.0f);
    static_assert(square(1.0f) == 1.0f);
    static_assert(square(-1.0f) == 1.0f);
    static_assert(square(2.5f) == 6.25f);
    static_assert(square(0.1f) == 0.010000001f);
    static_assert(square(-3.0f) == 9.0f);
}

TEST(testScalarLerp)
{
    static_assert(lerp(0.0f, 10.0f, 0.0f) == 0.0f);
    static_assert(lerp(0.0f, 10.0f, 1.0f) == 10.0f);
    static_assert(lerp(0.0f, 10.0f, 0.5f) == 5.0f);
    static_assert(lerp(5.0f, 5.0f, 0.3f) == 5.0f);
    static_assert(lerp(0.0f, 10.0f, 2.0f) == 20.0f);
    static_assert(lerp(0.0f, 10.0f, -1.0f) == -10.0f);
    static_assert(lerp(2.0f, 4.0f, 0.25f) == 2.5f);
}

TEST(testScalarSmooth)
{
    static_assert(smooth(0.0f) == 0.0f);
    static_assert(smooth(1.0f) == 1.0f);
    static_assert(smooth(0.5f) == 0.5f);
    static_assert(smooth(0.25f) == 0.15625f);
    static_assert(smooth(0.25f) < smooth(0.75f));
    static_assert(smoothQuintic(0.0f) == 0.0f);
    static_assert(smoothQuintic(1.0f) == 1.0f);
    static_assert(smoothQuintic(0.5f) == 0.5f);
    static_assert(smoothQuintic(0.25f) < smoothQuintic(0.75f));
    static_assert(smoothQuintic(0.25f) < smooth(0.25f));
    static_assert(smoothQuintic(0.75f) > smooth(0.75f));
}

TEST(testMathConstants)
{
    static_assert(pi == 3.1415926535897932);
    static_assert(euler == 2.7182818284590452);
    static_assert(root2 == 1.4142135623730951);
    static_assert(root3 == 1.7320508075688772);
    static_assert(pif == static_cast<f32>(pi));
    static_assert(eulerf == static_cast<f32>(euler));
    static_assert(root2f == static_cast<f32>(root2));
    static_assert(root3f == static_cast<f32>(root3));
    static_assert(epsilonf == FLT_EPSILON);

    static_assert(pi > 3.141592653589792 && pi < 3.141592653589795);
    static_assert(euler > 2.718281828459044 && euler < 2.718281828459047);
    static_assert(root2 > 1.414213562373094 && root2 < 1.414213562373096);
    static_assert(root3 > 1.7320508075688770 && root3 < 1.7320508075688776);
    static_assert(pif > 3.1415923f && pif < 3.1415932f);
    static_assert(eulerf > 2.7182813f && eulerf < 2.7182822f);
    static_assert(root2f > 1.4142132f && root2f < 1.4142139f);
    static_assert(root3f > 1.7320503f && root3f < 1.7320513f);

    static_assert(root2 * root2 > 2.0 - 1e-15 && root2 * root2 < 2.0 + 1e-15);
    static_assert(root3 * root3 > 3.0 - 1e-15 && root3 * root3 < 3.0 + 1e-15);
    static_assert(root2f * root2f > 2.0f - 1e-6f && root2f * root2f < 2.0f + 1e-6f);
    static_assert(root3f * root3f > 3.0f - 1e-6f && root3f * root3f < 3.0f + 1e-6f);
}

TEST(testVecConstruction)
{
    static_assert(Vec2::isVec);
    static_assert(Vec3::isVec);
    static_assert(Vec2::n == 2);
    static_assert(Vec3::n == 3);
    static_assert(Vec4::n == 4);

    constexpr Vec2 v2{1.0f, 2.0f};
    static_assert(v2.x() == 1.0f && v2.y() == 2.0f);
    constexpr Vec3 v3{1.0f, 2.0f, 3.0f};
    static_assert(v3.z() == 3.0f);
    constexpr Vec4 v4{1.0f, 2.0f, 3.0f, 4.0f};
    static_assert(v4.w() == 4.0f);

    constexpr Vec2 s2{5.0f};
    static_assert(s2.x() == 5.0f && s2.y() == 5.0f);
    constexpr Vec3 s3{5.0f};
    static_assert(s3.x() == 5.0f && s3.z() == 5.0f);
    constexpr Vec4 s4{5.0f};
    static_assert(s4.x() == 5.0f && s4.w() == 5.0f);

    static_assert(!std::is_convertible_v<f32, Vec2>);
    static_assert(!std::is_convertible_v<Vec2, Vec3>);
    static_assert(!std::is_convertible_v<Vec3, Vec2>);
    static_assert(!std::is_convertible_v<f32, Mat2>);

    static_assert([] {
        Vec2 d2;
        d2 = Vec2{3.0f, 4.0f};
        d2.x() = 1.0f;
        return d2 == Vec2{1.0f, 4.0f};
    }());

    static_assert([] {
        Vec2 a{1.0f, 2.0f};
        Vec2 b = a;
        b.y() = 5.0f;
        return a.y() == 2.0f && b == Vec2{1.0f, 5.0f};
    }());
}

TEST(testVecIndexAccessors)
{
    constexpr Vec3 v{1.0f, 2.0f, 3.0f};
    static_assert(v[0] == 1.0f && v[1] == 2.0f && v[2] == 3.0f);
    static_assert(v.x() == 1.0f && v.y() == 2.0f && v.z() == 3.0f);
    static_assert(Vec4{1.0f, 2.0f, 3.0f, 4.0f}.w() == 4.0f);

    static_assert([] {
        Vec3 v{1.0f, 2.0f, 3.0f};
        v.x() = 10.0f;
        v.y() = 20.0f;
        v.z() = 30.0f;
        bool ok = v[0] == 10.0f && v[1] == 20.0f && v[2] == 30.0f;
        v[1] = 5.0f;
        ok = ok && v.y() == 5.0f;
        Vec4 w{1.0f, 2.0f, 3.0f, 4.0f};
        w.w() = 40.0f;
        ok = ok && w[3] == 40.0f;
        return ok;
    }());
}

TEST(testVecRangeFor)
{
    static_assert([] {
        Vec4 v{1.0f, 2.0f, 3.0f, 4.0f};
        f32 sum = 0.0f;
        for (f32& f : v)
        {
            f += 1.0f;
            sum += f;
        }
        return sum == 14.0f && v == Vec4{2.0f, 3.0f, 4.0f, 5.0f};
    }());

    static_assert([] {
        f32 sum2 = 0.0f;
        for (f32 f : Vec2{5.0f, 6.0f})
            sum2 += f;
        f32 sum3 = 0.0f;
        for (const f32& f : Vec3{7.0f, 8.0f, 9.0f})
            sum3 += f;
        return sum2 == 11.0f && sum3 == 24.0f;
    }());
}

TEST(testVecEquality)
{
    static_assert(Vec2{1, 2} == Vec2{1, 2});
    static_assert(!(Vec2{1, 2} == Vec2{1, 3}));
    static_assert(!(Vec2{1, 2} == Vec2{3, 2}));
    static_assert(Vec3{1, 2, 3} == Vec3{1, 2, 3});
    static_assert(!(Vec3{1, 2, 3} == Vec3{1, 2, 4}));
    static_assert(!(Vec3{1, 2, 3} == Vec3{4, 2, 3}));
    static_assert(Vec4{1, 2, 3, 4} == Vec4{1, 2, 3, 4});
    static_assert(!(Vec4{1, 2, 3, 4} == Vec4{1, 2, 3, 5}));
    static_assert(!(Vec4{1, 2, 3, 4} == Vec4{5, 2, 3, 4}));
    static_assert(!(Vec4{1, 2, 3, 4} == Vec4{1, 6, 3, 4}));
    static_assert(!(Vec4{1, 2, 3, 4} == Vec4{1, 2, 7, 4}));
}

TEST(testVecInequality)
{
    static_assert(Vec2{1, 2} != Vec2{1, 3});
    static_assert(Vec2{1, 2} != Vec2{3, 2});
    static_assert(!(Vec2{1, 2} != Vec2{1, 2}));
    static_assert(Vec3{1, 2, 3} != Vec3{1, 2, 4});
    static_assert(Vec3{1, 2, 3} != Vec3{4, 2, 3});
    static_assert(Vec3{1, 2, 3} != Vec3{1, 5, 3});
    static_assert(!(Vec3{1, 2, 3} != Vec3{1, 2, 3}));
    static_assert(Vec4{1, 2, 3, 4} != Vec4{1, 2, 3, 5});
    static_assert(Vec4{1, 2, 3, 4} != Vec4{5, 2, 3, 4});
    static_assert(Vec4{1, 2, 3, 4} != Vec4{1, 6, 3, 4});
    static_assert(Vec4{1, 2, 3, 4} != Vec4{1, 2, 7, 4});
    static_assert(!(Vec4{1, 2, 3, 4} != Vec4{1, 2, 3, 4}));
}

TEST(testVecNegation)
{
    static_assert(-Vec2{1.0f, -2.0f} == Vec2{-1.0f, 2.0f});
    static_assert(-Vec3{1.0f, -2.0f, 3.0f} == Vec3{-1.0f, 2.0f, -3.0f});
    static_assert(-Vec4{1.0f, -2.0f, 3.0f, -4.0f} == Vec4{-1.0f, 2.0f, -3.0f, 4.0f});
    static_assert(-Vec2{0.0f} == Vec2{0.0f});
}

TEST(testVecArithmetic)
{
    constexpr Vec2 a2{1.0f, 2.0f};
    constexpr Vec2 b2{3.0f, 4.0f};
    static_assert(a2 + b2 == Vec2{4.0f, 6.0f});
    static_assert(b2 - a2 == Vec2{2.0f, 2.0f});
    static_assert(a2 * b2 == Vec2{3.0f, 8.0f});
    static_assert(b2 / a2 == Vec2{3.0f, 2.0f});
    static_assert((a2 + b2) - b2 == a2);
    static_assert((a2 * b2) / b2 == a2);

    constexpr Vec3 a3{1.0f, 2.0f, 3.0f};
    constexpr Vec3 b3{4.0f, 5.0f, 6.0f};
    static_assert(a3 + b3 == Vec3{5.0f, 7.0f, 9.0f});
    static_assert(b3 - a3 == Vec3{3.0f, 3.0f, 3.0f});
    static_assert(a3 * b3 == Vec3{4.0f, 10.0f, 18.0f});
    static_assert(b3 / a3 == Vec3{4.0f, 2.5f, 2.0f});

    constexpr Vec4 a4{1.0f, 2.0f, 3.0f, 4.0f};
    constexpr Vec4 b4{5.0f, 6.0f, 7.0f, 8.0f};
    static_assert(a4 + b4 == Vec4{6.0f, 8.0f, 10.0f, 12.0f});
    static_assert(b4 - a4 == Vec4{4.0f, 4.0f, 4.0f, 4.0f});
    static_assert(a4 * b4 == Vec4{5.0f, 12.0f, 21.0f, 32.0f});
    static_assert(b4 / a4 == Vec4{5.0f, 3.0f, 7.0f / 3.0f, 2.0f});
}

TEST(testVecScalarOps)
{
    constexpr Vec2 v2{2.0f, 3.0f};
    static_assert((5.0f * v2) == Vec2{10.0f, 15.0f});
    static_assert((v2 * 5.0f) == Vec2{10.0f, 15.0f});
    static_assert((v2 / 2.0f) == Vec2{1.0f, 1.5f});
    static_assert((v2 * -2.0f) == Vec2{-4.0f, -6.0f});
    static_assert((v2 * 1.0f) == v2);
    static_assert((v2 / 1.0f) == v2);
    static_assert((0.0f * v2) == Vec2{0.0f});

    constexpr Vec3 v3{1.0f, 2.0f, 3.0f};
    static_assert(2.0f * v3 == Vec3{2.0f, 4.0f, 6.0f});
    static_assert(v3 * 2.0f == Vec3{2.0f, 4.0f, 6.0f});
    static_assert(v3 / 2.0f == Vec3{0.5f, 1.0f, 1.5f});
    static_assert(v3 * 0.5f * 2.0f == v3);

    constexpr Vec4 v4{1.0f, 2.0f, 3.0f, 4.0f};
    static_assert(3.0f * v4 == Vec4{3.0f, 6.0f, 9.0f, 12.0f});
    static_assert(v4 / 2.0f == Vec4{0.5f, 1.0f, 1.5f, 2.0f});
}

TEST(testVecInPlace)
{
    static_assert([] {
        Vec2 v2{1.0f, 2.0f};
        v2 += Vec2{3.0f, 4.0f};
        bool ok = v2 == Vec2{4.0f, 6.0f};
        v2 -= Vec2{1.0f, 1.0f};
        ok = ok && v2 == Vec2{3.0f, 5.0f};
        v2 *= Vec2{2.0f, 3.0f};
        ok = ok && v2 == Vec2{6.0f, 15.0f};
        v2 /= Vec2{3.0f, 5.0f};
        ok = ok && v2 == Vec2{2.0f, 3.0f};
        v2 *= 2.0f;
        ok = ok && v2 == Vec2{4.0f, 6.0f};
        v2 /= 2.0f;
        ok = ok && v2 == Vec2{2.0f, 3.0f};
        v2 *= -1.0f;
        ok = ok && v2 == Vec2{-2.0f, -3.0f};
        return ok;
    }());

    static_assert([] {
        Vec3 v3{1.0f, 2.0f, 3.0f};
        v3 += Vec3{4.0f, 5.0f, 6.0f};
        bool ok = v3 == Vec3{5.0f, 7.0f, 9.0f};
        v3 -= Vec3{1.0f, 2.0f, 3.0f};
        ok = ok && v3 == Vec3{4.0f, 5.0f, 6.0f};
        v3 *= Vec3{2.0f, 2.0f, 2.0f};
        ok = ok && v3 == Vec3{8.0f, 10.0f, 12.0f};
        v3 /= Vec3{2.0f, 5.0f, 4.0f};
        ok = ok && v3 == Vec3{4.0f, 2.0f, 3.0f};
        return ok;
    }());

    static_assert([] {
        Vec4 v4{1.0f, 2.0f, 3.0f, 4.0f};
        v4 += Vec4{5.0f, 6.0f, 7.0f, 8.0f};
        bool ok = v4 == Vec4{6.0f, 8.0f, 10.0f, 12.0f};
        v4 -= Vec4{1.0f, 2.0f, 3.0f, 4.0f};
        ok = ok && v4 == Vec4{5.0f, 6.0f, 7.0f, 8.0f};
        v4 *= Vec4{2.0f, 2.0f, 2.0f, 2.0f};
        ok = ok && v4 == Vec4{10.0f, 12.0f, 14.0f, 16.0f};
        v4 /= Vec4{2.0f, 3.0f, 2.0f, 4.0f};
        ok = ok && v4 == Vec4{5.0f, 4.0f, 7.0f, 4.0f};
        return ok;
    }());
}

TEST(testVecInPlaceSmaller)
{
    static_assert([] {
        Vec4 v{1.0f, 2.0f, 3.0f, 4.0f};
        v += Vec2{10.0f, 20.0f};
        bool ok = v == Vec4{11.0f, 22.0f, 3.0f, 4.0f};
        v -= Vec2{1.0f, 2.0f};
        ok = ok && v == Vec4{10.0f, 20.0f, 3.0f, 4.0f};
        v *= Vec2{2.0f, 3.0f};
        ok = ok && v == Vec4{20.0f, 60.0f, 3.0f, 4.0f};
        v /= Vec2{2.0f, 4.0f};
        ok = ok && v == Vec4{10.0f, 15.0f, 3.0f, 4.0f};
        return ok;
    }());

    static_assert([] {
        Vec3 v3{1.0f, 2.0f, 3.0f};
        v3 += Vec2{1.0f, 1.0f};
        bool ok = v3 == Vec3{2.0f, 3.0f, 3.0f};
        v3 -= Vec2{2.0f, 1.0f};
        ok = ok && v3 == Vec3{0.0f, 2.0f, 3.0f};
        v3 *= Vec2{4.0f, 3.0f};
        ok = ok && v3 == Vec3{0.0f, 6.0f, 3.0f};
        v3 /= Vec2{2.0f, 3.0f};
        ok = ok && v3 == Vec3{0.0f, 2.0f, 3.0f};
        return ok;
    }());
}

TEST(testVecPromotion)
{
    constexpr Vec2 a{1.0f, 2.0f};
    constexpr Vec3 b{10.0f, 20.0f, 30.0f};
    static_assert(a + b == Vec3{11.0f, 22.0f, 30.0f});
    static_assert(b + a == Vec3{11.0f, 22.0f, 30.0f});
    static_assert(b - a == Vec3{9.0f, 18.0f, 30.0f});
    static_assert(a * b == Vec3{10.0f, 40.0f, 30.0f});
    static_assert(b * a == Vec3{10.0f, 40.0f, 30.0f});
    static_assert(b / a == Vec3{10.0f, 10.0f, 30.0f});

    constexpr Vec2 c{4.0f, 3.0f};
    constexpr Vec3 d{8.0f, 6.0f, 3.0f};
    static_assert(c - d == Vec3{4.0f, 3.0f, 3.0f});
    static_assert(d - c == Vec3{4.0f, 3.0f, 3.0f});
    static_assert(c * d == Vec3{32.0f, 18.0f, 3.0f});
    static_assert(d * c == Vec3{32.0f, 18.0f, 3.0f});
    static_assert(c / d == Vec3{2.0f, 2.0f, 3.0f});
    static_assert(d / c == Vec3{2.0f, 2.0f, 3.0f});

    constexpr Vec2 e{1.0f, 2.0f};
    constexpr Vec4 f{1.0f, 1.0f, 1.0f, 1.0f};
    static_assert(e + f == Vec4{2.0f, 3.0f, 1.0f, 1.0f});
    static_assert(f - e == Vec4{0.0f, -1.0f, 1.0f, 1.0f});
    static_assert(f + e == Vec4{2.0f, 3.0f, 1.0f, 1.0f});
    static_assert(e - f == Vec4{0.0f, -1.0f, 1.0f, 1.0f});
    static_assert(e * f == Vec4{1.0f, 2.0f, 1.0f, 1.0f});
    static_assert(f * e == Vec4{1.0f, 2.0f, 1.0f, 1.0f});
    static_assert(f / e == Vec4{1.0f, 0.5f, 1.0f, 1.0f});
    static_assert(e / f == Vec4{1.0f, 0.5f, 1.0f, 1.0f});

    constexpr Vec3 g{4.0f, 9.0f, 16.0f};
    constexpr Vec2 h{2.0f, 3.0f};
    static_assert(g / h == Vec3{2.0f, 3.0f, 16.0f});

    constexpr Vec2 p2{4.0f, 9.0f};
    constexpr Vec3 p3{8.0f, 6.0f, 3.0f};
    static_assert(p2 + p3 == Vec3{12.0f, 15.0f, 3.0f});
    static_assert(p3 + p2 == Vec3{12.0f, 15.0f, 3.0f});
    static_assert(p2 * p3 == Vec3{32.0f, 54.0f, 3.0f});
    static_assert(p3 * p2 == Vec3{32.0f, 54.0f, 3.0f});
}

TEST(testVecConversion)
{
    static_assert(static_cast<Vec3>(Vec4{1.0f, 2.0f, 3.0f, 4.0f}) == Vec3{1.0f, 2.0f, 3.0f});
    static_assert(static_cast<Vec2>(Vec4{1.0f, 2.0f, 3.0f, 4.0f}) == Vec2{1.0f, 2.0f});
    static_assert(static_cast<Vec2>(Vec3{1.0f, 2.0f, 3.0f}) == Vec2{1.0f, 2.0f});

    static_assert(Vec4{Vec2{1.0f, 2.0f}} == Vec4{1.0f, 2.0f, 0.0f, 0.0f});
    static_assert(Vec3{Vec2{1.0f, 2.0f}} == Vec3{1.0f, 2.0f, 0.0f});
    static_assert(Vec4{Vec3{1.0f, 2.0f, 3.0f}} == Vec4{1.0f, 2.0f, 3.0f, 0.0f});

    static_assert(Vec2{Vec4{1.0f, 2.0f, 3.0f, 4.0f}} == Vec2{1.0f, 2.0f});
    static_assert(Vec2{Vec3{1.0f, 2.0f, 3.0f}} == Vec2{1.0f, 2.0f});
    static_assert(Vec3{Vec4{1.0f, 2.0f, 3.0f, 4.0f}} == Vec3{1.0f, 2.0f, 3.0f});

    static_assert(Vec3{Vec2{1.0f, 2.0f}, 3.0f} == Vec3{1.0f, 2.0f, 3.0f});
    static_assert(Vec4{Vec2{1.0f, 2.0f}, 3.0f, 4.0f} == Vec4{1.0f, 2.0f, 3.0f, 4.0f});
    static_assert(Vec4{Vec3{1.0f, 2.0f, 3.0f}, 4.0f} == Vec4{1.0f, 2.0f, 3.0f, 4.0f});
}

TEST(testVecDot)
{
    static_assert(dot(Vec2{1.0f, 0.0f}, Vec2{0.0f, 1.0f}) == 0.0f);
    static_assert(dot(Vec2{1.0f, 0.0f}, Vec2{1.0f, 0.0f}) == 1.0f);
    static_assert(dot(Vec2{3.0f, 4.0f}, Vec2{5.0f, 6.0f}) == 39.0f);
    static_assert(dot(Vec2{-1.0f, 0.0f}, Vec2{1.0f, 0.0f}) == -1.0f);
    static_assert(dot(Vec3{1.0f, 0.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f}) == 0.0f);
    static_assert(dot(Vec3{1.0f, 2.0f, 3.0f}, Vec3{4.0f, 5.0f, 6.0f}) == 32.0f);
    static_assert(dot(Vec4{1.0f, 2.0f, 3.0f, 4.0f}, Vec4{5.0f, 6.0f, 7.0f, 8.0f}) == 70.0f);
    static_assert(dot(Vec4{1.0f, 1.0f, 1.0f, 1.0f}, Vec4{1.0f, -1.0f, 1.0f, -1.0f}) == 0.0f);
}

TEST(testVecLength)
{
    static_assert(lengthSquared(Vec2{0.0f, 0.0f}) == 0.0f);
    static_assert(lengthSquared(Vec2{1.0f, 0.0f}) == 1.0f);
    static_assert(lengthSquared(Vec2{3.0f, 4.0f}) == 25.0f);
    static_assert(lengthSquared(Vec3{1.0f, 2.0f, 2.0f}) == 9.0f);
    static_assert(lengthSquared(Vec3{0.0f, 0.0f, 0.0f}) == 0.0f);
    static_assert(lengthSquared(Vec4{1.0f, 0.0f, 0.0f, 0.0f}) == 1.0f);
    static_assert(lengthSquared(Vec4{1.0f, 2.0f, 3.0f, 4.0f}) == 30.0f);

    ASSERT((std::abs(length(Vec2{3.0f, 4.0f}) - 5.0f) < FLT_EPSILON));
    ASSERT((std::abs(length(Vec2{-3.0f, -4.0f}) - 5.0f) < FLT_EPSILON));
    ASSERT((std::abs(length(Vec3{1.0f, 2.0f, 2.0f}) - 3.0f) < FLT_EPSILON));
    ASSERT((std::abs(length(Vec4{1.0f, 2.0f, 3.0f, 4.0f}) - std::sqrt(30.0f)) < FLT_EPSILON));
}

TEST(testVecNormalize)
{
    Vec2 n = normalize(Vec2{3.0f, 0.0f});
    ASSERT((n == Vec2{1.0f, 0.0f}));
    ASSERT((std::abs(length(normalize(Vec2{3.0f, 4.0f})) - 1.0f) < FLT_EPSILON));

    Vec3 n3 = normalize(Vec3{0.0f, -5.0f, 0.0f});
    ASSERT((n3 == Vec3{0.0f, -1.0f, 0.0f}));
    ASSERT((std::abs(length(normalize(Vec3{1.0f, 2.0f, 3.0f})) - 1.0f) < FLT_EPSILON));

    Vec4 n4 = normalize(Vec4{-5.0f, 0.0f, 0.0f, 0.0f});
    ASSERT((n4 == Vec4{-1.0f, 0.0f, 0.0f, 0.0f}));
    ASSERT((std::abs(length(normalize(Vec4{1.0f, 2.0f, 3.0f, 4.0f})) - 1.0f) < FLT_EPSILON));

    Vec2 u = normalize(Vec2{3.0f, 4.0f});
    ASSERT((normalize(u) == u));
}

TEST(testVecCross)
{
    static_assert(cross(Vec2{1.0f, 0.0f}, Vec2{0.0f, 1.0f}) == 1.0f);
    static_assert(cross(Vec2{0.0f, 1.0f}, Vec2{1.0f, 0.0f}) == -1.0f);
    static_assert(cross(Vec2{2.0f, 3.0f}, Vec2{4.0f, 6.0f}) == 0.0f);
    static_assert(cross(Vec2{1.0f, 0.0f}, Vec2{1.0f, 0.0f}) == 0.0f);
    static_assert(cross(Vec2{3.0f, 4.0f}, Vec2{3.0f, 4.0f}) == 0.0f);
    static_assert(cross(Vec2{5.0f, 0.0f}, Vec2{0.0f, 2.0f}) == 10.0f);

    constexpr Vec3 x{1.0f, 0.0f, 0.0f};
    constexpr Vec3 y{0.0f, 1.0f, 0.0f};
    constexpr Vec3 z{0.0f, 0.0f, 1.0f};
    static_assert(cross(x, y) == z);
    static_assert(cross(y, z) == x);
    static_assert(cross(z, x) == y);
    static_assert(cross(y, x) == -z);
    static_assert(cross(z, y) == -x);
    static_assert(cross(x, z) == -y);
    static_assert(cross(x, x) == Vec3{0.0f});
    static_assert(cross(Vec3{1.0f, 2.0f, 3.0f}, Vec3{4.0f, 5.0f, 6.0f}) == Vec3{-3.0f, 6.0f, -3.0f});
    static_assert(dot(cross(x, y), x) == 0.0f);
    static_assert(dot(cross(x, y), y) == 0.0f);
}

TEST(testVecConstexprOps)
{
    static_assert(Vec2{1.0f, 2.0f}.n == 2);

    constexpr Vec2 a{1.0f, 2.0f};
    constexpr Vec2 b{3.0f, 4.0f};
    constexpr Vec2 sum = a + b;
    static_assert(sum == Vec2{4.0f, 6.0f});
    constexpr Vec2 diff = b - a;
    static_assert(diff == Vec2{2.0f, 2.0f});
    constexpr Vec2 prod = a * b;
    static_assert(prod == Vec2{3.0f, 8.0f});
    constexpr Vec2 quot = Vec2{4.0f, 9.0f} / Vec2{2.0f, 3.0f};
    static_assert(quot == Vec2{2.0f, 3.0f});
    constexpr Vec2 scaled = a * 3.0f;
    static_assert(scaled == Vec2{3.0f, 6.0f});
    constexpr Vec2 divided = Vec2{2.0f, 3.0f} / 2.0f;
    static_assert(divided == Vec2{1.0f, 1.5f});
    constexpr Vec3 promoted = a + Vec3{1.0f, 1.0f, 1.0f};
    static_assert(promoted == Vec3{2.0f, 3.0f, 1.0f});
    constexpr Vec2 neg = -a;
    static_assert(neg == Vec2{-1.0f, -2.0f});
    constexpr Vec2 s2 = Vec2{5.0f};
    static_assert(s2 + s2 == Vec2{10.0f});
}

TEST(testMatConstruction)
{
    static_assert(Mat2::isMat);
    static_assert(Mat2::width == 2 && Mat2::height == 2);
    static_assert(Mat<2, 3>::width == 2 && Mat<2, 3>::height == 3);

    static_assert(!std::is_convertible_v<f32, Mat2>);
    static_assert(!std::is_convertible_v<Mat3, Mat4>);
    static_assert(!std::is_convertible_v<Mat4, Mat3>);
    static_assert(!std::is_convertible_v<Mat2, Mat<2, 3>>);

    constexpr Mat2 m2{Vec2{1.0f, 2.0f}, Vec2{3.0f, 4.0f}};
    static_assert(m2[0] == Vec2{1.0f, 2.0f});
    static_assert(m2[1] == Vec2{3.0f, 4.0f});

    constexpr Mat3 m3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
    static_assert(m3[0] == Vec3{1, 2, 3});
    static_assert(m3[1] == Vec3{4, 5, 6});
    static_assert(m3[2] == Vec3{7, 8, 9});

    constexpr Mat4 m4{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}};
    static_assert(m4[0] == Vec4{1, 2, 3, 4});
    static_assert(m4[3] == Vec4{13, 14, 15, 16});

    constexpr Mat2 c2{1.0f, 2.0f, 3.0f, 4.0f};
    static_assert(c2[0] == Vec2{1.0f, 2.0f});
    static_assert(c2[1] == Vec2{3.0f, 4.0f});

    constexpr Mat<2, 3> rect{1, 2, 3, 4, 5, 6};
    static_assert(rect[0] == Vec3{1, 2, 3});
    static_assert(rect[1] == Vec3{4, 5, 6});

    static_assert(Mat3{Mat2{Vec2{1.0f, 2.0f}, Vec2{3.0f, 4.0f}}}[0] == Vec3{1.0f, 2.0f, 0.0f});

    static_assert(Mat2{1.0f}[0] == Vec2{1.0f, 0.0f});
    static_assert(Mat2{1.0f}[1] == Vec2{0.0f, 1.0f});
    static_assert(Mat3{1.0f}[0] == Vec3{1, 0, 0});
    static_assert(Mat3{1.0f}[1] == Vec3{0, 1, 0});
    static_assert(Mat3{1.0f}[2] == Vec3{0, 0, 1});
    static_assert(Mat4{1.0f}[0] == Vec4{1, 0, 0, 0});
    static_assert(Mat4{1.0f}[3] == Vec4{0, 0, 0, 1});

    static_assert(Mat2{0.0f}[0] == Vec2{0.0f, 0.0f});
    static_assert(Mat2{0.0f}[1] == Vec2{0.0f, 0.0f});

    static_assert([] {
        Mat2 a{Vec2{1.0f, 2.0f}, Vec2{3.0f, 4.0f}};
        Mat2 b = a;
        b[1][1] = 9.0f;
        return a[1][1] == 4.0f && b[1][1] == 9.0f;
    }());

    static_assert([] {
        Mat3 d;
        d[0] = Vec3{1, 2, 3};
        d[1] = Vec3{4, 5, 6};
        d[2] = Vec3{7, 8, 9};
        return d == Mat3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
    }());
}

TEST(testMatIndexAccessors)
{
    constexpr Mat3 m{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
    static_assert(m[1][2] == 6.0f);
    static_assert(m.x().y() == 2.0f);
    static_assert(m.y().z() == 6.0f);
    static_assert(m.z().x() == 7.0f);

    constexpr Mat4 m4{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}};
    static_assert(m4.w().x() == 13.0f);
    static_assert(m4.w().w() == 16.0f);

    constexpr Mat<2, 3> r{1, 2, 3, 4, 5, 6};
    static_assert(r[1][0] == 4.0f);

    static_assert([] {
        Mat3 m{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
        m.y().z() = 60.0f;
        bool ok = m[1][2] == 60.0f;
        m[1][2] = 6.0f;
        ok = ok && m[1][2] == 6.0f;
        Mat4 m4{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}};
        m4.w().x() = 130.0f;
        ok = ok && m4[3][0] == 130.0f;
        Mat<2, 3> r{1, 2, 3, 4, 5, 6};
        r.x().z() = 30.0f;
        ok = ok && r[0][2] == 30.0f && r[1][0] == 4.0f;
        return ok;
    }());
}

TEST(testMatRangeFor)
{
    static_assert([] {
        Mat3 m{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
        f32 sum = 0.0f;
        usize count = 0;
        for (Vec3& col : m)
        {
            for (f32 f : col)
            {
                sum += f;
                ++count;
            }
        }
        bool ok = sum == 45.0f && count == 9;

        for (Vec3& col : m)
        {
            col.x() = 0.0f;
        }
        ok = ok && m == Mat3{Vec3{0, 2, 3}, Vec3{0, 5, 6}, Vec3{0, 8, 9}};

        f32 rsum = 0.0f;
        for (const Vec3& col : Mat<2, 3>{1, 2, 3, 4, 5, 6})
        {
            for (const f32& f : col)
            {
                rsum += f;
            }
        }
        ok = ok && rsum == 21.0f;
        return ok;
    }());
}

TEST(testMatConvert)
{
    constexpr Mat3 m3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};

    constexpr Mat4 up4{m3};
    static_assert(up4[0] == Vec4{1, 2, 3, 0});
    static_assert(up4[1] == Vec4{4, 5, 6, 0});
    static_assert(up4[2] == Vec4{7, 8, 9, 0});
    static_assert(up4[3] == Vec4{0, 0, 0, 1});

    constexpr Mat2 down2{m3};
    static_assert(down2[0] == Vec2{1, 2});
    static_assert(down2[1] == Vec2{4, 5});

    constexpr Mat3 down3{Mat4{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}}};
    static_assert(down3[0] == Vec3{1, 2, 3});
    static_assert(down3[1] == Vec3{5, 6, 7});
    static_assert(down3[2] == Vec3{9, 10, 11});

    constexpr Mat3 mid{Mat2{Vec2{1, 2}, Vec2{3, 4}}};
    static_assert(mid[0] == Vec3{1, 2, 0});
    static_assert(mid[1] == Vec3{3, 4, 0});
    static_assert(mid[2] == Vec3{0, 0, 1});

    constexpr Mat4 from2{Mat2{Vec2{1, 2}, Vec2{3, 4}}};
    static_assert(from2[0] == Vec4{1, 2, 0, 0});
    static_assert(from2[1] == Vec4{3, 4, 0, 0});
    static_assert(from2[2] == Vec4{0, 0, 1, 0});
    static_assert(from2[3] == Vec4{0, 0, 0, 1});

    constexpr Mat<2, 3> rectUp{Mat2{Vec2{1, 2}, Vec2{3, 4}}};
    static_assert(rectUp[0] == Vec3{1, 2, 0});
    static_assert(rectUp[1] == Vec3{3, 4, 0});

    static_assert(Mat<2, 3>{Mat<3, 2>{1, 2, 3, 4, 5, 6}} == Mat<2, 3>{1, 2, 0, 3, 4, 0});
    static_assert(Mat<3, 2>{Mat<2, 3>{1, 2, 3, 4, 5, 6}} == Mat<3, 2>{1, 2, 4, 5, 0, 0});
    static_assert(Mat2{Mat4{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}}} == Mat2{Vec2{1, 2}, Vec2{5, 6}});
}

TEST(testMatEquality)
{
    static_assert(Mat2{Vec2{1, 2}, Vec2{3, 4}} == Mat2{Vec2{1, 2}, Vec2{3, 4}});
    static_assert(Mat2{Vec2{1, 2}, Vec2{3, 4}} != Mat2{Vec2{5, 6}, Vec2{7, 8}});
    static_assert(!(Mat2{Vec2{1, 2}, Vec2{3, 4}} != Mat2{Vec2{1, 2}, Vec2{3, 4}}));
    static_assert(!(Mat2{Vec2{1, 2}, Vec2{3, 4}} == Mat2{Vec2{5, 6}, Vec2{7, 8}}));

    static_assert(Mat3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}} == Mat3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}});
    static_assert(Mat3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}} != Mat3{Vec3{9, 8, 7}, Vec3{6, 5, 4}, Vec3{3, 2, 1}});
    static_assert(!(Mat3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}} != Mat3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}}));
    static_assert(!(Mat3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}} == Mat3{Vec3{9, 8, 7}, Vec3{6, 5, 4}, Vec3{3, 2, 1}}));

    static_assert(Mat4{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}} == Mat4{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}});
    static_assert(Mat4{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}} != Mat4{0.0f});
    static_assert(!(Mat4{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}} != Mat4{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}}));

    static_assert(Mat3{1.0f} == Mat3{Vec3{1, 0, 0}, Vec3{0, 1, 0}, Vec3{0, 0, 1}});
    static_assert(Mat3{0.0f} != Mat3{1.0f});
    static_assert(Mat<2, 3>{1, 2, 3, 4, 5, 6} == Mat<2, 3>{1, 2, 3, 4, 5, 6});
    static_assert(Mat<2, 3>{1, 2, 3, 4, 5, 6} != Mat<2, 3>{6, 5, 4, 3, 2, 1});
}

TEST(testMatInPlace)
{
    static_assert([] {
        Mat2 m{Vec2{1, 2}, Vec2{3, 4}};
        m += Mat2{Vec2{5, 6}, Vec2{7, 8}};
        bool ok = m == Mat2{Vec2{6, 8}, Vec2{10, 12}};
        m -= Mat2{Vec2{1, 1}, Vec2{1, 1}};
        ok = ok && m == Mat2{Vec2{5, 7}, Vec2{9, 11}};
        m *= 2.0f;
        ok = ok && m == Mat2{Vec2{10, 14}, Vec2{18, 22}};
        m /= 2.0f;
        ok = ok && m == Mat2{Vec2{5, 7}, Vec2{9, 11}};
        return ok;
    }());

    static_assert([] {
        Mat3 n{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
        n += Mat2{Vec2{1, 1}, Vec2{1, 1}};
        bool ok = n == Mat3{Vec3{2, 3, 3}, Vec3{5, 6, 6}, Vec3{7, 8, 9}};
        n -= Mat2{Vec2{1, 1}, Vec2{1, 1}};
        ok = ok && n == Mat3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
        return ok;
    }());

    static_assert([] {
        Mat4 p{1.0f};
        p += Mat3{1.0f};
        bool ok = p[0] == Vec4{2, 0, 0, 0};
        ok = ok && p[1] == Vec4{0, 2, 0, 0};
        ok = ok && p[2] == Vec4{0, 0, 2, 0};
        ok = ok && p[3] == Vec4{0, 0, 0, 1};
        p -= Mat3{1.0f};
        ok = ok && p == Mat4{1.0f};
        p *= 2.0f;
        ok = ok && p[0] == Vec4{2, 0, 0, 0};
        ok = ok && p[3] == Vec4{0, 0, 0, 2};
        p /= 2.0f;
        ok = ok && p[0] == Vec4{1, 0, 0, 0};
        ok = ok && p[3] == Vec4{0, 0, 0, 1};
        return ok;
    }());
}

TEST(testMatInPlaceRect)
{
    static_assert([] {
        Mat<2, 3> m{1, 2, 3, 4, 5, 6};
        m += Mat2{Vec2{1, 1}, Vec2{1, 1}};
        bool ok = m[0] == Vec3{2, 3, 3} && m[1] == Vec3{5, 6, 6};
        m -= Mat2{Vec2{1, 1}, Vec2{1, 1}};
        ok = ok && m[0] == Vec3{1, 2, 3} && m[1] == Vec3{4, 5, 6};
        return ok;
    }());

    static_assert([] {
        Mat<3, 2> n{1, 2, 3, 4, 5, 6};
        n += Mat<2, 2>{1, 1, 1, 1};
        bool ok = n[0] == Vec2{2, 3} && n[1] == Vec2{4, 5} && n[2] == Vec2{5, 6};
        n -= Mat<2, 2>{1, 1, 1, 1};
        ok = ok && n[0] == Vec2{1, 2} && n[2] == Vec2{5, 6};
        return ok;
    }());
}

TEST(testMatAddSub)
{
    constexpr Mat2 a{Vec2{1, 2}, Vec2{3, 4}};
    constexpr Mat2 b{Vec2{5, 6}, Vec2{7, 8}};
    static_assert(a + b == Mat2{Vec2{6, 8}, Vec2{10, 12}});
    static_assert(b - a == Mat2{Vec2{4, 4}, Vec2{4, 4}});
    static_assert(-a == Mat2{Vec2{-1, -2}, Vec2{-3, -4}});
    static_assert(a + Mat2{0.0f} == a);
    static_assert(a - a == Mat2{0.0f});

    constexpr Mat4 x{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}};
    constexpr Mat4 y{Vec4{16, 15, 14, 13}, Vec4{12, 11, 10, 9}, Vec4{8, 7, 6, 5}, Vec4{4, 3, 2, 1}};
    constexpr Mat4 sum = x + y;
    constexpr Mat4 diff = x - y;
    static_assert(sum[0] == Vec4{17, 17, 17, 17});
    static_assert(sum[3] == Vec4{17, 17, 17, 17});
    static_assert(diff[0] == Vec4{-15, -13, -11, -9});
    static_assert(diff[3] == Vec4{9, 11, 13, 15});
    static_assert(x + Mat4{0.0f} == x);
    static_assert(x - x == Mat4{0.0f});
    static_assert(x + Mat4{1.0f} - Mat4{1.0f} == x);

    constexpr Mat<2, 3> r1{1, 2, 3, 4, 5, 6};
    constexpr Mat<2, 3> r2{1, 1, 1, 1, 1, 1};
    static_assert((r1 + r2)[0] == Vec3{2, 3, 4});
    static_assert((r1 - r2)[1] == Vec3{3, 4, 5});
}

TEST(testMat2Mul)
{
    constexpr Mat2 a{Vec2{1, 2}, Vec2{3, 4}};
    constexpr Mat2 b{Vec2{5, 6}, Vec2{7, 8}};
    constexpr Mat2 id{1.0f};
    static_assert(a * b == Mat2{Vec2{23, 34}, Vec2{31, 46}});
    static_assert(id * a == a);
    static_assert(a * id == a);
    static_assert(id * id == id);
    static_assert(a * a * a == a * (a * a));
}

TEST(testMat3Mul)
{
    constexpr Mat3 id{1.0f};
    constexpr Mat3 a{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
    constexpr Mat3 b{Vec3{9, 8, 7}, Vec3{6, 5, 4}, Vec3{3, 2, 1}};
    static_assert(id * a == a);
    static_assert(a * id == a);
    static_assert(a * b == Mat3{Vec3{90, 114, 138}, Vec3{54, 69, 84}, Vec3{18, 24, 30}});
    static_assert(a * b * id == a * b);
    static_assert((a * b) * id == a * (b * id));
}

TEST(testMat4Mul)
{
    constexpr Mat4 id{1.0f};
    constexpr Mat4 a{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}};
    constexpr Mat4 b{Vec4{16, 15, 14, 13}, Vec4{12, 11, 10, 9}, Vec4{8, 7, 6, 5}, Vec4{4, 3, 2, 1}};
    static_assert(id * a == a);
    static_assert(a * id == a);
    static_assert(a * b == Mat4{Vec4{386, 444, 502, 560}, Vec4{274, 316, 358, 400}, Vec4{162, 188, 214, 240}, Vec4{50, 60, 70, 80}});
    static_assert(id * id == id);
}

TEST(testMatVecMul)
{
    static_assert(Mat2{1.0f} * Vec2{5, 6} == Vec2{5, 6});
    static_assert(Mat2{Vec2{1, 2}, Vec2{3, 4}} * Vec2{5, 6} == Vec2{23, 34});

    static_assert(Mat3{1.0f} * Vec3{1, 2, 3} == Vec3{1, 2, 3});
    static_assert(Mat3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}} * Vec3{1, 2, 3} == Vec3{30, 36, 42});

    static_assert(Mat4{1.0f} * Vec4{1, 2, 3, 4} == Vec4{1, 2, 3, 4});
    static_assert([] {
        Mat4 t4{1.0f};
        t4.w() = Vec4{10, 20, 30, 1};
        return t4 * Vec4{1, 2, 3, 1} == Vec4{11, 22, 33, 1};
    }());

    static_assert(Mat<2, 3>{1, 2, 3, 4, 5, 6} * Vec2{7, 8} == Vec3{39, 54, 69});
    static_assert(Mat<3, 2>{1, 2, 3, 4, 5, 6} * Vec3{1, 1, 1} == Vec2{9, 12});
}

TEST(testMatScalarOps)
{
    constexpr Mat2 a{1.0f, 2.0f, 3.0f, 4.0f};
    static_assert(a * 2.0f == Mat2{Vec2{2, 4}, Vec2{6, 8}});
    static_assert(2.0f * a == Mat2{Vec2{2, 4}, Vec2{6, 8}});
    static_assert(a / 2.0f == Mat2{Vec2{0.5f, 1.0f}, Vec2{1.5f, 2.0f}});
    static_assert(a * 1.0f == a);
    static_assert(a / 1.0f == a);
    static_assert(a * 0.0f == Mat2{0.0f});
    static_assert(a * -1.0f == -a);

    static_assert(Mat4{1.0f} * 0.5f == Mat4{0.5f});
    static_assert(0.5f * Mat4{1.0f} == Mat4{0.5f});
    static_assert(Mat4{1.0f} / 2.0f == Mat4{0.5f});

    constexpr Mat<2, 3> r{1, 2, 3, 4, 5, 6};
    static_assert((r * 2.0f)[0] == Vec3{2, 4, 6});
    static_assert((2.0f * r)[1] == Vec3{8, 10, 12});
    static_assert((r / 2.0f)[0] == Vec3{0.5f, 1.0f, 1.5f});
}

TEST(testMatTranspose)
{
    constexpr Mat2 m2{Vec2{1, 2}, Vec2{3, 4}};
    constexpr Mat2 t2 = transpose(m2);
    static_assert(t2[0] == Vec2{1, 3});
    static_assert(t2[1] == Vec2{2, 4});
    static_assert(transpose(t2) == m2);

    constexpr Mat3 m3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
    constexpr Mat3 t3 = transpose(m3);
    static_assert(t3[0] == Vec3{1, 4, 7});
    static_assert(t3[1] == Vec3{2, 5, 8});
    static_assert(t3[2] == Vec3{3, 6, 9});
    static_assert(transpose(t3) == m3);

    constexpr Mat4 m4{Vec4{1, 2, 3, 4}, Vec4{5, 6, 7, 8}, Vec4{9, 10, 11, 12}, Vec4{13, 14, 15, 16}};
    constexpr Mat4 t4 = transpose(m4);
    static_assert(t4[0] == Vec4{1, 5, 9, 13});
    static_assert(t4[1] == Vec4{2, 6, 10, 14});
    static_assert(t4[2] == Vec4{3, 7, 11, 15});
    static_assert(t4[3] == Vec4{4, 8, 12, 16});
    static_assert(transpose(t4) == m4);

    static_assert(transpose(Mat3{1.0f}) == Mat3{1.0f});

    constexpr Mat<2, 3> r{1, 2, 3, 4, 5, 6};
    constexpr Mat<3, 2> rt = transpose(r);
    static_assert(rt[0] == Vec2{1, 4});
    static_assert(rt[1] == Vec2{2, 5});
    static_assert(rt[2] == Vec2{3, 6});
    static_assert(transpose(rt) == r);
}

TEST(testMatRectProduct)
{
    constexpr Mat<4, 3> left{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    static_assert(left * Mat<4, 4>{1.0f} == left);

    static_assert([left] {
        Mat<4, 4> trans{1.0f};
        trans.w() = Vec4{10, 20, 30, 1};
        return left * trans == Mat<4, 3>{1, 2, 3, 4, 5, 6, 7, 8, 9, 310, 371, 432};
    }());

    constexpr Mat<3, 2> a{1, 2, 3, 4, 5, 6};
    constexpr Mat<2, 3> b{1, 2, 3, 4, 5, 6};
    static_assert(a * b == Mat<2, 2>{22, 28, 49, 64});
    static_assert(a * Mat<3, 3>{1.0f} == a);
    static_assert(b * Mat<2, 2>{1.0f} == b);
}

TEST(testMatConstexpr)
{
    constexpr Mat2 a{1.0f, 2.0f, 3.0f, 4.0f};
    static_assert(a[0] == Vec2{1.0f, 2.0f});
    static_assert(a[1] == Vec2{3.0f, 4.0f});

    constexpr Mat2 id{1.0f};
    static_assert(id[0] == Vec2{1.0f, 0.0f});
    static_assert(id[1] == Vec2{0.0f, 1.0f});

    constexpr Mat2 t = transpose(a);
    static_assert(t[0] == Vec2{1.0f, 3.0f});
    static_assert(t[1] == Vec2{2.0f, 4.0f});
    static_assert(transpose(t) == a);

    constexpr Mat2 p = a * id;
    static_assert(p == a);
    static_assert(id * id == id);

    constexpr Mat2 v = Mat2{Vec2{1.0f, 2.0f}, Vec2{3.0f, 4.0f}};
    static_assert(v[0] == Vec2{1.0f, 2.0f});
    static_assert(v[1] == Vec2{3.0f, 4.0f});

    constexpr Mat2 sum = a + id;
    static_assert(sum[1] == Vec2{3.0f, 5.0f});
    constexpr Mat2 diff = a - id;
    static_assert(diff[0] == Vec2{0.0f, 2.0f});
    constexpr Mat2 neg = -a;
    static_assert(neg[0] == Vec2{-1.0f, -2.0f});

    constexpr Vec2 mv = a * Vec2{1.0f, 1.0f};
    static_assert(mv == Vec2{4.0f, 6.0f});
}

TEST(testComplexConstruction)
{
    static_assert(std::is_convertible_v<f32, Complex>);
    static_assert(!std::is_convertible_v<Vec2, Complex>);
    static_assert(!std::is_convertible_v<Complex, Vec2>);

    constexpr Complex c{3.0f, 4.0f};
    static_assert(c.r == 3.0f && c.i == 4.0f);
    constexpr Complex r{5.0f};
    static_assert(r.r == 5.0f && r.i == 0.0f);

    static_assert(static_cast<Complex>(Vec2{3.0f, 4.0f}) == Complex{3.0f, 4.0f});
    static_assert(static_cast<Vec2>(Complex{3.0f, 4.0f}) == Vec2{3.0f, 4.0f});

    static_assert(Complex{3.0f, 4.0f} == Complex{3.0f, 4.0f});
    static_assert(!(Complex{3.0f, 4.0f} == Complex{3.0f, 5.0f}));
    static_assert(Complex{3.0f, 4.0f} != Complex{3.0f, 5.0f});
    static_assert(!(Complex{3.0f, 4.0f} != Complex{3.0f, 4.0f}));

    static_assert([] {
        Complex imp = 3.0f;
        bool ok = imp == Complex{3.0f, 0.0f};
        Complex d;
        d.r = 1.0f;
        d.i = 2.0f;
        ok = ok && d == Complex{1.0f, 2.0f};
        d = Complex{5.0f, 6.0f};
        ok = ok && d == Complex{5.0f, 6.0f};
        Complex a{1.0f, 2.0f};
        Complex b = a;
        b.i = 9.0f;
        ok = ok && a.i == 2.0f && b == Complex{1.0f, 9.0f};
        return ok;
    }());

    static_assert(Complex{0.0f, 1.0f}.i == 1.0f);
}

TEST(testComplexArithmetic)
{
    constexpr Complex a{1, 2};
    constexpr Complex b{3, 4};
    static_assert(a + b == Complex{4, 6});
    static_assert(a - b == Complex{-2, -2});
    static_assert(a * b == Complex{-5, 10});
    static_assert(-a == Complex{-1, -2});
    static_assert(Complex{0, 1} * Complex{0, 1} == Complex{-1, 0});
    static_assert(Complex{0, 1} * Complex{0, 1} * Complex{0, 1} == Complex{0, -1});
    static_assert(Complex{0, 1} * Complex{0, -1} == Complex{1, 0});
    static_assert(Complex{1, 0} * Complex{0, 1} == Complex{0, 1});
    static_assert(b / 2.0f == Complex{1.5f, 2.0f});
    static_assert(a / 1.0f == a);
    static_assert(a * b == b * a);

    static_assert(a + Complex{1} == Complex{2, 2});
    static_assert(Complex{1} + a == Complex{2, 2});
    static_assert(a - Complex{1} == Complex{0, 2});
    static_assert(Complex{1} - a == Complex{0, -2});
    static_assert(a * Complex{2} == Complex{2, 4});
    static_assert(Complex{2} * a == Complex{2, 4});
    static_assert(Complex{2} * a == a * 2.0f);
    static_assert(a * Complex{1} == a);
    static_assert(Complex{3} * Complex{0, 4} == Complex{0, 12});
    static_assert(a + 2.0f == Complex{3, 2});
    static_assert(2.0f + a == Complex{3, 2});
    static_assert(a - 1.0f == Complex{0, 2});
    static_assert(1.0f - a == Complex{0, -2});
    static_assert(a * 2.0f == Complex{2, 4});
    static_assert(2.0f * a == Complex{2, 4});
    static_assert(-Complex{0, 1} == Complex{0, -1});

    static_assert(Complex{0} * a == Complex{0});
    static_assert(Complex{0} + a == a);
    static_assert(a - a == Complex{0});
}

TEST(testComplexInPlace)
{
    static_assert([] {
        Complex c{1, 2};
        c += Complex{3, 4};
        bool ok = c == Complex{4, 6};
        c -= Complex{1, 1};
        ok = ok && c == Complex{3, 5};
        c *= 2.0f;
        ok = ok && c == Complex{6, 10};
        c /= 2.0f;
        ok = ok && c == Complex{3, 5};
        c *= -1.0f;
        ok = ok && c == Complex{-3, -5};
        c = Complex{3, 5};
        ok = ok && c == Complex{3, 5};
        c += 2.0f;
        ok = ok && c == Complex{5, 5};
        c -= 1.0f;
        ok = ok && c == Complex{4, 5};
        c = Complex{6, 8};
        c /= 4.0f;
        ok = ok && c == Complex{1.5f, 2.0f};
        c *= 4.0f;
        ok = ok && c == Complex{6, 8};
        return ok;
    }());
}

TEST(testComplexConjAbsNorm)
{
    constexpr Complex c{3, 4};
    static_assert(conjugate(c) == Complex{3, -4});
    static_assert(conjugate(conjugate(c)) == c);
    static_assert(absSqr(c) == 25.0f);
    static_assert(absSqr(Complex{0, 0}) == 0.0f);
    static_assert(c * conjugate(c) == Complex{25.0f, 0.0f});
    static_assert(Complex{1, 2} + conjugate(Complex{1, 2}) == Complex{2, 0});
    static_assert(conjugate(Complex{5}) == Complex{5, 0});

    ASSERT((std::abs(abs(c) - 5.0f) < FLT_EPSILON));
    ASSERT((abs(Complex{0, 0}) == 0.0f));
    ASSERT((std::abs(abs(conjugate(c)) - abs(c)) < FLT_EPSILON));

    Complex n = normalize(c);
    ASSERT((std::abs(abs(n) - 1.0f) < FLT_EPSILON));
    ASSERT((normalize(Complex{3}) == Complex{1, 0}));
}

TEST(testComplexRotate)
{
    constexpr Vec2 x{1, 0};
    constexpr Vec2 y{0, 1};
    static_assert(rotate(Complex{0, 1}, x) == Vec2{0, 1});
    static_assert(rotate(Complex{-1, 0}, x) == Vec2{-1, 0});
    static_assert(rotate(Complex{1, 0}, x) == Vec2{1, 0});
    static_assert(rotate(Complex{0, -1}, x) == Vec2{0, -1});
    static_assert(rotate(Complex{0, 1}, y) == Vec2{-1, 0});
    static_assert(rotate(Complex{0, 1}, Vec2{3, 4}) == Vec2{-4, 3});
    static_assert(rotate(Complex{3, 4}, x) == Vec2{3, 4});
    static_assert(rotate(Complex{0, 1}, rotate(Complex{0, 1}, x)) == Vec2{-1, 0});
    static_assert(rotate(Complex{0, 1} * Complex{0, 1}, x) == Vec2{-1, 0});
    static_assert(rotate(Complex{1}, Vec2{2, 3}) == Vec2{2, 3});

    ASSERT((rotate(Complex{std::sqrt(0.5f), std::sqrt(0.5f)}, x) == Vec2{std::sqrt(0.5f), std::sqrt(0.5f)}));

    Vec2 roundtrip = rotate(normalize(conjugate(Complex{3, 4})), rotate(normalize(Complex{3, 4}), x));
    ASSERT((std::abs(roundtrip.x() - 1.0f) < 1e-5f));
    ASSERT((std::abs(roundtrip.y()) < 1e-5f));
}

TEST(testQuatConstruction)
{
    static_assert(std::is_convertible_v<f32, Quat>);
    static_assert(!std::is_convertible_v<Vec4, Quat>);
    static_assert(!std::is_convertible_v<Quat, Vec3>);
    static_assert(!std::is_convertible_v<Quat, Vec4>);

    constexpr Quat q{1, 2, 3, 4};
    static_assert(q.r == 1 && q.i == 2 && q.j == 3 && q.k == 4);
    constexpr Quat r{5};
    static_assert(r.r == 5 && r.i == 0 && r.j == 0 && r.k == 0);
    constexpr Quat v{Quat{5, Vec3{1, 2, 3}}};
    static_assert(v.r == 5 && v.i == 1 && v.j == 2 && v.k == 3);
    constexpr Quat w{Quat{Vec4{5, 1, 2, 3}}};
    static_assert(w.r == 5 && w.i == 1 && w.j == 2 && w.k == 3);

    static_assert(static_cast<Vec3>(Quat{1, 2, 3, 4}) == Vec3{2, 3, 4});
    static_assert(static_cast<Vec4>(Quat{1, 2, 3, 4}) == Vec4{1, 2, 3, 4});

    static_assert(Quat{1, 2, 3, 4} == Quat{1, 2, 3, 4});
    static_assert(!(Quat{1, 2, 3, 4} == Quat{1, 2, 3, 5}));
    static_assert(!(Quat{1, 2, 3, 4} == Quat{5, 2, 3, 4}));
    static_assert(!(Quat{1, 2, 3, 4} == Quat{1, 5, 3, 4}));
    static_assert(!(Quat{1, 2, 3, 4} == Quat{1, 2, 5, 4}));
    static_assert(Quat{1, 2, 3, 4} != Quat{1, 2, 3, 5});
    static_assert(Quat{1, 2, 3, 4} != Quat{5, 2, 3, 4});
    static_assert(Quat{1, 2, 3, 4} != Quat{1, 5, 3, 4});
    static_assert(Quat{1, 2, 3, 4} != Quat{1, 2, 5, 4});
    static_assert(!(Quat{1, 2, 3, 4} != Quat{1, 2, 3, 4}));

    static_assert([] {
        Quat imp = 2.0f;
        bool ok = imp == Quat{2, 0, 0, 0};
        Quat d;
        d.r = 1.0f;
        d.i = 2.0f;
        d.j = 3.0f;
        d.k = 4.0f;
        ok = ok && d == Quat{1, 2, 3, 4};
        Quat a{1, 2, 3, 4};
        Quat b = a;
        b.k = 9.0f;
        ok = ok && a.k == 4.0f && b == Quat{1, 2, 3, 9};
        return ok;
    }());
}

TEST(testQuatArithmetic)
{
    constexpr Quat a{1, 2, 3, 4};
    constexpr Quat b{5, 6, 7, 8};
    static_assert(a + b == Quat{6, 8, 10, 12});
    static_assert(a - b == Quat{-4, -4, -4, -4});
    static_assert(-a == Quat{-1, -2, -3, -4});
    static_assert(a + b == b + a);
    static_assert(a * b == Quat{-60, 12, 30, 24});
    static_assert(a * 2.0f == Quat{2, 4, 6, 8});
    static_assert(a / 2.0f == Quat{0.5f, 1.0f, 1.5f, 2.0f});
    static_assert(a / 1.0f == a);
    static_assert(2.0f * a == Quat{2, 4, 6, 8});
    static_assert(Quat{2} * a == Quat{2, 4, 6, 8});
    static_assert(a * Quat{2} == Quat{2, 4, 6, 8});
    static_assert(2.0f * a == a * 2.0f);
    static_assert(a * Quat{1} == a);
    static_assert(Quat{1} * a == a);
    static_assert(a + 1.0f == Quat{2, 2, 3, 4});
    static_assert(1.0f + a == Quat{2, 2, 3, 4});
    static_assert(a - 1.0f == Quat{0, 2, 3, 4});
    static_assert(1.0f - a == Quat{0, -2, -3, -4});
    static_assert(a + Quat{0} == a);
    static_assert(a - a == Quat{0});
}

TEST(testQuatMul)
{
    static_assert(Quat{1, 2, 3, 4} * Quat{5, 6, 7, 8} == Quat{-60, 12, 30, 24});
    static_assert(Quat{1} * Quat{1, 2, 3, 4} == Quat{1, 2, 3, 4});
    static_assert(Quat{1, 2, 3, 4} * Quat{1} == Quat{1, 2, 3, 4});
    static_assert(Quat{0, 1, 0, 0} * Quat{0, 0, 1, 0} == Quat{0, 0, 0, 1});
    static_assert(Quat{0, 0, 1, 0} * Quat{0, 0, 0, 1} == Quat{0, 1, 0, 0});
    static_assert(Quat{0, 0, 0, 1} * Quat{0, 1, 0, 0} == Quat{0, 0, 1, 0});
    static_assert(Quat{0, 0, 1, 0} * Quat{0, 1, 0, 0} == Quat{0, 0, 0, -1});
    static_assert(Quat{0, 0, 0, 1} * Quat{0, 0, 1, 0} == Quat{0, -1, 0, 0});
    static_assert(Quat{0, 1, 0, 0} * Quat{0, 0, 0, 1} == Quat{0, 0, -1, 0});
    static_assert(Quat{0, 1, 0, 0} * Quat{0, 1, 0, 0} == Quat{-1});
    static_assert(Quat{0, 0, 1, 0} * Quat{0, 0, 1, 0} == Quat{-1});
    static_assert(Quat{0, 0, 0, 1} * Quat{0, 0, 0, 1} == Quat{-1});
    static_assert(Quat{0, 1, 0, 0} * Quat{0, 1, 0, 0} * Quat{0, 1, 0, 0} * Quat{0, 1, 0, 0} == Quat{1});
    static_assert(Quat{0, 1, 0, 0} * Quat{0, 0, 1, 0} * Quat{0, 0, 0, 1} == Quat{-1});
    static_assert(Quat{0, 1, 0, 0} * Quat{0, 0, 1, 0} != Quat{0, 0, 1, 0} * Quat{0, 1, 0, 0});

    constexpr Quat a{1, 2, 3, 4};
    constexpr Quat b{5, 6, 7, 8};
    constexpr Quat c{9, 10, 11, 12};
    static_assert(a * b != b * a);
    static_assert((a * b) * c == a * (b * c));
    static_assert(a * (b * c) == Quat{-1278, -396, -294, -672});
}

TEST(testQuatInPlace)
{
    static_assert([] {
        Quat q{1, 2, 3, 4};
        q += Quat{1, 1, 1, 1};
        bool ok = q == Quat{2, 3, 4, 5};
        q -= Quat{0, 1, 2, 3};
        ok = ok && q == Quat{2, 2, 2, 2};
        q *= 2.0f;
        ok = ok && q == Quat{4, 4, 4, 4};
        q /= 4.0f;
        ok = ok && q == Quat{1, 1, 1, 1};
        q *= -1.0f;
        ok = ok && q == Quat{-1, -1, -1, -1};
        q = Quat{1, 1, 1, 1};
        ok = ok && q == Quat{1, 1, 1, 1};
        q += 1.0f;
        ok = ok && q == Quat{2, 1, 1, 1};
        q -= 1.0f;
        ok = ok && q == Quat{1, 1, 1, 1};
        q = Quat{6, 8, 10, 12};
        q /= 2.0f;
        ok = ok && q == Quat{3, 4, 5, 6};
        q *= 2.0f;
        ok = ok && q == Quat{6, 8, 10, 12};
        return ok;
    }());
}

TEST(testQuatConjAbsNorm)
{
    constexpr Quat q{1, 2, 3, 4};
    static_assert(conjugate(q) == Quat{1, -2, -3, -4});
    static_assert(conjugate(conjugate(q)) == q);
    static_assert(absSqr(q) == 30.0f);
    static_assert(absSqr(Quat{0, 0, 0, 0}) == 0.0f);
    static_assert(q + conjugate(q) == Quat{2, 0, 0, 0});
    static_assert(q * conjugate(q) == Quat{30, 0, 0, 0});
    static_assert(conjugate(q) * q == Quat{30, 0, 0, 0});
    static_assert(conjugate(Quat{5}) == Quat{5, 0, 0, 0});

    ASSERT((std::abs(abs(q) - std::sqrt(30.0f)) < FLT_EPSILON));
    ASSERT((abs(Quat{0, 0, 0, 0}) == 0.0f));
    ASSERT(((abs(Quat{1, 0, 0, 0}) - 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(abs(Quat{3}) - 3.0f) < FLT_EPSILON));
    ASSERT((std::abs(abs(conjugate(q)) - abs(q)) < FLT_EPSILON));

    Quat n = normalize(q);
    ASSERT((std::abs(abs(n) - 1.0f) < FLT_EPSILON));
    ASSERT((normalize(Quat{3}) == Quat{1, 0, 0, 0}));
}

TEST(testQuatRotateVec)
{
    constexpr Vec3 rIdent = rotate(Quat{1}, Vec3{1, 2, 3});
    static_assert(rIdent == Vec3{1, 2, 3});
    static_assert(rotate(Quat{1, 2, 3, 4}, Vec3{0, 0, 0}) == Vec3{0, 0, 0});
    constexpr Vec3 rX180 = rotate(Quat{0, 1, 0, 0}, Vec3{1, 0, 0});
    static_assert(rX180 == Vec3{1, 0, 0});
    constexpr Vec3 rY180 = rotate(Quat{0, 0, 1, 0}, Vec3{1, 0, 0});
    static_assert(rY180 == Vec3{-1, 0, 0});
    constexpr Vec3 rZ180 = rotate(Quat{0, 0, 0, 1}, Vec3{0, 0, 1});
    static_assert(rZ180 == Vec3{0, 0, 1});
    constexpr Vec3 rZ180x = rotate(Quat{0, 0, 0, 1}, Vec3{1, 0, 0});
    static_assert(rZ180x == Vec3{-1, 0, 0});

    ASSERT((rotate(Quat{1}, Vec3{1, 2, 3}) == Vec3{1, 2, 3}));

    Quat q1 = quatAxisAngle(Vec3{0, 0, 1}, pif / 3.0f);
    Quat q2 = quatAxisAngle(Vec3{1, 0, 0}, pif / 4.0f);
    Vec3 v{1, 2, 3};
    Vec3 sequential = rotate(q2, rotate(q1, v));
    Vec3 combined = rotate(q2 * q1, v);
    ASSERT((std::abs(sequential.x() - combined.x()) < 1e-4f));
    ASSERT((std::abs(sequential.y() - combined.y()) < 1e-4f));
    ASSERT((std::abs(sequential.z() - combined.z()) < 1e-4f));

    Quat q90 = quatAxisAngle(Vec3{0, 0, 1}, pif / 2.0f);
    Vec3 twice = rotate(q90, rotate(q90, Vec3{1, 0, 0}));
    ASSERT((std::abs(twice.x() + 1.0f) < 1e-5f));
    ASSERT((std::abs(twice.y()) < 1e-5f));
    Vec3 four = rotate(q90, twice);
    ASSERT((std::abs(four.x()) < 1e-5f));
    ASSERT((std::abs(four.y() + 1.0f) < 1e-5f));

    Vec3 rotated = rotate(quatAxisAngle(Vec3{0, 1, 0}, pif / 2.0f), Vec3{1, 2, 3});
    ASSERT((std::abs(rotated.x() - 3.0f) < 1e-5f));
    ASSERT((std::abs(rotated.y() - 2.0f) < 1e-5f));
    ASSERT((std::abs(rotated.z() + 1.0f) < 1e-5f));

    Quat n = normalize(Quat{1, 2, 3, 4});
    Vec3 u = rotate(n, v);
    Vec3 roundtrip = rotate(normalize(conjugate(n)), u);
    ASSERT((std::abs(roundtrip.x() - v.x()) < 1e-5f));
    ASSERT((std::abs(roundtrip.y() - v.y()) < 1e-5f));
    ASSERT((std::abs(roundtrip.z() - v.z()) < 1e-5f));
    ASSERT((std::abs(length(u) - length(v)) < 1e-5f));
}

TEST(testQuatRotateMat)
{
    static_assert(rotate(Quat{1}, Mat3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}}) == Mat3{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}});

    Quat q = quatAxisAngle(Vec3{0, 0, 1}, pif / 3.0f);
    Mat3 arbitrary{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
    Vec3 v{1, 2, 3};

    Mat3 r = rotate(q, Mat3{1.0f});
    Vec3 rv = rotate(q, v);
    Vec3 rm = r * v;
    ASSERT((std::abs(rv.x() - rm.x()) < 1e-5f));
    ASSERT((std::abs(rv.y() - rm.y()) < 1e-5f));
    ASSERT((std::abs(rv.z() - rm.z()) < 1e-5f));
    ASSERT((std::abs(dot(r.x(), r.y())) < 1e-5f));
    ASSERT((std::abs(dot(r.x(), r.z())) < 1e-5f));
    ASSERT((std::abs(dot(r.y(), r.z())) < 1e-5f));
    ASSERT((std::abs(length(r.x()) - 1.0f) < 1e-5f));
    ASSERT((std::abs(length(r.z()) - 1.0f) < 1e-5f));

    Mat3 rot = rotate(q, arbitrary);
    Vec3 rv2 = rotate(q, arbitrary * v);
    Vec3 rm2 = rot * v;
    ASSERT((std::abs(rv2.x() - rm2.x()) < 1e-5f));
    ASSERT((std::abs(rv2.y() - rm2.y()) < 1e-5f));
    ASSERT((std::abs(rv2.z() - rm2.z()) < 1e-5f));

    Quat rq = normalize(q) * normalize(q);
    Mat3 mq = rotate(rq, arbitrary);
    Mat3 msq = rotate(normalize(q * q), arbitrary);
    ASSERT((std::abs(mq[0].x() - msq[0].x()) < 1e-5f));
    ASSERT((std::abs(mq[1].y() - msq[1].y()) < 1e-5f));
    ASSERT((std::abs(mq[2].z() - msq[2].z()) < 1e-5f));
}

TEST(testQuatAxisAngle)
{
    Quat id = quatAxisAngle(Vec3{0, 0, 1}, 0.0f);
    ASSERT((std::abs(id.r - 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(abs(id) - 1.0f) < FLT_EPSILON));

    Quat q90 = quatAxisAngle(Vec3{0, 0, 1}, pif / 2.0f);
    Vec3 rx = rotate(q90, Vec3{1, 0, 0});
    ASSERT((std::abs(rx.x()) < FLT_EPSILON));
    ASSERT((std::abs(rx.y() - 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(rx.z()) < FLT_EPSILON));
    Vec3 ry = rotate(q90, Vec3{0, 1, 0});
    ASSERT((std::abs(ry.x() + 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(ry.y()) < FLT_EPSILON));
    ASSERT((std::abs(ry.z()) < FLT_EPSILON));
    Vec3 rz = rotate(q90, Vec3{0, 0, 1});
    ASSERT((std::abs(rz.x()) < FLT_EPSILON));
    ASSERT((std::abs(rz.y()) < FLT_EPSILON));
    ASSERT((std::abs(rz.z() - 1.0f) < FLT_EPSILON));

    Quat q180 = quatAxisAngle(Vec3{1, 0, 0}, pif);
    Vec3 r180z = rotate(q180, Vec3{0, 0, 1});
    ASSERT((std::abs(r180z.x()) < FLT_EPSILON));
    ASSERT((std::abs(r180z.y()) < FLT_EPSILON));
    ASSERT((std::abs(r180z.z() + 1.0f) < FLT_EPSILON));
    Vec3 r180x = rotate(q180, Vec3{1, 0, 0});
    ASSERT((std::abs(r180x.x() - 1.0f) < FLT_EPSILON));

    Quat qm90 = quatAxisAngle(Vec3{0, 0, 1}, -pif / 2.0f);
    Vec3 rmx = rotate(qm90, Vec3{1, 0, 0});
    ASSERT((std::abs(rmx.x()) < FLT_EPSILON));
    ASSERT((std::abs(rmx.y() + 1.0f) < FLT_EPSILON));

    Quat qy90 = quatAxisAngle(Vec3{0, 1, 0}, pif / 2.0f);
    Vec3 ryz = rotate(qy90, Vec3{0, 0, 1});
    ASSERT((std::abs(ryz.x() - 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(ryz.y()) < FLT_EPSILON));
    ASSERT((std::abs(ryz.z()) < FLT_EPSILON));

    Vec3 axis = normalize(Vec3{1.0f, 1.0f, 1.0f});
    Quat qa = quatAxisAngle(axis, pif / 3.0f);
    Vec3 ra = rotate(qa, axis);
    ASSERT((std::abs(ra.x() - axis.x()) < FLT_EPSILON));
    ASSERT((std::abs(ra.y() - axis.y()) < FLT_EPSILON));
    ASSERT((std::abs(ra.z() - axis.z()) < FLT_EPSILON));
    ASSERT((std::abs(abs(qa) - 1.0f) < 1e-5f));
}

TEST(testQuatBetween)
{
    Quat id = quatBetween(Vec3{1, 0, 0}, Vec3{1, 0, 0});
    ASSERT((std::abs(abs(id) - 1.0f) < FLT_EPSILON));
    Vec3 rs = rotate(id, Vec3{1, 0, 0});
    ASSERT((std::abs(rs.x() - 1.0f) < 1e-5f));
    ASSERT((std::abs(rs.y()) < 1e-5f));
    ASSERT((std::abs(rs.z()) < 1e-5f));

    Quat q = quatBetween(Vec3{1, 0, 0}, Vec3{0, 1, 0});
    Vec3 r = rotate(q, Vec3{1, 0, 0});
    ASSERT((std::abs(r.x()) < 1e-5f));
    ASSERT((std::abs(r.y() - 1.0f) < 1e-5f));
    ASSERT((std::abs(r.z()) < 1e-5f));

    Quat qz = quatBetween(Vec3{0, 0, 1}, Vec3{0, 0, 1});
    Vec3 rz = rotate(qz, Vec3{0, 0, 1});
    ASSERT((std::abs(rz.z() - 1.0f) < 1e-5f));
    ASSERT((std::abs(abs(qz) - 1.0f) < 1e-5f));

    Quat qd = quatBetween(Vec3{1, 1, 0}, Vec3{0, 0, 1});
    Vec3 rd = rotate(qd, normalize(Vec3{1.0f, 1.0f, 0.0f}));
    ASSERT((std::abs(rd.x()) < 1e-5f));
    ASSERT((std::abs(rd.y()) < 1e-5f));
    ASSERT((std::abs(rd.z() - 1.0f) < 1e-5f));

    Vec3 to = normalize(Vec3{1.0f, 0.1f, 0.0f});
    Quat qSmall = quatBetween(Vec3{1, 0, 0}, to);
    Vec3 rSmall = rotate(qSmall, Vec3{1, 0, 0});
    ASSERT((std::abs(rSmall.x() - to.x()) < 1e-4f));
    ASSERT((std::abs(rSmall.y() - to.y()) < 1e-4f));
    ASSERT((std::abs(rSmall.z()) < 1e-4f));
    ASSERT((std::abs(abs(qSmall) - 1.0f) < 1e-5f));
}

TEST(testQuatBetweenAntiparallel)
{
    Quat q = quatBetween(Vec3{1, 0, 0}, Vec3{-1, 0, 0});
    ASSERT((std::abs(abs(q) - 1.0f) < FLT_EPSILON));
    Vec3 r = rotate(q, Vec3{1, 0, 0});
    ASSERT((std::abs(r.x() + 1.0f) < 1e-5f));
    ASSERT((std::abs(r.y()) < 1e-5f));
    ASSERT((std::abs(r.z()) < 1e-5f));

    Quat qz = quatBetween(Vec3{0, 0, 1}, Vec3{0, 0, -1});
    ASSERT((std::abs(abs(qz) - 1.0f) < FLT_EPSILON));
    Vec3 rz = rotate(qz, Vec3{0, 0, 1});
    ASSERT((std::abs(rz.x()) < 1e-5f));
    ASSERT((std::abs(rz.y()) < 1e-5f));
    ASSERT((std::abs(rz.z() + 1.0f) < 1e-5f));

    Quat qd = quatBetween(Vec3{0.7f, 0.7f, 0}, Vec3{-0.7f, -0.7f, 0});
    ASSERT((std::abs(abs(qd) - 1.0f) < FLT_EPSILON));
    Vec3 rd = rotate(qd, Vec3{0.7f, 0.7f, 0});
    ASSERT((std::abs(rd.x() + 0.7f) < 1e-5f));
    ASSERT((std::abs(rd.y() + 0.7f) < 1e-5f));
    ASSERT((std::abs(rd.z()) < 1e-5f));

    Quat qy = quatBetween(Vec3{0, 1, 0}, Vec3{0, -1, 0});
    Vec3 ry = rotate(qy, Vec3{0, 1, 0});
    ASSERT((std::abs(ry.x()) < 1e-5f));
    ASSERT((std::abs(ry.y() + 1.0f) < 1e-5f));
}

TEST(testMatModel2D)
{
    Mat4 m0 = matModel2D(Vec3{0, 0, 0}, Vec2{1, 1}, 0.0f);
    ASSERT((m0[0] == Vec4{1, 0, 0, 0}));
    ASSERT((m0[1] == Vec4{0, 1, 0, 0}));
    ASSERT((m0[2] == Vec4{0, 0, 1, 0}));
    ASSERT((m0[3] == Vec4{0, 0, 0, 1}));

    Mat4 mt = matModel2D(Vec3{10, 20, 5}, Vec2{1, 1}, 0.0f);
    ASSERT((mt[3] == Vec4{10, 20, 5, 1}));
    ASSERT((mt[0] == Vec4{1, 0, 0, 0}));

    Mat4 ms = matModel2D(Vec3{0, 0, 0}, Vec2{2, 3}, 0.0f);
    Vec4 scaled = ms * Vec4{1, 1, 0, 1};
    ASSERT((std::abs(scaled.x() - 2.0f) < FLT_EPSILON));
    ASSERT((std::abs(scaled.y() - 3.0f) < FLT_EPSILON));
    ASSERT((std::abs(scaled.w() - 1.0f) < FLT_EPSILON));

    Mat4 mr = matModel2D(Vec3{0, 0, 0}, Vec2{1, 1}, pif / 2.0f);
    Vec4 rotated = mr * Vec4{1, 0, 0, 1};
    ASSERT((std::abs(rotated.x()) < 1e-5f));
    ASSERT((std::abs(rotated.y() - 1.0f) < 1e-5f));
    Vec4 rotatedY = mr * Vec4{0, 1, 0, 1};
    ASSERT((std::abs(rotatedY.x() + 1.0f) < 1e-5f));
    ASSERT((std::abs(rotatedY.y()) < 1e-5f));
    Vec4 rotatedNeg = mr * Vec4{-1, 0, 0, 1};
    ASSERT((std::abs(rotatedNeg.x()) < 1e-5f));
    ASSERT((std::abs(rotatedNeg.y() + 1.0f) < 1e-5f));

    Mat4 mc = matModel2D(Vec3{10, 20, 0}, Vec2{2, 2}, pif / 2.0f);
    Vec4 composed = mc * Vec4{1, 0, 0, 1};
    ASSERT((std::abs(composed.x() - 10.0f) < 1e-5f));
    ASSERT((std::abs(composed.y() - 22.0f) < 1e-5f));

    Mat4 m90 = matModel2D(Vec3{0, 0, 0}, Vec2{1, 1}, pif);
    Vec4 turned = m90 * Vec4{1, 0, 0, 1};
    ASSERT((std::abs(turned.x() + 1.0f) < 1e-5f));
    ASSERT((std::abs(turned.y()) < 1e-5f));
}

TEST(testMatModel3D)
{
    Mat4 m0 = matModel3D(Vec3{0, 0, 0}, Vec3{1, 1, 1}, Quat{1});
    ASSERT((m0[0] == Vec4{1, 0, 0, 0}));
    ASSERT((m0[1] == Vec4{0, 1, 0, 0}));
    ASSERT((m0[2] == Vec4{0, 0, 1, 0}));
    ASSERT((m0[3] == Vec4{0, 0, 0, 1}));

    Mat4 mt = matModel3D(Vec3{10, 20, 30}, Vec3{1, 1, 1}, Quat{1});
    ASSERT((mt[3] == Vec4{10, 20, 30, 1}));

    Mat4 ms = matModel3D(Vec3{0, 0, 0}, Vec3{2, 3, 4}, Quat{1});
    ASSERT((ms * Vec4{1, 0, 0, 1} == Vec4{2, 0, 0, 1}));
    ASSERT((ms * Vec4{0, 1, 0, 1} == Vec4{0, 3, 0, 1}));
    ASSERT((ms * Vec4{0, 0, 1, 1} == Vec4{0, 0, 4, 1}));

    Quat qz = quatAxisAngle(Vec3{0, 0, 1}, pif / 2.0f);
    Mat4 mr = matModel3D(Vec3{0, 0, 0}, Vec3{1, 1, 1}, qz);
    Vec4 rx = mr * Vec4{1, 0, 0, 1};
    ASSERT((std::abs(rx.x()) < 1e-5f));
    ASSERT((std::abs(rx.y() - 1.0f) < 1e-5f));
    ASSERT((std::abs(rx.z()) < 1e-5f));

    Quat qy = quatAxisAngle(Vec3{0, 1, 0}, pif / 2.0f);
    Mat4 mry = matModel3D(Vec3{0, 0, 0}, Vec3{1, 1, 1}, qy);
    Vec4 ryz = mry * Vec4{0, 0, 1, 1};
    ASSERT((std::abs(ryz.x() - 1.0f) < 1e-5f));
    ASSERT((std::abs(ryz.y()) < 1e-5f));
    ASSERT((std::abs(ryz.z()) < 1e-5f));

    Mat4 mc = matModel3D(Vec3{1, 2, 3}, Vec3{2, 2, 2}, qz);
    Vec4 rc = mc * Vec4{1, 0, 0, 1};
    ASSERT((std::abs(rc.x() - 1.0f) < 1e-5f));
    ASSERT((std::abs(rc.y() - 4.0f) < 1e-5f));
    ASSERT((std::abs(rc.z() - 3.0f) < 1e-5f));

    Mat4 zero = matModel3D(Vec3{1, 2, 3}, Vec3{0, 0, 0}, qz);
    ASSERT((zero[0] == Vec4{0, 0, 0, 0}));
    ASSERT((zero[1] == Vec4{0, 0, 0, 0}));
    ASSERT((zero[2] == Vec4{0, 0, 0, 0}));
    ASSERT((zero[3] == Vec4{1, 2, 3, 1}));
}

TEST(testMatModelToView)
{
    Mat4 model = matModel3D(Vec3{3, 4, 5}, Vec3{1, 1, 1}, Quat{1});
    Mat4 view = matModelToView(model);
    Vec4 origin = view * Vec4{3, 4, 5, 1};
    ASSERT((std::abs(origin.x()) < FLT_EPSILON));
    ASSERT((std::abs(origin.y()) < FLT_EPSILON));
    ASSERT((std::abs(origin.z()) < FLT_EPSILON));
    ASSERT((std::abs(origin.w() - 1.0f) < FLT_EPSILON));
    ASSERT((view * model * Vec4{1, 2, 3, 1} == Vec4{1, 2, 3, 1}));

    Quat q = quatAxisAngle(Vec3{0, 0, 1}, pif / 2.0f);
    Mat4 rotated = matModel3D(Vec3{3, 4, 5}, Vec3{1, 1, 1}, q);
    Mat4 rview = matModelToView(rotated);
    Vec4 roundtripQ = rview * rotated * Vec4{1, 2, 3, 1};
    ASSERT((std::abs(roundtripQ.x() - 1.0f) < 1e-5f));
    ASSERT((std::abs(roundtripQ.y() - 2.0f) < 1e-5f));
    ASSERT((std::abs(roundtripQ.z() - 3.0f) < 1e-5f));
    ASSERT((std::abs(roundtripQ.w() - 1.0f) < 1e-5f));
    Vec4 world = rview * Vec4{3, 5, 10, 1};
    ASSERT((std::abs(world.x() - 1.0f) < 1e-5f));
    ASSERT((std::abs(world.y()) < 1e-5f));
    ASSERT((std::abs(world.z() - 5.0f) < 1e-5f));

    Mat4 scaled = matModel3D(Vec3{3, 4, 5}, Vec3{2, 3, 4}, Quat{1});
    Mat4 sview = matModelToView(scaled);
    Mat4 roundtrip = sview * scaled;
    ASSERT((roundtrip[0] == Vec4{2, 0, 0, 0}));
    ASSERT((roundtrip[1] == Vec4{0, 3, 0, 0}));
    ASSERT((roundtrip[2] == Vec4{0, 0, 4, 0}));
    ASSERT((roundtrip[3] == Vec4{0, 0, 0, 1}));

    Mat4 zero = matModel3D(Vec3{1, 2, 3}, Vec3{0, 0, 0}, Quat{1});
    Mat4 zview = matModelToView(zero);
    ASSERT((zview[0] == Vec4{0, 0, 0, 0}));
    ASSERT((zview[2] == Vec4{0, 0, 0, 0}));
    ASSERT((zview[3] == Vec4{0, 0, 0, 1}));
}

TEST(testMatView)
{
    Quat id{1};
    Mat4 view = matView(Vec3{0, 0, 0}, Vec3{1.0f}, id);
    Vec4 v = view * Vec4{1, 2, 3, 1};
    ASSERT((v == Vec4{1, 2, 3, 1}));
    ASSERT((view * Vec4{0, 0, 0, 1} == Vec4{0, 0, 0, 1}));

    Vec3 p{3, 4, 5};
    Mat4 translated = matView(p, Vec3{1.0f}, id);
    Vec4 t = translated * Vec4{0, 0, 0, 1};
    ASSERT((std::abs(t.x() + 3.0f) < FLT_EPSILON));
    ASSERT((std::abs(t.y() + 4.0f) < FLT_EPSILON));
    ASSERT((std::abs(t.z() + 5.0f) < FLT_EPSILON));
    ASSERT((std::abs(t.w() - 1.0f) < FLT_EPSILON));
    ASSERT((translated * Vec4{3, 4, 5, 1} == Vec4{0, 0, 0, 1}));

    Quat qz = quatAxisAngle(Vec3{0, 0, 1}, pif / 2.0f);
    Mat4 rotated = matView(Vec3{0, 0, 0}, Vec3{1.0f}, qz);
    Vec4 r = rotated * Vec4{1, 0, 5, 1};
    ASSERT((std::abs(r.x()) < 1e-5f));
    ASSERT((std::abs(r.y() + 1.0f) < 1e-5f));
    ASSERT((std::abs(r.z() - 5.0f) < 1e-5f));

    Mat4 both = matView(Vec3{3, 4, 5}, Vec3{1.0f}, qz);
    Vec4 rb = both * Vec4{4, 3, 8, 1};
    ASSERT((std::abs(rb.x() + 1.0f) < 1e-5f));
    ASSERT((std::abs(rb.y() + 1.0f) < 1e-5f));
    ASSERT((std::abs(rb.z() - 3.0f) < 1e-5f));
}

TEST(testMatViewZoom)
{
    Quat id{1};
    Vec3 p{3, 4, 5};

    Mat4 view = matView(p, Vec3{1.0f}, id);
    Vec4 t = view * Vec4{0, 0, 0, 1};
    ASSERT((std::abs(t.x() + 3.0f) < FLT_EPSILON));
    ASSERT((std::abs(t.y() + 4.0f) < FLT_EPSILON));
    ASSERT((std::abs(t.z() + 5.0f) < FLT_EPSILON));
    ASSERT((std::abs(t.w() - 1.0f) < FLT_EPSILON));

    Mat4 zoomed = matView(p, Vec3{2.0f, 1.0f, 1.0f}, id);
    Vec4 z = zoomed * Vec4{4, 3, 2, 1};
    ASSERT((std::abs(z.x() - (2.0f * (4.0f - 3.0f))) < FLT_EPSILON));
    ASSERT((std::abs(z.y() - (1.0f * (3.0f - 4.0f))) < FLT_EPSILON));
    ASSERT((std::abs(z.z() - (1.0f * (2.0f - 5.0f))) < FLT_EPSILON));

    Mat4 rotated = matView(p, Vec3{2.0f, 1.0f, 1.0f}, quatAxisAngle(Vec3{0, 0, 1}, pif / 2.0f));
    Vec4 rz = rotated * Vec4{4, 3, 2, 1};
    ASSERT((std::abs(rz.x() + 2.0f) < 1e-5f));
    ASSERT((std::abs(rz.y() + 1.0f) < 1e-5f));
    ASSERT((std::abs(rz.z() + 3.0f) < 1e-5f));
}

TEST(testMatOrthographic)
{
    Mat4 o = matOrthographic(-1, 1, 1, -1, 0, 100);

    Vec4 center = o * Vec4{0, 0, 0, 1};
    ASSERT((std::abs(center.x()) < FLT_EPSILON));
    ASSERT((std::abs(center.y()) < FLT_EPSILON));
    ASSERT((std::abs(center.z()) < FLT_EPSILON));
    ASSERT((std::abs(center.w() - 1.0f) < FLT_EPSILON));

    Vec4 l = o * Vec4{-1, 0, 0, 1};
    Vec4 r = o * Vec4{1, 0, 0, 1};
    ASSERT((std::abs(l.x() + 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(r.x() - 1.0f) < FLT_EPSILON));

    Vec4 t = o * Vec4{0, 1, 0, 1};
    Vec4 b = o * Vec4{0, -1, 0, 1};
    ASSERT((std::abs(t.y() + 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(b.y() - 1.0f) < FLT_EPSILON));

    Vec4 n = o * Vec4{0, 0, 0, 1};
    Vec4 f = o * Vec4{0, 0, 100, 1};
    ASSERT((std::abs(n.z()) < FLT_EPSILON));
    ASSERT((std::abs(f.z() - 1.0f) < FLT_EPSILON));
    Vec4 mid = o * Vec4{0, 0, 50, 1};
    ASSERT((std::abs(mid.z() - 0.5f) < FLT_EPSILON));

    Mat4 big = matOrthographic(-10, 10, 10, -10, -5, 5);
    Vec4 bl = big * Vec4{-10, -10, -5, 1};
    Vec4 tr = big * Vec4{10, 10, 5, 1};
    ASSERT((std::abs(bl.x() + 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(bl.y() - 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(bl.z()) < FLT_EPSILON));
    ASSERT((std::abs(tr.x() - 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(tr.y() + 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(tr.z() - 1.0f) < FLT_EPSILON));

    Mat4 wide = matOrthographic(-2, 2, 1, -1, 0, 10);
    Vec4 wl = wide * Vec4{-2, 0, 5, 1};
    Vec4 wr = wide * Vec4{2, 0, 5, 1};
    ASSERT((std::abs(wl.x() + 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(wr.x() - 1.0f) < FLT_EPSILON));
    Vec4 wt = wide * Vec4{0, 1, 5, 1};
    Vec4 wb = wide * Vec4{0, -1, 5, 1};
    ASSERT((std::abs(wt.y() + 1.0f) < FLT_EPSILON));
    ASSERT((std::abs(wb.y() - 1.0f) < FLT_EPSILON));
}

TEST(testMatPerspective)
{
    Mat4 p = matPerspective(pif / 2.0f, 1.0f, 0.1f, 100.0f);

    Vec4 n = p * Vec4{0, 0, 0.1f, 1};
    ASSERT((std::abs(n.z()) < 1e-4f));
    Vec4 f = p * Vec4{0, 0, 100.0f, 1};
    ASSERT((std::abs(f.z() / f.w() - 1.0f) < 1e-3f));

    Vec4 center = p * Vec4{0, 0, 1, 1};
    ASSERT((std::abs(center.x()) < FLT_EPSILON));
    ASSERT((std::abs(center.y()) < FLT_EPSILON));
    ASSERT((center.w() > 0.0f));

    Vec4 right = p * Vec4{1, 0, 1, 1};
    ASSERT((right.x() > 0.0f));
    Vec4 left = p * Vec4{-1, 0, 1, 1};
    ASSERT((std::abs(right.x() + left.x()) < 1e-5f));
    Vec4 up = p * Vec4{0, 1, 1, 1};
    ASSERT((up.y() < 0.0f));
    Vec4 down = p * Vec4{0, -1, 1, 1};
    ASSERT((std::abs(up.y() + down.y()) < 1e-5f));

    Mat4 wide = matPerspective(pif / 2.0f, 2.0f, 0.1f, 100.0f);
    Vec4 wx = wide * Vec4{1, 0, 1, 1};
    Vec4 wxc = p * Vec4{1, 0, 1, 1};
    ASSERT((std::abs(wx.x() - 0.5f * wxc.x()) < 1e-5f));
    Vec4 wy = wide * Vec4{0, 1, 1, 1};
    Vec4 wyc = p * Vec4{0, 1, 1, 1};
    ASSERT((std::abs(wy.y() - wyc.y()) < 1e-5f));
}

TEST(testProjectionOrientation)
{
    Mat4 view = matView(Vec3{0, 0, 0}, Vec3{1.0f}, Quat{1});
    Mat4 persp = matPerspective(pif / 2.0f, 1.0f, 0.1f, 100.0f);

    Vec4 up = persp * view * Vec4{0, 1, 5, 1};
    ASSERT((up.w() > 0.0f));
    ASSERT((up.x() < FLT_EPSILON));
    ASSERT((up.y() < 0.0f));
    ASSERT((std::abs(up.z() / up.w()) < 1.0f));

    Vec4 nz = persp * view * Vec4{0, 0, 0.1f, 1};
    Vec4 fz = persp * view * Vec4{0, 0, 100.0f, 1};
    ASSERT((std::abs(nz.z()) < 1e-4f));
    ASSERT((std::abs(fz.z() / fz.w() - 1.0f) < 1e-3f));

    Mat4 ortho = matOrthographic(-1, 1, 1, -1, 0, 100);
    Vec4 oUp = ortho * view * Vec4{0, 0.5, 1, 1};
    ASSERT((oUp.y() < 0.0f));

    Vec4 onz = ortho * view * Vec4{0, 0, 0, 1};
    Vec4 ofz = ortho * view * Vec4{0, 0, 100, 1};
    ASSERT((std::abs(onz.z()) < 1e-5f));
    ASSERT((std::abs(ofz.z() - 1.0f) < 1e-5f));
}

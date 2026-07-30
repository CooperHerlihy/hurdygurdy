#include "tests.hpp"
#include "hg/math.hpp"

void testMath()
{
    // ============================================================================
    // Math
    // ============================================================================
    //
    // Scalar functions, Vec2/3/4, Mat2/3/4, Complex, Quat, matrix construction.

    // ------------------------------------------------------------------
    // Scalar functions
    // ------------------------------------------------------------------

    {
        // pow
        {
            TEST(pow(2.0f, 0) == 1.0f);
            TEST(pow(2.0f, 1) == 2.0f);
            TEST(pow(3.0f, 2) == 9.0f);
            TEST(pow(0.0f, 5) == 0.0f);
            TEST(pow(1.0f, 100) == 1.0f);
            TEST(pow(-2.0f, 3) == -8.0f);
            TEST(pow(-2.0f, 2) == 4.0f);
        }

        // square
        {
            TEST(square(0.0f) == 0.0f);
            TEST(square(1.0f) == 1.0f);
            TEST(square(-1.0f) == 1.0f);
            TEST(square(2.5f) == 6.25f);
        }

        // lerp
        {
            TEST(lerp(0.0f, 10.0f, 0.0f) == 0.0f);
            TEST(lerp(0.0f, 10.0f, 1.0f) == 10.0f);
            TEST(lerp(0.0f, 10.0f, 0.5f) == 5.0f);
            TEST(lerp(5.0f, 5.0f, 0.3f) == 5.0f);
        }

        // smooth / smoothQuintic
        {
            TEST(smooth(0.0f) == 0.0f);
            TEST(smooth(1.0f) == 1.0f);
            TEST(std::abs(smooth(0.5f) - 0.5f) < FLT_EPSILON);
            TEST(smoothQuintic(0.0f) == 0.0f);
            TEST(smoothQuintic(1.0f) == 1.0f);
            TEST(std::abs(smoothQuintic(0.5f) - 0.5f) < FLT_EPSILON);
        }
    }

    // ------------------------------------------------------------------
    // Vec2
    // ------------------------------------------------------------------

    {
        // Construction and operator[]
        {
            Vec2 v{1.0f, 2.0f};
            TEST(v.x == 1.0f && v.y == 2.0f);
            Vec2 s{5.0f};
            TEST(s.x == 5.0f && s.y == 5.0f);
            TEST(v[0] == 1.0f && v[1] == 2.0f);
        }

        // Negation
        {
            Vec2 n = -Vec2{1.0f, -2.0f};
            TEST(n.x == -1.0f && n.y == 2.0f);
        }

        // Arithmetic
        {
            Vec2 a{1.0f, 2.0f};
            Vec2 b{3.0f, 4.0f};
            TEST((a + b == Vec2{4.0f, 6.0f}));
            TEST((b - a == Vec2{2.0f, 2.0f}));
            TEST((a * b == Vec2{3.0f, 8.0f}));
            TEST((b / a == Vec2{3.0f, 2.0f}));
        }

        // Scalar multiply / divide
        {
            Vec2 v{2.0f, 3.0f};
            TEST((5.0f * v == Vec2{10.0f, 15.0f}));
            TEST((v * 5.0f == Vec2{10.0f, 15.0f}));
            TEST((v / 2.0f == Vec2{1.0f, 1.5f}));
        }

        // In-place
        {
            Vec2 v{1.0f, 2.0f};
            v += Vec2{3.0f, 4.0f};
            TEST(v.x == 4.0f && v.y == 6.0f);
            v -= Vec2{1.0f, 1.0f};
            TEST(v.x == 3.0f && v.y == 5.0f);
            v *= Vec2{2.0f, 3.0f};
            TEST(v.x == 6.0f && v.y == 15.0f);
            v /= Vec2{3.0f, 5.0f};
            TEST(v.x == 2.0f && v.y == 3.0f);
        }

        // vecDot2
        {
            TEST(vecDot2({1.0f, 0.0f}, {0.0f, 1.0f}) == 0.0f);
            TEST(vecDot2({1.0f, 0.0f}, {1.0f, 0.0f}) == 1.0f);
            TEST(vecDot2({3.0f, 4.0f}, {5.0f, 6.0f}) == 39.0f);
        }

        // vecLenSqr2 / vecLen2
        {
            TEST(vecLenSqr2({0.0f, 0.0f}) == 0.0f);
            TEST(vecLenSqr2({1.0f, 0.0f}) == 1.0f);
            TEST(vecLenSqr2({3.0f, 4.0f}) == 25.0f);
            TEST(std::abs(vecLen2({3.0f, 4.0f}) - 5.0f) < FLT_EPSILON);
        }

        // vecNorm2
        {
            Vec2 n = vecNorm2({3.0f, 0.0f});
            TEST(n.x == 1.0f && n.y == 0.0f);
            TEST(std::abs(vecLen2(vecNorm2({3.0f, 4.0f})) - 1.0f) < FLT_EPSILON);
        }

        // vecCross2
        {
            TEST(vecCross2({1.0f, 0.0f}, {0.0f, 1.0f}) == 1.0f);
            TEST(vecCross2({0.0f, 1.0f}, {1.0f, 0.0f}) == -1.0f);
            TEST(std::abs(vecCross2({2.0f, 3.0f}, {4.0f, 6.0f})) < FLT_EPSILON);
        }

        // vecEq2
        {
            TEST(vecEq2({1.0f, 2.0f}, {1.0f, 2.0f}));
            TEST(!vecEq2({1.0f, 2.0f}, {1.0f, 3.0f}));
            TEST(vecEq2({1.0f + 1e-7f, 2.0f}, {1.0f, 2.0f}));
            TEST(!vecEq2({1.0f + 1e-5f, 2.0f}, {1.0f, 2.0f}));
        }
    }

    // ------------------------------------------------------------------
    // Vec3
    // ------------------------------------------------------------------

    {
        // Construction
        {
            Vec3 v{1.0f, 2.0f, 3.0f};
            TEST(v.x == 1.0f && v.y == 2.0f && v.z == 3.0f);
            Vec3 s{5.0f};
            TEST(s.x == 5.0f && s.y == 5.0f && s.z == 5.0f);
            Vec3 v2{Vec2{1.0f, 2.0f}, 3.0f};
            TEST(v2.x == 1.0f && v2.y == 2.0f && v2.z == 3.0f);
        }

        // Downcast
        {
            Vec2 v = static_cast<Vec2>(Vec3{1.0f, 2.0f, 3.0f});
            TEST(v.x == 1.0f && v.y == 2.0f);
        }

        // operator[]
        {
            Vec3 v{3.0f, 4.0f, 5.0f};
            TEST(v[0] == 3.0f && v[1] == 4.0f && v[2] == 5.0f);
        }

        // Negation and arithmetic
        {
            TEST((-Vec3{1.0f, -2.0f, 3.0f} == Vec3{-1.0f, 2.0f, -3.0f}));
            Vec3 a{1.0f, 2.0f, 3.0f};
            Vec3 b{4.0f, 5.0f, 6.0f};
            TEST((a + b == Vec3{5.0f, 7.0f, 9.0f}));
            TEST((b - a == Vec3{3.0f, 3.0f, 3.0f}));
            TEST((a * b == Vec3{4.0f, 10.0f, 18.0f}));
        }

        // Scalar ops
        {
            Vec3 v{1.0f, 2.0f, 3.0f};
            TEST((2.0f * v == Vec3{2.0f, 4.0f, 6.0f}));
            TEST((v * 2.0f == Vec3{2.0f, 4.0f, 6.0f}));
            TEST((v / 2.0f == Vec3{0.5f, 1.0f, 1.5f}));
        }

        // vecDot3 / vecLen / vecNorm3
        {
            TEST(vecDot3({1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}) == 0.0f);
            TEST(vecDot3({1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}) == 32.0f);
            TEST(vecLenSqr3({1.0f, 2.0f, 2.0f}) == 9.0f);
            TEST(std::abs(vecLen3({1.0f, 2.0f, 2.0f}) - 3.0f) < FLT_EPSILON);
            Vec3 n = vecNorm3({0.0f, -5.0f, 0.0f});
            TEST(n.x == 0.0f && n.y == -1.0f && n.z == 0.0f);
            TEST(std::abs(vecLen3(vecNorm3({1.0f, 2.0f, 3.0f})) - 1.0f) < FLT_EPSILON);
        }

        // vecCross3
        {
            Vec3 x{1.0f, 0.0f, 0.0f};
            Vec3 y{0.0f, 1.0f, 0.0f};
            Vec3 z{0.0f, 0.0f, 1.0f};
            TEST(vecCross3(x, y) == z);
            TEST(vecCross3(y, z) == x);
            TEST(vecCross3(z, x) == y);
            TEST(vecCross3(y, x) == -z);
            TEST(vecCross3(x, x) == Vec3{0});
        }

        // vecEq3
        {
            TEST(vecEq3({1.0f, 2.0f, 3.0f}, {1.0f, 2.0f, 3.0f}));
            TEST(!vecEq3({1.0f, 2.0f, 3.0f}, {1.0f, 2.0f, 4.0f}));
            TEST(vecEq3({1.0f + 1e-7f, 2.0f, 3.0f}, {1.0f, 2.0f, 3.0f}));
        }
    }

    // ------------------------------------------------------------------
    // Vec4
    // ------------------------------------------------------------------

    {
        // Construction
        {
            Vec4 v{1.0f, 2.0f, 3.0f, 4.0f};
            TEST(v.x == 1.0f && v.y == 2.0f && v.z == 3.0f && v.w == 4.0f);
            Vec4 s{5.0f};
            TEST(s.x == 5.0f && s.y == 5.0f && s.z == 5.0f && s.w == 5.0f);
        }

        // Vec2/Vec3 promotion
        {
            Vec4 v1{Vec2{1.0f, 2.0f}, 3.0f, 4.0f};
            TEST(v1.x == 1.0f && v1.y == 2.0f && v1.z == 3.0f && v1.w == 4.0f);
            Vec4 v2{Vec3{1.0f, 2.0f, 3.0f}, 4.0f};
            TEST((v2 == Vec4{1.0f, 2.0f, 3.0f, 4.0f}));
        }

        // Downcasts
        {
            Vec2 v2 = static_cast<Vec2>(Vec4{1.0f, 2.0f, 3.0f, 4.0f});
            TEST(v2.x == 1.0f && v2.y == 2.0f);
            Vec3 v3 = static_cast<Vec3>(Vec4{1.0f, 2.0f, 3.0f, 4.0f});
            TEST(v3.x == 1.0f && v3.y == 2.0f && v3.z == 3.0f);
        }

        // Arithmetic
        {
            Vec4 a{1.0f, 2.0f, 3.0f, 4.0f};
            Vec4 b{5.0f, 6.0f, 7.0f, 8.0f};
            TEST((a + b == Vec4{6.0f, 8.0f, 10.0f, 12.0f}));
            TEST((b - a == Vec4{4.0f, 4.0f, 4.0f, 4.0f}));
            TEST((-Vec4{1.0f, -2.0f, 3.0f, -4.0f} == Vec4{-1.0f, 2.0f, -3.0f, 4.0f}));
            TEST((2.0f * a == Vec4{2.0f, 4.0f, 6.0f, 8.0f}));
            TEST((a / 2.0f == Vec4{0.5f, 1.0f, 1.5f, 2.0f}));
        }

        // vecDot4 / vecLen / vecNorm4
        {
            TEST(vecDot4({1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f}) == 70.0f);
            TEST(vecLenSqr4({1.0f, 0.0f, 0.0f, 0.0f}) == 1.0f);
            TEST(std::abs(vecLen4({1.0f, 2.0f, 3.0f, 4.0f}) - std::sqrt(30.0f)) < FLT_EPSILON);
            Vec4 n = vecNorm4({-5.0f, 0.0f, 0.0f, 0.0f});
            TEST(n.x == -1.0f && n.y == 0.0f && n.z == 0.0f && n.w == 0.0f);
            TEST(std::abs(vecLen4(vecNorm4({1.0f, 2.0f, 3.0f, 4.0f})) - 1.0f) < FLT_EPSILON);
        }

        // vecEq4
        {
            TEST(vecEq4({1.0f, 2.0f, 3.0f, 4.0f}, {1.0f, 2.0f, 3.0f, 4.0f}));
            TEST(!vecEq4({1.0f, 2.0f, 3.0f, 4.0f}, {1.0f, 2.0f, 3.0f, 5.0f}));
        }
    }

    // ------------------------------------------------------------------
    // Mat2
    // ------------------------------------------------------------------

    {
        // Construction
        {
            Mat2 m{Vec2{1.0f, 2.0f}, Vec2{3.0f, 4.0f}};
            TEST(m.x.x == 1.0f && m.x.y == 2.0f);
            TEST(m.y.x == 3.0f && m.y.y == 4.0f);
            Mat2 s{1.0f};
            TEST(s.x.x == 1.0f && s.x.y == 0.0f && s.y.x == 0.0f && s.y.y == 1.0f);
            Mat2 c{1.0f, 2.0f, 3.0f, 4.0f};
            TEST(c.x.x == 1.0f && c.x.y == 2.0f);
            TEST(c.y.x == 3.0f && c.y.y == 4.0f);
        }

        // operator[]
        {
            Mat2 m{1.0f, 2.0f, 3.0f, 4.0f};
            TEST(m[0].x == 1.0f && m[0].y == 2.0f);
            TEST(m[1].x == 3.0f && m[1].y == 4.0f);
        }

        // Comparison
        {
            Mat2 a{1.0f};
            Mat2 b{2.0f};
            TEST(a == a);
            TEST(a != b);
        }

        // Add / Sub
        {
            Mat2 a{Vec2{1, 2}, Vec2{3, 4}};
            Mat2 b{Vec2{5, 6}, Vec2{7, 8}};
            Mat2 sum = a + b;
            Mat2 diff = a - b;
            TEST(sum.x.x == 6 && sum.x.y == 8 && sum.y.x == 10 && sum.y.y == 12);
            TEST(diff.x.x == -4 && diff.x.y == -4 && diff.y.x == -4 && diff.y.y == -4);
        }

        // In-place
        {
            Mat2 m{Vec2{1, 2}, Vec2{3, 4}};
            m += Mat2{Vec2{5, 6}, Vec2{7, 8}};
            TEST(m.x.x == 6);
            m -= Mat2{Vec2{1, 1}, Vec2{1, 1}};
            TEST(m.x.x == 5);
        }

        // Mul
        {
            Mat2 a{Vec2{1, 2}, Vec2{3, 4}};
            Mat2 b{Vec2{5, 6}, Vec2{7, 8}};
            Mat2 id{1.0f};
            Mat2 p = a * b;
            TEST(p.x.x == 23 && p.x.y == 34 && p.y.x == 31 && p.y.y == 46);
            TEST(id * a == a);
            TEST(a * id == a);
        }

        // Matrix * Vec2
        {
            Mat2 id{1.0f};
            Mat2 m{Vec2{1, 2}, Vec2{3, 4}};
            Vec2 v{5, 6};
            TEST(id * v == v);
            Vec2 mv = m * v;
            TEST(mv.x == 23 && mv.y == 34);
        }

        // Transpose
        {
            Mat2 m{Vec2{1, 2}, Vec2{3, 4}};
            Mat2 t = matTranspose2(m);
            TEST(t.x.x == 1 && t.x.y == 3 && t.y.x == 2 && t.y.y == 4);
            TEST(matTranspose2(t) == m);
        }
    }

    // ------------------------------------------------------------------
    // Mat3
    // ------------------------------------------------------------------

    {
        // Construction
        {
            Mat3 m{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
            TEST(m.x.x == 1 && m.x.y == 2 && m.x.z == 3);
            TEST(m.y.x == 4 && m.y.y == 5 && m.y.z == 6);
            TEST(m.z.x == 7 && m.z.y == 8 && m.z.z == 9);
            Mat3 id{1.0f};
            TEST((id == Mat3{Vec3{1,0,0}, Vec3{0,1,0}, Vec3{0,0,1}}));
        }

        // Mat2 promotion / Mat2 demotion
        {
            Mat2 m2{Vec2{1, 2}, Vec2{3, 4}};
            Mat3 up{m2};
            TEST(up.x.x == 1 && up.x.y == 2 && up.x.z == 0);
            TEST(up.z.x == 0 && up.z.y == 0 && up.z.z == 1);
            Mat3 m3{Vec3{1,2,3}, Vec3{4,5,6}, Vec3{7,8,9}};
            Mat2 down = (Mat2)m3;
            TEST(down.x.x == 1 && down.x.y == 2);
            TEST(down.y.x == 4 && down.y.y == 5);
        }

        // Arithmetic
        {
            Mat3 a{1.0f};
            Mat3 b{2.0f};
            Mat3 zero{0.0f};
            TEST(a + b == Mat3{3.0f});
            TEST(b - a == Mat3{1.0f});
            TEST(a * zero == zero);
            TEST(a * a == a);
        }

        // Matrix * Vec3
        {
            Mat3 id{1.0f};
            Vec3 v{1, 2, 3};
            TEST(id * v == v);
        }

        // Transpose
        {
            Mat3 m{Vec3{1,2,3}, Vec3{4,5,6}, Vec3{7,8,9}};
            Mat3 t = matTranspose3(m);
            TEST(t.x.x == 1 && t.x.y == 4 && t.x.z == 7);
            TEST(t.y.x == 2 && t.y.y == 5 && t.y.z == 8);
            TEST(t.z.x == 3 && t.z.y == 6 && t.z.z == 9);
            TEST(matTranspose3(t) == m);
        }
    }

    // ------------------------------------------------------------------
    // Mat4
    // ------------------------------------------------------------------

    {
        // Construction
        {
            Mat4 m{Vec4{1,2,3,4}, Vec4{5,6,7,8}, Vec4{9,10,11,12}, Vec4{13,14,15,16}};
            TEST(m.x.x == 1 && m.x.y == 2 && m.x.z == 3 && m.x.w == 4);
            TEST(m.w.x == 13 && m.w.w == 16);
            Mat4 id{1.0f};
            TEST(id.x.x == 1 && id.y.y == 1 && id.z.z == 1 && id.w.w == 1);
            TEST(id.x.y == 0);
        }

        // Promotions and downcasts
        {
            Mat4 m2{Mat2{Vec2{1,2}, Vec2{3,4}}};
            TEST(m2.x.x == 1 && m2.x.y == 2 && m2.z.z == 1 && m2.w.w == 1);
            Mat3 m3{Vec3{1,2,3}, Vec3{4,5,6}, Vec3{7,8,9}};
            Mat4 m4{m3};
            TEST(m4.x.x == 1 && m4.y.y == 5 && m4.z.z == 9 && m4.w.w == 1);
            Mat4 full{Vec4{1,2,3,4}, Vec4{5,6,7,8}, Vec4{9,10,11,12}, Vec4{13,14,15,16}};
            TEST(((Mat2)full == Mat2{Vec2{1,2}, Vec2{5,6}}));
            TEST(((Mat3)full == Mat3{Vec3{1,2,3}, Vec3{5,6,7}, Vec3{9,10,11}}));
        }

        // Arithmetic
        {
            Mat4 id{1.0f};
            Mat4 a{Vec4{1,2,3,4}, Vec4{5,6,7,8}, Vec4{9,10,11,12}, Vec4{13,14,15,16}};
            TEST(id * a == a);
            TEST(a * id == a);
            Vec4 v{1, 2, 3, 4};
            TEST(id * v == v);
        }

        // Transpose
        {
            Mat4 m{Vec4{1,2,3,4}, Vec4{5,6,7,8}, Vec4{9,10,11,12}, Vec4{13,14,15,16}};
            Mat4 t = matTranspose4(m);
            TEST(t.x.x == 1 && t.x.y == 5 && t.x.z == 9 && t.x.w == 13);
            TEST(matTranspose4(t) == m);
        }
    }

    // ------------------------------------------------------------------
    // Complex
    // ------------------------------------------------------------------

    {
        // Construction
        {
            Complex c{3.0f, 4.0f};
            TEST(c.r == 3.0f && c.i == 4.0f);
            Complex r{5.0f};
            TEST(r.r == 5.0f && r.i == 0.0f);
        }

        // Add / Sub
        {
            Complex a{1, 2};
            Complex b{3, 4};
            TEST((a + b == Complex{4, 6}));
            TEST((a - b == Complex{-2, -2}));
        }

        // Mul
        {
            Complex a{1, 2};
            Complex b{3, 4};
            TEST((a * b == Complex{-5, 10}));
            TEST((Complex{0, 1} * Complex{0, 1} == Complex{-1, 0}));
        }

        // In-place
        {
            Complex c{1, 2};
            c += Complex{3, 4};
            TEST((c == Complex{4, 6}));
            c -= Complex{1, 1};
            TEST((c == Complex{3, 5}));
        }

        // Conjugate / Abs / Norm
        {
            Complex c{3, 4};
            Complex cj = complexConj(c);
            TEST(cj.r == 3 && cj.i == -4);
            TEST(complexConj(cj) == c);
            TEST(complexAbsSqr(c) == 25.0f);
            TEST(std::abs(complexAbs(c) - 5.0f) < FLT_EPSILON);
            Complex n = complexNorm(c);
            TEST(std::abs(complexAbs(n) - 1.0f) < FLT_EPSILON);
        }

        // vecRot2
        {
            Vec2 v{1, 0};
            Vec2 r90 = vecRot2(Complex{0, 1}, v);
            TEST(std::abs(r90.x) < FLT_EPSILON && std::abs(r90.y - 1.0f) < FLT_EPSILON);
            Vec2 r180 = vecRot2(Complex{-1, 0}, v);
            TEST(std::abs(r180.x + 1.0f) < FLT_EPSILON && std::abs(r180.y) < FLT_EPSILON);
            Vec2 r0 = vecRot2(Complex{1, 0}, v);
            TEST(r0.x == 1.0f && std::abs(r0.y) < FLT_EPSILON);
        }
    }

    // ------------------------------------------------------------------
    // Quat
    // ------------------------------------------------------------------

    {
        // Construction
        {
            Quat q{1, 2, 3, 4};
            TEST(q.r == 1 && q.i == 2 && q.j == 3 && q.k == 4);
            Quat r{5};
            TEST(r.r == 5 && r.i == 0 && r.j == 0 && r.k == 0);
        }

        // Add / Sub
        {
            Quat a{1, 2, 3, 4};
            Quat b{5, 6, 7, 8};
            TEST((a + b == Quat{6, 8, 10, 12}));
            TEST((a - b == Quat{-4, -4, -4, -4}));
        }

        // Mul
        {
            Quat id{1};
            Quat q{1, 2, 3, 4};
            TEST(id * q == q);
            TEST(q * id == q);
            Quat i{0, 1, 0, 0};
            Quat j{0, 0, 1, 0};
            Quat k{0, 0, 0, 1};
            TEST(i * j == k);
            TEST(j * k == i);
            TEST(k * i == j);
            TEST((j * i == Quat{0, 0, 0, -1}));
        }

        // In-place
        {
            Quat q{1, 2, 3, 4};
            q += Quat{1, 1, 1, 1};
            TEST((q == Quat{2, 3, 4, 5}));
            q -= Quat{0, 1, 2, 3};
            TEST((q == Quat{2, 2, 2, 2}));
        }

        // Conjugate / Abs / Norm
        {
            Quat q{1, 2, 3, 4};
            Quat cj = quatConj(q);
            TEST(cj.r == 1 && cj.i == -2 && cj.j == -3 && cj.k == -4);
            TEST(quatConj(cj) == q);
            TEST(quatAbsSqr(q) == 30.0f);
            TEST(std::abs(quatAbs(q) - std::sqrt(30.0f)) < FLT_EPSILON);
            Quat n = quatNorm(q);
            TEST(std::abs(quatAbs(n) - 1.0f) < FLT_EPSILON);
        }

        // quatAxisAngle / vecRot3
        {
            Quat id = quatAxisAngle({0, 0, 1}, 0.0f);
            TEST(std::abs(id.r - 1.0f) < FLT_EPSILON);
            Quat q90 = quatAxisAngle({0, 0, 1}, pif / 2.0f);
            Vec3 r = vecRot3(q90, {1, 0, 0});
            TEST(std::abs(r.x) < FLT_EPSILON);
            TEST(std::abs(r.y - 1.0f) < FLT_EPSILON);
            TEST(std::abs(r.z) < FLT_EPSILON);
        }

        // quatBetween
        {
            Quat id = quatBetween({1, 0, 0}, {1, 0, 0});
            TEST(std::abs(id.r - 1.0f) < FLT_EPSILON);
            Quat q = quatBetween({1, 0, 0}, {0, 1, 0});
            Vec3 r = vecRot3(q, {1, 0, 0});
            TEST(std::abs(r.x) < 1e-5f);
            TEST(std::abs(r.y - 1.0f) < 1e-5f);
            TEST(std::abs(r.z) < 1e-5f);
        }

        // matRot3 consistency with vecRot3
        {
            Quat q = quatAxisAngle({0, 0, 1}, pif / 3.0f);
            Vec3 v{1, 2, 3};
            Vec3 rv = vecRot3(q, v);
            Vec3 rm = matRot3(q, Mat3{1.0f}) * v;
            TEST(vecEq3(rv, rm));
        }
    }

    // ------------------------------------------------------------------
    // Matrix construction
    // ------------------------------------------------------------------

    {
        // matModel2D - identity
        {
            Mat4 m = matModel2D({0, 0, 0}, {1, 1}, 0);
            TEST(m == Mat4{1.0f});
        }

        // matModel2D - translation
        {
            Mat4 m = matModel2D({10, 20, 0}, {1, 1}, 0);
            TEST(m.w.x == 10 && m.w.y == 20);
        }

        // matModel2D - scale
        {
            Mat4 m = matModel2D({0, 0, 0}, {2, 3}, 0);
            Vec4 scaled = m * Vec4{1, 1, 0, 1};
            TEST(std::abs(scaled.x - 2.0f) < FLT_EPSILON);
            TEST(std::abs(scaled.y - 3.0f) < FLT_EPSILON);
        }

        // matOrthographic
        {
            Mat4 p = matOrthographic(-1, 1, 1, -1, 0, 100);
            Vec4 nc = p * Vec4{0, 0, 0, 1};
            TEST(std::abs(nc.x) < FLT_EPSILON);
            TEST(std::abs(nc.y) < FLT_EPSILON);
            TEST(std::abs(nc.z) < FLT_EPSILON);
            TEST(std::abs(nc.w - 1.0f) < FLT_EPSILON);
        }
    }
}


#include "tests.hpp"
#include "hg/math.hpp"

using namespace hg;

TEST(testScalarPow)
{
    ASSERT(pow(2.0f, 0) == 1.0f);
    ASSERT(pow(2.0f, 1) == 2.0f);
    ASSERT(pow(3.0f, 2) == 9.0f);
    ASSERT(pow(0.0f, 5) == 0.0f);
    ASSERT(pow(1.0f, 100) == 1.0f);
    ASSERT(pow(-2.0f, 3) == -8.0f);
    ASSERT(pow(-2.0f, 2) == 4.0f);
}

TEST(testScalarSquare)
{
    ASSERT(square(0.0f) == 0.0f);
    ASSERT(square(1.0f) == 1.0f);
    ASSERT(square(-1.0f) == 1.0f);
    ASSERT(square(2.5f) == 6.25f);
}

TEST(testScalarLerp)
{
    ASSERT(lerp(0.0f, 10.0f, 0.0f) == 0.0f);
    ASSERT(lerp(0.0f, 10.0f, 1.0f) == 10.0f);
    ASSERT(lerp(0.0f, 10.0f, 0.5f) == 5.0f);
    ASSERT(lerp(5.0f, 5.0f, 0.3f) == 5.0f);
}

TEST(testScalarSmooth)
{
    ASSERT(smooth(0.0f) == 0.0f);
    ASSERT(smooth(1.0f) == 1.0f);
    ASSERT(std::abs(smooth(0.5f) - 0.5f) < FLT_EPSILON);
    ASSERT(smoothQuintic(0.0f) == 0.0f);
    ASSERT(smoothQuintic(1.0f) == 1.0f);
    ASSERT(std::abs(smoothQuintic(0.5f) - 0.5f) < FLT_EPSILON);
}

TEST(testVec2Construction)
{
    Vec2 v{1.0f, 2.0f};
    ASSERT(v.x == 1.0f && v.y == 2.0f);
    Vec2 s{5.0f};
    ASSERT(s.x == 5.0f && s.y == 5.0f);
    ASSERT(v[0] == 1.0f && v[1] == 2.0f);
}

TEST(testVec2Negation)
{
    Vec2 n = -Vec2{1.0f, -2.0f};
    ASSERT(n.x == -1.0f && n.y == 2.0f);
}

TEST(testVec2Arithmetic)
{
    Vec2 a{1.0f, 2.0f};
    Vec2 b{3.0f, 4.0f};
    ASSERT((a + b == Vec2{4.0f, 6.0f}));
    ASSERT((b - a == Vec2{2.0f, 2.0f}));
    ASSERT((a * b == Vec2{3.0f, 8.0f}));
    ASSERT((b / a == Vec2{3.0f, 2.0f}));
}

TEST(testVec2ScalarOps)
{
    Vec2 v{2.0f, 3.0f};
    ASSERT((5.0f * v == Vec2{10.0f, 15.0f}));
    ASSERT((v * 5.0f == Vec2{10.0f, 15.0f}));
    ASSERT((v / 2.0f == Vec2{1.0f, 1.5f}));
}

TEST(testVec2InPlace)
{
    Vec2 v{1.0f, 2.0f};
    v += Vec2{3.0f, 4.0f};
    ASSERT(v.x == 4.0f && v.y == 6.0f);
    v -= Vec2{1.0f, 1.0f};
    ASSERT(v.x == 3.0f && v.y == 5.0f);
    v *= Vec2{2.0f, 3.0f};
    ASSERT(v.x == 6.0f && v.y == 15.0f);
    v /= Vec2{3.0f, 5.0f};
    ASSERT(v.x == 2.0f && v.y == 3.0f);
}

TEST(testVec2Dot)
{
    ASSERT(vecDot2({1.0f, 0.0f}, {0.0f, 1.0f}) == 0.0f);
    ASSERT(vecDot2({1.0f, 0.0f}, {1.0f, 0.0f}) == 1.0f);
    ASSERT(vecDot2({3.0f, 4.0f}, {5.0f, 6.0f}) == 39.0f);
}

TEST(testVec2Len)
{
    ASSERT(vecLenSqr2({0.0f, 0.0f}) == 0.0f);
    ASSERT(vecLenSqr2({1.0f, 0.0f}) == 1.0f);
    ASSERT(vecLenSqr2({3.0f, 4.0f}) == 25.0f);
    ASSERT(std::abs(vecLen2({3.0f, 4.0f}) - 5.0f) < FLT_EPSILON);
}

TEST(testVec2Norm)
{
    Vec2 n = vecNorm2({3.0f, 0.0f});
    ASSERT(n.x == 1.0f && n.y == 0.0f);
    ASSERT(std::abs(vecLen2(vecNorm2({3.0f, 4.0f})) - 1.0f) < FLT_EPSILON);
}

TEST(testVec2Cross)
{
    ASSERT(vecCross2({1.0f, 0.0f}, {0.0f, 1.0f}) == 1.0f);
    ASSERT(vecCross2({0.0f, 1.0f}, {1.0f, 0.0f}) == -1.0f);
    ASSERT(std::abs(vecCross2({2.0f, 3.0f}, {4.0f, 6.0f})) < FLT_EPSILON);
}

TEST(testVec2Eq)
{
    ASSERT(vecEq2({1.0f, 2.0f}, {1.0f, 2.0f}));
    ASSERT(!vecEq2({1.0f, 2.0f}, {1.0f, 3.0f}));
    ASSERT(vecEq2({1.0f + 1e-7f, 2.0f}, {1.0f, 2.0f}));
    ASSERT(!vecEq2({1.0f + 1e-5f, 2.0f}, {1.0f, 2.0f}));
}

TEST(testVec3Construction)
{
    Vec3 v{1.0f, 2.0f, 3.0f};
    ASSERT(v.x == 1.0f && v.y == 2.0f && v.z == 3.0f);
    Vec3 s{5.0f};
    ASSERT(s.x == 5.0f && s.y == 5.0f && s.z == 5.0f);
    Vec3 v2{Vec2{1.0f, 2.0f}, 3.0f};
    ASSERT(v2.x == 1.0f && v2.y == 2.0f && v2.z == 3.0f);
}

TEST(testVec3Downcast)
{
    Vec2 v = static_cast<Vec2>(Vec3{1.0f, 2.0f, 3.0f});
    ASSERT(v.x == 1.0f && v.y == 2.0f);
}

TEST(testVec3Index)
{
    Vec3 v{3.0f, 4.0f, 5.0f};
    ASSERT(v[0] == 3.0f && v[1] == 4.0f && v[2] == 5.0f);
}

TEST(testVec3Arithmetic)
{
    ASSERT((-Vec3{1.0f, -2.0f, 3.0f} == Vec3{-1.0f, 2.0f, -3.0f}));
    Vec3 a{1.0f, 2.0f, 3.0f};
    Vec3 b{4.0f, 5.0f, 6.0f};
    ASSERT((a + b == Vec3{5.0f, 7.0f, 9.0f}));
    ASSERT((b - a == Vec3{3.0f, 3.0f, 3.0f}));
    ASSERT((a * b == Vec3{4.0f, 10.0f, 18.0f}));
}

TEST(testVec3ScalarOps)
{
    Vec3 v{1.0f, 2.0f, 3.0f};
    ASSERT((2.0f * v == Vec3{2.0f, 4.0f, 6.0f}));
    ASSERT((v * 2.0f == Vec3{2.0f, 4.0f, 6.0f}));
    ASSERT((v / 2.0f == Vec3{0.5f, 1.0f, 1.5f}));
}

TEST(testVec3DotLenNorm)
{
    ASSERT(vecDot3({1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}) == 0.0f);
    ASSERT(vecDot3({1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}) == 32.0f);
    ASSERT(vecLenSqr3({1.0f, 2.0f, 2.0f}) == 9.0f);
    ASSERT(std::abs(vecLen3({1.0f, 2.0f, 2.0f}) - 3.0f) < FLT_EPSILON);
    Vec3 n = vecNorm3({0.0f, -5.0f, 0.0f});
    ASSERT(n.x == 0.0f && n.y == -1.0f && n.z == 0.0f);
    ASSERT(std::abs(vecLen3(vecNorm3({1.0f, 2.0f, 3.0f})) - 1.0f) < FLT_EPSILON);
}

TEST(testVec3Cross)
{
    Vec3 x{1.0f, 0.0f, 0.0f};
    Vec3 y{0.0f, 1.0f, 0.0f};
    Vec3 z{0.0f, 0.0f, 1.0f};
    ASSERT(vecCross3(x, y) == z);
    ASSERT(vecCross3(y, z) == x);
    ASSERT(vecCross3(z, x) == y);
    ASSERT(vecCross3(y, x) == -z);
    ASSERT(vecCross3(x, x) == Vec3{0});
}

TEST(testVec3Eq)
{
    ASSERT(vecEq3({1.0f, 2.0f, 3.0f}, {1.0f, 2.0f, 3.0f}));
    ASSERT(!vecEq3({1.0f, 2.0f, 3.0f}, {1.0f, 2.0f, 4.0f}));
    ASSERT(vecEq3({1.0f + 1e-7f, 2.0f, 3.0f}, {1.0f, 2.0f, 3.0f}));
}

TEST(testVec4Construction)
{
    Vec4 v{1.0f, 2.0f, 3.0f, 4.0f};
    ASSERT(v.x == 1.0f && v.y == 2.0f && v.z == 3.0f && v.w == 4.0f);
    Vec4 s{5.0f};
    ASSERT(s.x == 5.0f && s.y == 5.0f && s.z == 5.0f && s.w == 5.0f);
}

TEST(testVec4Promotion)
{
    Vec4 v1{Vec2{1.0f, 2.0f}, 3.0f, 4.0f};
    ASSERT(v1.x == 1.0f && v1.y == 2.0f && v1.z == 3.0f && v1.w == 4.0f);
    Vec4 v2{Vec3{1.0f, 2.0f, 3.0f}, 4.0f};
    ASSERT((v2 == Vec4{1.0f, 2.0f, 3.0f, 4.0f}));
}

TEST(testVec4Downcast)
{
    Vec2 v2 = static_cast<Vec2>(Vec4{1.0f, 2.0f, 3.0f, 4.0f});
    ASSERT(v2.x == 1.0f && v2.y == 2.0f);
    Vec3 v3 = static_cast<Vec3>(Vec4{1.0f, 2.0f, 3.0f, 4.0f});
    ASSERT(v3.x == 1.0f && v3.y == 2.0f && v3.z == 3.0f);
}

TEST(testVec4Arithmetic)
{
    Vec4 a{1.0f, 2.0f, 3.0f, 4.0f};
    Vec4 b{5.0f, 6.0f, 7.0f, 8.0f};
    ASSERT((a + b == Vec4{6.0f, 8.0f, 10.0f, 12.0f}));
    ASSERT((b - a == Vec4{4.0f, 4.0f, 4.0f, 4.0f}));
    ASSERT((-Vec4{1.0f, -2.0f, 3.0f, -4.0f} == Vec4{-1.0f, 2.0f, -3.0f, 4.0f}));
    ASSERT((2.0f * a == Vec4{2.0f, 4.0f, 6.0f, 8.0f}));
    ASSERT((a / 2.0f == Vec4{0.5f, 1.0f, 1.5f, 2.0f}));
}

TEST(testVec4DotLenNorm)
{
    ASSERT(vecDot4({1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f}) == 70.0f);
    ASSERT(vecLenSqr4({1.0f, 0.0f, 0.0f, 0.0f}) == 1.0f);
    ASSERT(std::abs(vecLen4({1.0f, 2.0f, 3.0f, 4.0f}) - std::sqrt(30.0f)) < FLT_EPSILON);
    Vec4 n = vecNorm4({-5.0f, 0.0f, 0.0f, 0.0f});
    ASSERT(n.x == -1.0f && n.y == 0.0f && n.z == 0.0f && n.w == 0.0f);
    ASSERT(std::abs(vecLen4(vecNorm4({1.0f, 2.0f, 3.0f, 4.0f})) - 1.0f) < FLT_EPSILON);
}

TEST(testVec4Eq)
{
    ASSERT(vecEq4({1.0f, 2.0f, 3.0f, 4.0f}, {1.0f, 2.0f, 3.0f, 4.0f}));
    ASSERT(!vecEq4({1.0f, 2.0f, 3.0f, 4.0f}, {1.0f, 2.0f, 3.0f, 5.0f}));
}

TEST(testMat2Construction)
{
    Mat2 m{Vec2{1.0f, 2.0f}, Vec2{3.0f, 4.0f}};
    ASSERT(m.x.x == 1.0f && m.x.y == 2.0f);
    ASSERT(m.y.x == 3.0f && m.y.y == 4.0f);
    Mat2 s{1.0f};
    ASSERT(s.x.x == 1.0f && s.x.y == 0.0f && s.y.x == 0.0f && s.y.y == 1.0f);
    Mat2 c{1.0f, 2.0f, 3.0f, 4.0f};
    ASSERT(c.x.x == 1.0f && c.x.y == 2.0f);
    ASSERT(c.y.x == 3.0f && c.y.y == 4.0f);
}

TEST(testMat2Index)
{
    Mat2 m{1.0f, 2.0f, 3.0f, 4.0f};
    ASSERT(m[0].x == 1.0f && m[0].y == 2.0f);
    ASSERT(m[1].x == 3.0f && m[1].y == 4.0f);
}

TEST(testMat2Comparison)
{
    Mat2 a{1.0f};
    Mat2 b{2.0f};
    ASSERT(a == a);
    ASSERT(a != b);
}

TEST(testMat2AddSub)
{
    Mat2 a{Vec2{1, 2}, Vec2{3, 4}};
    Mat2 b{Vec2{5, 6}, Vec2{7, 8}};
    Mat2 sum = a + b;
    Mat2 diff = a - b;
    ASSERT(sum.x.x == 6 && sum.x.y == 8 && sum.y.x == 10 && sum.y.y == 12);
    ASSERT(diff.x.x == -4 && diff.x.y == -4 && diff.y.x == -4 && diff.y.y == -4);
}

TEST(testMat2InPlace)
{
    Mat2 m{Vec2{1, 2}, Vec2{3, 4}};
    m += Mat2{Vec2{5, 6}, Vec2{7, 8}};
    ASSERT(m.x.x == 6);
    m -= Mat2{Vec2{1, 1}, Vec2{1, 1}};
    ASSERT(m.x.x == 5);
}

TEST(testMat2Mul)
{
    Mat2 a{Vec2{1, 2}, Vec2{3, 4}};
    Mat2 b{Vec2{5, 6}, Vec2{7, 8}};
    Mat2 id{1.0f};
    Mat2 p = a * b;
    ASSERT(p.x.x == 23 && p.x.y == 34 && p.y.x == 31 && p.y.y == 46);
    ASSERT(id * a == a);
    ASSERT(a * id == a);
}

TEST(testMat2VecMul)
{
    Mat2 id{1.0f};
    Mat2 m{Vec2{1, 2}, Vec2{3, 4}};
    Vec2 v{5, 6};
    ASSERT(id * v == v);
    Vec2 mv = m * v;
    ASSERT(mv.x == 23 && mv.y == 34);
}

TEST(testMat2Transpose)
{
    Mat2 m{Vec2{1, 2}, Vec2{3, 4}};
    Mat2 t = matTranspose2(m);
    ASSERT(t.x.x == 1 && t.x.y == 3 && t.y.x == 2 && t.y.y == 4);
    ASSERT(matTranspose2(t) == m);
}

TEST(testMat3Construction)
{
    Mat3 m{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
    ASSERT(m.x.x == 1 && m.x.y == 2 && m.x.z == 3);
    ASSERT(m.y.x == 4 && m.y.y == 5 && m.y.z == 6);
    ASSERT(m.z.x == 7 && m.z.y == 8 && m.z.z == 9);
    Mat3 id{1.0f};
    ASSERT((id == Mat3{Vec3{1,0,0}, Vec3{0,1,0}, Vec3{0,0,1}}));
}

TEST(testMat3Promotion)
{
    Mat2 m2{Vec2{1, 2}, Vec2{3, 4}};
    Mat3 up{m2};
    ASSERT(up.x.x == 1 && up.x.y == 2 && up.x.z == 0);
    ASSERT(up.z.x == 0 && up.z.y == 0 && up.z.z == 1);
    Mat3 m3{Vec3{1,2,3}, Vec3{4,5,6}, Vec3{7,8,9}};
    Mat2 down = (Mat2)m3;
    ASSERT(down.x.x == 1 && down.x.y == 2);
    ASSERT(down.y.x == 4 && down.y.y == 5);
}

TEST(testMat3Arithmetic)
{
    Mat3 a{1.0f};
    Mat3 b{2.0f};
    Mat3 zero{0.0f};
    ASSERT(a + b == Mat3{3.0f});
    ASSERT(b - a == Mat3{1.0f});
    ASSERT(a * zero == zero);
    ASSERT(a * a == a);
}

TEST(testMat3VecMul)
{
    Mat3 id{1.0f};
    Vec3 v{1, 2, 3};
    ASSERT(id * v == v);
}

TEST(testMat3Transpose)
{
    Mat3 m{Vec3{1,2,3}, Vec3{4,5,6}, Vec3{7,8,9}};
    Mat3 t = matTranspose3(m);
    ASSERT(t.x.x == 1 && t.x.y == 4 && t.x.z == 7);
    ASSERT(t.y.x == 2 && t.y.y == 5 && t.y.z == 8);
    ASSERT(t.z.x == 3 && t.z.y == 6 && t.z.z == 9);
    ASSERT(matTranspose3(t) == m);
}

TEST(testMat4Construction)
{
    Mat4 m{Vec4{1,2,3,4}, Vec4{5,6,7,8}, Vec4{9,10,11,12}, Vec4{13,14,15,16}};
    ASSERT(m.x.x == 1 && m.x.y == 2 && m.x.z == 3 && m.x.w == 4);
    ASSERT(m.w.x == 13 && m.w.w == 16);
    Mat4 id{1.0f};
    ASSERT(id.x.x == 1 && id.y.y == 1 && id.z.z == 1 && id.w.w == 1);
    ASSERT(id.x.y == 0);
}

TEST(testMat4Promotion)
{
    Mat4 m2{Mat2{Vec2{1,2}, Vec2{3,4}}};
    ASSERT(m2.x.x == 1 && m2.x.y == 2 && m2.z.z == 1 && m2.w.w == 1);
    Mat3 m3{Vec3{1,2,3}, Vec3{4,5,6}, Vec3{7,8,9}};
    Mat4 m4{m3};
    ASSERT(m4.x.x == 1 && m4.y.y == 5 && m4.z.z == 9 && m4.w.w == 1);
    Mat4 full{Vec4{1,2,3,4}, Vec4{5,6,7,8}, Vec4{9,10,11,12}, Vec4{13,14,15,16}};
    ASSERT(((Mat2)full == Mat2{Vec2{1,2}, Vec2{5,6}}));
    ASSERT(((Mat3)full == Mat3{Vec3{1,2,3}, Vec3{5,6,7}, Vec3{9,10,11}}));
}

TEST(testMat4Arithmetic)
{
    Mat4 id{1.0f};
    Mat4 a{Vec4{1,2,3,4}, Vec4{5,6,7,8}, Vec4{9,10,11,12}, Vec4{13,14,15,16}};
    ASSERT(id * a == a);
    ASSERT(a * id == a);
    Vec4 v{1, 2, 3, 4};
    ASSERT(id * v == v);
}

TEST(testMat4Transpose)
{
    Mat4 m{Vec4{1,2,3,4}, Vec4{5,6,7,8}, Vec4{9,10,11,12}, Vec4{13,14,15,16}};
    Mat4 t = matTranspose4(m);
    ASSERT(t.x.x == 1 && t.x.y == 5 && t.x.z == 9 && t.x.w == 13);
    ASSERT(matTranspose4(t) == m);
}

TEST(testComplexConstruction)
{
    Complex c{3.0f, 4.0f};
    ASSERT(c.r == 3.0f && c.i == 4.0f);
    Complex r{5.0f};
    ASSERT(r.r == 5.0f && r.i == 0.0f);
}

TEST(testComplexAddSub)
{
    Complex a{1, 2};
    Complex b{3, 4};
    ASSERT((a + b == Complex{4, 6}));
    ASSERT((a - b == Complex{-2, -2}));
}

TEST(testComplexMul)
{
    Complex a{1, 2};
    Complex b{3, 4};
    ASSERT((a * b == Complex{-5, 10}));
    ASSERT((Complex{0, 1} * Complex{0, 1} == Complex{-1, 0}));
}

TEST(testComplexInPlace)
{
    Complex c{1, 2};
    c += Complex{3, 4};
    ASSERT((c == Complex{4, 6}));
    c -= Complex{1, 1};
    ASSERT((c == Complex{3, 5}));
}

TEST(testComplexConjAbsNorm)
{
    Complex c{3, 4};
    Complex cj = complexConj(c);
    ASSERT(cj.r == 3 && cj.i == -4);
    ASSERT(complexConj(cj) == c);
    ASSERT(complexAbsSqr(c) == 25.0f);
    ASSERT(std::abs(complexAbs(c) - 5.0f) < FLT_EPSILON);
    Complex n = complexNorm(c);
    ASSERT(std::abs(complexAbs(n) - 1.0f) < FLT_EPSILON);
}

TEST(testComplexVecRot2)
{
    Vec2 v{1, 0};
    Vec2 r90 = vecRot2(Complex{0, 1}, v);
    ASSERT(std::abs(r90.x) < FLT_EPSILON && std::abs(r90.y - 1.0f) < FLT_EPSILON);
    Vec2 r180 = vecRot2(Complex{-1, 0}, v);
    ASSERT(std::abs(r180.x + 1.0f) < FLT_EPSILON && std::abs(r180.y) < FLT_EPSILON);
    Vec2 r0 = vecRot2(Complex{1, 0}, v);
    ASSERT(r0.x == 1.0f && std::abs(r0.y) < FLT_EPSILON);
}

TEST(testQuatConstruction)
{
    Quat q{1, 2, 3, 4};
    ASSERT(q.r == 1 && q.i == 2 && q.j == 3 && q.k == 4);
    Quat r{5};
    ASSERT(r.r == 5 && r.i == 0 && r.j == 0 && r.k == 0);
}

TEST(testQuatAddSub)
{
    Quat a{1, 2, 3, 4};
    Quat b{5, 6, 7, 8};
    ASSERT((a + b == Quat{6, 8, 10, 12}));
    ASSERT((a - b == Quat{-4, -4, -4, -4}));
}

TEST(testQuatMul)
{
    Quat id{1};
    Quat q{1, 2, 3, 4};
    ASSERT(id * q == q);
    ASSERT(q * id == q);
    Quat i{0, 1, 0, 0};
    Quat j{0, 0, 1, 0};
    Quat k{0, 0, 0, 1};
    ASSERT(i * j == k);
    ASSERT(j * k == i);
    ASSERT(k * i == j);
    ASSERT((j * i == Quat{0, 0, 0, -1}));
}

TEST(testQuatInPlace)
{
    Quat q{1, 2, 3, 4};
    q += Quat{1, 1, 1, 1};
    ASSERT((q == Quat{2, 3, 4, 5}));
    q -= Quat{0, 1, 2, 3};
    ASSERT((q == Quat{2, 2, 2, 2}));
}

TEST(testQuatConjAbsNorm)
{
    Quat q{1, 2, 3, 4};
    Quat cj = quatConj(q);
    ASSERT(cj.r == 1 && cj.i == -2 && cj.j == -3 && cj.k == -4);
    ASSERT(quatConj(cj) == q);
    ASSERT(quatAbsSqr(q) == 30.0f);
    ASSERT(std::abs(quatAbs(q) - std::sqrt(30.0f)) < FLT_EPSILON);
    Quat n = quatNorm(q);
    ASSERT(std::abs(quatAbs(n) - 1.0f) < FLT_EPSILON);
}

TEST(testQuatAxisAngle)
{
    Quat id = quatAxisAngle({0, 0, 1}, 0.0f);
    ASSERT(std::abs(id.r - 1.0f) < FLT_EPSILON);
    Quat q90 = quatAxisAngle({0, 0, 1}, pif / 2.0f);
    Vec3 r = vecRot3(q90, {1, 0, 0});
    ASSERT(std::abs(r.x) < FLT_EPSILON);
    ASSERT(std::abs(r.y - 1.0f) < FLT_EPSILON);
    ASSERT(std::abs(r.z) < FLT_EPSILON);
}

TEST(testQuatBetween)
{
    Quat id = quatBetween({1, 0, 0}, {1, 0, 0});
    ASSERT(std::abs(id.r - 1.0f) < FLT_EPSILON);
    Quat q = quatBetween({1, 0, 0}, {0, 1, 0});
    Vec3 r = vecRot3(q, {1, 0, 0});
    ASSERT(std::abs(r.x) < 1e-5f);
    ASSERT(std::abs(r.y - 1.0f) < 1e-5f);
    ASSERT(std::abs(r.z) < 1e-5f);
}

TEST(testQuatMatRot3Consistency)
{
    Quat q = quatAxisAngle({0, 0, 1}, pif / 3.0f);
    Vec3 v{1, 2, 3};
    Vec3 rv = vecRot3(q, v);
    Vec3 rm = matRot3(q, Mat3{1.0f}) * v;
    ASSERT(vecEq3(rv, rm));
}

TEST(testMatModel2D)
{
    Mat4 m0 = matModel2D({0, 0, 0}, {1, 1}, 0);
    ASSERT(m0 == Mat4{1.0f});

    Mat4 mt = matModel2D({10, 20, 0}, {1, 1}, 0);
    ASSERT(mt.w.x == 10 && mt.w.y == 20);

    Mat4 ms = matModel2D({0, 0, 0}, {2, 3}, 0);
    Vec4 scaled = ms * Vec4{1, 1, 0, 1};
    ASSERT(std::abs(scaled.x - 2.0f) < FLT_EPSILON);
    ASSERT(std::abs(scaled.y - 3.0f) < FLT_EPSILON);
}

TEST(testMatOrthographic)
{
    Mat4 p = matOrthographic(-1, 1, 1, -1, 0, 100);
    Vec4 nc = p * Vec4{0, 0, 0, 1};
    ASSERT(std::abs(nc.x) < FLT_EPSILON);
    ASSERT(std::abs(nc.y) < FLT_EPSILON);
    ASSERT(std::abs(nc.z) < FLT_EPSILON);
    ASSERT(std::abs(nc.w - 1.0f) < FLT_EPSILON);
}

TEST(testVec2EqualityOperators)
{
    ASSERT((Vec2{1, 2} == Vec2{1, 2}));
    ASSERT((Vec2{1, 2} != Vec2{1, 3}));
    ASSERT((Vec2{1, 2} != Vec2{3, 2}));
    ASSERT(!(Vec2{1, 2} != Vec2{1, 2}));
    ASSERT(!(Vec2{1, 2} == Vec2{1, 3}));
}

TEST(testVec3EqualityOperators)
{
    ASSERT((Vec3{1, 2, 3} == Vec3{1, 2, 3}));
    ASSERT((Vec3{1, 2, 3} != Vec3{1, 2, 4}));
    ASSERT((Vec3{1, 2, 3} != Vec3{4, 2, 3}));
    ASSERT((Vec3{1, 2, 3} != Vec3{1, 5, 3}));
    ASSERT(!(Vec3{1, 2, 3} != Vec3{1, 2, 3}));
    ASSERT(!(Vec3{1, 2, 3} == Vec3{1, 2, 4}));
}

TEST(testVec4EqualityOperators)
{
    ASSERT((Vec4{1, 2, 3, 4} == Vec4{1, 2, 3, 4}));
    ASSERT((Vec4{1, 2, 3, 4} != Vec4{1, 2, 3, 5}));
    ASSERT((Vec4{1, 2, 3, 4} != Vec4{5, 2, 3, 4}));
    ASSERT((Vec4{1, 2, 3, 4} != Vec4{1, 6, 3, 4}));
    ASSERT((Vec4{1, 2, 3, 4} != Vec4{1, 2, 7, 4}));
    ASSERT(!(Vec4{1, 2, 3, 4} != Vec4{1, 2, 3, 4}));
    ASSERT(!(Vec4{1, 2, 3, 4} == Vec4{1, 2, 3, 5}));
}

TEST(testMat2EqualityOperators)
{
    Mat2 a{Vec2{1, 2}, Vec2{3, 4}};
    Mat2 b{Vec2{1, 2}, Vec2{3, 4}};
    Mat2 c{Vec2{5, 6}, Vec2{7, 8}};
    ASSERT(a == b);
    ASSERT(a != c);
    ASSERT(!(a != b));
    ASSERT(!(a == c));
}

TEST(testMat3EqualityOperators)
{
    Mat3 a{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
    Mat3 b{Vec3{1, 2, 3}, Vec3{4, 5, 6}, Vec3{7, 8, 9}};
    Mat3 c{Vec3{9, 8, 7}, Vec3{6, 5, 4}, Vec3{3, 2, 1}};
    ASSERT(a == b);
    ASSERT(a != c);
    ASSERT(!(a != b));
    ASSERT(!(a == c));
}

TEST(testMat4EqualityOperators)
{
    Mat4 a{Vec4{1,2,3,4}, Vec4{5,6,7,8}, Vec4{9,10,11,12}, Vec4{13,14,15,16}};
    Mat4 b{Vec4{1,2,3,4}, Vec4{5,6,7,8}, Vec4{9,10,11,12}, Vec4{13,14,15,16}};
    Mat4 c{Vec4{0,0,0,0}, Vec4{0,0,0,0}, Vec4{0,0,0,0}, Vec4{0,0,0,0}};
    ASSERT(a == b);
    ASSERT(a != c);
    ASSERT(!(a != b));
    ASSERT(!(a == c));
}

TEST(testComplexEqualityOperators)
{
    ASSERT((Complex{1, 2} == Complex{1, 2}));
    ASSERT((Complex{1, 2} != Complex{1, 3}));
    ASSERT((Complex{1, 2} != Complex{3, 2}));
    ASSERT(!(Complex{1, 2} != Complex{1, 2}));
    ASSERT(!(Complex{1, 2} == Complex{1, 3}));
}

TEST(testQuatEqualityOperators)
{
    ASSERT((Quat{1, 2, 3, 4} == Quat{1, 2, 3, 4}));
    ASSERT((Quat{1, 2, 3, 4} != Quat{1, 2, 3, 5}));
    ASSERT((Quat{1, 2, 3, 4} != Quat{5, 2, 3, 4}));
    ASSERT((Quat{1, 2, 3, 4} != Quat{1, 6, 3, 4}));
    ASSERT((Quat{1, 2, 3, 4} != Quat{1, 2, 7, 4}));
    ASSERT(!(Quat{1, 2, 3, 4} != Quat{1, 2, 3, 4}));
    ASSERT(!(Quat{1, 2, 3, 4} == Quat{1, 2, 3, 5}));
}

TEST(testVec3InPlace)
{
    Vec3 v{1, 2, 3};
    v += Vec3{4, 5, 6};
    ASSERT((v == Vec3{5, 7, 9}));
    v -= Vec3{1, 2, 3};
    ASSERT((v == Vec3{4, 5, 6}));
}

TEST(testVec4InPlace)
{
    Vec4 v{1, 2, 3, 4};
    v += Vec4{5, 6, 7, 8};
    ASSERT((v == Vec4{6, 8, 10, 12}));
    v -= Vec4{1, 2, 3, 4};
    ASSERT((v == Vec4{5, 6, 7, 8}));
    v *= Vec4{2, 2, 2, 2};
    ASSERT((v == Vec4{10, 12, 14, 16}));
    v /= Vec4{2, 3, 2, 4};
    ASSERT((v == Vec4{5, 4, 7, 4}));
}

TEST(testVec3ScalarMulDiv)
{
    Vec3 v{1, 2, 3};
    ASSERT((v * 2.0f == Vec3{2, 4, 6}));
    ASSERT((3.0f * v == Vec3{3, 6, 9}));
    ASSERT((v / 2.0f == Vec3{0.5f, 1.0f, 1.5f}));
}

TEST(testVec4ScalarMulDiv)
{
    Vec4 v{1, 2, 3, 4};
    ASSERT((v * 2.0f == Vec4{2, 4, 6, 8}));
    ASSERT((3.0f * v == Vec4{3, 6, 9, 12}));
    ASSERT((v / 2.0f == Vec4{0.5f, 1.0f, 1.5f, 2.0f}));
}

TEST(testMat3Mul)
{
    Mat3 id{1.0f};
    Mat3 a{Vec3{1,2,3}, Vec3{4,5,6}, Vec3{7,8,9}};
    ASSERT(id * a == a);
    ASSERT(a * id == a);
    Mat3 b{Vec3{9,8,7}, Vec3{6,5,4}, Vec3{3,2,1}};
    Mat3 c = a * b;
    ASSERT(c.x.x == 90 && c.x.y == 114 && c.x.z == 138);
    ASSERT(c.y.x == 54 && c.y.y == 69 && c.y.z == 84);
    ASSERT(c.z.x == 18 && c.z.y == 24 && c.z.z == 30);
}

TEST(testMat4Mul)
{
    Mat4 id{1.0f};
    Mat4 a{Vec4{1,2,3,4}, Vec4{5,6,7,8}, Vec4{9,10,11,12}, Vec4{13,14,15,16}};
    ASSERT(id * a == a);
    ASSERT(a * id == a);
    Mat4 b{Vec4{16,15,14,13}, Vec4{12,11,10,9}, Vec4{8,7,6,5}, Vec4{4,3,2,1}};
    Mat4 c = a * b;
    ASSERT(c.x.x == 386 && c.x.w == 560);
    ASSERT(c.w.x == 50 && c.w.w == 80);
}

TEST(testMat4AddSub)
{
    Mat4 a{Vec4{1,2,3,4}, Vec4{5,6,7,8}, Vec4{9,10,11,12}, Vec4{13,14,15,16}};
    Mat4 b{Vec4{16,15,14,13}, Vec4{12,11,10,9}, Vec4{8,7,6,5}, Vec4{4,3,2,1}};
    Mat4 sum = a + b;
    Mat4 diff = a - b;
    ASSERT(sum.x.x == 17 && sum.w.w == 17);
    ASSERT(diff.x.x == -15 && diff.w.w == 15);
    Mat4 id{1.0f};
    ASSERT(a + Mat4{0.0f} == a);
    ASSERT(a - a == Mat4{0.0f});
    ASSERT(a + id - id == a);
}

TEST(testMat3AddSub)
{
    Mat3 a{Vec3{1,2,3}, Vec3{4,5,6}, Vec3{7,8,9}};
    Mat3 b{Vec3{9,8,7}, Vec3{6,5,4}, Vec3{3,2,1}};
    Mat3 sum = a + b;
    Mat3 diff = a - b;
    ASSERT(sum.x.x == 10 && sum.z.z == 10);
    ASSERT(diff.x.x == -8 && diff.z.z == 8);
    ASSERT(a + Mat3{0.0f} == a);
    ASSERT(a - a == Mat3{0.0f});
}

TEST(testComplexAbsZero)
{
    ASSERT(complexAbs(Complex{0, 0}) == 0.0f);
    ASSERT(complexAbsSqr(Complex{0, 0}) == 0.0f);
}

TEST(testQuatAbsZero)
{
    ASSERT(quatAbs(Quat{0, 0, 0, 0}) == 0.0f);
    ASSERT(quatAbsSqr(Quat{0, 0, 0, 0}) == 0.0f);
}

TEST(testSmoothQuinticBoundary)
{
    ASSERT(smoothQuintic(0.0f) == 0.0f);
    ASSERT(smoothQuintic(1.0f) == 1.0f);
    ASSERT(smooth(0.0f) == 0.0f);
    ASSERT(smooth(1.0f) == 1.0f);
}

TEST(testPowZeroExponent)
{
    ASSERT(pow(0.0f, 0) == 1.0f);
    ASSERT(pow(1.0f, 0) == 1.0f);
    ASSERT(pow(5.0f, 0) == 1.0f);
    ASSERT(pow(-3.0f, 0) == 1.0f);
    ASSERT(pow(0.001f, 0) == 1.0f);
}

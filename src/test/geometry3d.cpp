#include "tests.hpp"
#include "hg/geometry3d.hpp"

using namespace hg;

TEST(testSphereContainsPoint)
{
    Sphere s{{0, 0, 0}, 5};
    ASSERT(containsPointSphere({0, 0, 0}, s));
    ASSERT(containsPointSphere({5, 0, 0}, s));
    ASSERT(!containsPointSphere({5.01f, 0, 0}, s));
}

TEST(testSphereDistPoint)
{
    Sphere s{{0, 0, 0}, 5};
    ASSERT(std::abs(distPointSphere({0, 0, 0}, s) - (-5.0f)) < FLT_EPSILON);
    ASSERT(std::abs(distPointSphere({5, 0, 0}, s)) < FLT_EPSILON);
}

TEST(testSphereClosestPoint)
{
    Sphere s{{0, 0, 0}, 5};
    Vec3 p = closestPointSphere({10, 0, 0}, s);
    ASSERT(std::abs(p.x - 5.0f) < FLT_EPSILON);
}

TEST(testSphereIntersect)
{
    Sphere s{{0, 0, 0}, 5};
    Sphere b{{8, 0, 0}, 3};
    ASSERT(intersectSpheres(s, b));
    Sphere miss{{20, 0, 0}, 1};
    ASSERT(!intersectSpheres(s, miss));
    ASSERT(std::abs(distSpheres(s, b)) < FLT_EPSILON);
}

TEST(testBoxEmpty)
{
    Box b = boxEmpty();
    ASSERT(!containsPointBox({0, 0, 0}, b));
}

TEST(testBoxAddPoint)
{
    Box b = boxEmpty();
    b = boxAddPoint(b, {1, 2, 3});
    b = boxAddPoint(b, {4, 5, 6});
    ASSERT(containsPointBox({2, 3, 4}, b));
}

TEST(testBoxContainsClosestPoint)
{
    Box b{{0, 0, 0}, {10, 10, 10}};
    ASSERT(containsPointBox({5, 5, 5}, b));
    ASSERT(containsPointBox({0, 0, 0}, b));
    ASSERT(containsPointBox({10, 10, 10}, b));
    ASSERT(!containsPointBox({-0.01f, 5, 5}, b));
    Vec3 p = closestPointBox({-5, 5, 5}, b);
    ASSERT(p.x == 0 && p.y == 5 && p.z == 5);
}

TEST(testBoxIntersect)
{
    Box a{{0, 0, 0}, {10, 10, 10}};
    Box b{{5, 5, 5}, {15, 15, 15}};
    ASSERT(intersectBox(a, b));
    Box c{{20, 20, 20}, {30, 30, 30}};
    ASSERT(!intersectBox(a, c));
}

TEST(testBoxSphereIntersect)
{
    Box b{{0, 0, 0}, {10, 10, 10}};
    Sphere s{{5, 5, 5}, 3};
    ASSERT(intersectBoxSphere(b, s));
    Sphere far{{20, 20, 20}, 1};
    ASSERT(!intersectBoxSphere(b, far));
}

TEST(testPlaneFromPoint)
{
    Plane p = planeFromPoint({0, 5, 0}, {0, 1, 0});
    ASSERT(vecEq3(p.normal, {0, 1, 0}));
    ASSERT(std::abs(p.dist - 5.0f) < FLT_EPSILON);
}

TEST(testPlaneFromTri)
{
    Tri tri{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
    Plane p = planeFromTri(tri);
    ASSERT(std::abs(p.dist) < FLT_EPSILON);
}

TEST(testIntersectRaySphere)
{
    Ray3D ray{{0, 0, 0}, {0, 0, 1}};
    Sphere s{{0, 0, 10}, 3};
    Maybe<Hit3D> hit = intersectRaySphere(ray, s);
    ASSERT(hit.has);
    if (hit.has)
        ASSERT(std::abs(hit.val.dist - 7.0f) < FLT_EPSILON);
}

TEST(testIntersectRayBox)
{
    Ray3D ray{{-5, 5, 5}, {1, 0, 0}};
    Box b{{0, 0, 0}, {10, 10, 10}};
    Maybe<Hit3D> hit = intersectRayBox(ray, b);
    ASSERT(hit.has);
}

TEST(testIntersectRayTri)
{
    Ray3D ray{{0, 0, -5}, {0, 0, 1}};
    Tri tri{{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};
    Maybe<Hit3D> hit = intersectRayTri(ray, tri);
    ASSERT(hit.has);
    if (hit.has)
        ASSERT(std::abs(hit.val.dist - 5.0f) < FLT_EPSILON);
}

TEST(testIntersectRayPlane)
{
    Ray3D ray{{0, 0, -5}, {0, 0, 1}};
    Plane p{{0, 0, 1}, 0};
    Maybe<Hit3D> hit = intersectRayPlane(ray, p);
    ASSERT(hit.has);
    if (hit.has)
        ASSERT(std::abs(hit.val.dist - 5.0f) < FLT_EPSILON);
}

TEST(testIntersectRayPlaneParallel)
{
    Ray3D ray{{0, 0, 0}, {1, 0, 0}};
    Plane p{{0, 0, 1}, 10};
    ASSERT(!intersectRayPlane(ray, p).has);
}

TEST(testIntersectLineSphere)
{
    Line3D line{{-10, 3, 0}, {10, 3, 0}};
    Sphere s{{0, 5, 0}, 3};
    ASSERT(intersectLineSphere(line, s).has);
}

TEST(testIntersectLineBox)
{
    Line3D line{{-5, 5, 5}, {15, 5, 5}};
    Box b{{0, 0, 0}, {10, 10, 10}};
    ASSERT(intersectLineBox(line, b).has);
}

TEST(testIntersectLineTri)
{
    Line3D line{{0, 0, -5}, {0, 0, 5}};
    Tri tri{{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};
    ASSERT(intersectLineTri(line, tri).has);
}

TEST(testIntersectLinePlane)
{
    Line3D line{{0, 0, -5}, {0, 0, 5}};
    Plane p{{0, 0, 1}, 0};
    Maybe<Hit3D> hit = intersectLinePlane(line, p);
    ASSERT(hit.has);
    if (hit.has)
        ASSERT(std::abs(hit.val.dist - 0.5f) < FLT_EPSILON);
}

TEST(testIntersectLinePlaneParallel)
{
    Line3D line{{0, 0, -5}, {1, 0, -5}};
    Plane p{{0, 0, 1}, 0};
    ASSERT(!intersectLinePlane(line, p).has);
}

TEST(testSphereContainment)
{
    Sphere outer{{0, 0, 0}, 10};
    Sphere inner{{0, 0, 0}, 5};
    ASSERT(intersectSpheres(outer, inner));
    ASSERT(containsPointSphere({3, 0, 0}, inner));
}

TEST(testBoxContainment)
{
    Box outer{{0, 0, 0}, {10, 10, 10}};
    Box inner{{2, 2, 2}, {8, 8, 8}};
    ASSERT(intersectBox(outer, inner));
    ASSERT(containsPointBox({5, 5, 5}, inner));
}

TEST(testBoxIntersectEdgeTouching)
{
    Box a{{0, 0, 0}, {5, 5, 5}};
    Box b{{5, 0, 0}, {10, 5, 5}};
    ASSERT(intersectBox(a, b));
}

TEST(testBoxIntersectIdentical)
{
    Box a{{0, 0, 0}, {10, 10, 10}};
    Box b{{0, 0, 0}, {10, 10, 10}};
    ASSERT(intersectBox(a, b));
}

TEST(testRay3DMissSphere)
{
    Ray3D ray{{0, 0, 0}, {1, 0, 0}};
    Sphere s{{0, 20, 0}, 3};
    ASSERT(!intersectRaySphere(ray, s).has);
}

TEST(testRay3DMissBox)
{
    Ray3D ray{{0, 0, 0}, {0, 0, 1}};
    Box b{{5, 5, 5}, {10, 10, 10}};
    ASSERT(!intersectRayBox(ray, b).has);
}

TEST(testIntersectLineSphereMiss)
{
    Line3D line{{0, 0, 0}, {0, 100, 0}};
    Sphere s{{20, 0, 0}, 3};
    ASSERT(!intersectLineSphere(line, s).has);
}

TEST(testIntersectLineBoxMiss)
{
    Line3D line{{0, 0, 0}, {0, 100, 0}};
    Box b{{5, 20, 0}, {10, 25, 5}};
    ASSERT(!intersectLineBox(line, b).has);
}

TEST(testIntersectLineTriMiss)
{
    Line3D line{{0, 0, 0}, {0, 100, 0}};
    Tri tri{{5, 0, 0}, {10, 0, 0}, {7, 0, 10}};
    ASSERT(!intersectLineTri(line, tri).has);
}

TEST(testSphereDistPointOutside)
{
    Sphere s{{0, 0, 0}, 5};
    f32 d = distPointSphere({10, 0, 0}, s);
    ASSERT(std::abs(d - 5.0f) < FLT_EPSILON);
}

TEST(testBoxClosestPointInside)
{
    Box b{{0, 0, 0}, {10, 10, 10}};
    Vec3 p = closestPointBox({5, 5, 5}, b);
    ASSERT(p.x == 5 && p.y == 5 && p.z == 5);
}

TEST(testIntersectRayTriMiss)
{
    Ray3D ray{{0, 0, 0}, {0, 0, 1}};
    Tri tri{{5, 0, 0}, {10, 0, 0}, {7, 0, 10}};
    ASSERT(!intersectRayTri(ray, tri).has);
}

TEST(testPlaneFromPointNormal)
{
    Plane p = planeFromPoint({3, 7, 2}, {1, 0, 0});
    ASSERT(vecEq3(p.normal, {1, 0, 0}));
    ASSERT(std::abs(p.dist - 3.0f) < FLT_EPSILON);
}

TEST(testTriNormal)
{
    Tri tri{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    ASSERT(vecEq3(tri.a, {1, 0, 0}));
    ASSERT(vecEq3(tri.b, {0, 1, 0}));
    ASSERT(vecEq3(tri.c, {0, 0, 1}));
}

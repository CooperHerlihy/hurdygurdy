#include "hg_geometry3d.hpp"
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstdio>

namespace hg {

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
    newBox.begin.x = std::min(box.begin.x, point.x - FLT_EPSILON);
    newBox.begin.y = std::min(box.begin.y, point.y - FLT_EPSILON);
    newBox.begin.z = std::min(box.begin.z, point.z - FLT_EPSILON);
    newBox.end.x = std::max(box.end.x, point.x + FLT_EPSILON);
    newBox.end.y = std::max(box.end.y, point.y + FLT_EPSILON);
    newBox.end.z = std::max(box.end.z, point.z + FLT_EPSILON);
    return newBox;
}

bool containsPointBox(Vec3 point, Box box)
{
    return point.x >= box.begin.x && point.x <= box.end.x
        && point.y >= box.begin.y && point.y <= box.end.y
        && point.z >= box.begin.z && point.z <= box.end.z;
}

Vec3 closestPointBox(Vec3 pos, Box box)
{
    return {
        std::clamp(pos.x, box.begin.x, box.end.x),
        std::clamp(pos.y, box.begin.y, box.end.y),
        std::clamp(pos.z, box.begin.z, box.end.z),
    };
}

bool intersectBox(Box a, Box b)
{
    return a.end.x >= b.begin.x && a.begin.x <= b.end.x
        && a.end.y >= b.begin.y && a.begin.y <= b.end.y
        && a.end.z >= b.begin.z && a.begin.z <= b.end.z;
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

Maybe<Hit3D> intersectRaySphere(Ray3D ray, Sphere sphere)
{
    HG_ASSERT(ray.dir != Vec3{0});

    Vec3 rel = ray.pos - sphere.pos;
    f32 a = vecDot3(ray.dir, ray.dir);
    f32 b = 2 * vecDot3(ray.dir, rel);
    f32 c = vecDot3(rel, rel) - square(sphere.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return {};
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        return {};

    return some<Hit3D>(t, (ray.pos + t * ray.dir - sphere.pos) / sphere.radius);
}

Maybe<Hit3D> intersectRayBox(Ray3D ray, Box box)
{
    HG_ASSERT(ray.dir != Vec3{0});
    if (vecEq3(box.begin, box.end))
        return {};

    if (containsPointBox(ray.pos, box))
    {
        return some<Hit3D>(0.0f, -ray.dir);
    }

    f32 hits[6] = {
        (box.begin.x - ray.pos.x) / ray.dir.x,
        (box.begin.y - ray.pos.y) / ray.dir.y,
        (box.begin.z - ray.pos.z) / ray.dir.z,
        (box.end.x - ray.pos.x) / ray.dir.x,
        (box.end.y - ray.pos.y) / ray.dir.y,
        (box.end.z - ray.pos.z) / ray.dir.z,
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
    for (u32 i = 0; i < std::size(hits); ++i)
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
        return {};

    return some<Hit3D>(t, norm);
}

// Moller-Trumbore, Real Time Rendering 4th Edition
Maybe<Hit3D> intersectRayTri(Ray3D ray, Tri tri)
{
    HG_ASSERT(ray.dir != Vec3{0});

    if (tri.a == tri.b || tri.a == tri.c || tri.b == tri.c)
        return {};

    Vec3 e1 = tri.b - tri.a;
    Vec3 e2 = tri.c - tri.a;
    Vec3 q = vecCross3(ray.dir, e2);

    f32 a = vecDot3(e1, q);
    if (std::abs(a) < FLT_EPSILON)
        return {};

    Vec3 s = ray.pos - tri.a;

    f32 u = vecDot3(s, q) / a;
    if (u < -FLT_EPSILON)
        return {};

    Vec3 r = vecCross3(s, e1);

    f32 v = vecDot3(ray.dir, r) / a;
    if (v < -FLT_EPSILON || u + v > 1 + FLT_EPSILON)
        return {};

    f32 t = vecDot3(e2, r) / a;
    if (t < -FLT_EPSILON)
        return {};

    return some<Hit3D>(t, a < 0
        ? vecNorm3(vecCross3(e2, e1))
        : vecNorm3(vecCross3(e1, e2)));
}

Maybe<Hit3D> intersectRayPlane(Ray3D ray, Plane plane)
{
    HG_ASSERT(ray.dir != Vec3{0});
    HG_ASSERT(plane.normal != Vec3{0});

    f32 denom = vecDot3(ray.dir, plane.normal);
    if (std::abs(denom) < FLT_EPSILON)
        return {};

    f32 t = (plane.dist - vecDot3(ray.pos, plane.normal)) / denom;
    if (t < -FLT_EPSILON)
        return {};

    return some<Hit3D>(t, denom < 0
        ? plane.normal
        : -plane.normal);
}

Maybe<Hit3D> intersectLineSphere(Line3D line, Sphere sphere)
{
    Vec3 dir = line.end - line.begin;

    Vec3 rel = line.begin - sphere.pos;
    f32 a = vecDot3(dir, dir);
    f32 b = 2 * vecDot3(dir, rel);
    f32 c = vecDot3(rel, rel) - square(sphere.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return {};
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t > 1 + FLT_EPSILON)
        return {};
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return {};

    return some<Hit3D>(t, (line.begin + t * dir - sphere.pos) / sphere.radius);
}

Maybe<Hit3D> intersectLineBox(Line3D line, Box box)
{
    if (vecEq3(line.begin, line.end) || vecEq3(box.begin, box.end))
        return {};

    f32 hits[6] = {
        (box.begin.x - line.begin.x) / (line.end.x - line.begin.x),
        (box.begin.y - line.begin.y) / (line.end.y - line.begin.y),
        (box.begin.z - line.begin.z) / (line.end.z - line.begin.z),
        (box.end.x - line.begin.x) / (line.end.x - line.begin.x),
        (box.end.y - line.begin.y) / (line.end.y - line.begin.y),
        (box.end.z - line.begin.z) / (line.end.z - line.begin.z),
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
    for (u32 i = 0; i < std::size(hits); ++i)
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
        return {};

    return some<Hit3D>(t, norm);
}

// Moller-Trumbore, Real Time Rendering 4th Edition
Maybe<Hit3D> intersectLineTri(Line3D line, Tri tri)
{
    if (vecEq3(line.begin, line.end))
        return {};

    if (tri.a == tri.b || tri.a == tri.c || tri.b == tri.c)
        return {};

    Vec3 lineDir = line.end - line.begin;

    Vec3 e1 = tri.b - tri.a;
    Vec3 e2 = tri.c - tri.a;
    Vec3 q = vecCross3(lineDir, e2);

    f32 a = vecDot3(e1, q);
    if (std::abs(a) < FLT_EPSILON)
        return {};

    Vec3 s = line.begin - tri.a;

    f32 u = vecDot3(s, q) / a;
    if (u < -FLT_EPSILON)
        return {};

    Vec3 r = vecCross3(s, e1);

    f32 v = vecDot3(lineDir, r) / a;
    if (v < -FLT_EPSILON || u + v > 1 + FLT_EPSILON)
        return {};

    f32 t = vecDot3(e2, r) / a;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return {};

    return some<Hit3D>(t, a < 0
        ? vecNorm3(vecCross3(e2, e1))
        : vecNorm3(vecCross3(e1, e2)));
}

Maybe<Hit3D> intersectLinePlane(Line3D line, Plane plane)
{
    if (line.begin == line.end)
        return {};

    HG_ASSERT(plane.normal != Vec3{0});

    Vec3 lineDir = line.end - line.begin;

    f32 denom = vecDot3(lineDir, plane.normal);
    if (std::abs(denom) < FLT_EPSILON)
        return {};

    f32 t = (plane.dist - vecDot3(line.begin, plane.normal)) / denom;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return {};

    return some<Hit3D>(t, denom < 0
        ? plane.normal
        : -plane.normal);
}

} // namespace hg

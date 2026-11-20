#include "hg/geometry.hpp"

#include <cmath>
#include <algorithm>

namespace hg {

Rect emptyRect()
{
    return {
        Vec2{INFINITY},
        Vec2{-INFINITY},
    };
}

Rect rectAddPoint(Rect rect, Vec2 point)
{
    Rect newRect;
    newRect.begin.x() = std::min(rect.begin.x(), point.x() - epsilonf);
    newRect.begin.y() = std::min(rect.begin.y(), point.y() - epsilonf);
    newRect.end.x() = std::max(rect.end.x(), point.x() + epsilonf);
    newRect.end.y() = std::max(rect.end.y(), point.y() + epsilonf);
    return newRect;
}

bool containsPoint(Vec2 point, Rect rect)
{
    return point.x() >= rect.begin.x() - epsilonf && point.x() <= rect.end.x() + epsilonf
        && point.y() >= rect.begin.y() - epsilonf && point.y() <= rect.end.y() + epsilonf;
}

bool containsPoint(Vec2 point, Circle circle)
{
    return distance(point, circle) <= epsilonf;
}

Vec2 closestPoint(Vec2 pos, Rect rect)
{
    return {
        std::clamp(pos.x(), rect.begin.x(), rect.end.x()),
        std::clamp(pos.y(), rect.begin.y(), rect.end.y()),
    };
}

Vec2 closestPoint(Vec2 pos, Circle circle)
{
    return circle.pos + circle.radius * normalize(pos - circle.pos);
}

f32 distance(Vec2 point, Circle circle)
{
    return length(point - circle.pos) - circle.radius;
}

f32 distance(Circle a, Circle b)
{
    return length(a.pos - b.pos) - a.radius - b.radius;
}

bool intersect(Rect a, Rect b)
{
    return a.end.x() >= b.begin.x() && a.begin.x() <= b.end.x()
        && a.end.y() >= b.begin.y() && a.begin.y() <= b.end.y();
}

bool intersect(Circle a, Circle b)
{
    return distance(a, b) <= epsilonf;
}

bool intersect(Rect rect, Circle circle)
{
    return containsPoint(closestPoint(circle.pos, rect), circle);
}

std::optional<Hit2D> intersect(Ray2D ray, Rect rect)
{
    HG_ASSERT(ray.dir != Vec2{0});
    if (rect.begin == rect.end)
        return {};

    if (containsPoint(ray.pos, rect))
    {
        return std::make_optional<Hit2D>(0.0f, -ray.dir);
    }

    f32 hits[4] = {
        (rect.begin.x() - ray.pos.x()) / ray.dir.x(),
        (rect.begin.y() - ray.pos.y()) / ray.dir.y(),
        (rect.end.x() - ray.pos.x()) / ray.dir.x(),
        (rect.end.y() - ray.pos.y()) / ray.dir.y(),
    };

    constexpr Vec2 norms[4] = {
        {-1, 0},
        {0, -1},
        {1, 0},
        {0, 1},
    };

    f32 t = INFINITY;
    Vec2 norm;
    for (u32 i = 0; i < std::size(hits); ++i)
    {
        if (hits[i] < -epsilonf)
            continue;

        if (!containsPoint(ray.pos + hits[i] * ray.dir, rect))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return {};

    return std::make_optional<Hit2D>(t, norm);
}

std::optional<Hit2D> intersect(Ray2D ray, Circle circle)
{
    HG_ASSERT(ray.dir != Vec2{0});

    Vec2 rel = ray.pos - circle.pos;
    f32 a = dot(ray.dir, ray.dir);
    f32 b = 2 * dot(ray.dir, rel);
    f32 c = dot(rel, rel) - square(circle.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return {};
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t < -epsilonf)
        t = (-b + rtdet) / (2 * a);
    if (t < -epsilonf)
        return {};

    return std::make_optional<Hit2D>(t, (ray.pos + t * ray.dir - circle.pos) / circle.radius);
}

std::optional<Hit2D> intersect(Ray2D ray, Ray2D other)
{
    HG_ASSERT(ray.dir != Vec2{0});
    HG_ASSERT(other.dir != Vec2{0});

    f32 denom = cross(ray.dir, other.dir);
    if (std::abs(denom) < epsilonf)
        return {};

    Vec2 diff = other.pos - ray.pos;

    f32 t = cross(diff, other.dir) / denom;
    if (t < -epsilonf)
        return {};

    f32 tOther = cross(diff, ray.dir) / denom;
    if (tOther < -epsilonf)
        return {};

    return std::make_optional<Hit2D>(t, denom < 0
        ? normalize(Vec2{other.dir.y(), -other.dir.x()})
        : normalize(Vec2{-other.dir.y(), other.dir.x()}));
}

std::optional<Hit2D> intersect(Ray2D ray, Line2D line)
{
    HG_ASSERT(ray.dir != Vec2{0});
    if (line.begin == line.end)
        return {};

    Vec2 lineDir = line.end - line.begin;

    f32 denom = cross(ray.dir, lineDir);
    if (std::abs(denom) < epsilonf)
        return {};

    Vec2 diff = line.begin - ray.pos;

    f32 t = cross(diff, lineDir) / denom;
    if (t < -epsilonf)
        return {};

    f32 tOther = cross(diff, ray.dir) / denom;
    if (tOther < -epsilonf || tOther > 1 + epsilonf)
        return {};

    return std::make_optional<Hit2D>(t, denom < 0
        ? normalize(Vec2{lineDir.y(), -lineDir.x()})
        : normalize(Vec2{-lineDir.y(), lineDir.x()}));
}

std::optional<Hit2D> intersect(Line2D line, Rect rect)
{
    if (line.begin == line.end || rect.begin == rect.end)
        return {};

    f32 hits[4] = {
        (rect.begin.x() - line.begin.x()) / (line.end.x() - line.begin.x()),
        (rect.begin.y() - line.begin.y()) / (line.end.y() - line.begin.y()),
        (rect.end.x() - line.begin.x()) / (line.end.x() - line.begin.x()),
        (rect.end.y() - line.begin.y()) / (line.end.y() - line.begin.y()),
    };

    constexpr Vec2 norms[4] = {
        {-1, 0},
        {0, -1},
        {1, 0},
        {0, 1},
    };

    f32 t = INFINITY;
    Vec2 norm;
    for (u32 i = 0; i < std::size(hits); ++i)
    {
        if (hits[i] < -epsilonf || hits[i] > 1 + epsilonf)
            continue;

        if (!containsPoint(line.begin + hits[i] * (line.end - line.begin), rect))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return {};

    return std::make_optional<Hit2D>(t, norm);
}

std::optional<Hit2D> intersect(Line2D line, Circle circle)
{
    Vec2 dir = line.end - line.begin;

    Vec2 rel = line.begin - circle.pos;
    f32 a = dot(dir, dir);
    f32 b = 2 * dot(dir, rel);
    f32 c = dot(rel, rel) - square(circle.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return {};
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t > 1 + epsilonf)
        return {};
    if (t < -epsilonf)
        t = (-b + rtdet) / (2 * a);
    if (t < -epsilonf || t > 1 + epsilonf)
        return {};

    return std::make_optional<Hit2D>(t, (line.begin + t * dir - circle.pos) / circle.radius);
}

std::optional<Hit2D> intersect(Line2D line, Ray2D ray)
{
    if (line.begin == line.end)
        return {};
    HG_ASSERT(ray.dir != Vec2{0});

    Vec2 lineDir = line.end - line.begin;

    f32 denom = cross(lineDir, ray.dir);
    if (std::abs(denom) < epsilonf)
        return {};

    Vec2 diff = ray.pos - line.begin;

    f32 t = cross(diff, ray.dir) / denom;
    if (t < -epsilonf || t > 1 + epsilonf)
        return {};

    f32 tRay = cross(diff, lineDir) / denom;
    if (tRay < -epsilonf)
        return {};

    return std::make_optional<Hit2D>(t, denom < 0
        ? normalize(Vec2{ray.dir.y(), -ray.dir.x()})
        : normalize(Vec2{-ray.dir.y(), ray.dir.x()}));
}

std::optional<Hit2D> intersect(Line2D line, Line2D other)
{
    if (line.begin == line.end || (other.begin ==  other.end))
        return {};

    Vec2 lineDir = line.end - line.begin;
    Vec2 otherDir = other.end - other.begin;

    f32 denom = cross(lineDir, otherDir);
    if (std::abs(denom) < epsilonf)
        return {};

    Vec2 diff = other.begin - line.begin;

    f32 t = cross(diff, otherDir) / denom;
    if (t < -epsilonf || t > 1 + epsilonf)
        return {};

    f32 tOther = cross(diff, lineDir) / denom;
    if (tOther < -epsilonf || tOther > 1 + epsilonf)
        return {};

    return std::make_optional<Hit2D>(t, denom < 0
        ? normalize(Vec2{otherDir.y(), -otherDir.x()})
        : normalize(Vec2{-otherDir.y(), otherDir.x()}));
}

Box emptyBox()
{
    return {
        Vec3{INFINITY},
        Vec3{-INFINITY},
    };
}

Box boxAddPoint(Box box, Vec3 point)
{
    Box newBox;
    newBox.begin.x() = std::min(box.begin.x(), point.x() - epsilonf);
    newBox.begin.y() = std::min(box.begin.y(), point.y() - epsilonf);
    newBox.begin.z() = std::min(box.begin.z(), point.z() - epsilonf);
    newBox.end.x() = std::max(box.end.x(), point.x() + epsilonf);
    newBox.end.y() = std::max(box.end.y(), point.y() + epsilonf);
    newBox.end.z() = std::max(box.end.z(), point.z() + epsilonf);
    return newBox;
}

Plane planeFromPoint(Vec3 point, Vec3 normal)
{
    Plane plane;
    plane.normal = normalize(normal);
    plane.dist = dot(plane.normal, point);
    return plane;
}

Plane planeFromTri(Triangle tri)
{
    Plane plane;
    plane.normal = normalize(cross(tri.b - tri.a, tri.c - tri.a));
    plane.dist = dot(plane.normal, tri.a);
    return plane;
}

bool containsPoint(Vec3 point, Box box)
{
    return point.x() >= box.begin.x() && point.x() <= box.end.x()
        && point.y() >= box.begin.y() && point.y() <= box.end.y()
        && point.z() >= box.begin.z() && point.z() <= box.end.z();
}

bool containsPoint(Vec3 point, Sphere sphere)
{
    return distance(point, sphere) <= 0;
}

Vec3 closestPoint(Vec3 pos, Box box)
{
    return {
        std::clamp(pos.x(), box.begin.x(), box.end.x()),
        std::clamp(pos.y(), box.begin.y(), box.end.y()),
        std::clamp(pos.z(), box.begin.z(), box.end.z()),
    };
}

Vec3 closestPoint(Vec3 pos, Sphere sphere)
{
    Vec3 rel = pos - sphere.pos;
    if (lengthSquared(rel) <= sphere.radius + epsilonf)
        return pos;
    return sphere.pos + sphere.radius * normalize(rel);
}

f32 distance(Vec3 point, Sphere sphere)
{
    return length(point - sphere.pos) - sphere.radius;
}

f32 distance(Sphere a, Sphere b)
{
    return length(a.pos - b.pos) - a.radius - b.radius;
}

bool intersect(Box a, Box b)
{
    return a.end.x() >= b.begin.x() && a.begin.x() <= b.end.x()
        && a.end.y() >= b.begin.y() && a.begin.y() <= b.end.y()
        && a.end.z() >= b.begin.z() && a.begin.z() <= b.end.z();
}

bool intersect(Sphere a, Sphere b)
{
    return distance(a, b) <= 0;
}

bool intersect(Box box, Sphere sphere)
{
    return containsPoint(closestPoint(sphere.pos, box), sphere);
}

std::optional<Hit3D> intersect(Ray3D ray, Box box)
{
    HG_ASSERT(ray.dir != Vec3{0});
    if (box.begin == box.end)
        return {};

    if (containsPoint(ray.pos, box))
    {
        return std::make_optional<Hit3D>(0.0f, -ray.dir);
    }

    f32 hits[6] = {
        (box.begin.x() - ray.pos.x()) / ray.dir.x(),
        (box.begin.y() - ray.pos.y()) / ray.dir.y(),
        (box.begin.z() - ray.pos.z()) / ray.dir.z(),
        (box.end.x() - ray.pos.x()) / ray.dir.x(),
        (box.end.y() - ray.pos.y()) / ray.dir.y(),
        (box.end.z() - ray.pos.z()) / ray.dir.z(),
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
        if (hits[i] < -epsilonf)
            continue;

        if (!containsPoint(ray.pos + hits[i] * ray.dir, box))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return {};

    return std::make_optional<Hit3D>(t, norm);
}

std::optional<Hit3D> intersect(Ray3D ray, Sphere sphere)
{
    HG_ASSERT(ray.dir != Vec3{0});

    Vec3 rel = ray.pos - sphere.pos;
    f32 a = dot(ray.dir, ray.dir);
    f32 b = 2 * dot(ray.dir, rel);
    f32 c = dot(rel, rel) - square(sphere.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return {};
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t < -epsilonf)
        t = (-b + rtdet) / (2 * a);
    if (t < -epsilonf)
        return {};

    return std::make_optional<Hit3D>(t, (ray.pos + t * ray.dir - sphere.pos) / sphere.radius);
}

// Moller-Trumbore, Real Time Rendering 4th Edition
std::optional<Hit3D> intersect(Ray3D ray, Triangle tri)
{
    HG_ASSERT(ray.dir != Vec3{0});

    if (tri.a == tri.b || tri.a == tri.c || tri.b == tri.c)
        return {};

    Vec3 e1 = tri.b - tri.a;
    Vec3 e2 = tri.c - tri.a;
    Vec3 q = cross(ray.dir, e2);

    f32 a = dot(e1, q);
    if (std::abs(a) < epsilonf)
        return {};

    Vec3 s = ray.pos - tri.a;

    f32 u = dot(s, q) / a;
    if (u < -epsilonf)
        return {};

    Vec3 r = cross(s, e1);

    f32 v = dot(ray.dir, r) / a;
    if (v < -epsilonf || u + v > 1 + epsilonf)
        return {};

    f32 t = dot(e2, r) / a;
    if (t < -epsilonf)
        return {};

    return std::make_optional<Hit3D>(t, a < 0
        ? normalize(cross(e2, e1))
        : normalize(cross(e1, e2)));
}

std::optional<Hit3D> intersect(Ray3D ray, Plane plane)
{
    HG_ASSERT(ray.dir != Vec3{0});
    HG_ASSERT(plane.normal != Vec3{0});

    f32 denom = dot(ray.dir, plane.normal);
    if (std::abs(denom) < epsilonf)
        return {};

    f32 t = (plane.dist - dot(ray.pos, plane.normal)) / denom;
    if (t < -epsilonf)
        return {};

    return std::make_optional<Hit3D>(t, denom < 0
        ? plane.normal
        : -plane.normal);
}

std::optional<Hit3D> intersect(Line3D line, Box box)
{
    if (line.begin == line.end || box.begin == box.end)
        return {};

    f32 hits[6] = {
        (box.begin.x() - line.begin.x()) / (line.end.x() - line.begin.x()),
        (box.begin.y() - line.begin.y()) / (line.end.y() - line.begin.y()),
        (box.begin.z() - line.begin.z()) / (line.end.z() - line.begin.z()),
        (box.end.x() - line.begin.x()) / (line.end.x() - line.begin.x()),
        (box.end.y() - line.begin.y()) / (line.end.y() - line.begin.y()),
        (box.end.z() - line.begin.z()) / (line.end.z() - line.begin.z()),
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
        if (hits[i] < -epsilonf || hits[i] > 1 + epsilonf)
            continue;

        if (!containsPoint(line.begin + hits[i] * (line.end - line.begin), box))
            continue;

        if (hits[i] < t)
        {
            t = hits[i];
            norm = norms[i];
        }
    }
    if (t == INFINITY)
        return {};

    return std::make_optional<Hit3D>(t, norm);
}

std::optional<Hit3D> intersect(Line3D line, Sphere sphere)
{
    Vec3 dir = line.end - line.begin;

    Vec3 rel = line.begin - sphere.pos;
    f32 a = dot(dir, dir);
    f32 b = 2 * dot(dir, rel);
    f32 c = dot(rel, rel) - square(sphere.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return {};
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t > 1 + epsilonf)
        return {};
    if (t < -epsilonf)
        t = (-b + rtdet) / (2 * a);
    if (t < -epsilonf || t > 1 + epsilonf)
        return {};

    return std::make_optional<Hit3D>(t, (line.begin + t * dir - sphere.pos) / sphere.radius);
}

// Moller-Trumbore, Real Time Rendering 4th Edition
std::optional<Hit3D> intersect(Line3D line, Triangle tri)
{
    if (line.begin == line.end)
        return {};

    if (tri.a == tri.b || tri.a == tri.c || tri.b == tri.c)
        return {};

    Vec3 lineDir = line.end - line.begin;

    Vec3 e1 = tri.b - tri.a;
    Vec3 e2 = tri.c - tri.a;
    Vec3 q = cross(lineDir, e2);

    f32 a = dot(e1, q);
    if (std::abs(a) < epsilonf)
        return {};

    Vec3 s = line.begin - tri.a;

    f32 u = dot(s, q) / a;
    if (u < -epsilonf)
        return {};

    Vec3 r = cross(s, e1);

    f32 v = dot(lineDir, r) / a;
    if (v < -epsilonf || u + v > 1 + epsilonf)
        return {};

    f32 t = dot(e2, r) / a;
    if (t < -epsilonf || t > 1 + epsilonf)
        return {};

    return std::make_optional<Hit3D>(t, a < 0
        ? normalize(cross(e2, e1))
        : normalize(cross(e1, e2)));
}

std::optional<Hit3D> intersect(Line3D line, Plane plane)
{
    if (line.begin == line.end)
        return {};

    HG_ASSERT(plane.normal != Vec3{0});

    Vec3 lineDir = line.end - line.begin;

    f32 denom = dot(lineDir, plane.normal);
    if (std::abs(denom) < epsilonf)
        return {};

    f32 t = (plane.dist - dot(line.begin, plane.normal)) / denom;
    if (t < -epsilonf || t > 1 + epsilonf)
        return {};

    return std::make_optional<Hit3D>(t, denom < 0
        ? plane.normal
        : -plane.normal);
}

} // namespace hg

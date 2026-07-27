#include "hg_geometry2d.hpp"
#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstdio>

namespace hg {

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
    newRect.begin.x = std::min(rect.begin.x, point.x - FLT_EPSILON);
    newRect.begin.y = std::min(rect.begin.y, point.y - FLT_EPSILON);
    newRect.end.x = std::max(rect.end.x, point.x + FLT_EPSILON);
    newRect.end.y = std::max(rect.end.y, point.y + FLT_EPSILON);
    return newRect;
}

bool containsPointRect(Vec2 point, Rect rect)
{
    return point.x >= rect.begin.x - FLT_EPSILON && point.x <= rect.end.x + FLT_EPSILON
        && point.y >= rect.begin.y - FLT_EPSILON && point.y <= rect.end.y + FLT_EPSILON;
}

Vec2 closestPointRect(Vec2 pos, Rect rect)
{
    return {
        std::clamp(pos.x, rect.begin.x, rect.end.x),
        std::clamp(pos.y, rect.begin.y, rect.end.y),
    };
}

bool intersectRects(Rect a, Rect b)
{
    return a.end.x >= b.begin.x && a.begin.x <= b.end.x
        && a.end.y >= b.begin.y && a.begin.y <= b.end.y;
}

bool intersectRectCircle(Rect rect, Circle circle)
{
    return containsPointCircle(closestPointRect(circle.pos, rect), circle);
}

Maybe<Hit2D> intersectRays2D(Ray2D ray, Ray2D other)
{
    HG_ASSERT(ray.dir != Vec2{0});
    HG_ASSERT(other.dir != Vec2{0});

    f32 denom = vecCross2(ray.dir, other.dir);
    if (std::abs(denom) < FLT_EPSILON)
        return {};

    Vec2 diff = other.pos - ray.pos;

    f32 t = vecCross2(diff, other.dir) / denom;
    if (t < -FLT_EPSILON)
        return {};

    f32 tOther = vecCross2(diff, ray.dir) / denom;
    if (tOther < -FLT_EPSILON)
        return {};

    return some<Hit2D>(t, denom < 0
        ? vecNorm2({other.dir.y, -other.dir.x})
        : vecNorm2({-other.dir.y, other.dir.x}));
}

Maybe<Hit2D> intersectRayLine2D(Ray2D ray, Line2D line)
{
    HG_ASSERT(ray.dir != Vec2{0});
    if (vecEq2(line.begin, line.end))
        return {};

    Vec2 lineDir = line.end - line.begin;

    f32 denom = vecCross2(ray.dir, lineDir);
    if (std::abs(denom) < FLT_EPSILON)
        return {};

    Vec2 diff = line.begin - ray.pos;

    f32 t = vecCross2(diff, lineDir) / denom;
    if (t < -FLT_EPSILON)
        return {};

    f32 tOther = vecCross2(diff, ray.dir) / denom;
    if (tOther < -FLT_EPSILON || tOther > 1 + FLT_EPSILON)
        return {};

    return some<Hit2D>(t, denom < 0
        ? vecNorm2({lineDir.y, -lineDir.x})
        : vecNorm2({-lineDir.y, lineDir.x}));
}

Maybe<Hit2D> intersectRayCircle(Ray2D ray, Circle circle)
{
    HG_ASSERT(ray.dir != Vec2{0});

    Vec2 rel = ray.pos - circle.pos;
    f32 a = vecDot2(ray.dir, ray.dir);
    f32 b = 2 * vecDot2(ray.dir, rel);
    f32 c = vecDot2(rel, rel) - square(circle.radius);

    f32 det = square(b) - 4 * a * c;
    if (det < 0)
        return {};
    f32 rtdet = sqrtf(det);

    f32 t = (-b - rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        t = (-b + rtdet) / (2 * a);
    if (t < -FLT_EPSILON)
        return {};

    return some<Hit2D>(t, (ray.pos + t * ray.dir - circle.pos) / circle.radius);
}

Maybe<Hit2D> intersectRayRect(Ray2D ray, Rect rect)
{
    HG_ASSERT(ray.dir != Vec2{0});
    if (vecEq2(rect.begin, rect.end))
        return {};

    if (containsPointRect(ray.pos, rect))
    {
        return some<Hit2D>(0.0f, -ray.dir);
    }

    f32 hits[4] = {
        (rect.begin.x - ray.pos.x) / ray.dir.x,
        (rect.begin.y - ray.pos.y) / ray.dir.y,
        (rect.end.x - ray.pos.x) / ray.dir.x,
        (rect.end.y - ray.pos.y) / ray.dir.y,
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
        return {};

    return some<Hit2D>(t, norm);
}

Maybe<Hit2D> intersectLines2D(Line2D line, Line2D other)
{
    if (vecEq2(line.begin, line.end) || vecEq2(other.begin, other.end))
        return {};

    Vec2 lineDir = line.end - line.begin;
    Vec2 otherDir = other.end - other.begin;

    f32 denom = vecCross2(lineDir, otherDir);
    if (std::abs(denom) < FLT_EPSILON)
        return {};

    Vec2 diff = other.begin - line.begin;

    f32 t = vecCross2(diff, otherDir) / denom;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return {};

    f32 tOther = vecCross2(diff, lineDir) / denom;
    if (tOther < -FLT_EPSILON || tOther > 1 + FLT_EPSILON)
        return {};

    return some<Hit2D>(t, denom < 0
        ? vecNorm2({otherDir.y, -otherDir.x})
        : vecNorm2({-otherDir.y, otherDir.x}));
}

Maybe<Hit2D> intersectLineRay2D(Line2D line, Ray2D ray)
{
    if (vecEq2(line.begin, line.end))
        return {};
    HG_ASSERT(ray.dir != Vec2{0});

    Vec2 lineDir = line.end - line.begin;

    f32 denom = vecCross2(lineDir, ray.dir);
    if (std::abs(denom) < FLT_EPSILON)
        return {};

    Vec2 diff = ray.pos - line.begin;

    f32 t = vecCross2(diff, ray.dir) / denom;
    if (t < -FLT_EPSILON || t > 1 + FLT_EPSILON)
        return {};

    f32 tRay = vecCross2(diff, lineDir) / denom;
    if (tRay < -FLT_EPSILON)
        return {};

    return some<Hit2D>(t, denom < 0
        ? vecNorm2({ray.dir.y, -ray.dir.x})
        : vecNorm2({-ray.dir.y, ray.dir.x}));
}

Maybe<Hit2D> intersectLineCircle(Line2D line, Circle circle)
{
    Vec2 dir = line.end - line.begin;

    Vec2 rel = line.begin - circle.pos;
    f32 a = vecDot2(dir, dir);
    f32 b = 2 * vecDot2(dir, rel);
    f32 c = vecDot2(rel, rel) - square(circle.radius);

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

    return some<Hit2D>(t, (line.begin + t * dir - circle.pos) / circle.radius);
}

Maybe<Hit2D> intersectLineRect(Line2D line, Rect rect)
{
    if (vecEq2(line.begin, line.end) || vecEq2(rect.begin, rect.end))
        return {};

    f32 hits[4] = {
        (rect.begin.x - line.begin.x) / (line.end.x - line.begin.x),
        (rect.begin.y - line.begin.y) / (line.end.y - line.begin.y),
        (rect.end.x - line.begin.x) / (line.end.x - line.begin.x),
        (rect.end.y - line.begin.y) / (line.end.y - line.begin.y),
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
        return {};

    return some<Hit2D>(t, norm);
}

} // namespace hg

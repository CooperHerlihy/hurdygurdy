#include "hg/geometry2d.hpp"

#include <cmath>
#include <algorithm>

namespace hg {

bool containsPoint(Vec2 point, Circle circle)
{
    return distance(point, circle) <= epsilonf;
}

f32 distance(Vec2 point, Circle circle)
{
    return length(point - circle.pos) - circle.radius;
}

Vec2 closestPoint(Vec2 pos, Circle circle)
{
    return circle.pos + circle.radius * normalize(pos - circle.pos);
}

bool intersect(Circle a, Circle b)
{
    return distance(a, b) <= epsilonf;
}

f32 distance(Circle a, Circle b)
{
    return length(a.pos - b.pos) - a.radius - b.radius;
}

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

Vec2 closestPoint(Vec2 pos, Rect rect)
{
    return {
        std::clamp(pos.x(), rect.begin.x(), rect.end.x()),
        std::clamp(pos.y(), rect.begin.y(), rect.end.y()),
    };
}

bool intersect(Rect a, Rect b)
{
    return a.end.x() >= b.begin.x() && a.begin.x() <= b.end.x()
        && a.end.y() >= b.begin.y() && a.begin.y() <= b.end.y();
}

bool intersect(Rect rect, Circle circle)
{
    return containsPoint(closestPoint(circle.pos, rect), circle);
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

} // namespace hg

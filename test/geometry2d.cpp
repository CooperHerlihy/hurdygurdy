#include "tests.hpp"
#include "hg/geometry2d.hpp"

#include <cfloat>

using namespace hg;

TEST(testCircleContainsPoint)
{
    Circle c{{0, 0}, 5};
    ASSERT(containsPoint({0, 0}, c));
    ASSERT(containsPoint({3, 4}, c));
    ASSERT(containsPoint({5, 0}, c));
    ASSERT(!containsPoint({5.01f, 0}, c));
}

TEST(testCircleZeroRadius)
{
    Circle z{{0, 0}, 0};
    ASSERT(containsPoint({0, 0}, z));
    ASSERT(!containsPoint({0.01f, 0}, z));
}

TEST(testCircleDistPoint)
{
    Circle c{{0, 0}, 5};
    ASSERT(std::abs(distance({0, 0}, c) - (-5.0f)) < FLT_EPSILON);
    ASSERT(std::abs(distance({5, 0}, c)) < FLT_EPSILON);
    ASSERT(std::abs(distance({10, 0}, c) - 5.0f) < FLT_EPSILON);
}

TEST(testCircleClosestPoint)
{
    Circle c{{0, 0}, 5};
    Vec2 p = closestPoint({10, 0}, c);
    ASSERT(std::abs(p.x() - 5.0f) < FLT_EPSILON && std::abs(p.y()) < FLT_EPSILON);
}

TEST(testCircleIntersect)
{
    Circle a{{0, 0}, 5};
    Circle b{{8, 0}, 3};
    ASSERT(intersect(a, b));
    Circle miss{{20, 0}, 1};
    ASSERT(!intersect(a, miss));
    ASSERT(std::abs(distance(a, b)) < FLT_EPSILON);
}

TEST(testRectEmpty)
{
    Rect r = emptyRect();
    ASSERT(!containsPoint({0, 0}, r));
}

TEST(testRectAddPoint)
{
    Rect r = emptyRect();
    r = rectAddPoint(r, {2, 3});
    ASSERT(containsPoint({2, 3}, r));
    r = rectAddPoint(r, {5, 7});
    ASSERT(containsPoint({3, 4}, r));
}

TEST(testRectNegativeRegion)
{
    Rect r = emptyRect();
    r = rectAddPoint(r, {-2, -3});
    r = rectAddPoint(r, {5, 5});
    ASSERT(containsPoint({0, 0}, r));
    ASSERT(!containsPoint({6, 0}, r));
}

TEST(testRectContainsPointBoundary)
{
    Rect r{{0, 0}, {10, 5}};
    ASSERT(containsPoint({0, 0}, r));
    ASSERT(containsPoint({10, 5}, r));
    ASSERT(!containsPoint({-0.01f, 0}, r));
}

TEST(testRectClosestPoint)
{
    Rect r{{0, 0}, {10, 10}};
    Vec2 p1 = closestPoint({-5, 5}, r);
    ASSERT(p1.x() == 0 && p1.y() == 5);
    Vec2 p2 = closestPoint({15, 5}, r);
    ASSERT(p2.x() == 10 && p2.y() == 5);
    Vec2 p3 = closestPoint({5, -3}, r);
    ASSERT(p3.x() == 5 && p3.y() == 0);
    Vec2 p4 = closestPoint({5, 5}, r);
    ASSERT(p4.x() == 5 && p4.y() == 5);
}

TEST(testRectIntersect)
{
    Rect a{{0, 0}, {10, 10}};
    Rect b{{5, 5}, {15, 15}};
    ASSERT(intersect(a, b));
    Rect c{{20, 20}, {30, 30}};
    ASSERT(!intersect(a, c));
}

TEST(testRectCircleIntersect)
{
    Rect r{{0, 0}, {10, 10}};
    Circle c{{5, 5}, 3};
    ASSERT(intersect(r, c));
    Circle far{{20, 20}, 1};
    ASSERT(!intersect(r, far));
}

TEST(testIntersectRays2D)
{
    Ray2D a{{0, 0}, {1, 0}};
    Ray2D b{{0, 0}, {0, 1}};
    std::optional<Hit2D> hit = intersect(a, b);
    ASSERT(hit.has_value());
}

TEST(testIntersectRays2DParallel)
{
    Ray2D a{{0, 0}, {1, 0}};
    Ray2D b{{0, 1}, {1, 0}};
    ASSERT(!intersect(a, b).has_value());
}

TEST(testIntersectRayLine2D)
{
    Ray2D ray{{0, 0}, {1, 0}};
    Line2D line{{5, -1}, {5, 1}};
    std::optional<Hit2D> hit = intersect(ray, line);
    ASSERT(hit.has_value());
    if (hit.has_value())
        ASSERT(std::abs(hit->dist - 5.0f) < FLT_EPSILON);
}

TEST(testIntersectRayLine2DBehind)
{
    Ray2D ray{{0, 0}, {1, 0}};
    Line2D line{{-5, -1}, {-5, 1}};
    ASSERT(!intersect(ray, line).has_value());
}

TEST(testIntersectRayCircle)
{
    Ray2D ray{{0, 0}, {1, 0}};
    Circle c{{10, 0}, 3};
    std::optional<Hit2D> hit = intersect(ray, c);
    ASSERT(hit.has_value());
    if (hit.has_value())
        ASSERT(std::abs(hit->dist - 7.0f) < FLT_EPSILON);
}

TEST(testIntersectRayCircleMiss)
{
    Ray2D ray{{0, 0}, {1, 0}};
    Circle c{{10, 5}, 1};
    ASSERT(!intersect(ray, c).has_value());
}

TEST(testIntersectRayRect)
{
    Ray2D ray{{-5, 5}, {1, 0}};
    Rect r{{0, 0}, {10, 10}};
    std::optional<Hit2D> hit = intersect(ray, r);
    ASSERT(hit.has_value());
}

TEST(testIntersectLines2D)
{
    Line2D a{{0, 0}, {10, 0}};
    Line2D b{{5, -5}, {5, 5}};
    ASSERT(intersect(a, b).has_value());
}

TEST(testIntersectLines2DParallel)
{
    Line2D a{{0, 0}, {10, 0}};
    Line2D b{{0, 1}, {10, 1}};
    ASSERT(!intersect(a, b).has_value());
}

TEST(testIntersectLineCircle)
{
    Line2D line{{-10, 3}, {10, 3}};
    Circle c{{0, 5}, 3};
    ASSERT(intersect(line, c).has_value());
}

TEST(testIntersectLineRect)
{
    Line2D line{{-5, 5}, {15, 5}};
    Rect r{{0, 0}, {10, 10}};
    ASSERT(intersect(line, r).has_value());
}

TEST(testCircleContainment)
{
    Circle outer{{0, 0}, 10};
    Circle inner{{0, 0}, 3};
    ASSERT(intersect(outer, inner));
    Circle far{{50, 50}, 1};
    ASSERT(!intersect(outer, far));
}

TEST(testRectContainment)
{
    Rect outer{{0, 0}, {20, 20}};
    Rect inner{{5, 5}, {15, 15}};
    ASSERT(intersect(outer, inner));
    Rect far{{100, 100}, {110, 110}};
    ASSERT(!intersect(outer, far));
}

TEST(testRectIntersectEdgeTouching)
{
    Rect a{{0, 0}, {10, 10}};
    Rect b{{10, 0}, {20, 10}};
    ASSERT(intersect(a, b));
    Rect c{{0, 10}, {10, 20}};
    ASSERT(intersect(a, c));
}

TEST(testRectIntersectIdentical)
{
    Rect r{{3, 3}, {7, 7}};
    ASSERT(intersect(r, r));
}

TEST(testRay2DMissRect)
{
    Ray2D ray{{0, 20}, {1, 0}};
    Rect r{{5, -5}, {15, 5}};
    ASSERT(!intersect(ray, r).has_value());
}

TEST(testRay2DMissCircle)
{
    Ray2D ray{{0, 0}, {0, 1}};
    Circle c{{10, 0}, 2};
    ASSERT(!intersect(ray, c).has_value());
}

TEST(testIntersectRays2DCoincident)
{
    Ray2D ray{{0, 0}, {1, 0}};
    Ray2D same{{0, 0}, {1, 0}};
    ASSERT(!intersect(ray, same).has_value());
}

TEST(testIntersectLines2DCoincident)
{
    Line2D a{{0, 0}, {10, 0}};
    Line2D b{{0, 0}, {10, 0}};
    ASSERT(!intersect(a, b).has_value());
}

TEST(testCircleDistPointOnBoundary)
{
    Circle c{{0, 0}, 5};
    f32 d = distance({5, 0}, c);
    ASSERT(std::abs(d) < FLT_EPSILON);
}

TEST(testRectClosestPointInside)
{
    Rect r{{0, 0}, {10, 10}};
    Vec2 p = closestPoint({5, 5}, r);
    ASSERT(p.x() == 5 && p.y() == 5);
}

TEST(testIntersectLineCircleMiss)
{
    Line2D line{{-10, 20}, {10, 20}};
    Circle c{{0, 0}, 3};
    ASSERT(!intersect(line, c).has_value());
}

TEST(testIntersectLineRectMiss)
{
    Line2D line{{-10, 20}, {10, 20}};
    Rect r{{0, 0}, {10, 10}};
    ASSERT(!intersect(line, r).has_value());
}

TEST(testIntersectRayCircleTangent)
{
    Ray2D ray{{-10, 5}, {1, 0}};
    Circle c{{0, 0}, 5};
    ASSERT(intersect(ray, c).has_value());
}

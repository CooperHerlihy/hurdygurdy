#include "tests.hpp"
#include "hg/geometry2d.hpp"

using namespace hg;

TEST(testCircleContainsPoint)
{
    Circle c{{0, 0}, 5};
    ASSERT(containsPointCircle({0, 0}, c));
    ASSERT(containsPointCircle({3, 4}, c));
    ASSERT(containsPointCircle({5, 0}, c));
    ASSERT(!containsPointCircle({5.01f, 0}, c));
}

TEST(testCircleZeroRadius)
{
    Circle z{{0, 0}, 0};
    ASSERT(containsPointCircle({0, 0}, z));
    ASSERT(!containsPointCircle({0.01f, 0}, z));
}

TEST(testCircleDistPoint)
{
    Circle c{{0, 0}, 5};
    ASSERT(std::abs(distPointCircle({0, 0}, c) - (-5.0f)) < FLT_EPSILON);
    ASSERT(std::abs(distPointCircle({5, 0}, c)) < FLT_EPSILON);
    ASSERT(std::abs(distPointCircle({10, 0}, c) - 5.0f) < FLT_EPSILON);
}

TEST(testCircleClosestPoint)
{
    Circle c{{0, 0}, 5};
    Vec2 p = closestPointCircle({10, 0}, c);
    ASSERT(std::abs(p.x - 5.0f) < FLT_EPSILON && std::abs(p.y) < FLT_EPSILON);
}

TEST(testCircleIntersect)
{
    Circle a{{0, 0}, 5};
    Circle b{{8, 0}, 3};
    ASSERT(intersectCircles(a, b));
    Circle miss{{20, 0}, 1};
    ASSERT(!intersectCircles(a, miss));
    ASSERT(std::abs(distCircles(a, b)) < FLT_EPSILON);
}

TEST(testRectEmpty)
{
    Rect r = rectEmpty();
    ASSERT(!containsPointRect({0, 0}, r));
}

TEST(testRectAddPoint)
{
    Rect r = rectEmpty();
    r = rectAddPoint(r, {2, 3});
    ASSERT(containsPointRect({2, 3}, r));
    r = rectAddPoint(r, {5, 7});
    ASSERT(containsPointRect({3, 4}, r));
}

TEST(testRectNegativeRegion)
{
    Rect r = rectEmpty();
    r = rectAddPoint(r, {-2, -3});
    r = rectAddPoint(r, {5, 5});
    ASSERT(containsPointRect({0, 0}, r));
    ASSERT(!containsPointRect({6, 0}, r));
}

TEST(testRectContainsPointBoundary)
{
    Rect r{{0, 0}, {10, 5}};
    ASSERT(containsPointRect({0, 0}, r));
    ASSERT(containsPointRect({10, 5}, r));
    ASSERT(!containsPointRect({-0.01f, 0}, r));
}

TEST(testRectClosestPoint)
{
    Rect r{{0, 0}, {10, 10}};
    Vec2 p1 = closestPointRect({-5, 5}, r);
    ASSERT(p1.x == 0 && p1.y == 5);
    Vec2 p2 = closestPointRect({15, 5}, r);
    ASSERT(p2.x == 10 && p2.y == 5);
    Vec2 p3 = closestPointRect({5, -3}, r);
    ASSERT(p3.x == 5 && p3.y == 0);
    Vec2 p4 = closestPointRect({5, 5}, r);
    ASSERT(p4.x == 5 && p4.y == 5);
}

TEST(testRectIntersect)
{
    Rect a{{0, 0}, {10, 10}};
    Rect b{{5, 5}, {15, 15}};
    ASSERT(intersectRects(a, b));
    Rect c{{20, 20}, {30, 30}};
    ASSERT(!intersectRects(a, c));
}

TEST(testRectCircleIntersect)
{
    Rect r{{0, 0}, {10, 10}};
    Circle c{{5, 5}, 3};
    ASSERT(intersectRectCircle(r, c));
    Circle far{{20, 20}, 1};
    ASSERT(!intersectRectCircle(r, far));
}

TEST(testIntersectRays2D)
{
    Ray2D a{{0, 0}, {1, 0}};
    Ray2D b{{0, 0}, {0, 1}};
    Maybe<Hit2D> hit = intersectRays2D(a, b);
    ASSERT(hit.has);
}

TEST(testIntersectRays2DParallel)
{
    Ray2D a{{0, 0}, {1, 0}};
    Ray2D b{{0, 1}, {1, 0}};
    ASSERT(!intersectRays2D(a, b).has);
}

TEST(testIntersectRayLine2D)
{
    Ray2D ray{{0, 0}, {1, 0}};
    Line2D line{{5, -1}, {5, 1}};
    Maybe<Hit2D> hit = intersectRayLine2D(ray, line);
    ASSERT(hit.has);
    if (hit.has)
        ASSERT(std::abs(hit.val.dist - 5.0f) < FLT_EPSILON);
}

TEST(testIntersectRayLine2DBehind)
{
    Ray2D ray{{0, 0}, {1, 0}};
    Line2D line{{-5, -1}, {-5, 1}};
    ASSERT(!intersectRayLine2D(ray, line).has);
}

TEST(testIntersectRayCircle)
{
    Ray2D ray{{0, 0}, {1, 0}};
    Circle c{{10, 0}, 3};
    Maybe<Hit2D> hit = intersectRayCircle(ray, c);
    ASSERT(hit.has);
    if (hit.has)
        ASSERT(std::abs(hit.val.dist - 7.0f) < FLT_EPSILON);
}

TEST(testIntersectRayCircleMiss)
{
    Ray2D ray{{0, 0}, {1, 0}};
    Circle c{{10, 5}, 1};
    ASSERT(!intersectRayCircle(ray, c).has);
}

TEST(testIntersectRayRect)
{
    Ray2D ray{{-5, 5}, {1, 0}};
    Rect r{{0, 0}, {10, 10}};
    Maybe<Hit2D> hit = intersectRayRect(ray, r);
    ASSERT(hit.has);
}

TEST(testIntersectLines2D)
{
    Line2D a{{0, 0}, {10, 0}};
    Line2D b{{5, -5}, {5, 5}};
    ASSERT(intersectLines2D(a, b).has);
}

TEST(testIntersectLines2DParallel)
{
    Line2D a{{0, 0}, {10, 0}};
    Line2D b{{0, 1}, {10, 1}};
    ASSERT(!intersectLines2D(a, b).has);
}

TEST(testIntersectLineCircle)
{
    Line2D line{{-10, 3}, {10, 3}};
    Circle c{{0, 5}, 3};
    ASSERT(intersectLineCircle(line, c).has);
}

TEST(testIntersectLineRect)
{
    Line2D line{{-5, 5}, {15, 5}};
    Rect r{{0, 0}, {10, 10}};
    ASSERT(intersectLineRect(line, r).has);
}

TEST(testCircleContainment)
{
    Circle outer{{0, 0}, 10};
    Circle inner{{0, 0}, 3};
    ASSERT(intersectCircles(outer, inner));
    Circle far{{50, 50}, 1};
    ASSERT(!intersectCircles(outer, far));
}

TEST(testRectContainment)
{
    Rect outer{{0, 0}, {20, 20}};
    Rect inner{{5, 5}, {15, 15}};
    ASSERT(intersectRects(outer, inner));
    Rect far{{100, 100}, {110, 110}};
    ASSERT(!intersectRects(outer, far));
}

TEST(testRectIntersectEdgeTouching)
{
    Rect a{{0, 0}, {10, 10}};
    Rect b{{10, 0}, {20, 10}};
    ASSERT(intersectRects(a, b));
    Rect c{{0, 10}, {10, 20}};
    ASSERT(intersectRects(a, c));
}

TEST(testRectIntersectIdentical)
{
    Rect r{{3, 3}, {7, 7}};
    ASSERT(intersectRects(r, r));
}

TEST(testRay2DMissRect)
{
    Ray2D ray{{0, 20}, {1, 0}};
    Rect r{{5, -5}, {15, 5}};
    ASSERT(!intersectRayRect(ray, r).has);
}

TEST(testRay2DMissCircle)
{
    Ray2D ray{{0, 0}, {0, 1}};
    Circle c{{10, 0}, 2};
    ASSERT(!intersectRayCircle(ray, c).has);
}

TEST(testIntersectRays2DCoincident)
{
    Ray2D ray{{0, 0}, {1, 0}};
    Ray2D same{{0, 0}, {1, 0}};
    ASSERT(!intersectRays2D(ray, same).has);
}

TEST(testIntersectLines2DCoincident)
{
    Line2D a{{0, 0}, {10, 0}};
    Line2D b{{0, 0}, {10, 0}};
    ASSERT(!intersectLines2D(a, b).has);
}

TEST(testCircleDistPointOnBoundary)
{
    Circle c{{0, 0}, 5};
    f32 d = distPointCircle({5, 0}, c);
    ASSERT(std::abs(d) < FLT_EPSILON);
}

TEST(testRectClosestPointInside)
{
    Rect r{{0, 0}, {10, 10}};
    Vec2 p = closestPointRect({5, 5}, r);
    ASSERT(p.x == 5 && p.y == 5);
}

TEST(testIntersectLineCircleMiss)
{
    Line2D line{{-10, 20}, {10, 20}};
    Circle c{{0, 0}, 3};
    ASSERT(!intersectLineCircle(line, c).has);
}

TEST(testIntersectLineRectMiss)
{
    Line2D line{{-10, 20}, {10, 20}};
    Rect r{{0, 0}, {10, 10}};
    ASSERT(!intersectLineRect(line, r).has);
}

TEST(testIntersectRayCircleTangent)
{
    Ray2D ray{{-10, 5}, {1, 0}};
    Circle c{{0, 0}, 5};
    ASSERT(intersectRayCircle(ray, c).has);
}

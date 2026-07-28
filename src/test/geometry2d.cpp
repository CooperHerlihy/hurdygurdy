#include "tests.hpp"

void testGeometry2D()
{
    // Circle
    {
        Circle c{{0, 0}, 5};

        // containsPointCircle
        {
            TEST(containsPointCircle({0, 0}, c));
            TEST(containsPointCircle({3, 4}, c));
            TEST(containsPointCircle({5, 0}, c));
            TEST(!containsPointCircle({5.01f, 0}, c));
        }

        // Zero radius
        {
            Circle z{{0, 0}, 0};
            TEST(containsPointCircle({0, 0}, z));
            TEST(!containsPointCircle({0.01f, 0}, z));
        }

        // distPointCircle
        {
            TEST(std::abs(distPointCircle({0, 0}, c) - (-5.0f)) < FLT_EPSILON);
            TEST(std::abs(distPointCircle({5, 0}, c)) < FLT_EPSILON);
            TEST(std::abs(distPointCircle({10, 0}, c) - 5.0f) < FLT_EPSILON);
        }

        // closestPointCircle
        {
            Vec2 p = closestPointCircle({10, 0}, c);
            TEST(std::abs(p.x - 5.0f) < FLT_EPSILON && std::abs(p.y) < FLT_EPSILON);
        }

        // intersectCircles / distCircles
        {
            Circle a{{0, 0}, 5};
            Circle b{{8, 0}, 3};
            TEST(intersectCircles(a, b));
            Circle miss{{20, 0}, 1};
            TEST(!intersectCircles(a, miss));
            TEST(std::abs(distCircles(a, b)) < FLT_EPSILON);
        }
    }

    // Rect
    {
        // rectEmpty
        {
            Rect r = rectEmpty();
            TEST(!containsPointRect({0, 0}, r));
        }

        // rectAddPoint
        {
            Rect r = rectEmpty();
            r = rectAddPoint(r, {2, 3});
            TEST(containsPointRect({2, 3}, r));
            r = rectAddPoint(r, {5, 7});
            TEST(containsPointRect({3, 4}, r));
        }

        // Negative region
        {
            Rect r = rectEmpty();
            r = rectAddPoint(r, {-2, -3});
            r = rectAddPoint(r, {5, 5});
            TEST(containsPointRect({0, 0}, r));
            TEST(!containsPointRect({6, 0}, r));
        }

        // containsPointRect boundary
        {
            Rect r{{0, 0}, {10, 5}};
            TEST(containsPointRect({0, 0}, r));
            TEST(containsPointRect({10, 5}, r));
            TEST(!containsPointRect({-0.01f, 0}, r));
        }

        // closestPointRect
        {
            Rect r{{0, 0}, {10, 10}};
            Vec2 p1 = closestPointRect({-5, 5}, r);
            TEST(p1.x == 0 && p1.y == 5);
            Vec2 p2 = closestPointRect({15, 5}, r);
            TEST(p2.x == 10 && p2.y == 5);
            Vec2 p3 = closestPointRect({5, -3}, r);
            TEST(p3.x == 5 && p3.y == 0);
            Vec2 p4 = closestPointRect({5, 5}, r);
            TEST(p4.x == 5 && p4.y == 5);
        }

        // intersectRects
        {
            Rect a{{0, 0}, {10, 10}};
            Rect b{{5, 5}, {15, 15}};
            TEST(intersectRects(a, b));
            Rect c{{20, 20}, {30, 30}};
            TEST(!intersectRects(a, c));
        }

        // intersectRectCircle
        {
            Rect r{{0, 0}, {10, 10}};
            Circle c{{5, 5}, 3};
            TEST(intersectRectCircle(r, c));
            Circle far{{20, 20}, 1};
            TEST(!intersectRectCircle(r, far));
        }
    }

    // 2D intersections
    {
        // intersectRays2D
        {
            Ray2D a{{0, 0}, {1, 0}};
            Ray2D b{{0, 0}, {0, 1}};
            Maybe<Hit2D> hit = intersectRays2D(a, b);
            TEST(hit.has);
        }

        // intersectRays2D - parallel
        {
            Ray2D a{{0, 0}, {1, 0}};
            Ray2D b{{0, 1}, {1, 0}};
            TEST(!intersectRays2D(a, b).has);
        }

        // intersectRayLine2D
        {
            Ray2D ray{{0, 0}, {1, 0}};
            Line2D line{{5, -1}, {5, 1}};
            Maybe<Hit2D> hit = intersectRayLine2D(ray, line);
            TEST(hit.has);
            if (hit.has)
                TEST(std::abs(hit.val.dist - 5.0f) < FLT_EPSILON);
        }

        // intersectRayLine2D - behind
        {
            Ray2D ray{{0, 0}, {1, 0}};
            Line2D line{{-5, -1}, {-5, 1}};
            TEST(!intersectRayLine2D(ray, line).has);
        }

        // intersectRayCircle
        {
            Ray2D ray{{0, 0}, {1, 0}};
            Circle c{{10, 0}, 3};
            Maybe<Hit2D> hit = intersectRayCircle(ray, c);
            TEST(hit.has);
            if (hit.has)
                TEST(std::abs(hit.val.dist - 7.0f) < FLT_EPSILON);
        }

        // intersectRayCircle - miss
        {
            Ray2D ray{{0, 0}, {1, 0}};
            Circle c{{10, 5}, 1};
            TEST(!intersectRayCircle(ray, c).has);
        }

        // intersectRayRect
        {
            Ray2D ray{{-5, 5}, {1, 0}};
            Rect r{{0, 0}, {10, 10}};
            Maybe<Hit2D> hit = intersectRayRect(ray, r);
            TEST(hit.has);
        }

        // intersectLines2D
        {
            Line2D a{{0, 0}, {10, 0}};
            Line2D b{{5, -5}, {5, 5}};
            TEST(intersectLines2D(a, b).has);
        }

        // intersectLines2D - parallel
        {
            Line2D a{{0, 0}, {10, 0}};
            Line2D b{{0, 1}, {10, 1}};
            TEST(!intersectLines2D(a, b).has);
        }

        // intersectLineCircle
        {
            Line2D line{{-10, 3}, {10, 3}};
            Circle c{{0, 5}, 3};
            TEST(intersectLineCircle(line, c).has);
        }

        // intersectLineRect
        {
            Line2D line{{-5, 5}, {15, 5}};
            Rect r{{0, 0}, {10, 10}};
            TEST(intersectLineRect(line, r).has);
        }
    }
}


#include "tests.hpp"
#include "hg/geometry3d.hpp"

void testGeometry3D()
{
    // ============================================================================
    // Geometry 3D
    // ============================================================================
    //
    // Sphere, Box, Plane, and 3D intersection tests.

    // ------------------------------------------------------------------
    // Sphere
    // ------------------------------------------------------------------

    {
        Sphere s{{0, 0, 0}, 5};

        // containsPointSphere
        {
            TEST(containsPointSphere({0, 0, 0}, s));
            TEST(containsPointSphere({5, 0, 0}, s));
            TEST(!containsPointSphere({5.01f, 0, 0}, s));
        }

        // distPointSphere
        {
            TEST(std::abs(distPointSphere({0, 0, 0}, s) - (-5.0f)) < FLT_EPSILON);
            TEST(std::abs(distPointSphere({5, 0, 0}, s)) < FLT_EPSILON);
        }

        // closestPointSphere
        {
            Vec3 p = closestPointSphere({10, 0, 0}, s);
            TEST(std::abs(p.x - 5.0f) < FLT_EPSILON);
        }

        // intersectSpheres / distSpheres
        {
            Sphere b{{8, 0, 0}, 3};
            TEST(intersectSpheres(s, b));
            Sphere miss{{20, 0, 0}, 1};
            TEST(!intersectSpheres(s, miss));
            TEST(std::abs(distSpheres(s, b)) < FLT_EPSILON);
        }
    }

    // ------------------------------------------------------------------
    // Box
    // ------------------------------------------------------------------

    {
        // boxEmpty
        {
            Box b = boxEmpty();
            TEST(!containsPointBox({0, 0, 0}, b));
        }

        // boxAddPoint
        {
            Box b = boxEmpty();
            b = boxAddPoint(b, {1, 2, 3});
            b = boxAddPoint(b, {4, 5, 6});
            TEST(containsPointBox({2, 3, 4}, b));
        }

        // containsPointBox / closestPointBox
        {
            Box b{{0, 0, 0}, {10, 10, 10}};
            TEST(containsPointBox({5, 5, 5}, b));
            TEST(containsPointBox({0, 0, 0}, b));
            TEST(containsPointBox({10, 10, 10}, b));
            TEST(!containsPointBox({-0.01f, 5, 5}, b));
            Vec3 p = closestPointBox({-5, 5, 5}, b);
            TEST(p.x == 0 && p.y == 5 && p.z == 5);
        }

        // intersectBox
        {
            Box a{{0, 0, 0}, {10, 10, 10}};
            Box b{{5, 5, 5}, {15, 15, 15}};
            TEST(intersectBox(a, b));
            Box c{{20, 20, 20}, {30, 30, 30}};
            TEST(!intersectBox(a, c));
        }

        // intersectBoxSphere
        {
            Box b{{0, 0, 0}, {10, 10, 10}};
            Sphere s{{5, 5, 5}, 3};
            TEST(intersectBoxSphere(b, s));
            Sphere far{{20, 20, 20}, 1};
            TEST(!intersectBoxSphere(b, far));
        }
    }

    // ------------------------------------------------------------------
    // Plane
    // ------------------------------------------------------------------

    {
        // planeFromPoint
        {
            Plane p = planeFromPoint({0, 5, 0}, {0, 1, 0});
            TEST(vecEq3(p.normal, {0, 1, 0}));
            TEST(std::abs(p.dist - 5.0f) < FLT_EPSILON);
        }

        // planeFromTri
        {
            Tri tri{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}};
            Plane p = planeFromTri(tri);
            TEST(std::abs(p.dist) < FLT_EPSILON);
        }
    }

    // ------------------------------------------------------------------
    // Intersection functions
    // ------------------------------------------------------------------

    {
        // intersectRaySphere
        {
            Ray3D ray{{0, 0, 0}, {0, 0, 1}};
            Sphere s{{0, 0, 10}, 3};
            Maybe<Hit3D> hit = intersectRaySphere(ray, s);
            TEST(hit.has);
            if (hit.has)
                TEST(std::abs(hit.val.dist - 7.0f) < FLT_EPSILON);
        }

        // intersectRayBox
        {
            Ray3D ray{{-5, 5, 5}, {1, 0, 0}};
            Box b{{0, 0, 0}, {10, 10, 10}};
            Maybe<Hit3D> hit = intersectRayBox(ray, b);
            TEST(hit.has);
        }

        // intersectRayTri
        {
            Ray3D ray{{0, 0, -5}, {0, 0, 1}};
            Tri tri{{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};
            Maybe<Hit3D> hit = intersectRayTri(ray, tri);
            TEST(hit.has);
            if (hit.has)
                TEST(std::abs(hit.val.dist - 5.0f) < FLT_EPSILON);
        }

        // intersectRayPlane
        {
            Ray3D ray{{0, 0, -5}, {0, 0, 1}};
            Plane p{{0, 0, 1}, 0};
            Maybe<Hit3D> hit = intersectRayPlane(ray, p);
            TEST(hit.has);
            if (hit.has)
                TEST(std::abs(hit.val.dist - 5.0f) < FLT_EPSILON);
        }

        // intersectRayPlane - parallel
        {
            Ray3D ray{{0, 0, 0}, {1, 0, 0}};
            Plane p{{0, 0, 1}, 10};
            TEST(!intersectRayPlane(ray, p).has);
        }

        // intersectLineSphere
        {
            Line3D line{{-10, 3, 0}, {10, 3, 0}};
            Sphere s{{0, 5, 0}, 3};
            TEST(intersectLineSphere(line, s).has);
        }

        // intersectLineBox
        {
            Line3D line{{-5, 5, 5}, {15, 5, 5}};
            Box b{{0, 0, 0}, {10, 10, 10}};
            TEST(intersectLineBox(line, b).has);
        }

        // intersectLineTri
        {
            Line3D line{{0, 0, -5}, {0, 0, 5}};
            Tri tri{{-1, -1, 0}, {1, -1, 0}, {0, 1, 0}};
            TEST(intersectLineTri(line, tri).has);
        }

        // intersectLinePlane
        {
            Line3D line{{0, 0, -5}, {0, 0, 5}};
            Plane p{{0, 0, 1}, 0};
            Maybe<Hit3D> hit = intersectLinePlane(line, p);
            TEST(hit.has);
            if (hit.has)
                TEST(std::abs(hit.val.dist - 0.5f) < FLT_EPSILON);
        }

        // intersectLinePlane - parallel
        {
            Line3D line{{0, 0, -5}, {1, 0, -5}};
            Plane p{{0, 0, 1}, 0};
            TEST(!intersectLinePlane(line, p).has);
        }
    }
}


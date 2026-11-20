#pragma once

#include "hg/inttypes.hpp"
#include "hg/math.hpp"

#include <optional>

namespace hg {

/**
 * A 2D rectangle
 */
struct Rect {
    /**
     * The origin position
     */
    Vec2 begin;
    /**
     * The extent in each dimension
     */
    Vec2 end;
};

/**
 * Returns an empty rect
 */
Rect emptyRect();

/**
 * Expands the rect to include the point
 */
Rect rectAddPoint(Rect rect, Vec2 point);

/**
 * A 2D circle
 */
struct Circle {
    /**
     * The center position
     */
    Vec2 pos;
    /**
     * The radius
     */
    f32 radius;
};

/**
 * A 2D ray
 */
struct Ray2D {
    /**
     * The origin position
     */
    Vec2 pos;
    /**
     * The direction
     */
    Vec2 dir;
};

/**
 * A 2D line
 */
struct Line2D {
    /**
     * The begin vertex
     */
    Vec2 begin;
    /**
     * The end vertex
     */
    Vec2 end;
};

/**
 * 2D intersection info
 */
struct Hit2D {
    /**
     * The hit distance along the ray or line
     *
     * Ray hit pos: pos + dist * dir
     * Line hit pos: begin + dist * (end - begin)
     */
    f32 dist;
    /**
     * The normal at the hit position
     */
    Vec2 normal;
};

/**
 * Returns whether the rect contains the point
 */
bool containsPoint(Vec2 point, Rect rect);

/**
 * Returns whether the circle contains the point
 */
bool containsPoint(Vec2 point, Circle circle);

/**
 * Returns the closest point to pos which lies on the rect
 */
Vec2 closestPoint(Vec2 pos, Rect rect);

/**
 * Returns the closest point to pos which lies on the circle
 */
Vec2 closestPoint(Vec2 pos, Circle circle);

/**
 * Returns the distance between the point and the circle
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 distance(Vec2 point, Circle circle);

/**
 * Returns the distance squared between the circles
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 distance(Circle a, Circle b);

/**
 * Returns whether two rects intersect or not (includes touching)
 */
bool intersect(Rect a, Rect b);

/**
 * Returns whether two circles intersect or not (includes touching)
 */
bool intersect(Circle a, Circle b);

/**
 * Returns whether a rect and a circle intersect or not (includes touching)
 */
bool intersect(Rect rect, Circle circle);

/**
 * Intersect a ray and a rect
 */
std::optional<Hit2D> intersect(Ray2D ray, Rect rect);

/**
 * Intersect a ray and a circle
 */
std::optional<Hit2D> intersect(Ray2D ray, Circle circle);

/**
 * Intersect two rays
 */
std::optional<Hit2D> intersect(Ray2D ray, Ray2D other);

/**
 * Intersect a ray and a line
 */
std::optional<Hit2D> intersect(Ray2D ray, Line2D line);

/**
 * Intersect a line and a rect
 */
std::optional<Hit2D> intersect(Line2D line, Rect rect);

/**
 * Intersect a line and a circle
 */
std::optional<Hit2D> intersect(Line2D line, Circle circle);

/**
 * Intersect a line and a ray
 */
std::optional<Hit2D> intersect(Line2D line, Ray2D ray);

/**
 * Intersect two lines
 */
std::optional<Hit2D> intersect(Line2D line, Line2D other);

/**
 * A 3D box
 */
struct Box {
    /**
     * The origin position
     */
    Vec3 begin;
    /**
     * The extent in each dimension
     */
    Vec3 end;
};

/**
 * Returns an empty box
 */
Box emptyBox();

/**
 * Expands the box to include the point
 */
Box boxAddPoint(Box box, Vec3 point);

 /**
 * A 3D sphere
 */
struct Sphere {
    /**
     * The center position
     */
    Vec3 pos;
    /**
     * The radius from the center
     */
    f32 radius;
};

/**
 * A 3D ray
 */
struct Ray3D {
    /**
     * The origin position
     */
    Vec3 pos;
    /**
     * The direction
     */
    Vec3 dir;
};

/**
 * A 3D line
 */
struct Line3D {
    /**
     * The begin vertex
     */
    Vec3 begin;
    /**
     * The end vertex
     */
    Vec3 end;
};

/**
 * A 3D triangle
 */
struct Triangle {
    /**
     * The first vertex
     */
    Vec3 a;
    /**
     * The second vertex
     */
    Vec3 b;
    /**
     * The third vertex
     */
    Vec3 c;
};

/**
 * A 3D plane
 */
struct Plane {
    /**
     * The plane's normal
     */
    Vec3 normal;
    /**
     * The distance in the direction of the normal
     */
    f32 dist;
};

/**
 * Create a plane at the point
 */
Plane planeFromPoint(Vec3 point, Vec3 normal);

/**
 * Create a plane from a triangle
 */
Plane planeFromTri(Triangle tri);

/**
 * 3D intersection info
 */
struct Hit3D {
    /**
     * The hit distance along the ray or line
     *
     * Ray hit pos: pos + dist * dir
     * Line hit pos: begin + dist * (end - begin)
     */
    f32 dist;
    /**
     * The normal at the hit position
     */
    Vec3 normal;
};

/**
 * Returns whether the box contains the point
 */
bool containsPoint(Vec3 point, Box box);

/**
 * Returns whether the sphere contains the point
 */
bool containsPoint(Vec3 point, Sphere sphere);

/**
 * Returns the closest point to pos which lies on the box
 */
Vec3 closestPoint(Vec3 pos, Box box);

/**
 * Returns the closest point to pos which lies on the sphere
 */
Vec3 closestPoint(Vec3 pos, Sphere sphere);

/**
 * Returns the distance squared between the point and the sphere
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 distance(Vec3 point, Sphere sphere);

/**
 * Returns the distance squared between the spheres
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 distance(Sphere a, Sphere b);

/**
 * Returns whether two boxs intersect or not (includes touching)
 */
bool intersect(Box a, Box b);

/**
 * Returns whether two spheres intersect or not (includes touching)
 */
bool intersect(Sphere a, Sphere b);

/**
 * Returns whether a box and a sphere intersect or not (includes touching)
 */
bool intersect(Box box, Sphere sphere);

/**
 * Intersect a ray and a box
 */
std::optional<Hit3D> intersect(Ray3D ray, Box box);

/**
 * Intersect a ray and a sphere
 */
std::optional<Hit3D> intersect(Ray3D ray, Sphere sphere);

/**
 * Intersect a ray and a triangle
 */
std::optional<Hit3D> intersect(Ray3D ray, Triangle tri);

/**
 * Intersect a ray and a plane
 */
std::optional<Hit3D> intersect(Ray3D ray, Plane plane);

/**
 * Intersect a line and a box
 */
std::optional<Hit3D> intersect(Line3D line, Box box);

/**
 * Intersect a line and a sphere
 */
std::optional<Hit3D> intersect(Line3D line, Sphere sphere);

/**
 * Intersect a line and a triangle
 */
std::optional<Hit3D> intersect(Line3D line, Triangle tri);

/**
 * Intersect a line and a plane
 */
std::optional<Hit3D> intersect(Line3D line, Plane plane);

} // namespace hg

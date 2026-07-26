#pragma once

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
 * Returns whether the circle contains the point
 */
bool containsPointCircle(Vec2 point, Circle circle);

/**
 * Returns the distance between the point and the circle
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 distPointCircle(Vec2 point, Circle circle);

/**
 * Returns the closest point to pos which lies on the circle
 */
Vec2 closestPointCircle(Vec2 pos, Circle circle);

/**
 * Returns whether two circles intersect or not (includes touching)
 */
bool intersectCircles(Circle a, Circle b);

/**
 * Returns the distance squared between the circles
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 distCircles(Circle a, Circle b);

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
Rect rectEmpty();

/**
 * Expands the rect to include the point
 */
Rect rectAddPoint(Rect rect, Vec2 point);

/**
 * Returns whether the rect contains the point
 */
bool containsPointRect(Vec2 point, Rect rect);

/**
 * Returns the closest point to pos which lies on the rect
 */
Vec2 closestPointRect(Vec2 pos, Rect rect);

/**
 * Returns whether two rects intersect or not (includes touching)
 */
bool intersectRects(Rect a, Rect b);

/**
 * Returns whether a rect and a circle intersect or not (includes touching)
 */
bool intersectRectCircle(Rect rect, Circle circle);

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
 * Intersect two rays
 */
Maybe<Hit2D> intersectRays2D(Ray2D ray, Ray2D other);

/**
 * Intersect a ray and a line
 */
Maybe<Hit2D> intersectRayLine2D(Ray2D ray, Line2D line);

/**
 * Intersect a ray and a circle
 */
Maybe<Hit2D> intersectRayCircle(Ray2D ray, Circle circle);

/**
 * Intersect a ray and a rect
 */
Maybe<Hit2D> intersectRayRect(Ray2D ray, Rect rect);

/**
 * Intersect two lines
 */
Maybe<Hit2D> intersectLines2D(Line2D line, Line2D other);

/**
 * Intersect a line and a ray
 */
Maybe<Hit2D> intersectLineRay2D(Line2D line, Ray2D ray);

/**
 * Intersect a line and a circle
 */
Maybe<Hit2D> intersectLineCircle(Line2D line, Circle circle);

/**
 * Intersect a line and a rect
 */
Maybe<Hit2D> intersectLineRect(Line2D line, Rect rect);

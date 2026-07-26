#pragma once

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
 * Returns whether the sphere contains the point
 */
bool containsPointSphere(Vec3 point, Sphere sphere);

/**
 * Returns the distance squared between the point and the sphere
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 distPointSphere(Vec3 point, Sphere sphere);

/**
 * Returns the closest point to pos which lies on the sphere
 */
Vec3 closestPointSphere(Vec3 pos, Sphere sphere);

/**
 * Returns whether two spheres intersect or not (includes touching)
 */
bool intersectSpheres(Sphere a, Sphere b);

/**
 * Returns the distance squared between the spheres
 *
 * Notes returns 0 if touching, and negative if overlapping
 */
f32 distSpheres(Sphere a, Sphere b);

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
Box boxEmpty();

/**
 * Expands the box to include the point
 */
Box boxAddPoint(Box box, Vec3 point);

/**
 * Returns whether the box contains the point
 */
bool containsPointBox(Vec3 point, Box box);

/**
 * Returns the closest point to pos which lies on the box
 */
Vec3 closestPointBox(Vec3 pos, Box box);

/**
 * Returns whether two boxs intersect or not (includes touching)
 */
bool intersectBox(Box a, Box b);

/**
 * Returns whether a box and a sphere intersect or not (includes touching)
 */
bool intersectBoxSphere(Box box, Sphere sphere);

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
struct Tri {
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
Plane planeFromTri(Tri tri);

/**
 * Intersect a ray and a sphere
 */
Maybe<Hit3D> intersectRaySphere(Ray3D ray, Sphere sphere);

/**
 * Intersect a ray and a box
 */
Maybe<Hit3D> intersectRayBox(Ray3D ray, Box box);

/**
 * Intersect a ray and a triangle
 */
Maybe<Hit3D> intersectRayTri(Ray3D ray, Tri tri);

/**
 * Intersect a ray and a plane
 */
Maybe<Hit3D> intersectRayPlane(Ray3D ray, Plane plane);

/**
 * Intersect a line and a sphere
 */
Maybe<Hit3D> intersectLineSphere(Line3D line, Sphere sphere);

/**
 * Intersect a line and a box
 */
Maybe<Hit3D> intersectLineBox(Line3D line, Box box);

/**
 * Intersect a line and a triangle
 */
Maybe<Hit3D> intersectLineTri(Line3D line, Tri tri);

/**
 * Intersect a line and a plane
 */
Maybe<Hit3D> intersectLinePlane(Line3D line, Plane plane);

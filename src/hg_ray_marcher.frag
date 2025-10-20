#version 450

#define MAX_ITERATIONS 100
#define MAX_DISTANCE 1000.0
#define MIN_DISTANCE 0.01

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec3 v_dir;

layout(location = 0) out vec4 out_color;

struct Sphere {
    vec3 center;
    float radius;
};

struct Ray {
    vec3 origin;
    vec3 direction;
};

float sphere_sdf(Sphere sphere, vec3 point) {
    return length(sphere.center - point) - sphere.radius;
}

vec3 sphere_normal(Sphere sphere, vec3 point) {
    return normalize(sphere.center - point);
}

#define SPHERE_COUNT 1
const Sphere spheres[SPHERE_COUNT] = Sphere[](
    Sphere(vec3(0.0, 0.0, 1.0), 0.5)
);

#define LIGHT_COUNT 1
const vec3 lights[LIGHT_COUNT] = vec3[](
    vec3(-2.0, -2.0, 0.0)
);

void main() {
    Ray ray = Ray(v_pos, normalize(v_dir));
    for (int i = 0; i < MAX_ITERATIONS; ++i) {
        float distance = MAX_DISTANCE;
        int sphere_index = SPHERE_COUNT;
        for (int j = 0; j < SPHERE_COUNT; ++j) {
            float sdf = sphere_sdf(spheres[j], ray.origin);
            if (sdf < distance) {
                distance = sdf;
                sphere_index = j;
            }
        }

        if (distance < MIN_DISTANCE) {
            out_color = dot(normalize(ray.origin - lights[0]), sphere_normal(spheres[sphere_index], ray.origin)) * vec4(1.0, 0.0, 0.0, 1.0);
            return;
        }

        if (length(ray.origin - v_pos) >= MAX_DISTANCE) {
            out_color = vec4(0.0, 0.0, 1.0, 1.0);
            return;
        }

        ray.origin += ray.direction * distance;
    }
    out_color = vec4(0.0, 1.0, 0.0, 1.0);
    return;
}

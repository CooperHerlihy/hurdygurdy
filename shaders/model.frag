#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;
layout(location = 3) in float v_roughness;
layout(location = 4) in float v_metalness;

layout(set = 1, binding = 0) uniform sampler2D u_sampler;

const float pi = 3.141592653589793238;

const uint MaxLights = 10;

struct Light {
    vec4 pos;
    vec4 color;
};

layout(set = 0, binding = 1) uniform LightUniform {
    Light vals[MaxLights];
    uint count;
} u_light;

const vec3 ambient = vec3(0.01, 0.01, 0.01);

float square(const float x) {
    return x * x;
}

float ggx(const vec3 normal, const vec3 half_dir, const float roughness) {
    const float cos_half = clamp(dot(half_dir, normal), 0.0, 1.0);
    const float denom = square(cos_half) * (square(roughness) - 1.0) + 1;
    return square(roughness) / (pi * denom * denom);
}

float brdf(const vec3 normal, const vec3 view_dir, const vec3 light_dir, const float roughness, const float metal) {
    const float lambertian = clamp(dot(light_dir, normal), 0.0, 1.0);

    const vec3 half_dir = normalize(view_dir + light_dir);
    const float normal_distribution = ggx(normal, half_dir, roughness);
    const float fresnel = 1.0 / clamp(dot(normal, view_dir), 0.1, 1.0);
    const float specular = normal_distribution * fresnel;

    const float diffuse = 1.0 / pi;

    return lambertian * mix(diffuse, specular, max(metal, 0.15));
}

vec3 calc_light(const vec3 normal, const Light light, const float roughness, const float metal) {
    const vec3 emitted = vec3(0.0);

    const vec3 view_dir = normalize(-v_pos);

    const vec3 rel_pos = light.pos.xyz - v_pos;
    const vec3 light_dir = normalize(rel_pos);

    const float distance_sqrd = dot(rel_pos, rel_pos);
    const vec3 reflected = light.color.xyz * (brdf(normal, view_dir, light_dir, roughness, metal) / distance_sqrd);

    return emitted + reflected;
}

void main() {
    const vec3 normal = normalize(v_normal);

    vec3 total_light = ambient;
    for (uint i = 0u; i < u_light.count; i++) {
        total_light += calc_light(normal, u_light.vals[i], v_roughness, v_metalness);
    }

    const vec4 tex = texture(u_sampler, v_uv);
    const vec3 hdr_color = total_light * tex.xyz;
    const vec3 ldr_color = vec3(1.0) - exp(-hdr_color);
    out_color = vec4(ldr_color, tex.w);
}

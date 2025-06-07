#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_sampler;

const uint MaxLights = 10;

struct Light {
    vec4 pos;
    vec4 color;
};

layout(set = 0, binding = 1) uniform LightUniform {
    Light vals[MaxLights];
    uint count;
} u_light;

const vec3 ambient = vec3(0.05, 0.05, 0.05);

vec3 calc_light(const vec3 normal, const Light light) {
    const vec3 pos = light.pos.xyz;

    const vec3 rel_pos = pos - v_pos;
    const vec3 dir = normalize(rel_pos);
    const float diffuse = max(dot(normal, dir), 0.0);

    const vec3 camera_dir = normalize(-v_pos);
    const vec3 half_dir = normalize(dir + camera_dir);
    const float specular = diffuse * pow(max(dot(half_dir, normal), 0.0), 32.0);

    const float distance_sqrd = dot(rel_pos, rel_pos);
    return light.color.xyz * ((diffuse + specular) / max(distance_sqrd, 0.01));
}

void main() {
    const vec3 normal = normalize(v_normal);

    vec3 total_light = ambient;
    for (uint i = 0u; i < u_light.count; i++) {
        total_light += calc_light(normal, u_light.vals[i]);
    }

    const vec4 tex = texture(u_sampler, v_uv);
    out_color = vec4(tex.xyz * total_light, tex.w);
}

#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(set = 1, binding = 0) uniform sampler2D u_sampler;

const vec3 ambient = vec3(0.05, 0.05, 0.05);

const vec3 light_position = vec3(10.0, -10.0, -10.0);
const vec3 light_color = vec3(1.0, 0.9, 0.7);
const float light_strength = 700.0;

void main() {
    vec3 light_relative_position = light_position - in_position;
    vec3 light_direction = normalize(light_relative_position);
    float light_distance = dot(light_relative_position, light_relative_position);

    float diffuse = max(dot(normalize(in_normal), light_direction), 0.0);
    vec3 light = ambient + light_color * (light_strength / light_distance * diffuse);

    vec4 tex = texture(u_sampler, in_uv);
    out_color = vec4(tex.xyz * light, tex.w);
}

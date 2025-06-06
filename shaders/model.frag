#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_camera;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec2 in_uv;

layout(set = 1, binding = 0) uniform sampler2D u_sampler;

const vec3 ambient = vec3(0.05, 0.05, 0.05);

const vec3 light_pos = vec3(3.0, -3.0, -3.0);
const vec3 light_color = vec3(1.0, 0.9, 0.7);
const float light_strength = 10.0;

void main() {
    vec3 normal = normalize(in_normal);

    vec3 light_rel_pos = light_pos - in_pos;
    vec3 light_dir = normalize(light_rel_pos);
    float light_distance = sqrt(dot(light_rel_pos, light_rel_pos));

    float diffuse = max(dot(normal, light_dir), 0.0);

    float specular = 0.0;
    if (diffuse > 0.0) {
        vec3 camera_dir = normalize(in_camera - in_pos);
        specular = pow(max(dot(normalize(light_dir + camera_dir), normal), 0.0), 32.0);
    }

    vec3 light = ambient + light_color * (light_strength / light_distance * (diffuse + specular));

    vec4 tex = texture(u_sampler, in_uv);
    out_color = vec4(tex.xyz * light, tex.w);
}

#version 460

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec4 v_tangent;
layout(location = 3) in vec2 v_uv;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 0) uniform VPUniform {
    mat4 u_view;
    mat4 u_proj;
    uint u_dir_light_count;
    uint u_point_light_count;
};

struct DirectionalLight {
    vec3 direction;
    vec3 color;
    float intensity;
};
layout(set = 0, binding = 1) readonly buffer DirectionalLights {
    DirectionalLight u_directional_lights[];
};

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};
layout(set = 0, binding = 2) readonly buffer PointLights {
    PointLight u_point_lights[];
};

layout(set = 1, binding = 0) uniform sampler2D u_textures[2];

void main() {
    // mat3 tangent_to_world = mat3(
    //     v_tangent.xyz,
    //     cross(v_tangent.xyz, v_normal) * v_tangent.w,
    //     v_normal
    // );
    // vec3 normal = normalize(tangent_to_world * texture(u_textures[1], v_uv).xyz);
    // vec3 normal = normalize(v_normal);

    vec3 lighting = vec3(0.0);
    // for (uint i = 0; i < u_dir_light_count; ++i) {
    //     vec3 light_dir = normalize(u_directional_lights[i].direction);
    //
    //     float diffuse = max(dot(normal, -light_dir), 0.0);
    //     lighting += diffuse * u_directional_lights[i].intensity * u_directional_lights[i].color;
    // }
    // for (uint i = 0; i < u_point_light_count; ++i) {
    //     vec3 light_pos = (u_view * vec4(u_point_lights[i].position, 1.0)).xyz;
    //     vec3 light_dir = normalize(light_pos - v_pos);
    //     vec3 light_dist = light_pos - v_pos;
    //
    //     float diffuse = max(dot(normal, light_dir), 0.0) / dot(light_dist, light_dist);
    //     lighting += diffuse * u_point_lights[i].intensity * u_point_lights[i].color;
    // }

    vec3 light_dir = normalize(u_directional_lights[0].direction);
    vec4 color = dot(v_normal, -light_dir) * texture(u_textures[0], v_uv);
    out_color = color;
}


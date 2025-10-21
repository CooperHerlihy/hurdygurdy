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
    vec4 direction;
    vec4 color;
};
layout(set = 0, binding = 1) readonly buffer DirectionalLights {
    DirectionalLight u_directional_lights[];
};

struct PointLight {
    vec4 position;
    vec4 color;
};
layout(set = 0, binding = 2) readonly buffer PointLights {
    PointLight u_point_lights[];
};

layout(set = 1, binding = 0) uniform sampler2D u_textures[2];

float blinn_phong(vec3 normal, vec3 light_dir, float shininess) {
    float ambient = 0.03;
    float diffuse = max(dot(normal, normalize(light_dir)), 0.0);
    float specular = pow(max(dot(normal, normalize(light_dir + normalize(-v_pos))), 0.0), shininess);
    return ambient + diffuse + specular;
}

void main() {
    mat3 tangent_to_world = mat3(
        v_tangent.xyz,
        cross(v_tangent.xyz, v_normal) * v_tangent.w,
        -v_normal
    );
    vec3 normal = normalize(tangent_to_world * texture(u_textures[1], v_uv).xyz);

    vec3 lighting = vec3(0.0);
    for (uint i = 0; i < u_dir_light_count; ++i) {
        vec3 light_dir = -normalize(mat3(u_view) * u_directional_lights[i].direction.xyz);
        vec3 light_color = u_directional_lights[i].color.xyz * u_directional_lights[i].color.w;
        lighting += blinn_phong(normal, light_dir, 16.0) * light_color;
    }
    for (uint i = 0; i < u_point_light_count; ++i) {
        vec3 light_pos = (u_view * u_point_lights[i].position).xyz;
        vec3 light_diff = light_pos - v_pos;
        float light_dist = dot(light_diff, light_diff);
        vec3 light_dir = normalize(light_diff);
        vec3 light_color = u_point_lights[i].color.xyz * u_point_lights[i].color.w;
        lighting += blinn_phong(normal, light_dir, 16.0) * light_color / light_dist;
    }

    vec4 hdr_color = vec4(lighting, 1.0) * texture(u_textures[0], v_uv);
    vec4 ldr_color = vec4(1.0) - exp(-hdr_color);
    out_color = ldr_color;
}


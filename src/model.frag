#version 460

layout (location = 0) out vec4 out_color;

layout (location = 0) in vec3 f_pos;
layout (location = 1) in vec3 f_norm;
layout (location = 2) in vec4 f_tan;
layout (location = 3) in vec2 f_uv;

layout (set = 0, binding = 0) uniform UViewProjection {
    mat4 u_proj;
    mat4 u_view;
    uint u_dir_light_count;
    uint u_point_light_count;
};

struct DirectionalLight {
    vec4 dir;
    vec4 color;
};
layout (set = 0, binding = 1) readonly buffer DirectionalLights {
    DirectionalLight u_dir_lights[];
};

struct PointLight {
    vec4 pos;
    vec4 color;
};
layout (set = 0, binding = 2) readonly buffer PointLights {
    PointLight u_point_lights[];
};

// [0] = color map, [1] = normal map
layout (set = 1, binding = 0) uniform sampler2D u_textures[2];

float blinn_phong(vec3 normal, vec3 light_dir, float shininess) {
    float ambient = 0.03;
    float diffuse = max(dot(normal, normalize(light_dir)), 0.0);
    float specular = 0.0; //pow(max(dot(normal, normalize(light_dir + normalize(-f_pos))), 0.0), shininess);
    return ambient + diffuse + specular;
}

void main() {
    mat3 tex_to_model = mat3(
        f_tan.xyz,
        cross(f_tan.xyz, f_norm) * f_tan.w,
        -f_norm
    );
    vec3 normal = normalize(tex_to_model * texture(u_textures[1], f_uv).xyz);

    vec3 lighting = vec3(0.0);
    for (uint i = 0; i < u_dir_light_count; ++i) {
        vec3 light_dir = -normalize((u_dir_lights[i].dir).xyz);
        vec3 light_color = u_dir_lights[i].color.xyz * u_dir_lights[i].color.w;
        lighting += blinn_phong(normal, light_dir, 16.0) * light_color;
    }
    for (uint i = 0; i < u_point_light_count; ++i) {
        vec3 light_pos = (u_point_lights[i].pos).xyz;
        vec3 light_diff = light_pos - f_pos;
        float light_dist = dot(light_diff, light_diff);
        vec3 light_dir = normalize(light_diff);
        vec3 light_color = u_point_lights[i].color.xyz * u_point_lights[i].color.w;
        lighting += blinn_phong(normal, light_dir, 16.0) * light_color / light_dist;
    }

    vec4 hdr_color = vec4(lighting, 1.0) * texture(u_textures[0], f_uv);
    vec4 ldr_color = vec4(1.0) - exp(-hdr_color);
    out_color = hdr_color;
}


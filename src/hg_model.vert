#version 460

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec3 f_position;
layout(location = 1) out vec3 f_normal;
layout(location = 2) out vec4 f_tangent;
layout(location = 3) out vec2 f_uv;

layout(set = 0, binding = 0) uniform VPUniform {
    mat4 u_view;
    mat4 u_proj;
    uint u_dir_light_count;
    uint u_point_light_count;
};

layout(push_constant) uniform ModelPush {
    mat4 p_model;
};

void main() {
    vec4 model_space = vec4(in_pos, 1.0);
    vec4 world_space = p_model * model_space;
    vec4 view_space = u_view * world_space;

    f_position = view_space.xyz;
    f_normal = in_normal;
    f_tangent = in_tangent;
    f_uv = in_uv;

    gl_Position = u_proj * view_space;
}


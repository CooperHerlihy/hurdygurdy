#version 460

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(location = 0) out vec3 f_pos;
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

struct Space {
    vec4 position;
    vec3 normal;
    vec3 tangent;
};

void main() {
    const mat4 mv = u_view * p_model;
    const mat3 imv = mat3(transpose(inverse(mv)));
    const vec4 pos = mv * vec4(in_pos, 1.0);

    f_pos = pos.xyz;
    f_normal = imv * in_normal;
    f_tangent = vec4(imv * in_tangent.xyz, in_tangent.w);
    f_uv = in_uv;

    gl_Position = u_proj * pos;
}


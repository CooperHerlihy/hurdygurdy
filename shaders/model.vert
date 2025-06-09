#version 450

layout(location = 0) out vec3 f_pos;
layout(location = 1) out vec3 f_normal;
layout(location = 2) out vec2 f_uv;
layout(location = 3) out float f_roughness;
layout(location = 4) out float f_metalness;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(set = 0, binding = 0) uniform VP {
    mat4 projection;
    mat4 view;
} u_vp;

layout(push_constant) uniform PushConstant {
    mat4 model;
    float roughness;
    float metalness;
} push;

void main() {
    const mat4 mv = u_vp.view * push.model;
    const vec4 pos = mv * vec4(in_pos, 1.0);

    f_pos = pos.xyz;
    f_normal = mat3(transpose(inverse(mv))) * in_normal;
    f_uv = in_uv;
    f_roughness = push.roughness;
    f_metalness = push.metalness;

    gl_Position = u_vp.projection * pos;
}

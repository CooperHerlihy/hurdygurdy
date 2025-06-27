#version 450

layout(location = 0) out vec3 f_pos;
layout(location = 1) out vec3 f_normal;
layout(location = 2) out vec4 f_tangent;
layout(location = 3) out vec2 f_uv;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec4 in_tangent;
layout(location = 3) in vec2 in_uv;

layout(set = 0, binding = 0) uniform VP {
    mat4 projection;
    mat4 view;
} u_vp;

layout(push_constant) uniform PushConstant {
    mat4 model;
    uint normal_map_index;
    uint texture_index;
    float roughness;
    float metal;
} push;

void main() {
    const mat4 mv = u_vp.view * push.model;
    const vec4 pos = mv * vec4(in_pos, 1.0);

    f_pos = pos.xyz;
    f_normal = mat3(transpose(inverse(mv))) * in_normal;
    f_tangent = vec4(mat3(transpose(inverse(mv))) * in_tangent.xyz, in_tangent.w);
    f_uv = in_uv;

    gl_Position = u_vp.projection * pos;
}

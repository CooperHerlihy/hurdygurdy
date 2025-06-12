#version 450

layout(location = 0) out vec2 f_uv;

layout(location = 0) in vec2 in_pos;

layout(set = 0, binding = 0) uniform VP {
    mat4 projection;
    mat4 view;
} u_vp;

layout(push_constant) uniform PushConstant {
    mat4 model;
} push;

void main() {
    f_uv = in_pos;

    gl_Position = u_vp.projection * u_vp.view * push.model * vec4(in_pos, 5.0, 1.0);
}

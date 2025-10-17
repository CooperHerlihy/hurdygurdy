#version 460

layout(location = 0) in vec2 in_pos;

layout(location = 0) out vec2 f_uv;

layout(set = 0, binding = 0) uniform VPUniform {
    mat4 view;
    mat4 proj;
} u_vp;

layout(push_constant) uniform Push {
    mat4 model;
    vec2 offset;
    vec2 size;
} push;

void main() {
    f_uv = push.offset + in_pos * push.size;
    gl_Position = u_vp.proj * u_vp.view * push.model * vec4(in_pos, 0.0, 1.0);
}


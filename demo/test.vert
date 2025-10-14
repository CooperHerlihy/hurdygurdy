#version 450

layout(location = 0) out vec3 f_pos;
layout(location = 1) out vec2 f_uv;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;

layout(push_constant) uniform Push {
    mat4 model;
} push;

layout(set = 0, binding = 0) uniform ViewProj {
    mat4 view;
    mat4 proj;
} u_vp;

void main() {
    f_uv = in_uv;

    vec4 vertex = vec4(in_pos, 1.0);
    vec4 world = push.model * vertex;
    vec4 view = u_vp.view * world;

    f_pos = view.xyz;
    gl_Position = u_vp.proj * view;
}


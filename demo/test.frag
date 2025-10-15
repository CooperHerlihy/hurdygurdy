#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec2 v_uv;

layout(set = 1, binding = 0) uniform sampler2D u_texture;

void main() {
    out_color = texture(u_texture, v_uv);
}


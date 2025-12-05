#version 460

layout (location = 0) out vec4 out_color;

layout (location = 0) in vec2 v_uv;

layout (set = 1, binding = 0) uniform sampler2D u_sprite;

void main() {
    out_color = texture(u_sprite, v_uv);
}


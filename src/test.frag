#version 460

layout (location = 0) in vec2 v_pos;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = vec4(v_pos, 0.0, 1.0);
}


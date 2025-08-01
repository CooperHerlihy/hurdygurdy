#version 450

layout(location = 0) in vec2 f_pos;
layout(location = 0) out vec4 f_color;

layout(set = 0, binding = 0) uniform sampler2D u_input;

void main() {
    f_color = texture(u_input.zxyw);
}


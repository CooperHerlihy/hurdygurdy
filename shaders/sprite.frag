#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 in_uv;

layout(set = 1, binding = 0) uniform sampler2D u_sampler;

void main() {
    out_color = texture(u_sampler, in_uv);
}

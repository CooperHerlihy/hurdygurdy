#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_uv;

layout(set = 0, binding = 1) uniform sampler2D u_sampler;

void main() {
    out_color = texture(u_sampler, v_uv);
}

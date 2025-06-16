#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 v_pos;

layout(set = 1, binding = 0) uniform samplerCube u_sampler;

void main() {
    out_color = texture(u_sampler, v_pos);
}

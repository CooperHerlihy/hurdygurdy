#version 460

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_position;

layout(set = 0, binding = 0) uniform sampler2D u_depth_buffer;

void main() {
    float depth = texture(u_depth_buffer, v_position).x;
    out_color = vec4(vec3(depth * depth * depth * depth * depth * depth * depth), 1.0);
}

#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_pos;

layout(set = 0, binding = 2) uniform sampler2D u_samplers[];

layout(push_constant) uniform Push {
    uint input_index;
} push;

void main() {
    const vec3 hdr = texture(u_samplers[push.input_index], v_pos).rgb;
    const vec3 ldr = vec3(1.0) - exp(-hdr);
    out_color = vec4(ldr, 1.0);
}


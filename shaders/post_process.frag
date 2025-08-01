#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 f_pos;

layout(set = 0, binding = 2) uniform sampler2D u_samplers[];

layout(push_constant) uniform Push {
    uint input_index;
} push;

void main() {
    out_color = vec4(texture(u_samplers[push.input_index], f_pos).zxyw);
}


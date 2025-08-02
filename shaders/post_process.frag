#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 f_pos;

layout(set = 0, binding = 2) uniform sampler2D u_samplers[];

layout(push_constant) uniform Push {
    uint input_index;
    uint depth_index;
    uvec2 input_size;
} push;

void main() {
    vec4 color = texture(u_samplers[push.input_index], f_pos);
    float depth = texture(u_samplers[push.depth_index], f_pos).x;
    for (int i = 0; i < 5; ++i) {
        depth *= depth;
    }

    if (f_pos.x <= 0.5 ) {
        out_color = color;
    } else {
        out_color = vec4(vec3(depth), 1.0);
    }
}


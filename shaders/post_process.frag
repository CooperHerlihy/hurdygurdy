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

float sqrf(float x) {
    return x * x;
}

float calc_edge() {
    float weight_h[3][3] = float[][](
        float[](-1.0, 0.0, 1.0),
        float[](-2.0, 0.0, 2.0),
        float[](-1.0, 0.0, 1.0)
    );

    float convolution_h = 0.0;
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            vec2 offset = vec2(x - 1, y - 1) / push.input_size;
            vec4 sampled = texture(u_samplers[push.input_index], f_pos + offset);
            vec3 sampled_rgb = vec3(sampled.r, sampled.g, sampled.b) * weight_h[y][x];
            convolution_h += (sampled_rgb.r) + (sampled_rgb.g) + (sampled_rgb.b);
        }
    }

    float weight_v[3][3] = float[][](
        float[](-1.0, -2.0, -1.0),
        float[](0.0, 0.0, 0.0),
        float[](1.0, 2.0, 1.0)
    );

    float convolution_v = 0.0;
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 3; ++x) {
            vec2 offset = vec2(x - 1, y - 1) / push.input_size;
            vec4 sampled = texture(u_samplers[push.input_index], f_pos + offset);
            vec3 sampled_rgb = vec3(sampled.r, sampled.g, sampled.b) * weight_v[y][x];
            convolution_v += (sampled_rgb.r) + (sampled_rgb.g) + (sampled_rgb.b);
        }
    }

    return clamp(sqrf(convolution_h) + sqrf(convolution_v), 0.0, 1.0);
}

vec4 hdr_to_ldr(const vec4 hdr) {
    return vec4(vec3(1.0) - exp(-hdr.xyz), hdr.w);
}

void main() {
    const vec4 in_color = texture(u_samplers[push.input_index], f_pos);
    // float depth = texture(u_samplers[push.depth_index], f_pos).x;

    out_color = hdr_to_ldr(mix(in_color, vec4(vec3(0.0), 1.0), calc_edge()));
}


#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 v_pos;

layout(set = 1, binding = 0) uniform samplerCube u_sampler;

layout(set = 1, binding = 1) uniform Material {
    float roughness;
    float metal;
} u_material;

const uint MaxLights = 10;

struct Light {
    vec4 pos;
    vec4 color;
};

layout(set = 0, binding = 1) uniform LightUniform {
    Light vals[MaxLights];
    uint count;
} u_light;

void main() {
    out_color = texture(u_sampler, v_pos);
}

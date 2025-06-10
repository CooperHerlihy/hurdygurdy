#version 450

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 v_pos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;

const float pi = 3.14159265;

layout(set = 1, binding = 0) uniform sampler2D u_sampler;

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

float square(const float x) {
    return x * x;
}

float d_ggx(const float roughness, const float ndoth) {
    const float num = square(roughness);
    const float denom = pi * square(square(ndoth) * (square(roughness) - 1) + 1);
    return num / denom;
}

float g_ggx(const float roughness, const float theta) {
    const float k = square(roughness + 1.0) / 8.0;
    const float num = theta;
    const float denom = theta * (1.0 - k) + k;
    return num / denom;
}

vec3 brdf(const vec3 albedo, const float metal, const float roughness, const vec3 f0, const float ndotv, const float ndotl, const float ndoth, const float vdoth) {
    const vec3 diffuse = albedo / pi;

    const vec3 F = f0 + (vec3(1.0) - f0) * pow((1 - vdoth), 5.0);
    const float D = d_ggx(roughness, ndoth);
    const float G = g_ggx(roughness, ndotv) * g_ggx(roughness, ndotl);
    const vec3 specular = (F / 4.0) * (D * G / max((ndotv * ndotl), 0.0001));

    const vec3 ks = F;
    const vec3 kd = (1 - ks) * (1 - metal);
    return kd * diffuse + specular;
}

vec3 calc_reflection(const Light light, const vec3 normal, const vec3 albedo, const float metal, const float roughness, const vec3 f0) {
    const vec3 view_dir = normalize(-v_pos);
    const vec3 light_rel_pos = light.pos.xyz - v_pos;
    const vec3 light_dir = normalize(light_rel_pos);
    const vec3 half_dir = normalize(view_dir + light_dir);

    const float ndotv = clamp(dot(normal, view_dir), 0.0, 1.0);
    const float ndotl = clamp(dot(normal, light_dir), 0.0, 1.0);
    const float ndoth = clamp(dot(normal, half_dir), 0.0, 1.0);
    const float vdoth = clamp(dot(view_dir, half_dir), 0.0, 1.0);

    const float attenuation = 1.0 / dot(light_rel_pos, light_rel_pos);
    return (light.color.xyz * attenuation) * (brdf(albedo, metal, roughness, f0, ndotv, ndotl, ndoth, vdoth) * ndotl);
}

void main() {
    const vec4 tex = texture(u_sampler, v_uv);

    const vec3 normal = normalize(v_normal);
    const vec3 albedo = tex.xyz;
    const float metal = u_material.metal;
    const float roughness = u_material.roughness;
    const vec3 f0 = mix(vec3(0.04), albedo, metal);

    const vec3 ambient = vec3(0.03, 0.03, 0.03);

    vec3 total_light = ambient;
    for (uint i = 0u; i < u_light.count; i++) {
        total_light += calc_reflection(u_light.vals[i], normal, albedo, metal, roughness, f0);
    }

    const vec3 hdr_color = total_light * tex.xyz;
    const vec3 ldr_color = vec3(1.0) - exp(-hdr_color);
    out_color = vec4(ldr_color, tex.w);
}

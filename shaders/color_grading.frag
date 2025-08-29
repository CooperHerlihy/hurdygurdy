#version 450
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec2 v_pos;

layout(set = 0, binding = 2) uniform sampler2D u_samplers[];

layout(push_constant) uniform Push {
    uint input_index;

    uint tonemapper;

    float exposure;
    float saturation;
    float contrast;

    float lift;
    float gamma;
    float gain;

    float temperature;
    float tint;
    // float red_filter;
    // float green_filter;
    // float blue_filter;

} push;

const uint Reinhard = 0;
const uint Uncharted2 = 1;
const uint ACESApprox = 2;
const uint ACESFitted = 3;
const uint PBRNeutral = 4;

vec3 sample_color() {
    return texture(u_samplers[push.input_index], v_pos).rgb;
}

float luma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

// see John Hable http://filmicworlds.com/blog/minimal-color-grading-tools/

#define TEMPERATURE_COUNT 5
const vec3 temperatures[TEMPERATURE_COUNT] = vec3[](
    vec3(0.5, 0.5, 1.0),
    vec3(0.8, 0.8, 1.0),
    vec3(1.0, 1.0, 1.0),
    vec3(1.0, 0.8, 0.3),
    vec3(1.0, 0.3, 0.0)
);

#define TINT_COUNT 3
const vec3 tints[TINT_COUNT] = vec3[](
    vec3(0.0, 1.0, 0.0),
    vec3(1.0, 1.0, 1.0),
    vec3(1.0, 0.0, 1.0)
);

vec3 color_correct(vec3 color) {
    float temperature = (push.temperature * 0.5 + 0.5) * (TEMPERATURE_COUNT - 1);
    int temp_floor = int(temperature);
    vec3 temp_color = mix(temperatures[temp_floor], temperatures[temp_floor + 1], temperature - temp_floor);

    float tint = (push.tint * 0.5 + 0.5) * (TINT_COUNT - 1);
    int tint_floor = int(tint);
    vec3 tint_color = mix(tints[tint_floor], tints[tint_floor + 1], tint - tint_floor);

    return temp_color * tint_color * color;
}

vec3 exposure(vec3 color) {
    return color * push.exposure;
}

vec3 saturation(vec3 color) {
    return mix(vec3(luma(color)), color, push.saturation);
}

vec3 contrast(vec3 color) {
    return clamp(mix(vec3(0.5), color, push.contrast), 0.0, 1.0);
}

vec3 lift(vec3 color) {
    return color;
}

vec3 gamma(vec3 color) {
    return color;
}

vec3 gain(vec3 color) {
    return color;
}

float luminance(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

vec3 reinhard(vec3 color) {
    return color / (vec3(1.0) + luminance(color));
}

// John Hable https://64.github.io/tonemapping/#uncharted-2 (find original source?)
vec3 uncharted2_inter(vec3 x) {
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}
vec3 uncharted2(vec3 v) {
    const float exposure_bias = 2.0;
    const vec3 curr = uncharted2_inter(v * exposure_bias);

    const vec3 W = vec3(11.2);
    const vec3 white_scale = vec3(1.0) / uncharted2_inter(W);
    return curr * white_scale;
}

// Krzysztof Narkowicz https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACES_approx(vec3 color) {
    color *= 0.6;
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

// Stephen Hill https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
vec3 ACES_fitted(vec3 color) {
    const mat3 ACESInputMat = mat3(
        0.59719, 0.07600, 0.02840,
        0.35458, 0.90834, 0.13383,
        0.04823, 0.01566, 0.83777
    );
    color = ACESInputMat * color;

    // Apply RRT and ODT
    const vec3 a = color * (color + 0.0245786) - 0.000090537;
    const vec3 b = color * (0.983729 * color + 0.4329510) + 0.238081;
    color = a / b;

    // ODT_SAT => XYZ => D60_2_D65 => sRGB
    const mat3 ACESOutputMat = mat3(
         1.60475, -0.10208, -0.00327,
        -0.53108,  1.10813, -0.07276,
        -0.07367, -0.00605,  1.07602
    );
    color = ACESOutputMat * color;

    return clamp(color, 0.0, 1.0);
}

// https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/pbrNeutral.glsl
vec3 PBR_neutral(vec3 color) {
    const float startCompression = 0.8 - 0.04;
    const float desaturation = 0.15;

    float x = min(color.r, min(color.g, color.b));
    float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
    color -= offset;

    float peak = max(color.r, max(color.g, color.b));
    if (peak < startCompression) return color;

    const float d = 1. - startCompression;
    float newPeak = 1. - d * d / (peak + d - startCompression);
    color *= newPeak / peak;

    float g = 1. - 1. / (desaturation * (peak - newPeak) + 1.);
    return mix(color, newPeak * vec3(1, 1, 1), g);
}

vec3 tonemap(vec3 color) {
    switch (push.tonemapper) {
        case Reinhard: return reinhard(color);
        case Uncharted2: return uncharted2(color);
        case ACESApprox: return ACES_approx(color);
        case ACESFitted: return ACES_fitted(color);
        case PBRNeutral: return PBR_neutral(color);
        default: return clamp(color, 0.0, 1.0);;
    }
}

void main() {
    const vec3 color =
        contrast(
        saturation(
        tonemap(
        color_correct(
        exposure(
        sample_color(
        ))))))
    ;

    out_color = vec4(color, 1.0);
}


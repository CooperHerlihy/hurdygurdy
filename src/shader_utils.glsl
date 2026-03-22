#extension GL_EXT_nonuniform_qualifier : enable

/**
 * Access bindless descriptors with these binding indices
 */
#define HgBindingIdx_sampler 0
#define HgBindingIdx_combinedImageSampler 1
#define HgBindingIdx_sampledImage 2
#define HgBindingIdx_storageImage 3
#define HgBindingIdx_uniformTexelBuffer 4
#define HgBindingIdx_storageTexelBuffer 5
#define HgBindingIdx_uniformBuffer 6
#define HgBindingIdx_storageBuffer 7

/**
 * Access bindless descriptors with these as layout qualifiers
 */
#define HgSampler binding = HgBindingIdx_sampler
#define HgCombinedImageSampler binding = HgBindingIdx_combinedImageSampler
#define HgSampledImage binding = HgBindingIdx_sampledImage
#define HgStorageImage binding = HgBindingIdx_storageImage
#define HgUniformTexelBuffer binding = HgBindingIdx_uniformTexelBuffer
#define HgStorageTexelBuffer binding = HgBindingIdx_storageTexelBuffer
#define HgUniformBuffer binding = HgBindingIdx_uniformBuffer
#define HgStorageBuffer binding = HgBindingIdx_storageBuffer

/**
 * The maximum value of uint
 */
#define UINT_MAX 4294967295u
/**
 * The maximum value of int
 */
#define INT_MAX 2147483647

/**
 * The value of pi
 */
#define hgPi 3.1415926535897932
/**
 * The value of Euler's number
 */
#define hgEuler 2.7182818284590452

/**
 * Square a value
 */
uint square(uint val)
{
    return val * val;
}

/**
 * Square a value
 */
int square(int val)
{
    return val * val;
}

/**
 * Square a value
 */
float square(float val)
{
    return val * val;
}

/**
 * Square a value
 */
double square(double val)
{
    return val * val;
}

/**
 * Smooth a 0.0 to 1.0 linear gradient
 */
float hgSmooth(float t)
{
    return t * t * (3 - 2 * t);
}

/**
 * Smooth a 0.0 to 1.0 linear gradient with a quintic function
 */
float hgSmoothQuintic(float t)
{
    return t * t * t * (t * (t * 6 - 15) + 10);
}

/**
 * Generate white noise
 */
uint hgNoise(uint seed, uint pos)
{
    uint ret = (pos + 384521713u) * 955740521u;
    ret ^= ret >> 13;
    ret *= seed * 725937977u;
    ret ^= ret >> 7;
    ret *= 358166231u;
    ret ^= ret >> 11;
    return ret;
}

/**
 * Generate white noise
 */
uint hgNoise(uint seed, uvec2 pos)
{
    return hgNoise(seed, pos.x + (pos.y * 425537443u));
}

/**
 * Generate white noise
 */
uint hgNoise(uint seed, uvec3 pos)
{
    return hgNoise(seed, pos.x + pos.y * 425537443u + pos.z * 682607u);
}

/**
 * Generate white noise
 */
uint hgNoise(uint seed, uvec4 pos)
{
    return hgNoise(seed, pos.x + pos.y * 425537443u + pos.z * 682607u + pos.w * 9067);
}

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
float hgNoiseNorm(uint seed, float pos)
{
    return float(hgNoise(seed, floatBitsToUint(pos))) / float(UINT_MAX);
}

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
float hgNoiseNorm(uint seed, vec2 pos)
{
    return float(hgNoise(seed, floatBitsToUint(pos))) / float(UINT_MAX);
}

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
float hgNoiseNorm(uint seed, vec3 pos)
{
    return float(hgNoise(seed, floatBitsToUint(pos))) / float(UINT_MAX);
}

/**
 * Generate white noise normalized from 0.0 to 1.0
 */
float hgNoiseNorm(uint seed, vec4 pos)
{
    return float(hgNoise(seed, floatBitsToUint(pos))) / float(UINT_MAX);
}

/**
 * Generate white noise unit vector
 */
float hgNoiseVec1D(uint seed, float pos)
{
    return hgNoiseNorm(seed, pos) * 2.0 - 1.0;
}

/**
 * Generate white noise unit vector
 */
vec2 hgNoiseVec2D(uint seed, vec2 pos)
{
    float rot = 2.0 * hgPi * hgNoiseNorm(seed, pos);
    return vec2(cos(rot), sin(rot));
}

/**
 * Define a function which returns fractal noise from a noise function
 */
#define hgFractalNoise1DFunctionDef(fractalFunc, noiseFunc) \
    float fractalFunc(uint seed, float scaleBegin, float scaleEnd, float pos) \
    { \
        float noise = 0.0; \
        float octave = 1.0; \
        while (scaleEnd * octave > scaleBegin) \
        { \
            noise += octave * noiseFunc(seed, octave * scaleEnd, pos); \
            octave *= 0.5; \
        } \
        return noise; \
    }

/**
 * Define a function which returns fractal noise from a noise function
 */
#define hgFractalNoise2DFunctionDef(fractalFunc, noiseFunc) \
    float fractalFunc(uint seed, float scaleBegin, float scaleEnd, vec2 pos) \
    { \
        float noise = 0.0; \
        float octave = 1.0; \
        while (scaleEnd * octave > scaleBegin) \
        { \
            noise += octave * noiseFunc(seed, octave * scaleEnd, pos); \
            octave *= 0.5; \
        } \
        return noise; \
    }

/**
 * Define a function which returns fractal noise from a noise function
 */
#define hgFractalNoise3DFunctionDef(fractalFunc, noiseFunc) \
    float fractalFunc(uint seed, float scaleBegin, float scaleEnd, vec3 pos) \
    { \
        float noise = 0.0; \
        float octave = 1.0; \
        while (scaleEnd * octave > scaleBegin) \
        { \
            noise += octave * noiseFunc(seed, octave * scaleEnd, pos); \
            octave *= 0.5; \
        } \
        return noise; \
    }

/**
 * Define a function which returns fractal noise from a noise function
 */
#define hgFractalNoise4DFunctionDef(fractalFunc, noiseFunc) \
    float fractalFunc(uint seed, float scaleBegin, float scaleEnd, vec4 pos) \
    { \
        float noise = 0.0; \
        float octave = 1.0; \
        while (scaleEnd * octave > scaleBegin) \
        { \
            noise += octave * noiseFunc(seed, octave * scaleEnd, pos); \
            octave *= 0.5; \
        } \
        return noise; \
    }

/**
 * Generate value noise
 *
 * Parameters
 * - seed The seed for the noise function
 * - scale The number of positions before a new value
 * - pos The position for the noise function
 */
float hgValueNoise1D(uint seed, float scale, float pos)
{
    pos /= scale;
    return mix(
        hgNoiseNorm(seed, floor(pos)) * 2.0 - 1.0,
        hgNoiseNorm(seed, floor(pos) + 1) * 2.0 - 1.0,
        hgSmooth(fract(pos)));
}

/**
 * Generate value noise
 *
 * Parameters
 * - seed The seed for the noise function
 * - scale The number of positions before a new value
 * - pos The position for the noise function
 */
float hgValueNoise2D(uint seed, float scale, vec2 pos)
{
    pos /= scale;
    vec2 t = fract(pos);
    return mix(
        mix(
            hgNoiseNorm(seed, floor(pos)) * 2.0 - 1.0,
            hgNoiseNorm(seed, floor(pos) + vec2(1, 0)) * 2.0 - 1.0,
            hgSmooth(t.x)),
        mix(
            hgNoiseNorm(seed, floor(pos) + vec2(0, 1)) * 2.0 - 1.0,
            hgNoiseNorm(seed, floor(pos) + vec2(1, 1)) * 2.0 - 1.0,
            hgSmooth(t.x)),
        hgSmooth(t.y));
}

/**
 * Generate value noise
 *
 * Parameters
 * - seed The seed for the noise function
 * - scale The number of positions before a new value
 * - pos The position for the noise function
 */
float hgValueNoise3D(uint seed, float scale, vec3 pos)
{
    pos /= scale;
    vec3 t = fract(pos);
    return mix(
        mix(
            mix(
                hgNoiseNorm(seed, floor(pos)) * 2.0 - 1.0,
                hgNoiseNorm(seed, floor(pos) + vec3(1, 0, 0)) * 2.0 - 1.0,
                hgSmooth(t.x)),
            mix(
                hgNoiseNorm(seed, floor(pos) + vec3(0, 1, 0)) * 2.0 - 1.0,
                hgNoiseNorm(seed, floor(pos) + vec3(1, 1, 0)) * 2.0 - 1.0,
                hgSmooth(t.x)),
            hgSmooth(t.y)),
        mix(
            mix(
                hgNoiseNorm(seed, floor(pos) + vec3(0, 0, 1)) * 2.0 - 1.0,
                hgNoiseNorm(seed, floor(pos) + vec3(1, 0, 1)) * 2.0 - 1.0,
                hgSmooth(t.x)),
            mix(
                hgNoiseNorm(seed, floor(pos) + vec3(0, 1, 1)) * 2.0 - 1.0,
                hgNoiseNorm(seed, floor(pos) + vec3(1, 1, 1)) * 2.0 - 1.0,
                hgSmooth(t.x)),
            hgSmooth(t.y)),
        hgSmooth(t.z));
}

/**
 * Generate value noise
 *
 * Parameters
 * - seed The seed for the noise function
 * - scale The number of positions before a new value
 * - pos The position for the noise function
 */
float hgValueNoise4D(uint seed, float scale, vec4 pos)
{
    pos /= scale;
    vec4 t = fract(pos);
    return mix(
        mix(
            mix(
                mix(
                    hgNoiseNorm(seed, floor(pos)) * 2.0 - 1.0,
                    hgNoiseNorm(seed, floor(pos) + vec4(1, 0, 0, 0)) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                mix(
                    hgNoiseNorm(seed, floor(pos) + vec4(0, 1, 0, 0)) * 2.0 - 1.0,
                    hgNoiseNorm(seed, floor(pos) + vec4(1, 1, 0, 0)) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                hgSmooth(t.y)),
            mix(
                mix(
                    hgNoiseNorm(seed, floor(pos) + vec4(0, 0, 1, 0)) * 2.0 - 1.0,
                    hgNoiseNorm(seed, floor(pos) + vec4(1, 0, 1, 0)) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                mix(
                    hgNoiseNorm(seed, floor(pos) + vec4(0, 1, 1, 0)) * 2.0 - 1.0,
                    hgNoiseNorm(seed, floor(pos) + vec4(1, 1, 1, 0)) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                hgSmooth(t.y)),
            hgSmooth(t.z)),
        mix(
            mix(
                mix(
                    hgNoiseNorm(seed, floor(pos) + vec4(0, 0, 0, 1)) * 2.0 - 1.0,
                    hgNoiseNorm(seed, floor(pos) + vec4(1, 0, 0, 1)) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                mix(
                    hgNoiseNorm(seed, floor(pos) + vec4(0, 1, 0, 1)) * 2.0 - 1.0,
                    hgNoiseNorm(seed, floor(pos) + vec4(1, 1, 0, 1)) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                hgSmooth(t.y)),
            mix(
                mix(
                    hgNoiseNorm(seed, floor(pos) + vec4(0, 0, 1, 1)) * 2.0 - 1.0,
                    hgNoiseNorm(seed, floor(pos) + vec4(1, 0, 1, 1)) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                mix(
                    hgNoiseNorm(seed, floor(pos) + vec4(0, 1, 1, 1)) * 2.0 - 1.0,
                    hgNoiseNorm(seed, floor(pos) + vec4(1, 1, 1, 1)) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                hgSmooth(t.y)),
            hgSmooth(t.z)),
        hgSmooth(t.w));
}

hgFractalNoise1DFunctionDef(hgFractalValueNoise1D, hgValueNoise1D)
hgFractalNoise2DFunctionDef(hgFractalValueNoise2D, hgValueNoise2D)
hgFractalNoise3DFunctionDef(hgFractalValueNoise3D, hgValueNoise3D)
hgFractalNoise4DFunctionDef(hgFractalValueNoise4D, hgValueNoise4D)

/**
 * Generate Perlin noise
 *
 * Parameters
 * - seed The seed for the noise function
 * - scale The number of positions before a new gradient
 * - pos The position for the noise function
 */
float hgPerlin1D(uint seed, float scale, float pos)
{
    pos /= scale;

    float grad0 = hgNoiseVec1D(seed, floor(pos));
    float grad1 = hgNoiseVec1D(seed, floor(pos) + 1);

    float off0 = fract(pos);
    float off1 = 1.0 - off0;

    return mix(grad0 * off0, grad1 * off1, hgSmoothQuintic(off0));
}

/**
 * Generate Perlin noise
 *
 * Parameters
 * - seed The seed for the noise function
 * - scale The number of positions before a new gradient
 * - pos The position for the noise function
 */
float hgPerlin2D(uint seed, float scale, vec2 pos)
{
    pos /= scale;

    vec2 grad00 = hgNoiseVec2D(seed, floor(pos));
    vec2 grad10 = hgNoiseVec2D(seed, floor(pos) + vec2(1, 0));
    vec2 grad01 = hgNoiseVec2D(seed, floor(pos) + vec2(0, 1));
    vec2 grad11 = hgNoiseVec2D(seed, floor(pos) + vec2(1, 1));

    vec2 off00 = fract(pos);
    vec2 off10 = vec2(off00.x - 1.0, off00.y);
    vec2 off01 = vec2(off00.x, off00.y - 1.0);
    vec2 off11 = vec2(off00.x - 1.0, off00.y - 1.0);

    return mix(
        mix(dot(grad00, off00), dot(grad10, off10), hgSmoothQuintic(off00.x)),
        mix(dot(grad01, off01), dot(grad11, off11), hgSmoothQuintic(off00.x)),
        hgSmoothQuintic(off00.y));
}

hgFractalNoise1DFunctionDef(hgFractalPerlin1D, hgPerlin1D)
hgFractalNoise2DFunctionDef(hgFractalPerlin2D, hgPerlin2D)

/**
 * A vertex
 */
struct HgVertex
{
    vec3 position;
    vec3 normal;
    vec4 tangent;
    vec2 uvCoord;
};

/**
 * Transform a vertex, keeping its normal and tangent intact
 */
HgVertex hgTransformVertex(HgVertex vertex, mat4 matrix)
{
    vec4 pos = matrix * vec4(vertex.position, 1.0);

    mat3 imatrix = mat3(transpose(inverse(matrix)));
    vec3 normal = imatrix * vertex.normal;
    vec4 tangent = vec4(imatrix * vertex.tangent.xyz, vertex.tangent.w);

    return HgVertex(pos.xyz, normal, tangent, vertex.uvCoord);
}

/**
 * Transform the normal from a normal map onto the model
 *
 * Parameters
 * - mapNormal The normal from the normal map
 * - vertexNormal The normal interpolated from the vertices
 * - vertexTangent The tangent interpolated from the vertices
 */
vec3 hgTransformNormalMap(vec3 mapNormal, vec3 vertexNormal, vec4 vertexTangent)
{
    mat3 texToModel = mat3(
        vertexTangent.xyz,
        cross(vertexTangent.xyz, vertexNormal) * vertexTangent.w,
        -vertexNormal
    );
    return normalize(texToModel * mapNormal);
}

/**
 * Approximate the luma of a color, not to be confused with luminance
 */
float luma(vec3 color) {
    return dot(color, vec3(0.299, 0.587, 0.114));
}

/**
 * Approximate the luminance of a color, not to be confused with luma
 */
float luminance(vec3 color) {
    return dot(color, vec3(0.2126, 0.7152, 0.0722));
}

/**
 * Tonemap from hdr to ldr using the Reinhard model
 */
vec3 hgReinhardTonemap(vec3 color) {
    return color / (vec3(1.0) + luminance(color));
}

vec3 hgUncharted2TonemapInter(vec3 x) {
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

/**
 * John Hable https://64.github.io/tonemapping/#uncharted-2 (find original source?)
 */
vec3 hgUncharted2Tonemap(vec3 v) {
    const float exposure_bias = 2.0;
    const vec3 curr = hgUncharted2TonemapInter(v * exposure_bias);

    const vec3 W = vec3(11.2);
    const vec3 white_scale = vec3(1.0) / hgUncharted2TonemapInter(W);
    return curr * white_scale;
}

/**
 * Krzysztof Narkowicz https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
 */
vec3 hgAcesApproxTonemap(vec3 color) {
    color *= 0.6;
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

/**
 * Stephen Hill https://github.com/TheRealMJP/BakingLab/blob/master/BakingLab/ACES.hlsl
 */
vec3 hgAcesFittedTonemap(vec3 color) {
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

/**
 * https://github.com/KhronosGroup/ToneMapping/blob/main/PBR_Neutral/pbrNeutral.glsl
 */
vec3 hgPbrNeutralTonemap(vec3 color) {
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

/**
 * Calculate the lambertain diffuse
 */
float hgLambertianDiffuse(
    vec3 normal,
    vec3 lightDir)
{
    return max(dot(normal, lightDir), 0.0);
}

/**
 * Calculate the specular part of the Blinn-Phong lighting model
 */
float hgBlinnPhongSpecular(
    vec3 normal,
    vec3 lightDir,
    vec3 viewDir,
    float shininess)
{
    return pow(max(dot(normal, normalize(lightDir + viewDir)), 0.0), shininess);
}

/**
 * Calculate lighting using the Blinn-Phong model
 */
float hgBlinnPhong(
    vec3 normal,
    vec3 lightDir,
    vec3 viewDir,
    float shininess,
    float kdiffuse,
    float kspecular,
    float ambient)
{
    float diffuse = hgLambertianDiffuse(normal, lightDir);
    float specular = diffuse > 0.0
        ? hgBlinnPhongSpecular(normal, lightDir, viewDir, shininess)
        : 0.0;
    return ambient + diffuse * kdiffuse + specular * kspecular;
};


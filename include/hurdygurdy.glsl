/*
 * =============================================================================
 *
 * Copyright (c) 2025-2026 Cooper Herlihy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 */

#ifndef HURDYGURDY_GLSL
#define HURDYGURDY_GLSL

#extension GL_EXT_nonuniform_qualifier : enable

/**
 * Access bindless descriptors with these binding indices
 */
#define HgBindingIdx_combinedImageSampler 0
#define HgBindingIdx_storageImage 1
#define HgBindingIdx_uniformBuffer 2
#define HgBindingIdx_storageBuffer 3

/**
 * Access bindless descriptors with these as layout qualifiers
 */
#define HgCombinedImageSampler binding = HgBindingIdx_combinedImageSampler
#define HgStorageImage binding = HgBindingIdx_storageImage
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
    float fractalFunc(uint seed, float scaleBegin, float scaleEnd, float pos, float tile) \
    { \
        float noise = 0.0; \
        float octave = 1.0; \
        while (scaleEnd * octave > scaleBegin) \
        { \
            noise += octave * noiseFunc(seed, octave * scaleEnd, pos, tile); \
            octave *= 0.5; \
        } \
        return noise; \
    }

/**
 * Define a function which returns fractal noise from a noise function
 */
#define hgFractalNoise2DFunctionDef(fractalFunc, noiseFunc) \
    float fractalFunc(uint seed, float scaleBegin, float scaleEnd, vec2 pos, vec2 tile) \
    { \
        float noise = 0.0; \
        float octave = 1.0; \
        while (scaleEnd * octave > scaleBegin) \
        { \
            noise += octave * noiseFunc(seed, octave * scaleEnd, pos, tile); \
            octave *= 0.5; \
        } \
        return noise; \
    }

/**
 * Define a function which returns fractal noise from a noise function
 */
#define hgFractalNoise3DFunctionDef(fractalFunc, noiseFunc) \
    float fractalFunc(uint seed, float scaleBegin, float scaleEnd, vec3 pos, vec3 tile) \
    { \
        float noise = 0.0; \
        float octave = 1.0; \
        while (scaleEnd * octave > scaleBegin) \
        { \
            noise += octave * noiseFunc(seed, octave * scaleEnd, pos, tile); \
            octave *= 0.5; \
        } \
        return noise; \
    }

/**
 * Define a function which returns fractal noise from a noise function
 */
#define hgFractalNoise4DFunctionDef(fractalFunc, noiseFunc) \
    float fractalFunc(uint seed, float scaleBegin, float scaleEnd, vec4 pos, vec4 tile) \
    { \
        float noise = 0.0; \
        float octave = 1.0; \
        while (scaleEnd * octave > scaleBegin) \
        { \
            noise += octave * noiseFunc(seed, octave * scaleEnd, pos, tile); \
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
 * - tile The distance before the repeating, or 0 to never repeat
 */
float hgValueNoise1D(uint seed, float scale, float pos, float tile)
{
    pos /= scale;
    tile /= scale;

    float pos0 = floor(pos);
    float pos1 = pos0 + 1;

    if (tile > 0.0)
    {
        pos0 = fract(pos0 / tile) * tile;
        pos1 = fract(pos1 / tile) * tile;
    }

    return mix(
        hgNoiseNorm(seed, pos0) * 2.0 - 1.0,
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
 * - tile The distance before the repeating, or 0 to never repeat
 */
float hgValueNoise2D(uint seed, float scale, vec2 pos, vec2 tile)
{
    pos /= scale;
    tile /= scale;

    vec2 pos00 = floor(pos);
    vec2 pos10 = pos00 + vec2(1, 0);
    vec2 pos01 = pos00 + vec2(0, 1);
    vec2 pos11 = pos00 + vec2(1, 1);

    if (tile.x > 0 && tile.y > 0)
    {
        pos00 = fract(pos00 / tile) * tile;
        pos01 = fract(pos01 / tile) * tile;
        pos10 = fract(pos10 / tile) * tile;
        pos11 = fract(pos11 / tile) * tile;
    }

    vec2 t = fract(pos);
    return mix(
        mix(
            hgNoiseNorm(seed, pos00) * 2.0 - 1.0,
            hgNoiseNorm(seed, pos10) * 2.0 - 1.0,
            hgSmooth(t.x)),
        mix(
            hgNoiseNorm(seed, pos01) * 2.0 - 1.0,
            hgNoiseNorm(seed, pos11) * 2.0 - 1.0,
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
 * - tile The distance before the repeating, or 0 to never repeat
 */
float hgValueNoise3D(uint seed, float scale, vec3 pos, vec3 tile)
{
    pos /= scale;
    tile /= scale;

    vec3 pos000 = floor(pos);
    vec3 pos100 = pos000 + vec3(1, 0, 0);
    vec3 pos010 = pos000 + vec3(0, 1, 0);
    vec3 pos110 = pos000 + vec3(1, 1, 0);
    vec3 pos001 = pos000 + vec3(0, 0, 1);
    vec3 pos101 = pos000 + vec3(1, 0, 1);
    vec3 pos011 = pos000 + vec3(0, 1, 1);
    vec3 pos111 = pos000 + vec3(1, 1, 1);

    if (tile.x > 0 && tile.y > 0)
    {
        pos000 = fract(pos000 / tile) * tile;
        pos010 = fract(pos010 / tile) * tile;
        pos100 = fract(pos100 / tile) * tile;
        pos110 = fract(pos110 / tile) * tile;
        pos001 = fract(pos001 / tile) * tile;
        pos011 = fract(pos011 / tile) * tile;
        pos101 = fract(pos101 / tile) * tile;
        pos111 = fract(pos111 / tile) * tile;
    }

    vec3 t = fract(pos);
    return mix(
        mix(
            mix(
                hgNoiseNorm(seed, pos000) * 2.0 - 1.0,
                hgNoiseNorm(seed, pos100) * 2.0 - 1.0,
                hgSmooth(t.x)),
            mix(
                hgNoiseNorm(seed, pos010) * 2.0 - 1.0,
                hgNoiseNorm(seed, pos110) * 2.0 - 1.0,
                hgSmooth(t.x)),
            hgSmooth(t.y)),
        mix(
            mix(
                hgNoiseNorm(seed, pos001) * 2.0 - 1.0,
                hgNoiseNorm(seed, pos101) * 2.0 - 1.0,
                hgSmooth(t.x)),
            mix(
                hgNoiseNorm(seed, pos011) * 2.0 - 1.0,
                hgNoiseNorm(seed, pos111) * 2.0 - 1.0,
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
 * - tile The distance before the repeating, or 0 to never repeat
 */
float hgValueNoise4D(uint seed, float scale, vec4 pos, vec4 tile)
{
    pos /= scale;
    tile /= scale;

    vec4 pos0000 = floor(pos);
    vec4 pos1000 = pos0000 + vec4(1, 0, 0, 0);
    vec4 pos0100 = pos0000 + vec4(0, 1, 0, 0);
    vec4 pos1100 = pos0000 + vec4(1, 1, 0, 0);
    vec4 pos0010 = pos0000 + vec4(0, 0, 1, 0);
    vec4 pos1010 = pos0000 + vec4(1, 0, 1, 0);
    vec4 pos0110 = pos0000 + vec4(0, 1, 1, 0);
    vec4 pos1110 = pos0000 + vec4(1, 1, 1, 0);
    vec4 pos0001 = pos0000 + vec4(0, 0, 0, 1);
    vec4 pos1001 = pos0000 + vec4(1, 0, 0, 1);
    vec4 pos0101 = pos0000 + vec4(0, 1, 0, 1);
    vec4 pos1101 = pos0000 + vec4(1, 1, 0, 1);
    vec4 pos0011 = pos0000 + vec4(0, 0, 1, 1);
    vec4 pos1011 = pos0000 + vec4(1, 0, 1, 1);
    vec4 pos0111 = pos0000 + vec4(0, 1, 1, 1);
    vec4 pos1111 = pos0000 + vec4(1, 1, 1, 1);

    if (tile.x > 0 && tile.y > 0)
    {
        pos0000 = fract(pos0000 / tile) * tile;
        pos0100 = fract(pos0100 / tile) * tile;
        pos1000 = fract(pos1000 / tile) * tile;
        pos1100 = fract(pos1100 / tile) * tile;
        pos0010 = fract(pos0010 / tile) * tile;
        pos0110 = fract(pos0110 / tile) * tile;
        pos1010 = fract(pos1010 / tile) * tile;
        pos1110 = fract(pos1110 / tile) * tile;
        pos0001 = fract(pos0001 / tile) * tile;
        pos0101 = fract(pos0101 / tile) * tile;
        pos1001 = fract(pos1001 / tile) * tile;
        pos1101 = fract(pos1101 / tile) * tile;
        pos0011 = fract(pos0011 / tile) * tile;
        pos0111 = fract(pos0111 / tile) * tile;
        pos1011 = fract(pos1011 / tile) * tile;
        pos1111 = fract(pos1111 / tile) * tile;
    }

    vec4 t = fract(pos);
    return mix(
        mix(
            mix(
                mix(
                    hgNoiseNorm(seed, pos0000) * 2.0 - 1.0,
                    hgNoiseNorm(seed, pos1000) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                mix(
                    hgNoiseNorm(seed, pos0100) * 2.0 - 1.0,
                    hgNoiseNorm(seed, pos1100) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                hgSmooth(t.y)),
            mix(
                mix(
                    hgNoiseNorm(seed, pos0010) * 2.0 - 1.0,
                    hgNoiseNorm(seed, pos1010) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                mix(
                    hgNoiseNorm(seed, pos0110) * 2.0 - 1.0,
                    hgNoiseNorm(seed, pos1110) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                hgSmooth(t.y)),
            hgSmooth(t.z)),
        mix(
            mix(
                mix(
                    hgNoiseNorm(seed, pos0001) * 2.0 - 1.0,
                    hgNoiseNorm(seed, pos1001) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                mix(
                    hgNoiseNorm(seed, pos0101) * 2.0 - 1.0,
                    hgNoiseNorm(seed, pos1101) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                hgSmooth(t.y)),
            mix(
                mix(
                    hgNoiseNorm(seed, pos0011) * 2.0 - 1.0,
                    hgNoiseNorm(seed, pos1011) * 2.0 - 1.0,
                    hgSmooth(t.x)),
                mix(
                    hgNoiseNorm(seed, pos0111) * 2.0 - 1.0,
                    hgNoiseNorm(seed, pos1111) * 2.0 - 1.0,
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
 * - tile The distance before the repeating, or 0 to never repeat
 */
float hgPerlin1D(uint seed, float scale, float pos, float tile)
{
    pos /= scale;
    tile /= scale;

    float pos0 = floor(pos);
    float pos1 = pos0 + 1;

    if (tile > 0.0)
    {
        pos0 = fract(pos0 / tile) * tile;
        pos1 = fract(pos1 / tile) * tile;
    }

    float t = fract(pos);
    return mix(
        hgNoiseVec1D(seed, pos0) * t,
        hgNoiseVec1D(seed, pos1) * 1.0 - t,
        hgSmoothQuintic(t));
}

/**
 * Generate Perlin noise
 *
 * Parameters
 * - seed The seed for the noise function
 * - scale The number of positions before a new gradient
 * - pos The position for the noise function
 * - tile The distance before the repeating, or 0 to never repeat
 */
float hgPerlin2D(uint seed, float scale, vec2 pos, vec2 tile)
{
    pos /= scale;
    tile /= scale;

    vec2 pos00 = floor(pos);
    vec2 pos10 = pos00 + vec2(1, 0);
    vec2 pos01 = pos00 + vec2(0, 1);
    vec2 pos11 = pos00 + vec2(1, 1);

    if (tile.x > 0 && tile.y > 0)
    {
        pos00 = fract(pos00 / tile) * tile;
        pos01 = fract(pos01 / tile) * tile;
        pos10 = fract(pos10 / tile) * tile;
        pos11 = fract(pos11 / tile) * tile;
    }

    vec2 grad00 = hgNoiseVec2D(seed, pos00);
    vec2 grad10 = hgNoiseVec2D(seed, pos10);
    vec2 grad01 = hgNoiseVec2D(seed, pos01);
    vec2 grad11 = hgNoiseVec2D(seed, pos11);

    vec2 t = fract(pos);
    vec2 off00 = t;
    vec2 off10 = vec2(t.x - 1.0, t.y);
    vec2 off01 = vec2(t.x, t.y - 1.0);
    vec2 off11 = vec2(t.x - 1.0, t.y - 1.0);

    return mix(
        mix(dot(grad00, off00), dot(grad10, off10), hgSmoothQuintic(t.x)),
        mix(dot(grad01, off01), dot(grad11, off11), hgSmoothQuintic(t.x)),
        hgSmoothQuintic(t.y));
}

hgFractalNoise1DFunctionDef(hgFractalPerlin1D, hgPerlin1D)
hgFractalNoise2DFunctionDef(hgFractalPerlin2D, hgPerlin2D)

/**
 * A vertex
 */
struct HgVertex {
    vec3 position;
    float uvU;
    vec3 normal;
    float uvV;
    vec4 tangent;
};

/**
 * Transform a vertex, keeping its normal and tangent intact
 */
HgVertex hgTransformVertex(mat4 matrix, HgVertex vertex)
{
    vec3 position = (matrix * vec4(vertex.position, 1.0)).xyz;

    mat3 imatrix = transpose(inverse(mat3(matrix)));
    vec3 normal = imatrix * vertex.normal;
    vec4 tangent = vec4(imatrix * vertex.tangent.xyz, vertex.tangent.w);

    return HgVertex(
        position,
        vertex.uvU,
        normal,
        vertex.uvV,
        tangent);
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

#endif

#version 450

#include "hurdygurdy.glsl"

layout (location = 0) in VertexOutput {
    HgVertex vert;
} vIn;

struct Light {
    vec4 pos;
    vec4 color;
};

layout (HgStorageBuffer) readonly buffer Lights {
    Light lights[];
} lights[];

layout (HgCombinedImageSampler) uniform sampler2D uTextures[];

layout (push_constant) uniform Push {
    mat4 model;
    uint dirLights;
    uint dirLightCount;
    uint pointLights;
    uint pointLightCount;
    uint colorMap;
    uint normalMap;
    float diffuse;
    float shininess;
    uint verts;
    uint inds;
    uint vp;
} push;

layout (location = 0) out vec4 outColor;

void main()
{
    vec3 normal = hgTransformNormalMap(
        texture(
            uTextures[push.normalMap],
            vec2(vIn.vert.uvU, vIn.vert.uvV)).xyz,
        vIn.vert.normal,
        vIn.vert.tangent);

    vec3 viewDir = -normalize(vIn.vert.position);

    vec3 lighting = vec3(0.0);

    for (uint i = 0; i < push.dirLightCount; ++i)
    {
        Light light = lights[push.dirLights].lights[i];

        lighting +=
            light.color.xyz *
            light.color.w *
            (0.03 + hgBlinnPhong(
                normal,
                -normalize(light.pos.xyz),
                viewDir,
                16.0,
                0.7,
                0.3));
    }

    for (uint i = 0; i < push.pointLightCount; ++i)
    {
        Light light = lights[push.pointLights].lights[i];
        vec3 lightPosRel = light.pos.xyz - vIn.vert.position;

        lighting +=
            light.color.xyz *
            light.color.w *
            (hgBlinnPhong(
                normal,
                normalize(lightPosRel),
                viewDir,
                16.0,
                0.7,
                0.3)
            / dot(lightPosRel, lightPosRel));
    }

    vec3 color = lighting * texture(
        uTextures[push.colorMap],
        vec2(vIn.vert.uvU, vIn.vert.uvV)).xyz;
    outColor = vec4(hgAcesFittedTonemap(color), 1.0);
}


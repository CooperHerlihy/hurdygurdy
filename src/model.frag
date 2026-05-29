#version 460

#include "hurdygurdy.glsl"

layout (location = 0) in VertexOutput {
    HgVertex vertex;
} vIn;

struct DirLight {
    vec4 dir;
    vec4 color;
};

struct PointLight {
    vec4 pos;
    vec4 color;
};

layout (HgStorageBuffer) readonly buffer DirLights {
    DirLight lights[];
} dirLights[];

layout (HgStorageBuffer) readonly buffer PointLights {
    PointLight lights[];
} pointLights[];

layout (HgCombinedImageSampler) uniform sampler2D uTextures[];

layout (push_constant) uniform Push {
    mat4 model;
    uint indices;
    uint vertices;
    uint viewProj;
    uint normalMap;
    uint colorMap;
    uint dirLights;
    uint dirLightCount;
    uint pointLights;
    uint pointLightCount;
} push;

layout (location = 0) out vec4 outColor;

void main()
{
    vec3 normal = hgTransformNormalMap(
        texture(uTextures[push.normalMap], vIn.vertex.uvCoord).xyz,
        vIn.vertex.normal,
        vIn.vertex.tangent);

    vec3 lighting = vec3(0.0);

    vec3 viewDir = -normalize(vIn.vertex.position);

    for (uint i = 0; i < push.dirLightCount; ++i)
    {
        DirLight light = dirLights[push.dirLights].lights[i];

        lighting +=
            light.color.xyz *
            light.color.w *
            hgBlinnPhong(
                normal,
                -normalize(light.dir.xyz),
                viewDir,
                16.0,
                0.7,
                0.3,
                0.03);
    }

    for (uint i = 0; i < push.pointLightCount; ++i)
    {
        PointLight light = pointLights[push.pointLights].lights[i];

        vec3 lightPosRel = light.pos.xyz - vIn.vertex.position;
        lighting +=
            light.color.xyz *
            light.color.w *
            hgBlinnPhong(
                normal,
                normalize(lightPosRel),
                viewDir,
                16.0,
                0.7,
                0.3,
                0.03)
            / dot(lightPosRel, lightPosRel);
    }

    vec3 color = texture(
        uTextures[push.colorMap],
        vIn.vertex.uvCoord).xyz
        * lighting;
    outColor = vec4(hgAcesFittedTonemap(color), 1.0);
}


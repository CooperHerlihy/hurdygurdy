#version 460

#include "bindless.glsl"

layout (location = 0) out vec3 fPos;
layout (location = 1) out vec3 fNorm;
layout (location = 2) out vec4 fTan;
layout (location = 3) out vec2 fUV;

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNorm;
layout (location = 2) in vec4 vTan;
layout (location = 3) in vec2 vUV;

layout (binding = HgBinding_uniformBuffer) uniform VP {
    mat4 proj;
    mat4 view;
} uniformBuffers[];

layout (push_constant) uniform Push
{
    mat4 pModel;
    uint pVpIdx;
    uint pDirLightIdx;
    uint pDirLightCount;
    uint pPointLightIdx;
    uint pPointLightCount;
    uint pColorMapIdx;
    uint pNormalMapIdx;
};

void main()
{
    mat4 proj = uniformBuffers[pVpIdx].proj;
    mat4 view = uniformBuffers[pVpIdx].view;

    mat4 mv = view * pModel;
    mat3 imv = mat3(transpose(inverse(mv)));
    vec4 pos = mv * vec4(vPos, 1.0);

    fPos = pos.xyz;
    fNorm = imv * vNorm;
    fTan = vec4(imv * vTan.xyz, vTan.w);
    fUV = vUV;

    gl_Position = proj * pos;
}


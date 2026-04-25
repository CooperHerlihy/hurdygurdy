#version 460

#include "hurdygurdy.glsl"

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec4 inTangent;
layout (location = 3) in vec2 inUVCoord;

layout (HgUniformBuffer) uniform VP {
    mat4 proj;
    mat4 view;
} uniformBuffers[];

layout (push_constant) uniform Push {
    mat4 model;
    uint vpIdx;
    uint dirLightIdx;
    uint dirLightCount;
    uint pointLightIdx;
    uint pointLightCount;
    uint colorMapIdx;
    uint normalMapIdx;
} push;

layout (location = 0) out VertexOutput {
    HgVertex vertex;
} vOut;

void main()
{
    vOut.vertex = hgTransformVertex(
        HgVertex(inPosition, inNormal, inTangent, inUVCoord),
        uniformBuffers[push.vpIdx].view * push.model);

    gl_Position = uniformBuffers[push.vpIdx].proj * vec4(vOut.vertex.position, 1.0);
}


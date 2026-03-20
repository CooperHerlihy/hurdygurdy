#version 460

#include "bindless.glsl"

layout (location = 0) out vec2 fUV;

layout (binding = HgBinding_uniformBuffer) uniform ViewProjection {
    mat4 proj;
    mat4 view;
} uniformBuffers[];

layout (push_constant) uniform Push
{
    mat4 pModel;
    vec2 pUVPos;
    vec2 pUVSize;
    uint pVpIdx;
    uint pTexIdx;
};

const vec2 positions[] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

void main()
{
    mat4 proj = uniformBuffers[pVpIdx].proj;
    mat4 view = uniformBuffers[pVpIdx].view;

    fUV = positions[gl_VertexIndex] * pUVSize + pUVPos;

    vec2 vertexPos = positions[gl_VertexIndex] - vec2(0.5);
    gl_Position = proj * view * pModel * vec4(vertexPos, 0.0, 1.0);
}


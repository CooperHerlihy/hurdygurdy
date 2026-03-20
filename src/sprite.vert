#version 460

#include "bindless.glsl"

layout (location = 0) out vec2 fUV;

layout (binding = HgBinding_uniformBuffer) uniform ViewProjection {
    mat4 proj;
    mat4 view;
} uniformBuffers[];

layout (push_constant) uniform Push {
    mat4 model;
    vec2 uvPos;
    vec2 uvSize;
    uint vpIdx;
    uint texIdx;
} p;

const vec2 positions[] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0),
    vec2(0.0, 0.0)
);

void main()
{
    mat4 proj = uniformBuffers[p.vpIdx].proj;
    mat4 view = uniformBuffers[p.vpIdx].view;

    fUV = positions[gl_VertexIndex] * p.uvSize + p.uvPos;

    vec2 vertexPos = positions[gl_VertexIndex] - vec2(0.5);
    gl_Position = proj * view * p.model * vec4(vertexPos, 0.0, 1.0);
}


#version 460

#include "shader_utils.glsl"

layout (HgUniformBuffer) uniform ViewProjection {
    mat4 proj;
    mat4 view;
} uniformBuffers[];

layout (push_constant) uniform Push {
    mat4 model;
    vec2 uvPos;
    vec2 uvSize;
    uint vpIdx;
    uint texIdx;
} push;

layout (location = 0) out VertexOutput {
    vec2 vUVCoord;
};

void main()
{
    mat4 proj = uniformBuffers[push.vpIdx].proj;
    mat4 view = uniformBuffers[push.vpIdx].view;

    const vec2 positions[] = vec2[](
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 0.0)
    );

    vUVCoord = positions[gl_VertexIndex] * push.uvSize + push.uvPos;

    vec2 vertexPos = positions[gl_VertexIndex] - vec2(0.5);
    gl_Position = proj * view * push.model * vec4(vertexPos, 0.0, 1.0);
}


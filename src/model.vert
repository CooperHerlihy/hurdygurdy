#version 460

#include "hurdygurdy.glsl"

layout (HgStorageBuffer) readonly buffer Indices {
    uint indices[];
} indexBufs[];

layout (HgStorageBuffer) readonly buffer Vertices {
    HgVertex verts[];
} vertexBufs[];

layout (HgUniformBuffer) uniform VP {
    mat4 proj;
    mat4 view;
} uniformBuffers[];

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

layout (location = 0) out VertexOutput {
    HgVertex vertex;
} vOut;

void main()
{
    uint idx = indexBufs[push.indices].indices[gl_VertexIndex];
    HgVertex vert = vertexBufs[push.vertices].verts[idx];
    mat4 mv = uniformBuffers[push.viewProj].view * push.model;
    mat4 p = uniformBuffers[push.viewProj].proj;

    vOut.vertex = hgTransformVertex(vert, mv);
    gl_Position = p * vec4(vOut.vertex.position.xyz, 1.0);
}


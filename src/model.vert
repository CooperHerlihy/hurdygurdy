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
    uint indicesIdx;
    uint verticesIdx;
    uint viewProjIdx;
    uint normalMapIdx;
    uint colorMapIdx;
    uint dirLightIdx;
    uint dirLightCount;
    uint pointLightIdx;
    uint pointLightCount;
} push;

layout (location = 0) out VertexOutput {
    HgVertex vertex;
} vOut;

void main()
{
    uint idx = indexBufs[push.indicesIdx].indices[gl_VertexIndex];
    HgVertex vert = vertexBufs[push.verticesIdx].verts[idx];
    mat4 mv = uniformBuffers[push.viewProjIdx].view * push.model;
    mat4 p = uniformBuffers[push.viewProjIdx].proj;

    vOut.vertex = hgTransformVertex(vert, mv);
    gl_Position = p * vec4(vOut.vertex.position, 1.0);
}


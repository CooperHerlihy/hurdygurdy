#version 450

#include "hurdygurdy.glsl"

layout (HgUniformBuffer) uniform ViewProjection {
    mat4 proj;
    mat4 view;
} uVP[];

layout (HgStorageBuffer) readonly buffer Vertices {
    HgVertex verts[];
} vertBufs[];

layout (HgStorageBuffer) readonly buffer Indices {
    uint indices[];
} indexBufs[];

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


layout (location = 0) out VertexOutput {
    HgVertex vert;
} vOut;

void main()
{
    uint idx = indexBufs[push.indices].indices[gl_VertexIndex];
    HgVertex vert = vertexBufs[push.vertices].verts[idx];
    mat4 mv = uVP[push.viewProj].view * push.model;
    mat4 p = uVP[push.viewProj].proj;

    vOut.vert = hgTransformVertex(mv, vert);
    gl_Position = p * vec4(vOut.vert.position, 1.0);
}


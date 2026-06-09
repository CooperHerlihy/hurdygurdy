#version 460

#include "hurdygurdy.glsl"

layout (HgUniformBuffer) uniform ViewProjection {
    mat4 proj;
    mat4 view;
} uVP[];

struct Vertex {
    vec4 color;
    vec2 pos;
    vec2 texUV;
    uint texIdx;
    uint type;
    uint pad0;
    uint pad1;
};

layout (HgStorageBuffer) readonly buffer Vertices {
    Vertex verts[];
} vertexBufs[];

layout (HgStorageBuffer) readonly buffer Indices {
    uint indices[];
} indexBufs[];

layout (push_constant) uniform Push {
    mat4 model;
    uint vp;
    uint verts;
    uint inds;
} push;

layout (location = 0) out VertexOutput {
    vec4 color;
    vec2 texUV;
    flat uint texIdx;
    flat uint type;
} vOut;

void main()
{
    mat4 proj = uVP[push.vp].proj;
    mat4 view = uVP[push.vp].view;

    uint idx = indexBufs[push.inds].indices[gl_VertexIndex];
    Vertex vert = vertexBufs[push.verts].verts[idx];

    vOut.color = vert.color;
    vOut.texUV = vert.texUV;
    vOut.texIdx = vert.texIdx;
    vOut.type = vert.type;

    gl_Position = proj * view * push.model * vec4(vert.pos.xy, 0.0, 1.0);
}


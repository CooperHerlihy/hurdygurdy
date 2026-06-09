#version 460

#include "hurdygurdy.glsl"

layout (HgUniformBuffer) uniform ViewProjection {
    mat4 proj;
    mat4 view;
} uVP[];

struct VertexRect {
    vec2 pos;
    uint type;
    uint pad;
    vec4 color;
};

struct VertexSprite {
    vec2 pos;
    uint type;
    uint pad0;
    vec2 uv;
    uint tex;
    uint pad1;
};

layout (HgStorageBuffer) readonly buffer RectVertices {
    VertexRect verts[];
} rectBufs[];

layout (HgStorageBuffer) readonly buffer SpriteVertices {
    VertexSprite verts[];
} spriteBufs[];

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
    flat uint type;
    vec4 color;
    vec2 texUV;
    flat uint texIdx;
} vOut;

void main()
{
    mat4 proj = uVP[push.vp].proj;
    mat4 view = uVP[push.vp].view;

    uint idx = indexBufs[push.inds].indices[gl_VertexIndex];
    VertexRect rectVert = rectBufs[push.verts].verts[idx];
    VertexSprite spriteVert = spriteBufs[push.verts].verts[idx];

    vOut.type = rectVert.type;
    vOut.color = rectVert.color;
    vOut.texUV = spriteVert.uv;
    vOut.texIdx = spriteVert.tex;

    gl_Position = proj * view * push.model * vec4(rectVert.pos.xy, 0.0, 1.0);
}


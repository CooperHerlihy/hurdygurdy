#version 460

#include "hurdygurdy.glsl"

#define TYPE_RECT 0u
#define TYPE_SPRITE 1u

layout (HgUniformBuffer) uniform ViewProjection {
    mat4 proj;
    mat4 view;
} uVP[];

struct VertexRect {
    vec2 pos;
    vec2 size;
    uint type;
    uint pad[3];
    vec4 color;
};

struct VertexSprite {
    vec2 pos;
    vec2 size;
    uint type;
    uint pad[2];
    uint tex;
    vec2 uvPos;
    vec2 uvSize;
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
    uint inst;
} push;

layout (location = 0) out VertexOutput {
    flat uint type;
    flat uint texIdx;
    vec2 texUV;
    vec4 color;
} vOut;

void main()
{
    mat4 proj = uVP[push.vp].proj;
    mat4 view = uVP[push.vp].view;

    const vec2 positions[] = vec2[](
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 0.0)
    );

    VertexRect rectVert = rectBufs[push.inst].verts[gl_InstanceIndex];
    VertexSprite spriteVert = spriteBufs[push.inst].verts[gl_InstanceIndex];

    if (rectVert.type == TYPE_RECT)
    {
        vOut.type = rectVert.type;
        vOut.color = rectVert.color;
    }
    else if (spriteVert.type == TYPE_SPRITE)
    {
        vOut.type = spriteVert.type;
        vOut.texIdx = spriteVert.tex;
        vOut.texUV = positions[gl_VertexIndex] * spriteVert.uvSize + spriteVert.uvPos;
    }

    vec2 pos = rectVert.pos + rectVert.size * positions[gl_VertexIndex];

    gl_Position = proj * view * push.model * vec4(pos, 0.0, 1.0);
}


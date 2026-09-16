#version 450

#include "hurdygurdy.glsl"

layout (HgUniformBuffer) uniform ViewProjection {
    mat4 proj;
    mat4 view;
} uVP[];

struct Instance {
    vec4 color;
    vec2 pos;
    vec2 size;
    vec2 uvPos;
    vec2 uvSize;
    vec2 origin;
    float rotation;
    uint texIdx;
};

layout (HgStorageBuffer) readonly buffer Vertices {
    Instance verts[];
} vertBufs[];

layout (HgStorageBuffer) readonly buffer Indices {
    uint indices[];
} indexBufs[];

layout (push_constant) uniform Push {
    mat4 model;
    uint vp;
    uint inst;
} push;

layout (location = 0) out VertexOutput {
    vec4 color;
    vec2 texUV;
    flat uint texIdx;
} vOut;

void main()
{
    mat4 proj = uVP[push.vp].proj;
    mat4 view = uVP[push.vp].view;

    const vec2 positions[] = vec2[](
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(1.0, 1.0)
    );

    Instance vert = vertBufs[push.inst].verts[gl_InstanceIndex];

    vOut.color = vert.color;
    vOut.texUV = positions[gl_VertexIndex] * vert.uvSize + vert.uvPos;
    vOut.texIdx = vert.texIdx;

    mat2 rot = mat2(cos(vert.rotation), sin(vert.rotation), -sin(vert.rotation), cos(vert.rotation));
    vec2 pos = vert.pos + vert.size * (vert.origin + rot * (positions[gl_VertexIndex] - vert.origin));
    gl_Position = proj * view * push.model * vec4(pos, 0.0, 1.0);
}


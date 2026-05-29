#version 460

#include "hurdygurdy.glsl"

layout (location = 0) in VertexOutput {
    vec2 vUVCoord;
};

layout (HgCombinedImageSampler) uniform sampler2D uTextures[];

layout (push_constant) uniform Push {
    mat4 model;
    vec2 uvPos;
    vec2 uvSize;
    uint vp;
    uint tex;
} push;

layout (location = 0) out vec4 outColor;

void main()
{
    outColor = texture(uTextures[push.tex], vUVCoord);
    if (outColor.w == 0.0)
        discard;
}


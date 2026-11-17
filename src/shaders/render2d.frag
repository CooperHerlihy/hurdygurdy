#version 450

#include "hurdygurdy.glsl"

layout (HgCombinedImageSampler) uniform sampler2D uTextures[];

layout (location = 0) in VertexInput {
    vec4 color;
    vec2 texUV;
    flat uint texIdx;
} vIn;

layout (location = 0) out vec4 outColor;

void main()
{
    outColor = vIn.color;
    if (vIn.texIdx != UINT_MAX)
    {
        outColor *= texture(uTextures[vIn.texIdx], vIn.texUV);
    }
}


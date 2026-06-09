#version 460

#include "hurdygurdy.glsl"

#define TYPE_RECT 0u
#define TYPE_SPRITE 1u

layout (HgCombinedImageSampler) uniform sampler2D uTextures[];

layout (location = 0) in VertexInput {
    flat uint type;
    flat uint texIdx;
    vec2 texUV;
    vec4 color;
} vIn;

layout (location = 0) out vec4 outColor;

void main()
{
    if (vIn.type == TYPE_RECT)
    {
        outColor = vIn.color;
    }
    else if (vIn.type == TYPE_SPRITE)
    {
        outColor = texture(uTextures[vIn.texIdx], vIn.texUV);
    }
    else
    {
        outColor = vec4(0.0);
    }
}


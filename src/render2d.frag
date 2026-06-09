#version 460

#include "hurdygurdy.glsl"

#define TYPE_COLOR 0u
#define TYPE_TEXTURE 1u

layout (HgCombinedImageSampler) uniform sampler2D uTextures[];

layout (location = 0) in VertexInput {
    flat uint type;
    vec4 color;
    vec2 texUV;
    flat uint texIdx;
} vIn;

layout (location = 0) out vec4 outColor;

void main()
{
    if (vIn.type == TYPE_COLOR)
    {
        outColor = vIn.color;
    }
    else if (vIn.type == TYPE_TEXTURE)
    {
        outColor = texture(uTextures[vIn.texIdx], vIn.texUV);
    }
    else
    {
        outColor = vec4(0.0);
    }
}


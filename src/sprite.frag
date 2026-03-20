#version 460

#include "bindless.glsl"

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec2 vUV;

layout (binding = HgBinding_combinedImageSampler) uniform sampler2D uTextures[];

layout (push_constant) uniform Push
{
    mat4 pModel;
    vec2 pUVPos;
    vec2 pUVSize;
    uint uTexIdx;
};

void main()
{
    outColor = texture(uTextures[uTexIdx], vUV);
}


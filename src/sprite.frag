#version 460

#include "hurdygurdy.glsl"

layout (location = 0) in VertexOutput {
    vec2 vUVCoord;
};

layout (HgCombinedImageSampler) uniform sampler2D uTextures[];

layout (location = 0) out vec4 outColor;

layout (push_constant) uniform Push {
    mat4 model;
    vec2 uvPos;
    vec2 uvSize;
    uint vpIdx;
    uint texIdx;
} push;

void main()
{
    outColor = texture(uTextures[push.texIdx], vUVCoord);
}


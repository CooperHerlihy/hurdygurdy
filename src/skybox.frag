#version 460

#include "hurdygurdy.glsl"

layout (location = 0) in VertexOutput {
    vec3 vUVCoord;
};

layout (HgCombinedImageSampler) uniform samplerCube uTextures[];

layout (push_constant) uniform Push {
    uint vpIdx;
    uint texIdx;
} push;

layout (location = 0) out vec4 outColor;

void main()
{
    outColor = texture(uTextures[push.texIdx], vUVCoord);
}


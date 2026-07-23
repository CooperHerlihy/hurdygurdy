#version 460

#include "hurdygurdy.glsl"

layout(push_constant) uniform Push {
    uint texIdx;
} push;

layout(HgCombinedImageSampler) uniform sampler2D tex[];

layout(location = 0) out vec4 fragColor;

void main()
{
    ivec2 texSize = textureSize(tex[push.texIdx], 0);
    vec2 uv = gl_FragCoord.xy / vec2(texSize);
    fragColor = vec4(1.0) - texture(tex[push.texIdx], uv);
}

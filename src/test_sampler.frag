#version 460
#include "hurdygurdy.glsl"

layout(push_constant) uniform Push {
    vec2 uv;
    uint texIdx;
} push;

layout(HgCombinedImageSampler) uniform sampler2D tex[];

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor = texture(tex[push.texIdx], push.uv);
}

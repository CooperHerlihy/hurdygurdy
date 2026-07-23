#version 460
#include "hurdygurdy.glsl"

layout(push_constant) uniform Push {
    uint colorIdx;
} push;

layout(HgUniformBuffer) uniform ColorBlock {
    vec4 color;
} ubo[];

layout(location = 0) out vec4 vColor;

void main() {
    vec2 positions[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0));
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    vColor = ubo[push.colorIdx].color;
}

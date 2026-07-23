#version 460

#include "hurdygurdy.glsl"

layout(push_constant) uniform Push {
    uint dataIdx;
} push;

layout(HgUniformBuffer) uniform DataBuf {
    vec4 offsets[4];
} dataBufs[];

void main()
{
    vec2 quad[4] = vec2[](
        vec2( 0.0,  0.0),
        vec2( 1.0,  0.0),
        vec2( 0.0,  1.0),
        vec2( 1.0,  1.0)
    );
    vec2 off = dataBufs[push.dataIdx].offsets[gl_InstanceIndex].xy;
    gl_Position = vec4(quad[gl_VertexIndex] + off, 0.0, 1.0);
}

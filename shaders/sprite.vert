#version 450

layout(location = 0) out vec2 frag_uv;

layout(set = 0, binding = 0) uniform VpUniform {
    mat4 view;
    mat4 projection;
} u_vp;

layout(push_constant) uniform PushConstant {
    mat4 model;
    vec2 top_left;
    vec2 bottom_right;
} push;

// use triangle fan
const vec2[4] vertices = vec2[](
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0)
    );

void main() {
    gl_Position = u_vp.projection * u_vp.view * push.model * vec4(vertices[gl_VertexIndex] - 0.5, 0.0, 1.0);

    frag_uv = push.top_left * vertices[(gl_VertexIndex + 2) % 4] + push.bottom_right * vertices[gl_VertexIndex];
}

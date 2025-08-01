#version 450

layout(location = 0) out vec2 f_pos;

const vec2 vertices[3] = vec2[3](
    vec2(-1.0, -1.0),
    vec2(-1.0,  3.0),
    vec2( 3.0, -1.0)
);

void main() {
    gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);
    f_pos = vertices[gl_VertexIndex] / 2.0 + vec2(0.5);
}


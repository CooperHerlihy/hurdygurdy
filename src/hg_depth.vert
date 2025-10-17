#version 460

layout(location = 0) out vec2 f_position;

const vec2 positions[3] = vec2[3](
    vec2(-1.0, -1.0),
    vec2(3.0, -1.0),
    vec2(-1.0, 3.0)
);

void main() {
    f_position = positions[gl_VertexIndex] / 2.0 + 0.5;
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
}


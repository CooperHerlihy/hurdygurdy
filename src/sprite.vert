#version 460

layout (location = 0) out vec2 f_uv;

layout (binding = 0) uniform UViewProjection {
    mat4 u_proj;
    mat4 u_view;
};

layout (push_constant) uniform Push {
    mat4 p_model;
};

const vec2 positions[] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

void main() {
    f_uv = positions[gl_VertexIndex];

    vec2 vertex_pos = (positions[gl_VertexIndex] * 2.0) - vec2(1.0, 1.0);
    gl_Position = u_proj * u_view * p_model * vec4(vertex_pos, 0.0, 1.0);
}


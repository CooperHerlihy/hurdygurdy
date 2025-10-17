#version 460

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 tangent;
layout(location = 3) in vec2 uv;

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec4 v_tangent;
layout(location = 3) out vec2 v_uv;

layout(set = 0, binding = 0) uniform VPUniform {
    mat4 view;
    mat4 proj;
} u_vp;

layout(push_constant) uniform ModelPush {
    mat4 model;
} push;

void main() {
    vec4 model = vec4(position, 1.0);
    vec4 world = push.model * model;
    vec4 view = u_vp.view * world;

    v_position = view.xyz;
    v_normal = normal;
    v_tangent = tangent;
    v_uv = uv;

    gl_Position = u_vp.proj * view;
}


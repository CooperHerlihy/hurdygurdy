#version 450

layout(location = 0) out vec3 frag_position;
layout(location = 1) out vec3 frag_camera;
layout(location = 2) out vec3 frag_normal;
layout(location = 3) out vec2 frag_uv;

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_uv;

layout(set = 0, binding = 0) uniform VP {
    mat4 view;
    mat4 projection;
} u_vp;

layout(push_constant) uniform PushConstant {
    mat4 model;
} push;

void main() {
    vec4 model_position = vec4(in_position, 1.0);
    vec4 absolute_position = push.model * model_position;
    vec4 relative_position = u_vp.view * absolute_position;

    frag_position = absolute_position.xyz;
    frag_camera = inverse(u_vp.view)[3].xyz;
    frag_normal = in_normal;
    frag_uv = in_uv;

    gl_Position = u_vp.projection * relative_position;
}

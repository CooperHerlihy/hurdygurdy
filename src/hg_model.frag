#version 460

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec4 v_tangent;
layout(location = 3) in vec2 v_uv;

layout(location = 0) out vec4 f_color;

layout(set = 0, binding = 0) uniform VPUniform {
    mat4 view;
    mat4 proj;
} u_vp;

layout(set = 1, binding = 0) uniform sampler2D u_textures[2];

void main() {
    vec4 color = texture(u_textures[0], v_uv);

    mat3 tangent_to_world = mat3(
        v_tangent.xyz,
        cross(v_normal, v_tangent.xyz) * v_tangent.w,
        v_normal
    );
    vec3 normal = normalize(tangent_to_world * texture(u_textures[1], v_uv).xyz);

    f_color = color;
}


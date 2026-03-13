#version 460

layout (location = 0) out vec3 f_pos;
layout (location = 1) out vec3 f_norm;
layout (location = 2) out vec4 f_tan;
layout (location = 3) out vec2 f_uv;

layout (location = 0) in vec3 v_pos;
layout (location = 1) in vec3 v_norm;
layout (location = 2) in vec4 v_tan;
layout (location = 3) in vec2 v_uv;

layout (set = 0, binding = 0) uniform UViewProjection {
    mat4 u_proj;
    mat4 u_view;
    uint u_dir_light_count;
    uint u_point_light_count;
};

layout (push_constant) uniform Push {
    mat4 p_model;
};

void main() {
    const mat4 mv = u_view * p_model;
    const mat3 imv = mat3(transpose(inverse(mv)));
    const vec4 pos = mv * vec4(v_pos, 1.0);

    f_pos = pos.xyz;
    f_norm = imv * v_norm;
    f_tan = vec4(imv * v_tan.xyz, v_tan.w);
    f_uv = v_uv;

    gl_Position = u_proj * pos;
}


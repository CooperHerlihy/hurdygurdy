#version 460

layout (location = 0) out vec3 fPos;
layout (location = 1) out vec3 fNorm;
layout (location = 2) out vec4 fTan;
layout (location = 3) out vec2 fUV;

layout (location = 0) in vec3 vPos;
layout (location = 1) in vec3 vNorm;
layout (location = 2) in vec4 vTan;
layout (location = 3) in vec2 vUV;

layout (set = 0, binding = 0) uniform UViewProjection
{
    mat4 u_proj;
    mat4 u_view;
    uint u_dir_light_count;
    uint u_point_light_count;
};

layout (push_constant) uniform Push
{
    mat4 p_model;
};

void main()
{
    mat4 mv = u_view * p_model;
    mat3 imv = mat3(transpose(inverse(mv)));
    vec4 pos = mv * vec4(vPos, 1.0);

    fPos = pos.xyz;
    fNorm = imv * vNorm;
    fTan = vec4(imv * vTan.xyz, vTan.w);
    fUV = vUV;

    gl_Position = u_proj * pos;
}


#version 450

layout(location = 0) in vec2 in_pos;

layout(location = 0) out vec3 f_pos;
layout(location = 1) out vec3 f_dir;

layout(push_constant) uniform Push {
    mat4 camera;
    float aspect;
} push;

void main() {
    f_pos = (push.camera * vec4(vec3(0.0), 1.0)).xyz;
    f_dir = mat3(push.camera) * vec3(in_pos.x * push.aspect, in_pos.y, 1.0);

    gl_Position = vec4(in_pos, 1.0, 1.0);
}

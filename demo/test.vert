#version 450

layout(location = 0) out vec3 f_pos;
layout(location = 1) out vec2 f_uv;

layout(location = 0) in vec3 in_pos;
layout(location = 1) in vec2 in_uv;

layout(push_constant) uniform Push {
    vec2 offset;
} push;

void main() {
    f_uv = in_uv;

    vec3 pos = vec3(
        in_pos.x + push.offset.x,
        in_pos.y + push.offset.y,
        in_pos.z
    );

    f_pos = pos;
    gl_Position = vec4(pos, 1.0);
}


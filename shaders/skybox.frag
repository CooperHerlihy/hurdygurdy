#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 v_pos;

layout(set = 0, binding = 2) uniform samplerCube u_samplers[];

layout(push_constant) uniform PushConstant {
    uint cubemap_index;
} push;

void main() {
    out_color = texture(u_samplers[push.cubemap_index], v_pos);
}

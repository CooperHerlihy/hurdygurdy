#version 460

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec2 vUV;

layout (set = 1, binding = 0) uniform sampler2D uSprite;

void main() {
    outColor = texture(uSprite, vUV);
}


#version 460

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec3 fPos;
layout (location = 1) in vec3 fNorm;
layout (location = 2) in vec4 fTan;
layout (location = 3) in vec2 fUV;

layout (set = 0, binding = 0) uniform UViewProjection {
    mat4 uProj;
    mat4 uView;
    uint uDirLightCount;
    uint uPointLightCount;
};

struct DirectionalLight {
    vec4 dir;
    vec4 color;
};
layout (set = 0, binding = 1) readonly buffer DirectionalLights {
    DirectionalLight uDirLights[];
};

struct PointLight {
    vec4 pos;
    vec4 color;
};
layout (set = 0, binding = 2) readonly buffer PointLights {
    PointLight uPointLights[];
};

// [0] = color map, [1] = normal map
layout (set = 1, binding = 0) uniform sampler2D uTextures[2];

float blinnPhong(vec3 normal, vec3 lightDir, float shininess, float kd, float ks) {
    float ambient = 0.03;
    float diffuse = max(dot(normal, lightDir), 0.0);
    float specular = diffuse > 0.0
        ? pow(max(dot(normal, normalize(lightDir + normalize(-fPos))), 0.0), shininess)
        : 0.0;
    return ambient + diffuse * kd + specular * ks;
};

void main() {
    mat3 texToModel = mat3(
        fTan.xyz,
        cross(fTan.xyz, fNorm) * fTan.w,
        -fNorm
    );
    vec3 normal = normalize(texToModel * texture(uTextures[1], fUV).xyz);

    vec3 lighting = vec3(0.0);

    for (uint i = 0; i < uDirLightCount; ++i) {
        vec3 lightDir = -normalize(uDirLights[i].dir.xyz);
        vec3 lightColor = uDirLights[i].color.xyz * uDirLights[i].color.w;
        lighting += blinnPhong(normal, lightDir, 16.0, 0.7, 0.3) * lightColor;
    }

    for (uint i = 0; i < uPointLightCount; ++i) {
        vec3 lightPos = uPointLights[i].pos.xyz;
        vec3 lightDiff = lightPos - fPos;
        float lightDist = dot(lightDiff, lightDiff);
        vec3 lightDir = normalize(lightDiff);
        vec3 lightColor = uPointLights[i].color.xyz * uPointLights[i].color.w;
        lighting += blinnPhong(normal, lightDir, 16.0, 0.7, 0.3) * lightColor / lightDist;
    }

    vec4 hdrColor = vec4(lighting, 1.0) * texture(uTextures[0], fUV);
    vec4 ldrColor = vec4(1.0) - exp(-hdrColor);
    outColor = hdrColor;
}


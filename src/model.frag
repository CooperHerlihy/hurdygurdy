#version 460

#include "bindless.glsl"

layout (location = 0) out vec4 outColor;

layout (location = 0) in vec3 fPos;
layout (location = 1) in vec3 fNorm;
layout (location = 2) in vec4 fTan;
layout (location = 3) in vec2 fUV;

struct DirLight
{
    vec4 dir;
    vec4 color;
};

struct PointLight
{
    vec4 pos;
    vec4 color;
};

layout (binding = HgBinding_storageBuffer) readonly buffer DirLights {
    DirLight lights[];
} dirLights[];

layout (binding = HgBinding_storageBuffer) readonly buffer PointLights {
    PointLight lights[];
} pointLights[];

layout (binding = HgBinding_combinedImageSampler) uniform sampler2D uTextures[];

layout (push_constant) uniform Push {
    mat4 model;
    uint vpIdx;
    uint dirLightIdx;
    uint dirLightCount;
    uint pointLightIdx;
    uint pointLightCount;
    uint colorMapIdx;
    uint normalMapIdx;
} p;

float blinnPhong(vec3 normal, vec3 lightDir, float shininess, float kd, float ks)
{
    float ambient = 0.03;
    float diffuse = max(dot(normal, lightDir), 0.0);
    float specular = diffuse > 0.0
        ? pow(max(dot(normal, normalize(lightDir + normalize(-fPos))), 0.0), shininess)
        : 0.0;
    return ambient + diffuse * kd + specular * ks;
};

void main()
{
    mat3 texToModel = mat3(
        fTan.xyz,
        cross(fTan.xyz, fNorm) * fTan.w,
        -fNorm
    );
    vec3 normal = normalize(texToModel * texture(uTextures[p.normalMapIdx], fUV).xyz);

    vec3 lighting = vec3(0.0);

    for (uint i = 0; i < p.dirLightCount; ++i) {
        DirLight light = dirLights[p.dirLightIdx].lights[i];
        vec3 lightDir = -normalize(light.dir.xyz);
        vec3 lightColor = light.color.xyz * light.color.w;
        lighting += blinnPhong(normal, lightDir, 16.0, 0.7, 0.3) * lightColor;
    }

    for (uint i = 0; i < p.pointLightCount; ++i) {
        PointLight light = pointLights[p.pointLightIdx].lights[i];
        vec3 lightPos = light.pos.xyz;
        vec3 lightDiff = lightPos - fPos;
        float lightDist = dot(lightDiff, lightDiff);
        vec3 lightDir = normalize(lightDiff);
        vec3 lightColor = light.color.xyz * light.color.w;
        lighting += blinnPhong(normal, lightDir, 16.0, 0.7, 0.3) * lightColor / lightDist;
    }

    vec4 hdrColor = vec4(lighting, 1.0) * texture(uTextures[p.colorMapIdx], fUV);
    vec4 ldrColor = vec4(1.0) - exp(-hdrColor);
    outColor = hdrColor;
}


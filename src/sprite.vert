#version 460

layout (location = 0) out vec2 fUV;

layout (set = 0, binding = 0) uniform UViewProjection
{
    mat4 uProj;
    mat4 uView;
};

layout (push_constant) uniform Push
{
    mat4 pModel;
    vec2 pUVPos;
    vec2 pUVSize;
};

const vec2 positions[] = vec2[](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0),
    vec2(1.0, 0.0)
);

void main()
{
    fUV = positions[gl_VertexIndex] * pUVSize + pUVPos;

    vec2 vertexPos = positions[gl_VertexIndex] - vec2(0.5);
    gl_Position = uProj * uView * pModel * vec4(vertexPos, 0.0, 1.0);
}


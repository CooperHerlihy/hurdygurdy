#pragma once

#include "hg/render2d.hpp"

namespace hg {

/**
 * A vertex in a mesh
 */
struct MeshVertex {
    /**
     * The vertex position
     */
    Vec3 pos{};
    /**
     * The u part of the vertex uv coordinate
     */
    f32 uvU = 0.0f;
    /**
     * The vertex normal
     */
    Vec3 norm{};
    /**
     * The v part of the vertex uv coordinate
     */
    f32 uvV = 0.0f;
    /**
     * The vertex tangent
     */
    Vec4 tan{};

    /**
     * Construct the vertex
     */
    MeshVertex(Vec3 pos, Vec3 norm, Vec4 tan, Vec2 uv)
        : pos{pos}, uvU{uv.x}, norm{norm}, uvV{uv.y}, tan{tan}
    {}
};

/**
 * A 3D mesh asset
 */
struct MeshData {
    /**
     * The file index of the first vertex
     */
    Array<MeshVertex> vertices{};
    /**
     * The file index of the first geometry index
     */
    Array<u32> indices{};
    /**
     * How the vertices should be interpreted in sequence
     */
    GpuTopology topology = {};
};

/**
 * Store the model data to disc in gltf format : TODO
 */
void meshStoreGltf(MeshData* data, StringView path);

/**
 * Mesh asset load implementation
 */
template<>
void assetLoadImpl(AssetData<MeshData>* data);

/**
 * A 3D mesh asset stored on the gpu
 */
struct Mesh {
    /**
     * The vertex buffer
     */
    GpuBuffer vertexBuffer{};
    /**
     * The index buffer
     **/
    GpuBuffer indexBuffer{};
    /**
     * The number of vertices
     */
    u32 vertexCount = 0;
    /**
     * The size of each vertex in bytes
     */
    u32 vertexWidth = 0;
    /**
     * The number of indices (4 bytes each)
     */
    u32 indexCount = 0;
};

/**
 * GpuMesh asset load implementation
 */
template<>
void assetLoadImpl(AssetData<Mesh>* data);

/**
 * A 3D model
 */
struct Material3D {
    /**
     * The 3D geometry mesh
     */
    Asset<Mesh> mesh{};
    /**
     * The color map
     */
    Asset<Texture> colorMap;
    /**
     * The normal map
     */
    Asset<Texture> normalMap;
    /**
     * The lighting diffuse proportion
     */
    f32 diffuse;
    /**
     * The lighting shininess value
     */
    f32 shininess;
};

/**
 * Serialization for Material3D
 */
template<>
void serialize<>(Serializer* s, Material3D* material);

/**
 * A basic 3D renderer
 */
struct Renderer3D {
    /**
     * The light data
     */
    struct LightData {
        /**
         * The color of the light (brightness is implicit)
         */
        Vec3 color;
        /**
         * The position of the point light, or direction of the directional light
         */
        Vec3 pos;
    };

    /**
     * The model data
     */
    struct ModelData {
        /**
         * The position, scale, rotation, etc.
         */
        Mat4 transform{};
        /**
         * The 3D geometry mesh
         */
        const Mesh* mesh{};
        /**
         * The color map
         */
        const Texture* colorMap;
        /**
         * The normal map
         */
        const Texture* normalMap;
        /**
         * The lighting diffuse proportion
         */
        f32 diffuse;
        /**
         * The lighting shininess value
         */
        f32 shininess;
    };

    /**
     * The render pipeline
     */
    GpuPipeline pipeline{};
    /**
     * The directional light gpu buffer
     */
    GpuBuffer dirLightBuffer{};
    /**
     * The point light gpu buffer
     */
    GpuBuffer pointLightBuffer{};
    /**
     * The directional lights
     */
    Array<LightData> dirLights{};
    /**
     * The point lights
     */
    Array<LightData> pointLights{};
    /**
     * The models to render this frame
     */
    Array<ModelData> models{};

    /**
     * Create uninitialized
     */
    Renderer3D() noexcept = default;

    /**
     * Create pipeline
     */
    Renderer3D(Format colorFormat, Format depthFormat);

    /**
     * Add a directional light to the scene
     */
    void drawDirectionalLight(Vec3 color, Vec3 dir);

    /**
     * Add a point light to the scene
     */
    void drawPointLight(Vec3 color, Vec3 pos);

    /**
     * Add a model to the scene
     */
    void drawModel(
        const Mesh& mesh,
        const Texture& colorMap,
        const Texture& normalMap,
        f32 diffuse,
        f32 shininess,
        const Mat4& transform);

    /**
     * Add a model to the scene
     */
    void drawModelMaterial(const Material3D& material, const Mat4& transform);

    /**
     * Issue gpu commands to render the scene then clear the scene
     */
    void renderScene(GpuCmd* cmd, Camera& camera);
};

} // namespace hg

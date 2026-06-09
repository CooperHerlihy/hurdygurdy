/*
 * =============================================================================
 *
 * Copyright (c) 2025 Cooper Herlihy
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * =============================================================================
 */

#ifndef HG_RENDERING_HPP
#define HG_RENDERING_HPP

#include "hg_assets.hpp"
#include "hg_concurrency.hpp"
#include "hg_core.hpp"
#include "hg_ecs.hpp"
#include "hg_gpu.hpp"
#include "hg_math.hpp"
#include "hg_serialization.hpp"
#include "hg_strings.hpp"
#include "hg_window.hpp"

/**
 * A texture asset
 */
struct HgTextureData {
    /**
     * The width of the texture in pixels
     */
    u32 width;
    /**
     * The height of the texture in pixels
     */
    u32 height;
    /**
     * The depth of the texture in pixels
     */
    u32 depth;
    /**
     * The format of each pixel
     */
    HgFormat format;
    /**
     * The pixel data, aligned to 16 bytes
     */
    void* pixels;
};

/**
 * A handle to a texture
 */
typedef HgAsset<HgTextureData> HgTextureDataAsset;

/**
 * HgTexture asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAsset<HgTextureData>* data);

/**
 * HgTexture asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAsset<HgTextureData>* data);

/**
 * Store an image to disc in the png format
 */
void hgTextureStorePng(HgTextureData* texture, HgStringView path, HgFence* fence);

/**
 * A texture asset stored on the gpu
 */
struct HgTexture {
    /**
     * The image
     */
    HgGpuImage* image;
    /**
     * The image view
     */
    HgGpuView* view;
};

/**
 * A handle to a texture asset
 */
typedef HgAsset<HgTexture> HgTextureAsset;

/**
 * HgGpuTexture asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAsset<HgTexture>* data);

/**
 * HgGpuTexture asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAsset<HgTexture>* data);
/**
 * The types of camera projections
 */
enum HgCameraType : u32 {
    HgCameraType_perspective,
    HgCameraType_orthographic,
};

/**
 * A perspective camera
 */
struct HgCameraPerspective {
    /**
     * The aspect ratio
     */
    f32 aspect;
    /**
     * The field of view
     */
    f32 fov;
    /**
     * The near clipping plane
     */
    f32 near;
    /**
     * The far clipping plane
     */
    f32 far;
};

/**
 * An orthographic camera
 */
struct HgCameraOrthographic {
    /**
     * The clipping planes in each direction
     */
    f32 left, right, top, bottom, near, far;
};

/**
 * A camera component
 */
struct HgCamera {
    /**
     * The gpu view projection data, created/destroyed on add/remove
     */
    HgGpuBuffer* vpBuffer;
    /**
     * The current rotation
     */
    HgQuat rotation;
    /**
     * The current position
     */
    HgVec3 position;
    /**
     * The type of projection
     */
    HgCameraType type;
    /**
     * The projection data
     */
    union {
        /**
         * An orthographic camera
         */
        HgCameraOrthographic orthographic;
        /**
         * A perspective camera
         */
        HgCameraPerspective perspective;
    };
};

/**
 * HgCamera serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgCamera* camera);

/**
 * Create a camera
 */
HgCamera hgCameraCreate();

/**
 * Destroy a camera
 */
void hgCameraDestroy(HgCamera* camera);

/**
 * The the camera to a perspective projection
 */
void hgCameraSetPerspective(HgCamera* camera, f32 aspect, f32 fov = (f32)hgPi / 2.0f, f32 near = 0.01f, f32 far = 1000.0f);

/**
 * The the camera to an orthographic projection using reasonable defaults
 */
void hgCameraSetOrthographic(HgCamera* camera, f32 aspect);

/**
 * Update the camera's gpu side data
 */
void hgCameraUpdate(HgCamera* camera);

/**
 * HgCamera ecs add implementation
 */
HgCamera* hgCameraAdd(HgEcs* ecs, HgEntity e);

/**
 * HgCamera ecs destructor
 */
template<>
void hgEcsDtor(HgCamera* camera);

/**
 * Update the camera's gpu side data, must have a camera and transform
 */
void hgCameraUpdateEcs(HgEcs* ecs, HgEntity e);

/**
 * Initialize the 2D renderer
 */
void hgInit2D(HgFormat colorFormat);

/**
 * Deinitialize the 2D renderer
 */
void hgDeinit2D();

/**
 * The 2D vertex types
 */
enum HgVertexType2D : u32 {
    /**
     * A vertex with a color value
     */
    HgVertexType2D_color = 0,
    /**
     * A vertex with a pointer to a texture
     */
    HgVertexType2D_texture = 1,
};

/**
 * A vertex in a 2D layer
 */
struct HgVertex2D {
    /**
     * The color, if a color vertex
     */
    HgVec4 color;
    /**
     * The vertex position
     */
    HgVec2 pos;
    /**
     * The texture uv coord, if a texture vertex
     */
    HgVec2 texUV;
    /**
     * The texture descriptor index, if a texture vertex
     */
    u32 texIdx;
    /**
     * The type of vertex
     */
    HgVertexType2D type;
    /**
     * Padding to match glsl
     */
    u32 pad0;
    /**
     * Padding to match glsl
     */
    u32 pad1;
};

/**
 * A 2D render layer
 */
struct HgLayer2D {
    /**
     * The gpu vertex buffer
     */
    HgGpuBuffer* vertexBuffer;
    /**
     * The gpu index buffer
     */
    HgGpuBuffer* indexBuffer;
    /**
     * The current capacity of the vertex buffer
     */
    u32 vertexBufferCapacity;
    /**
     * The current capacity of the index buffer
     */
    u32 indexBufferCapacity;
    /**
     * The vertex data
     */
    HgArray<HgVertex2D> vertices;
    /**
     * The index data
     */
    HgArray<u32> indices;
    /**
     * The transform, does not affect changed
     */
    HgMat4 transform;
    /**
     * Whether the gpu data needs to be updated
     */
    bool changed;
};

/**
 * Create a 2D render layer
 */
HgLayer2D hgLayerCreate2D();

/**
 * Destroy a 2D render layer
 */
void hgLayerDestroy2D(HgLayer2D* layer);

/**
 * Remove all drawings from the layer
 */
void hgLayerClear2D(HgLayer2D* layer);

/**
 * Draw a rectangle on the layer
 */
void hgRect2D(HgLayer2D* layer, HgVec2 pos, HgVec2 size, HgVec4 color);

/**
 * A 2D sprite which can be drawn
 */
struct HgSprite2D {
    /**
     * The sprite's texture
     */
    HgTextureAsset* texture;
    /**
     * The u coord in the texture
     */
    f32 u;
    /**
     * The v coord in the texture
     */
    f32 v;
    /**
     * The width in the texture
     */
    f32 width;
    /**
     * The height in the texture
     */
    f32 height;
};

/**
 *  Draw the sprite on the layer
 */
void hgSprite2D(HgLayer2D* layer, HgVec2 pos, HgVec2 size, HgSprite2D* sprite);

/**
 * Issue draw commands for a 2D scene
 */
void hgDraw2D(HgGpuCmd* cmd, HgCamera* camera, HgLayer2D* layer);

/**
 * A vertex in a mesh
 */
struct HgMeshVertex {
    /**
     * The vertex position
     */
    HgVec3 pos;
    /**
     * The u part of the vertex uv coordinate
     */
    f32 uvU;
    /**
     * The vertex normal
     */
    HgVec3 norm;
    /**
     * The v part of the vertex uv coordinate
     */
    f32 uvV;
    /**
     * The vertex tangent
     */
    HgVec4 tan;

    /**
     * Construct the vertex
     */
    HgMeshVertex(HgVec3 pos, HgVec3 norm, HgVec4 tan, HgVec2 uv)
        : pos{pos}, uvU{uv.x}, norm{norm}, uvV{uv.y}, tan{tan} {}
};

/**
 * A 3d mesh asset
 */
struct HgMeshData {
    /**
     * The file index of the first vertex
     */
    HgMeshVertex* vertices;
    /**
     * The file index of the first geometry index
     */
    u32* indices;
    /**
     * The number of vertices
     */
    u32 vertexCount;
    /**
     * The size of each vertex in bytes
     */
    u32 vertexWidth;
    /**
     * The number of indices (4 bytes each)
     */
    u32 indexCount;
    /**
     * How the vertices should be interpreted in sequence
     */
    HgGpuTopology topology;
};

/**
 * A handle to a 3d mesh asset
 */
typedef HgAsset<HgMeshData> HgMeshDataAsset;

/**
 * HgMesh asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAsset<HgMeshData>* data);

/**
 * HgMesh asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAsset<HgMeshData>* data);

/**
 * Store the model data to disc in gltf format : TODO
 */
void hgMeshStoreGltf(HgMeshData* data, HgStringView path, HgFence* fence);

/**
 * A 3d mesh asset stored on the gpu
 */
struct HgMesh {
    /**
     * The vertex buffer
     */
    HgGpuBuffer* vertexBuffer;
    /**
     * The index buffer
     **/
    HgGpuBuffer* indexBuffer;
    /**
     * The number of vertices
     */
    u32 vertexCount;
    /**
     * The size of each vertex in bytes
     */
    u32 vertexWidth;
    /**
     * The number of indices (4 bytes each)
     */
    u32 indexCount;
};

/**
 * A gpu mesh asset handle
 */
typedef HgAsset<HgMesh> HgMeshAsset;

/**
 * HgGpuMesh asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAsset<HgMesh>* data);

/**
 * HgGpuMesh asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAsset<HgMesh>* data);


/**
 * Initialize the sprite pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be undefined
 * - depthFormat The format of the depth attachment, must not be undefined
 */
void hgSpritesInit(HgFormat colorFormat, HgFormat depthFormat);

/**
 * Deinitialize the sprite pipeline
 */
void hgSpritesDeinit();

/**
 * A sprite component
 */
struct HgSprite {
    /**
     * The texture to draw from
     */
    HgTextureAsset* texture = nullptr;
    /**
     * The beginning coordinate to read from texture, [0.0, 1.0]
     */
    HgVec2 uvPos{0.0f, 0.0f};
    /**
     * The size of the region to read from texture, [0.0, 1.0]
     */
    HgVec2 uvSize{1.0f, 1.0f};
};

/**
 * HgSprite serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgSprite* sprite);

/**
 * Add a sprite to an entity
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to add to
 * - texture A copy of the sprite's texture
 */
HgSprite* hgSpriteAdd(
    HgEcs* ecs,
    HgEntity e,
    HgTextureAsset* texture,
    HgVec2 uvPos = HgVec2{0.0f},
    HgVec2 uvSize = HgVec2{1.0f});

/**
 * HgSprite ecs destructor
 */
template<>
void hgEcsDtor(HgSprite* sprite);

/**
 * Issue draw commands for all HgSprite2D components in the ecs
 *
 * Parameters
 * - ecs The ecs to draw
 * - camera The camera entity to use
 * - cmd The command buffer to record to, must not be nullptr
 */
void hgSpritesDraw(HgEcs* ecs, HgEntity camera, HgGpuCmd* cmd);

/**
 * Initialize the skybox pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be undefined
 * - depthFormat The format of the depth attachment, if used
 */
void hgSkyboxInit(HgFormat colorFormat, HgFormat depthFormat);

/**
 * Deinitialize the skybox pipeline
 */
void hgSkyboxDeinit();

/**
 * A skybox component
 */
struct HgSkybox {
    /**
     * The cubemap texture
     */
    HgTextureAsset* texture;
};

/**
 * HgSkybox serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgSkybox* skybox);

/**
 * Add a skybox to an entity
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to add to
 * - texture A copy of the skybox's texture
 */
HgSkybox* hgSkyboxAdd(HgEcs* ecs, HgEntity e, HgTextureAsset* texture);

/**
 * HgSkybox ecs destructor
 */
template<>
void hgEcsDtor(HgSkybox* skybox);

/**
 * Draw a skybox from a texture
 *
 * Parameters
 * - ecs The ecs with the camera
 * - camera The camera entity to use
 * - cmd The command buffer to record to, must not be nullptr
 */
void hgSkyboxDraw(HgEcs* ecs, HgEntity camera, HgGpuCmd* cmd);

/**
 * A directional light component
 */
struct HgDirLight {
    /**
     * The direction of the light
     */
    HgVec3 dir;
    /**
     * The color of the light
     */
    HgVec4 color;
};

/**
 * HgDirLight serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgDirLight* light);

/**
 * Add a directional light to an entity
 */
HgDirLight* hgDirLightAdd(HgEcs* ecs, HgEntity e, HgVec3 dir, HgVec4 color);

/**
 * A point light component, should have a transform
 */
struct HgPointLight {
    /**
     * The color of the light
     */
    HgVec4 color;
};

/**
 * HgPointLight serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgPointLight* light);

/**
 * Add a point light to an entity
 */
HgPointLight* hgPointLightAdd(HgEcs* ecs, HgEntity e, HgVec4 color);

/**
 * Initialize the 3D model pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be undefined
 * - depthFormat The format of the depth attachment, must not be undefined
 */
void hgModelsInit(HgFormat colorFormat, HgFormat depthFormat);

/**
 * Deinitialize the 3D model pipeline
 */
void hgModelsDeinit();

/**
 * A 3D model component
 */
struct HgModel {
    /**
     * The model to render
     */
    HgMeshAsset* mesh;
    /**
     * The model's color map
     */
    HgTextureAsset* colorMap;
    /**
     * The model's normal map
     */
    HgTextureAsset* normalMap;
};

/**
 * HgModel serialization
 */
template<>
void hgSerialize(HgSerializer* s, HgModel* model);

/**
 * Add a model to an entity
 */
HgModel* hgModelAdd(
    HgEcs* ecs,
    HgEntity e,
    HgMeshAsset* mesh,
    HgTextureAsset* colorMap,
    HgTextureAsset* normalMap);

/**
 * HgModel ecs destructor
 */
template<>
void hgEcsDtor(HgModel* model);

/**
 * Issue draw commands for all HgModel3D components in the ecs
 *
 * Parameters
 * - ecs The ecs to draw
 * - camera The camera entity to use
 * - cmd The command buffer to record to, must not be nullptr
 */
void hgModelsDraw(HgEcs* ecs, HgEntity camera, HgGpuCmd* cmd);

/**
 * Initialize ImGui platform backend
 *
 * Note, requires GLFW on Linux (for now)
 *
 * Parameters
 * - window The window for ImGui to use
 * - colorFormat The format the color target will be in
 * - depthFormat The format the depth buffer will be in, if used
 * - stencilFormat The format the stencil will be in, if used
 */
void hgImGuiInit(
    HgWindow* window,
    HgFormat colorFormat,
    HgFormat depthFormat = HgFormat_undefined,
    HgFormat stencilFormat = HgFormat_undefined);

/**
 * Deinitializes ImGui platform backend
 */
void hgImGuiDeinit();

/**
 * Create an ImGui texture
 */
void* hgImGuiTextureCreate(HgGpuView* view, HgGpuLayout layout);

/**
 * Create an ImGui texture
 */
void hgImGuiTextureDestroy(void* texture);

/**
 * Create a new ImGui frame for the platform backend
 */
void hgImGuiNewFrame();

/**
 * Draw the ImGui frame
 *
 * Parameters
 * - cmd The command buffer to record to
 */
void hgImGuiDraw(HgGpuCmd* cmd);

#endif // HG_RENDERING_HPP

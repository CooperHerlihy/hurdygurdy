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
#include "hg_memory.hpp"
#include "hg_serialization.hpp"
#include "hg_strings.hpp"
#include "hg_window.hpp"

/**
 * A texture asset
 */
struct HgTexture {
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
typedef HgAsset<HgTexture> HgTextureAsset;

/**
 * HgTexture asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAsset<HgTexture>* data);

/**
 * HgTexture asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAsset<HgTexture>* data);

/**
 * Store an image to disc in the png format
 */
void hgTextureStorePng(HgTexture* texture, HgStringView path, HgFence* fence);

/**
 * A texture asset stored on the gpu
 */
struct HgGpuTexture {
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
typedef HgAsset<HgGpuTexture> HgGpuTextureAsset;

/**
 * HgGpuTexture asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAsset<HgGpuTexture>* data);

/**
 * HgGpuTexture asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAsset<HgGpuTexture>* data);

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
struct HgMesh {
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
typedef HgAsset<HgMesh> HgMeshAsset;

/**
 * HgMesh asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAsset<HgMesh>* data);

/**
 * HgMesh asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAsset<HgMesh>* data);

/**
 * Store the model data to disc in gltf format : TODO
 */
void hgMeshStoreGltf(HgMesh* data, HgStringView path, HgFence* fence);

/**
 * A 3d mesh asset stored on the gpu
 */
struct HgGpuMesh {
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
typedef HgAsset<HgGpuMesh> HgGpuMeshAsset;

/**
 * HgGpuMesh asset load implementation
 */
template<>
void hgAssetLoadImpl(HgAsset<HgGpuMesh>* data);

/**
 * HgGpuMesh asset unload implementation
 */
template<>
void hgAssetUnloadImpl(HgAsset<HgGpuMesh>* data);

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
     * The field of view
     */
    f32 fov;
    /**
     * The aspect ratio
     */
    f32 aspect;
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
 * HgCameraPerspective serialization
 */
template<>
void hgSerializeImpl(HgSerializer* s, HgCameraPerspective* camera);

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
 * HgCameraOrthographic serialization
 */
template<>
void hgSerializeImpl(HgSerializer* s, HgCameraOrthographic* camera);

/**
 * A camera component
 */
struct HgCamera {
    /**
     * The gpu view projection data, created/destroyed on add/remove
     */
    HgGpuBuffer* vpBuffer;
    /**
     * The type of camera
     */
    HgCameraType type;
    /**
     * An orthographic camera
     */
    HgCameraOrthographic orthographic;
    /**
     * A perspective camera
     */
    HgCameraPerspective perspective;
};

/**
 * HgCamera serialization
 */
template<>
void hgSerializeImpl(HgSerializer* s, HgCamera* camera);

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
void hgCameraUpdate(HgEcs* ecs, HgEntity e);

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
    HgGpuTextureAsset* texture = nullptr;
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
void hgSerializeImpl(HgSerializer* s, HgSprite* sprite);

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
    HgGpuTextureAsset* texture,
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
    HgGpuTextureAsset* texture;
};

/**
 * HgSkybox serialization
 */
template<>
void hgSerializeImpl(HgSerializer* s, HgSkybox* skybox);

/**
 * Add a skybox to an entity
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to add to
 * - texture A copy of the skybox's texture
 */
HgSkybox* hgSkyboxAdd(HgEcs* ecs, HgEntity e, HgGpuTextureAsset* texture);

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
void hgSerializeImpl(HgSerializer* s, HgDirLight* light);

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
void hgSerializeImpl(HgSerializer* s, HgPointLight* light);

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
    HgGpuMeshAsset* mesh;
    /**
     * The model's color map
     */
    HgGpuTextureAsset* colorMap;
    /**
     * The model's normal map
     */
    HgGpuTextureAsset* normalMap;
};

/**
 * HgModel serialization
 */
template<>
void hgSerializeImpl(HgSerializer* s, HgModel* model);

/**
 * Add a model to an entity
 */
HgModel* hgModelAdd(
    HgEcs* ecs,
    HgEntity e,
    HgGpuMeshAsset* mesh,
    HgGpuTextureAsset* colorMap,
    HgGpuTextureAsset* normalMap);

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

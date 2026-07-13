/*
 * =============================================================================
 *
 * Copyright (c) 2025-2026 Cooper Herlihy
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
#include "hg_window.hpp"

namespace hg {

/**
 * The types of camera projections
 */
enum CameraType : u32 {
    CameraType_perspective,
    CameraType_orthographic,
};

/**
 * A perspective camera
 */
struct CameraPerspective {
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
struct CameraOrthographic {
    /**
     * The clipping planes in each direction
     */
    f32 left, right, top, bottom, near, far;
};

/**
 * A camera component
 */
struct Camera {
    /**
     * The gpu view projection data
     */
    GpuBuffer* vpBuffer;
    /**
     * The current rotation
     */
    Quat rotation;
    /**
     * The current position
     */
    Vec3 position;
    /**
     * The type of projection
     */
    CameraType type;
    /**
     * The projection data
     */
    union {
        /**
         * An orthographic camera
         */
        CameraOrthographic orthographic;
        /**
         * A perspective camera
         */
        CameraPerspective perspective;
    };
};

/**
 * Camera serialization
 */
template<>
void serialize(Serializer* s, Camera* camera);

/**
 * Create a camera
 */
Camera cameraCreate();

/**
 * Destroy a camera
 */
void cameraDestroy(Camera* camera);

/**
 * The the camera to a perspective projection
 */
void cameraSetPerspective(
    Camera* camera,
    f32 aspect,
    f32 fov = (f32)HG_PI / 2.0f,
    f32 near = 0.01f,
    f32 far = 1000.0f);

/**
 * The the camera to an orthographic projection
 *
 * Parameters
 * - camera The camera to set
 * - width The desired width of the render space
 * - height The desired height of the render space
 * - actualAspect The actual aspect, so margins can be added, or 0 to ignore
 */
void cameraSetOrthographic(Camera* camera, f32 width, f32 height, f32 actualAspect = 0.0f);

/**
 * Update the camera's gpu side data
 */
void cameraUpdate(Camera* camera);

/**
 * Camera ecs add implementation
 */
Camera* cameraAdd(Ecs* ecs, Entity e);

/**
 * Camera ecs destructor
 */
template<>
void ecsDtor(Camera* camera);

/**
 * Update the camera's gpu side data, must have a camera and transform
 */
void cameraUpdateEcs(Ecs* ecs, Entity e);

/**
 * A texture asset
 */
struct TextureData {
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
    Format format;
    /**
     * The pixel data, aligned to 16 bytes
     */
    void* pixels;
};

/**
 * A handle to a texture
 */
typedef Asset<TextureData> TextureDataAsset;

/**
 * Texture asset load implementation
 */
template<>
void assetLoadImpl(Asset<TextureData>* data);

/**
 * Texture asset unload implementation
 */
template<>
void assetUnloadImpl(Asset<TextureData>* data);

/**
 * Store an image to disc in the png format
 *
 * Returns
 * - Whether the write succeeded
 */
bool textureStorePng(TextureData* texture, String path);

/**
 * A texture asset stored on the gpu
 */
struct Texture {
    /**
     * The image
     */
    GpuImage* image;
    /**
     * The image view
     */
    GpuView* view;
};

/**
 * A handle to a texture asset
 */
typedef Asset<Texture> TextureAsset;

/**
 * GpuTexture asset load implementation
 */
template<>
void assetLoadImpl(Asset<Texture>* data);

/**
 * GpuTexture asset unload implementation
 */
template<>
void assetUnloadImpl(Asset<Texture>* data);

/**
 * Initialize the 2D renderer
 */
void rendererInit2D(Format colorFormat);

/**
 * Deinitialize the 2D renderer
 */
void rendererDeinit2D();

/**
 * The 2D instance types
 */
enum Render2DInstanceType : u32 {
    /**
     * A instance with a color value
     */
    Render2DInstanceType_color = 0,
    /**
     * A instance with a sprite
     */
    Render2DInstanceType_sprite = 1,
};

/**
 * A rectangle instance
 */
struct Rect2DInstance {
    /**
     * The instance position
     */
    Vec2 pos;
    /**
     * The instance size
     */
    Vec2 size;
    /**
     * The instance type
     */
    u32 type;
    /**
     * Padding for 16 byte alignment
     */
    u32 pad[3];
    /**
     * The rectangle fill color
     */
    Vec4 color;
};

/**
 * A sprite instance
 */
struct Sprite2DInstance {
    /**
     * The instance position
     */
    Vec2 pos;
    /**
     * The instance size
     */
    Vec2 size;
    /**
     * The instance type
     */
    u32 type;
    /**
     * Padding for 16 byte alignment
     */
    u32 pad[2];
    /**
     * The texture index
     */
    u32 tex;
    /**
     * The texture uv coordinates
     */
    Vec2 uvPos;
    /**
     * The texture uv coordinates
     */
    Vec2 uvSize;
};

/**
 * An instance in a 2D layer
 */
union Render2DInstance {
    /**
     * The rectangle data
     */
    Rect2DInstance rect;
    /**
     * The sprite data
     */
    Sprite2DInstance sprite;
};

/**
 * A 2D render layer
 */
struct Layer2D {
    /**
     * The transform, does not affect changed
     */
    Mat4 transform;
    /**
     * The instance data
     */
    Array<Render2DInstance> instances;
    /**
     * The gpu side instance buffer
     */
    GpuBuffer* instanceBuffer;
    /**
     * The capacity of the instance buffer
     */
    u32 instanceCapacity;
    /**
     * Whether the gpu data needs to be updated
     */
    bool changed;
};

/**
 * Create a 2D render layer
 */
Layer2D layerCreate2D();

/**
 * Destroy a 2D render layer
 */
void layerDestroy2D(Layer2D* layer);

/**
 * Remove all drawings from the layer
 */
void layerClear2D(Layer2D* layer);

/**
 * Issue draw commands for a 2D layer
 */
void renderLayer2D(GpuCmd* cmd, Camera* camera, Layer2D* layer);

/**
 * Issue draw commands for a 2D layer using debug lines
 */
void renderDebug2D(GpuCmd* cmd, Camera* camera, Layer2D* layer);

/**
 * Draw a rectangle on the layer
 */
void drawRect2D(Layer2D* layer, Vec4 color, Rect dst);

/**
 * A 2D sprite which can be drawn
 */
struct Sprite2D {
    /**
     * The sprite's texture
     */
    TextureAsset* texture;
    /**
     * The uv coords in the texture
     */
    Rect uv;
};

/**
 *  Draw the sprite on the layer
 */
void drawSprite2D(Layer2D* layer, Sprite2D* sprite, Rect dst);

/**
 * A texture atlas
 */
struct Atlas2D {
    /**
     * The texture
     */
    TextureAsset* texture;
    /**
     * The sprites
     */
    Array<Rect> sprites;
};

/**
 * Create a new texture atlas
 */
Atlas2D atlasCreate2D(TextureAsset* texture);

/**
 * Destroy a texture atlas
 */
void atlasDestroy2D(Atlas2D* atlas);

/**
 * Add a sprite to the atlas
 */
u32 atlasAdd2D(Atlas2D* atlas, Rect sprite);

/**
 * Add a grid of sprites to the atlas
 *
 * Parameters
 * - atlas The atlas to add to
 * - grid The uv coords of the grid
 * - width The number of horizontal subdivisions
 * - height The number of vertical subdivisions
 *
 * Returns
 * - The first sprite index
 */
u32 atlasAddGrid2D(Atlas2D* atlas, Rect grid, u32 width, u32 height);

/**
 * Get a sprite from the atlas
 */
Sprite2D atlasGet2D(Atlas2D* atlas, u32 idx);

/**
 * A world map of tiles
 */
struct Tilemap2D {
    /**
     * The tilemap data
     */
    u32* tiles;
    /**
     * The width of the tilemap in tiles
     */
    u32 width;
    /**
     * The height of the tilemap in tiles
     */
    u32 height;
};

/**
 * Create an empty tilemap
 */
Tilemap2D tilemapCreate2D(u32 width, u32 height);

/**
 * Destroy a tilemap
 */
void tilemapDestroy2D(Tilemap2D* tilemap);

/**
 * Get the value of a tile in a tilemap
 */
u32 tilemapGet2D(Tilemap2D* tilemap, u32 x, u32 y);

/**
 * Set the value of a tile in a tilemap
 */
void tilemapSet2D(Tilemap2D* tilemap, u32 x, u32 y, u32 tile);

/**
 * Draw a tilemap to the layer
 */
void drawTilemap2D(Layer2D* layer, Atlas2D* atlas, Tilemap2D* tilemap, Rect dst);

/**
 * A vertex in a mesh
 */
struct MeshVertex {
    /**
     * The vertex position
     */
    Vec3 pos;
    /**
     * The u part of the vertex uv coordinate
     */
    f32 uvU;
    /**
     * The vertex normal
     */
    Vec3 norm;
    /**
     * The v part of the vertex uv coordinate
     */
    f32 uvV;
    /**
     * The vertex tangent
     */
    Vec4 tan;

    /**
     * Construct the vertex
     */
    MeshVertex(Vec3 pos, Vec3 norm, Vec4 tan, Vec2 uv)
        : pos{pos}, uvU{uv.x}, norm{norm}, uvV{uv.y}, tan{tan} {}
};

/**
 * A 3d mesh asset
 */
struct MeshData {
    /**
     * The file index of the first vertex
     */
    MeshVertex* vertices;
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
    GpuTopology topology;
};

/**
 * A handle to a 3d mesh asset
 */
typedef Asset<MeshData> MeshDataAsset;

/**
 * Mesh asset load implementation
 */
template<>
void assetLoadImpl(Asset<MeshData>* data);

/**
 * Mesh asset unload implementation
 */
template<>
void assetUnloadImpl(Asset<MeshData>* data);

/**
 * Store the model data to disc in gltf format : TODO
 */
void meshStoreGltf(MeshData* data, String path, Fence* fence);

/**
 * A 3d mesh asset stored on the gpu
 */
struct Mesh {
    /**
     * The vertex buffer
     */
    GpuBuffer* vertexBuffer;
    /**
     * The index buffer
     **/
    GpuBuffer* indexBuffer;
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
typedef Asset<Mesh> MeshAsset;

/**
 * GpuMesh asset load implementation
 */
template<>
void assetLoadImpl(Asset<Mesh>* data);

/**
 * GpuMesh asset unload implementation
 */
template<>
void assetUnloadImpl(Asset<Mesh>* data);


/**
 * Initialize the sprite pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be undefined
 * - depthFormat The format of the depth attachment, must not be undefined
 */
void spritesInit(Format colorFormat, Format depthFormat);

/**
 * Deinitialize the sprite pipeline
 */
void spritesDeinit();

/**
 * A sprite component
 */
struct Sprite {
    /**
     * The texture to draw from
     */
    TextureAsset* texture = nullptr;
    /**
     * The beginning coordinate to read from texture, [0.0, 1.0]
     */
    Vec2 uvPos{0.0f, 0.0f};
    /**
     * The size of the region to read from texture, [0.0, 1.0]
     */
    Vec2 uvSize{1.0f, 1.0f};
};

/**
 * Sprite serialization
 */
template<>
void serialize(Serializer* s, Sprite* sprite);

/**
 * Add a sprite to an entity
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to add to
 * - texture A copy of the sprite's texture
 */
Sprite* spriteAdd(
    Ecs* ecs,
    Entity e,
    TextureAsset* texture,
    Vec2 uvPos = Vec2{0.0f},
    Vec2 uvSize = Vec2{1.0f});

/**
 * Sprite ecs destructor
 */
template<>
void ecsDtor(Sprite* sprite);

/**
 * Issue draw commands for all Sprite2D components in the ecs
 *
 * Parameters
 * - ecs The ecs to draw
 * - camera The camera entity to use
 * - cmd The command buffer to record to, must not be nullptr
 */
void spritesDraw(Ecs* ecs, Entity camera, GpuCmd* cmd);

/**
 * Initialize the skybox pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be undefined
 * - depthFormat The format of the depth attachment, if used
 */
void skyboxInit(Format colorFormat, Format depthFormat);

/**
 * Deinitialize the skybox pipeline
 */
void skyboxDeinit();

/**
 * A skybox component
 */
struct Skybox {
    /**
     * The cubemap texture
     */
    TextureAsset* texture;
};

/**
 * Skybox serialization
 */
template<>
void serialize(Serializer* s, Skybox* skybox);

/**
 * Add a skybox to an entity
 *
 * Parameters
 * - ecs The ecs
 * - e The entity to add to
 * - texture A copy of the skybox's texture
 */
Skybox* skyboxAdd(Ecs* ecs, Entity e, TextureAsset* texture);

/**
 * Skybox ecs destructor
 */
template<>
void ecsDtor(Skybox* skybox);

/**
 * Draw a skybox from a texture
 *
 * Parameters
 * - ecs The ecs with the camera
 * - camera The camera entity to use
 * - cmd The command buffer to record to, must not be nullptr
 */
void skyboxDraw(Ecs* ecs, Entity camera, GpuCmd* cmd);

/**
 * A directional light component
 */
struct DirLight {
    /**
     * The direction of the light
     */
    Vec3 dir;
    /**
     * The color of the light
     */
    Vec4 color;
};

/**
 * DirLight serialization
 */
template<>
void serialize(Serializer* s, DirLight* light);

/**
 * Add a directional light to an entity
 */
DirLight* dirLightAdd(Ecs* ecs, Entity e, Vec3 dir, Vec4 color);

/**
 * A point light component, should have a transform
 */
struct PointLight {
    /**
     * The color of the light
     */
    Vec4 color;
};

/**
 * PointLight serialization
 */
template<>
void serialize(Serializer* s, PointLight* light);

/**
 * Add a point light to an entity
 */
PointLight* pointLightAdd(Ecs* ecs, Entity e, Vec4 color);

/**
 * Initialize the 3D model pipeline
 *
 * Parameters
 * - colorFormat The format of the color attachment, must not be undefined
 * - depthFormat The format of the depth attachment, must not be undefined
 */
void modelsInit(Format colorFormat, Format depthFormat);

/**
 * Deinitialize the 3D model pipeline
 */
void modelsDeinit();

/**
 * A 3D model component
 */
struct Model {
    /**
     * The model to render
     */
    MeshAsset* mesh;
    /**
     * The model's color map
     */
    TextureAsset* colorMap;
    /**
     * The model's normal map
     */
    TextureAsset* normalMap;
};

/**
 * Model serialization
 */
template<>
void serialize(Serializer* s, Model* model);

/**
 * Add a model to an entity
 */
Model* modelAdd(
    Ecs* ecs,
    Entity e,
    MeshAsset* mesh,
    TextureAsset* colorMap,
    TextureAsset* normalMap);

/**
 * Model ecs destructor
 */
template<>
void ecsDtor(Model* model);

/**
 * Issue draw commands for all Model3D components in the ecs
 *
 * Parameters
 * - ecs The ecs to draw
 * - camera The camera entity to use
 * - cmd The command buffer to record to, must not be nullptr
 */
void modelsDraw(Ecs* ecs, Entity camera, GpuCmd* cmd);

/**
 * Initialize ImGui platform backend
 *
 * Parameters
 * - window The window for ImGui to use
 * - colorFormat The format the color target will be in
 * - depthFormat The format the depth buffer will be in, if used
 * - stencilFormat The format the stencil will be in, if used
 */
void imGuiInit(
    Window* window,
    Format colorFormat,
    Format depthFormat = Format_undefined,
    Format stencilFormat = Format_undefined);

/**
 * Deinitializes ImGui platform backend
 */
void imGuiDeinit();

/**
 * Create an ImGui texture
 */
void* imGuiTextureCreate(GpuView* view, GpuLayout layout);

/**
 * Create an ImGui texture
 */
void imGuiTextureDestroy(void* texture);

/**
 * Create a new ImGui frame for the platform backend
 */
void imGuiNewFrame();

/**
 * Draw the ImGui frame
 *
 * Parameters
 * - cmd The command buffer to record to
 */
void imGuiDraw(GpuCmd* cmd);

} // namespace hg

#endif // HG_RENDERING_HPP

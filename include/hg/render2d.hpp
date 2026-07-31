#pragma once

#include "hg/inttypes.hpp"
#include "hg/strings.hpp"
#include "hg/sum.hpp"
#include "hg/math.hpp"
#include "hg/geometry2d.hpp"
#include "hg/array.hpp"
#include "hg/assets.hpp"
#include "hg/serialization.hpp"
#include "hg/gpu.hpp"

namespace hg {

/**
 * A texture asset
 */
struct TextureData {
    /**
     * The pixel data, aligned to 16 bytes
     */
    void* pixels = nullptr;
    /**
     * The width of the texture in pixels
     */
    u32 width = 0;
    /**
     * The height of the texture in pixels
     */
    u32 height = 0;
    /**
     * The depth of the texture in pixels
     */
    u32 depth = 0;
    /**
     * The format of each pixel
     */
    Format format = Format_undefined;

    /**
     * Construct empty
     */
    TextureData() noexcept = default;

    /**
     * Destroy the texture data
     */
    ~TextureData() noexcept;

    /**
     * Move construct
     */
    TextureData(TextureData&& other) noexcept
        : pixels{std::exchange(other.pixels, nullptr)}
        , width{std::exchange(other.width, 0)}
        , height{std::exchange(other.height, 0)}
        , depth{std::exchange(other.depth, 0)}
        , format{std::exchange(other.format, Format_undefined)}
    {}

    /**
     * Move assign
     */
    TextureData& operator=(TextureData&& other) noexcept
    {
        if (this != &other)
        {
            this->~TextureData();
            new (this) TextureData{std::move(other)};
        }
        return *this;
    }
};

/**
 * Texture asset load implementation
 */
template<>
void assetLoadImpl(AssetData<TextureData>* data);

/**
 * Store an image to disc in the png format
 *
 * Returns
 * - Whether the write succeeded
 */
bool textureStorePng(TextureData* texture, StringView path);

/**
 * A texture asset stored on the gpu
 */
struct Texture {
    /**
     * The image
     */
    GpuImage image{};
    /**
     * The image view
     */
    GpuView view{};
};

/**
 * Create a gpu texture from cpu data
 */
Texture createTextureFromData(const TextureData& data);

/**
 * GpuTexture asset load implementation
 */
template<>
void assetLoadImpl(AssetData<Texture>* data);

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
    Array<MeshVertex> vertices{};
    /**
     * The file index of the first geometry index
     */
    Array<u32> indices;
    /**
     * How the vertices should be interpreted in sequence
     */
    GpuTopology topology = {};
};

/**
 * Mesh asset load implementation
 */
template<>
void assetLoadImpl(AssetData<MeshData>* data);

/**
 * Store the model data to disc in gltf format : TODO
 */
void meshStoreGltf(MeshData* data, StringView path);

/**
 * A 3d mesh asset stored on the gpu
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
 * A perspective camera
 */
struct CameraPerspective {
    /**
     * The aspect ratio
     */
    f32 aspect = 0.0f;
    /**
     * The field of view
     */
    f32 fov = 0.0f;
    /**
     * The near clipping plane
     */
    f32 near = 0.0f;
    /**
     * The far clipping plane
     */
    f32 far = 0.0f;
};

/**
 * An orthographic camera
 */
struct CameraOrthographic {
    /**
     * The clipping planes in each direction
     */
    f32 left = 0, right = 0, top = 0, bottom = 0, near = 0, far = 0;
};

/**
 * A camera component
 */
struct Camera {
    /**
     * The gpu view projection data
     */
    GpuBuffer vpBuffer{};
    /**
     * The current rotation
     */
    Quat rotation = {};
    /**
     * The current position
     */
    Vec3 position = {};
    /**
     * The projection
     */
    Sum<CameraOrthographic, CameraPerspective> projection = {};

    /**
     * Create a camera
     */
    static Camera create();

    /**
     * The the camera to a perspective projection
     */
    void setPerspective(
        f32 aspect,
        f32 fov = pif / 2.0f,
        f32 near = 0.01f,
        f32 far = 1000.0f);

    /**
     * The the camera to an orthographic projection
     *
     * Parameters
     * - width The desired width of the render space
     * - height The desired height of the render space
     * - actualAspect The actual aspect, so margins can be added, or 0 to ignore
     */
    void setOrthographic(f32 width, f32 height, f32 actualAspect = 0.0f);

    /**
     * Update the camera's gpu side data
     */
    void update();
};

/**
 * CameraPerspective serialization
 */
template<>
void serialize(Serializer* s, CameraPerspective* camera);

/**
 * CameraOrthographic serialization
 */
template<>
void serialize(Serializer* s, CameraOrthographic* camera);

/**
 * Camera serialization
 */
template<>
void serialize(Serializer* s, Camera* camera);

/**
 * Initialize the 2D renderer
 */
void initRenderer2D(Format colorFormat);

/**
 * Deinitialize the 2D renderer
 */
void deinitRenderer2D();

/**
 * A 2D sprite which can be drawn
 */
struct Sprite2D {
    /**
     * The sprite's texture
     */
    Asset<Texture> texture = nullptr;
    /**
     * The uv coords in the texture
     */
    Rect uv = {};
};

/**
 * A texture atlas
 */
struct Atlas2D {
    /**
     * The texture
     */
    Asset<Texture> texture = nullptr;
    /**
     * The sprites
     */
    Array<Rect> sprites{};

    /**
     * Create a new texture atlas
     */
    static Atlas2D create(const Asset<Texture>& texture);

    /**
     * Add empty sprites, to control indices
     */
    u32 addEmpty(u32 count = 1);

    /**
     * Add a sprite to the atlas
     */
    u32 add(Rect sprite);

    /**
     * Add a grid of sprites to the atlas
     *
     * Parameters
     * - grid The uv coords of the grid
     * - width The number of horizontal subdivisions
     * - height The number of vertical subdivisions
     * - count The number of sprites in the grid (so empty ends are not added)
     * - marginX The margin on the right of each sprite, as a fraction of the
     *   subdivided sprite rect
     * - marginY The margin on the bottom of each sprite, as a fraction of the
     *   subdivided sprite rect
     *
     * Returns
     * - The first sprite index
     */
    u32 addGrid(Rect grid, u32 width, u32 height, u32 count = (u32)-1, f32 marginX = 0.0f, f32 marginY = 0.0f);

    /**
     * Get a sprite from the atlas
     */
    Sprite2D get(u32 idx) const;
};

/**
 * Atlas2D serialization
 */
template<>
void serialize(Serializer* s, Atlas2D* atlas);

/**
 * Atlas2D asset loading
 */
template<>
void assetLoadImpl(AssetData<Atlas2D>* data);

/**
 * Get an atlas for a default pixel font
 */
const Atlas2D& getDefaultFont();

/**
 * A world map of tiles
 */
struct Tilemap2D {
    /**
     * The texture atlas
     */
    Asset<Atlas2D> atlas = nullptr;
    /**
     * The tilemap data
     */
    Array<u32> tiles{};
    /**
     * The width of the tilemap in tiles
     */
    u32 width = 0;
    /**
     * The height of the tilemap in tiles
     */
    u32 height = 0;

    /**
     * Create an empty tilemap
     */
    static Tilemap2D create(u32 width, u32 height, const Asset<Atlas2D>& atlas);

    /**
     * Get the value of a tile in a tilemap
     */
    u32 get(u32 x, u32 y) const;

    /**
     * Set the value of a tile in a tilemap
     */
    u32 set(u32 x, u32 y, u32 tile);
};

/**
 * Tilemap2D serialization
 */
template<>
void serialize(Serializer* s, Tilemap2D* tilemap);

namespace internal {

/**
 * Instance data for the 2D renderer
 */
struct Render2DInstance {
    /**
     * The color, or tint
     */
    Vec4 color{};
    /**
     * The quad top left position
     */
    Vec2 pos{};
    /**
     * The quad size
     */
    Vec2 size{};
    /**
     * The texture coordinate beginning
     */
    Vec2 uvPos{};
    /**
     * The texture coordinate extent
     */
    Vec2 uvSize{};
    /**
     * The origin position for rotation
     */
    Vec2 origin{};
    /**
     * The angle of rotation about origin
     */
    f32 rotation = 0;
    /**
     * The texture descriptor index, or -1 to draw a plain rect
     */
    u32 texIdx = (u32)-1;
};

};

/**
 * A 2D render layer
 */
struct Layer2D {
    /**
     * The transform, does not affect changed
     */
    Mat4 transform = {};
    /**
     * The instance data
     */
    Array<internal::Render2DInstance> instances = {};
    /**
     * The gpu side instance buffer
     */
    GpuBuffer instanceBuffer{};
    /**
     * The capacity of the instance buffer
     */
    u32 instanceCapacity = 0;
    /**
     * Whether the gpu data needs to be updated
     */
    bool changed = false;

    /**
     * Create a 2D render layer
     */
    static Layer2D create();

    /**
     * Remove all drawings from the layer
     */
    void clear();

    /**
     * Issue draw commands for a 2D layer
     */
    void render(GpuCmd* cmd, Camera* camera);

    /**
     * Issue draw commands for a 2D layer using debug lines
     */
    void renderDebug(GpuCmd* cmd, Camera* camera);

    /**
     * Draw a rectangle on the layer
     */
    void drawRect(Vec4 color, Rect dst);

    /**
     *  Draw the sprite on the layer
     */
    void drawSprite(const Sprite2D& sprite, Rect dst, Vec4 tint = Vec4{1});

    /**
     *  Draw the sprite on the layer
     */
    void drawSpriteRot(const Sprite2D& sprite, Rect dst, Vec2 origin, f32 rotation, Vec4 tint = Vec4{1});

    /**
     * Draw a tilemap to the layer
     */
    void drawTilemap(const Tilemap2D& tilemap, Rect dst);

    /**
     * Draw text to the layer
     *
     * Parameters
     * - text The text to draw
     * - font The character sprites, indices assumed to be ascii values
     * - color The font color
     * - bounds The bounding box to draw in, characters are scaled to fit the
     *   height, then drawn in sequence until the width is reached
     * - spacing The space between each character
     * - breakAtSpace Whether the text should be cut off at a space, or wherever
     *   happens to break the bounds
     *
     * Returns
     * - The characters that could not fit
     */
    StringView drawText(StringView text, const Atlas2D& font, Vec4 color, Rect bounds, f32 spacing, bool breakAtSpace = true);
};

} // namespace hg


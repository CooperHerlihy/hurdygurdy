#pragma once

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
void meshStoreGltf(MeshData* data, StringView path, Fence* fence);

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

namespace internal {

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

};

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
    Array<Rect> sprites = {};

    /**
     * Create a new texture atlas
     */
    static Atlas2D create(const Asset<Texture>& texture);

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
     *
     * Returns
     * - The first sprite index
     */
    u32 addGrid(Rect grid, u32 width, u32 height);

    /**
     * Get a sprite from the atlas
     */
    Sprite2D get(u32 idx);
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
    void drawSprite(const Sprite2D& sprite, Rect dst);

    /**
     * Draw a tilemap to the layer
     */
    void drawTilemap(const Tilemap2D& tilemap, Rect dst);
};

/**
 * Initialize ImGui platform backend
 *
 * Parameters
 * - window The window for ImGui to use
 * - colorFormat The format the color target will be in
 * - depthFormat The format the depth buffer will be in, if used
 * - stencilFormat The format the stencil will be in, if used
 */
void initImGui(
    const Window& window,
    Format colorFormat,
    Format depthFormat = Format_undefined,
    Format stencilFormat = Format_undefined);

/**
 * Deinitializes ImGui platform backend
 */
void deinitImGui();

/**
 * Create an ImGui texture
 */
void* createImGuiTexture(const GpuView& view, GpuLayout layout);

/**
 * Create an ImGui texture
 */
void destroyImGuiTexture(void* texture);

/**
 * Create a new ImGui frame for the platform backend
 */
void beginImGuiFrame();

/**
 * Draw the ImGui frame
 *
 * Parameters
 * - cmd The command buffer to record to
 */
void renderImGui(GpuCmd* cmd);


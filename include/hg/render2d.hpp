#pragma once

#include "hg/inttypes.hpp"
#include "hg/math.hpp"
#include "hg/geometry.hpp"
#include "hg/serialization.hpp"
#include "hg/gpu.hpp"

#include <cmath>

namespace hg {

/**
 * An orthographic camera
 */
struct Orthographic {
    /**
     * The clipping planes in each direction
     */
    f32 left = 0, right = 0, top = 0, bottom = 0, near = 0, far = 0;

    /**
     * Initialize to zero
     */
    Orthographic() noexcept = default;

    /**
     * Initialize with rect and clips or sensible defaults
     */
    Orthographic(Rect rect, f32 nearVal = 0.0f, f32 farVal = 1.0f)
        : left{rect.begin.x()}
        , right{rect.end.x()}
        , top{rect.begin.y()}
        , bottom{rect.end.y()}
        , near{nearVal}
        , far{farVal}
    {}
};

/**
 * A perspective camera
 */
struct Perspective {
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

    /**
     * Initialize to zero
     */
    Perspective() noexcept = default;

    /**
     * Initialize with aspect and others or sensible defaults
     */
    explicit Perspective(
        f32 aspectVal,
        f32 fovVal = pif / 2.0f,
        f32 nearVal = 0.01f,
        f32 farVal = 1000.0f)
        : aspect{aspectVal}
        , fov{fovVal}
        , near{nearVal}
        , far{farVal}
    {}
};

/**
 * A 2d or 3d camera
 */
struct Camera {
    /**
     * The projection
     */
    std::variant<Orthographic, Perspective> projection{};
    /**
     * The current position
     */
    Vec3 position{0.0f};
    /**
     * The current rotation
     */
    Quat rotation{1.0f};
};

/**
 * Camera serialization
 */
template<>
bool serialize(Serializer* s, Camera* camera);

/**
 * A texture asset
 */
struct TextureData {
    /**
     * The pixel data
     */
    std::vector<byte> pixels{};
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
    Format format = Format::undefined;
};

/**
 * TextureData serialization
 */
template<>
bool serialize(Serializer* s, TextureData* tex);

/**
 * Load an image from disc and try to decode with all supported formats
 */
std::optional<TextureData> textureLoad(std::string_view path);

/**
 * Store an image to disc in png format
 *
 * Note, png expects 8 bit rgba values
 */
bool textureStorePng(const TextureData& texture, std::string_view path);

/**
 * Store an image to disc in jpg format
 *
 * Note, jpg expects 8 bit rgba values
 */
bool textureStoreJpg(const TextureData& texture, u32 quality, std::string_view path);

/**
 * Store an image to disc in hdr format
 *
 * Note, hdr expects 32 bit float format
 */
bool textureStoreHdr(const TextureData& texture, std::string_view path);

/**
 * A texture asset stored on the gpu
 */
struct Texture {
    /**
     * The image
     */
    std::unique_ptr<GpuImage> image{};
    /**
     * The image view
     */
    std::unique_ptr<GpuView> view{};
};

/**
 * Create a gpu texture from cpu texture data
 */
Texture uploadTextureToGpu(Gpu* gpu, const TextureData& data);

/**
 * Create cpu texture data from a gpu texture
 */
TextureData downloadTextureFromGpu(const Texture& tex);

/**
 * Load a texture from disc and upload it to the gpu
 */
std::optional<Texture> textureLoadToGpu(Gpu* gpu, std::string_view path);

/**
 * A texture atlas
 */
struct TextureAtlas {
    /**
     * The texture
     */
    const Texture* texture = nullptr;
    /**
     * The sprites
     */
    std::vector<Rect> sprites{};

    /**
     * Construct empty
     */
    TextureAtlas() noexcept = default;

    /**
     * Construct with texture
     */
    TextureAtlas(const Texture& textureVal)
        : texture{&textureVal}
    {}

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
    u32 addGrid(Rect grid, u32 width, u32 height, u32 count = static_cast<u32>(-1), f32 marginX = 0.0f, f32 marginY = 0.0f);

    /**
     * Get a sprite from the atlas
     */
    Rect get(u32 idx) const;
};

/**
 * TextureAtlas serialization
 */
template<>
bool serialize(Serializer* s, TextureAtlas* atlas);

/**
 * Get an atlas for a default pixel font
 */
const TextureAtlas& getDefaultFont();

/**
 * A builder for text positioning
 */
struct TextBuilder2D {
    /**
     * The font to use
     */
    const TextureAtlas* font = nullptr;
    /**
     * Where to start drawing from in relation to the text
     */
    Vec2 beginDir{};
    /**
     * Where to start drawing from in the world
     */
    Vec2 pos{};
    /**
     * The max height to draw in
     */
    f32 height = 1;
    /**
     * The max width to draw in
     */
    f32 width = INFINITY;
    /**
     * Where to cut off text width
     */
    f32 cutoff = INFINITY;
    /**
     * Whether the text should (if cut off), break at a space, or any character
     */
    bool shouldBreakAtSpace = false;

    /**
     * Begin building
     */
    TextBuilder2D(const TextureAtlas& fontVal)
        : font{&fontVal}
    {}

    /**
     * Break the text at a spece, instead of where the end falls
     */
    TextBuilder2D& breakAtSpace(bool val = true)
    {
        shouldBreakAtSpace = val;
        return *this;
    }

    /**
     * Set the max height, defaults to 1
     */
    TextBuilder2D& setHeight(f32 val)
    {
        height = val;
        return *this;
    }

    /**
     * Set the max width, defaults to infinity
     */
    TextBuilder2D& setWidth(f32 val)
    {
        width = val;
        return *this;
    }

    /**
     * Set the max width and height
     */
    TextBuilder2D& setBounds(f32 widthVal, f32 heightVal)
    {
        width = widthVal;
        height = heightVal;
        return *this;
    }

    /**
     * Set the cutoff width, defaults to infinity
     */
    TextBuilder2D& setCutoff(f32 val)
    {
        cutoff = val;
        return *this;
    }

    /**
     * Orient the position from the center of the text box
     */
    TextBuilder2D& setCenter(Vec2 val)
    {
        beginDir = {0, 0};
        pos = val;
        return *this;
    }

    /**
     * Orient the position from the center left of the text box
     */
    TextBuilder2D& setCenterLeft(Vec2 val)
    {
        beginDir = {1, 0};
        pos = val;
        return *this;
    }

    /**
     * Orient the position from the center right of the text box
     */
    TextBuilder2D& setCenterRight(Vec2 val)
    {
        beginDir = {-1, 0};
        pos = val;
        return *this;
    }

    /**
     * Orient the position from the top left of the text box
     */
    TextBuilder2D& setTopLeft(Vec2 val)
    {
        beginDir = {1, 1};
        pos = val;
        return *this;
    }

    /**
     * Orient the position from the top center of the text box
     */
    TextBuilder2D& setTopCenter(Vec2 val)
    {
        beginDir = {0, 1};
        pos = val;
        return *this;
    }

    /**
     * Orient the position from the top right of the text box
     */
    TextBuilder2D& setTopRight(Vec2 val)
    {
        beginDir = {-1, 1};
        pos = val;
        return *this;
    }

    /**
     * Orient the position from the bottom left of the text box
     */
    TextBuilder2D& setBottomLeft(Vec2 val)
    {
        beginDir = {1, -1};
        pos = val;
        return *this;
    }

    /**
     * Orient the position from the bottom center of the text box
     */
    TextBuilder2D& setBottomCenter(Vec2 val)
    {
        beginDir = {0, -1};
        pos = val;
        return *this;
    }

    /**
     * Orient the position from the bottom right of the text box
     */
    TextBuilder2D& setBottomRight(Vec2 val)
    {
        beginDir = {-1, -1};
        pos = val;
        return *this;
    }
};

/**
 * A 2D render layer
 */
class Layer2D {
private:
    /**
     * Instance data for the 2D renderer
     */
    struct Instance {
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
        u32 texIdx = static_cast<u32>(-1);
    };

    /**
     * The transform, does not affect changed
     */
    Mat4 transform{1.0};
    /**
     * The instance data
     */
    std::vector<Instance> instances{};
    /**
     * The gpu side instance buffer
     */
    std::unique_ptr<GpuBuffer> instanceBuffer{};
    /**
     * The capacity of the instance buffer
     */
    u32 instanceCapacity = 0;
    /**
     * Whether the gpu data needs to be updated
     */
    bool changed = false;

public:
    /**
     * The renderer can access the layer
     */
    friend class Renderer2D;

    /**
     * Remove all drawings from the layer
     */
    void clear();

    /**
     * Draw a rectangle on the layer
     *
     *  Parameters
     *  - color The color to draw
     *  - dst The rect in world space to draw to
     */
    void drawRect(Vec4 color, Rect dst);

    /**
     * Draw a rotated rectangle on the layer
     *
     *  Parameters
     *  - color The color to draw
     *  - dst The rect in world space to draw to
     *  - origin The point to rotate about
     *  - rotation The angle (in radians) to rotate by
     */
    void drawRectRot(Vec4 color, Rect dst, Vec2 origin, f32 rotation);

    /**
     *  Draw a sprite on the layer
     *
     *  Parameters
     *  - texture The texture to draw from
     *  - src The rect in the texture to draw from
     *  - dst The rect in world space to draw to
     *  - tint The color to multiply the texture by
     */
    void drawSprite(const Texture& texture, Rect src, Rect dst, Vec4 tint = Vec4{1});

    /**
     *  Draw a rotated sprite on the layer
     *
     *  Parameters
     *  - texture The texture to draw from
     *  - src The rect in the texture to draw from
     *  - dst The rect in world space to draw to
     *  - origin The point to rotate about
     *  - rotation The angle (in radians) to rotate by
     *  - tint The color to multiply the texture by
     */
    void drawSpriteRot(const Texture& texture, Rect src, Rect dst, Vec2 origin, f32 rotation, Vec4 tint = Vec4{1});

    /**
     * Draw text to the layer
     *
     * Parameters
     * - text The text to draw
     * - color The font color
     * - box Where to draw the text
     *
     * Returns
     * - The characters that could not fit
     */
    std::string_view drawText(std::string_view text, Vec4 color, const TextBuilder2D& box);
};

/**
 * A layer based 2D renderer
 */
class Renderer2D {
private:
    /**
     * The gpu
     */
    Gpu* gpu = nullptr;
    /**
     * The render pipeline
     */
    std::unique_ptr<GpuPipeline> pipeline = nullptr;
    /**
     * The view projection buffer
     */
    std::unique_ptr<GpuBuffer> vpBuffer = nullptr;
    /**
     * The layers to render this frame
     */
    std::vector<Layer2D*> layerQueue{};

public:
    /**
     * Construct without init
     */
    Renderer2D() noexcept = default;

    /**
     * Initialize the renderer
     */
    Renderer2D(Gpu* gpu, Format colorFormat);

    /**
     * Deinitialize the renderer and the default pixel font if unused
     */
    ~Renderer2D() noexcept;

    /**
     * Queue a layer to draw this frame
     */
    void drawLayer(Layer2D& layer);

    /**
     * Issue draw commands
     */
    void render(GpuCommandBuffer* cmd, const Camera& camera);

    /**
     * Move construct
     */
    Renderer2D(Renderer2D&& other) noexcept;

    /**
     * Move assign
     */
    Renderer2D& operator=(Renderer2D&& other) noexcept;

    Renderer2D(const Renderer2D&) = delete;
    Renderer2D& operator=(const Renderer2D&) = delete;
};

} // namespace hg

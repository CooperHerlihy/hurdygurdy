#include "tests.hpp"
#include "hg/render2d.hpp"
#include "hg/serialization.hpp"

#include <cmath>
#include <cstring>

using namespace hg;

// ---- camera serialization ----

TEST(testCameraSerializeRoundtripOrthographic)
{
    Camera cam{};
    cam.projection = Orthographic{{{-2.0f, -3.0f}, {4.0f, 5.0f}}, 0.5f, 20.0f};
    cam.position = Vec3{1.0f, 2.0f, 3.0f};
    cam.rotation = Quat{0.5f};

    Serializer s = Serializer::writer();
    ASSERT(serialize(&s, &cam));
    std::vector<byte> bin = s.writeBinary();
    ASSERT(bin.size() > 0);

    std::optional<Serializer> r = Serializer::readBinary(bin);
    ASSERT(r.has_value());
    Camera out{};
    ASSERT(serialize(&*r, &out));

    const Orthographic* o = std::get_if<Orthographic>(&out.projection);
    ASSERT(o != nullptr);
    ASSERT(o->left == -2.0f);
    ASSERT(o->right == 4.0f);
    ASSERT(o->top == -3.0f);
    ASSERT(o->bottom == 5.0f);
    ASSERT(o->near == 0.5f);
    ASSERT(o->far == 20.0f);
    ASSERT(out.position.x() == 1.0f);
    ASSERT(out.position.y() == 2.0f);
    ASSERT(out.position.z() == 3.0f);
}

TEST(testCameraSerializeRoundtripPerspective)
{
    Camera cam{};
    cam.projection = Perspective{1.777f, pif / 3.0f, 0.1f, 500.0f};
    cam.position = Vec3{0.0f};
    cam.rotation = Quat{1.0f};

    Serializer s = Serializer::writer();
    ASSERT(serialize(&s, &cam));
    std::vector<byte> bin = s.writeBinary();

    std::optional<Serializer> r = Serializer::readBinary(bin);
    ASSERT(r.has_value());
    Camera out{};
    ASSERT(serialize(&*r, &out));

    const Perspective* p = std::get_if<Perspective>(&out.projection);
    ASSERT(p != nullptr);
    ASSERT(p->aspect == 1.777f);
    ASSERT(p->fov == pif / 3.0f);
    ASSERT(p->near == 0.1f);
    ASSERT(p->far == 500.0f);
}

// ---- texture data serialization ----

TEST(testTextureDataSerializeRoundtrip)
{
    TextureData tex{};
    tex.width = 3;
    tex.height = 2;
    tex.depth = 1;
    tex.format = Format::rgba8_unorm;
    tex.pixels.resize(3 * 2 * 4);
    for (u32 i = 0; i < tex.pixels.size(); ++i)
        tex.pixels[i] = static_cast<byte>(i * 7);

    Serializer s = Serializer::writer();
    ASSERT(serialize(&s, &tex));
    std::vector<byte> bin = s.writeBinary();

    std::optional<Serializer> r = Serializer::readBinary(bin);
    ASSERT(r.has_value());
    TextureData out{};
    ASSERT(serialize(&*r, &out));

    ASSERT(out.width == 3);
    ASSERT(out.height == 2);
    ASSERT(out.depth == 1);
    ASSERT(out.format == Format::rgba8_unorm);
    ASSERT(out.pixels.size() == tex.pixels.size());
    for (usize i = 0; i < tex.pixels.size(); ++i)
        ASSERT(out.pixels[i] == tex.pixels[i]);
}

// ---- texture atlas ----

TEST(testTextureAtlasAddGet)
{
    TextureAtlas atlas{};

    // addEmpty returns the index of the first empty sprite
    u32 emptyIdx = atlas.addEmpty(3);
    ASSERT(emptyIdx == 0);
    Rect empty = atlas.get(0);
    // emptyRect spans from +inf to -inf: it contains no area
    ASSERT(empty.begin == Vec2{INFINITY});
    ASSERT(empty.end == Vec2{-INFINITY});

    Rect r1{{0.0f, 0.0f}, {0.5f, 0.5f}};
    u32 idx1 = atlas.add(r1);
    ASSERT(idx1 == 3);
    ASSERT(atlas.get(idx1).begin == r1.begin);
    ASSERT(atlas.get(idx1).end == r1.end);

    Rect r2{{0.5f, 0.5f}, {1.0f, 1.0f}};
    ASSERT(atlas.add(r2) == 4);
    ASSERT(atlas.get(4).end == r2.end);
}

TEST(testTextureAtlasAddGrid)
{
    TextureAtlas atlas{};

    // 2x3 grid over [0,1)x[0,1), only 4 sprites requested
    u32 first = atlas.addGrid({{0.0f, 0.0f}, {1.0f, 1.0f}}, 2, 3, 4);
    ASSERT(first == 0);
    ASSERT((atlas.get(0).begin == Vec2{0.0f, 0.0f}));
    ASSERT((atlas.get(0).end == Vec2{0.5f, 1.0f / 3.0f}));
    ASSERT((atlas.get(1).begin == Vec2{0.5f, 0.0f}));
    ASSERT((atlas.get(2).begin == Vec2{0.0f, 1.0f / 3.0f}));
    ASSERT((atlas.get(3).begin == Vec2{0.5f, 1.0f / 3.0f}));

    // Only 4 sprites were added out of the 6 cell grid: the next add
    // continues at index 4
    ASSERT((atlas.add({{0.0f, 0.0f}, {0.1f, 0.1f}}) == 4));
}

TEST(testTextureAtlasAddGridCountClamped)
{
    TextureAtlas atlas{};

    // count larger than the grid is clamped to width * height
    u32 first = atlas.addGrid({{0.0f, 0.0f}, {1.0f, 1.0f}}, 2, 2, 100);
    ASSERT(first == 0);

    // All 4 cells were filled, the next add continues at index 4
    ASSERT((atlas.add({{0.0f, 0.0f}, {0.1f, 0.1f}}) == 4));
    ASSERT((atlas.get(3).end == Vec2{1.0f, 1.0f}));
}

TEST(testTextureAtlasAddGridMargins)
{
    TextureAtlas atlas{};

    // marginX and marginY shrink each sprite within its cell
    u32 first = atlas.addGrid({{0.0f, 0.0f}, {1.0f, 1.0f}}, 2, 1, 2, 0.5f, 0.5f);
    ASSERT(first == 0);

    // Cell width 0.5, margin 0.5 -> sprite width 0.25
    ASSERT((atlas.get(0).begin == Vec2{0.0f, 0.0f}));
    ASSERT((atlas.get(0).end == Vec2{0.25f, 0.5f}));
    // Second cell starts at x=0.5
    ASSERT((atlas.get(1).begin == Vec2{0.5f, 0.0f}));
    ASSERT((atlas.get(1).end == Vec2{0.75f, 0.5f}));
}

// ---- texture upload / download ----

TEST(testTextureUpload)
{
    // Note: downloadTextureFromGpu cannot be roundtrip tested because
    // uploadTextureToGpu creates the image without GpuImageUsage::transferSrc,
    // so readWhole aborts on the validation layer
    TextureData data{};
    data.width = 4;
    data.height = 4;
    data.depth = 1;
    data.format = Format::rgba8_unorm;
    data.pixels.resize(4 * 4 * 4);
    u32* pixels = reinterpret_cast<u32*>(data.pixels.data());
    for (u32 i = 0; i < 16; ++i)
        pixels[i] = 0xFF000000u | (i + 1);

    Texture tex = uploadTextureToGpu(testGpu, data);
    ASSERT(tex.image != nullptr);
    ASSERT(tex.view != nullptr);
    ASSERT(tex.view->width() == 4);
    ASSERT(tex.view->height() == 4);
    ASSERT(tex.view->depth() == 1);
    ASSERT(tex.view->format() == Format::rgba8_unorm);
}

TEST(testTextureUploadEmpty)
{
    TextureData data{};
    Texture tex = uploadTextureToGpu(testGpu, data);
    ASSERT(tex.image == nullptr);
    ASSERT(tex.view == nullptr);
}

// ---- text builder ----

TEST(testTextBuilder2DSetters)
{
    TextureAtlas atlas{};
    TextBuilder2D builder{atlas};

    ASSERT(builder.height == 1.0f);
    ASSERT(builder.width == INFINITY);
    ASSERT(builder.cutoff == INFINITY);
    ASSERT(!builder.shouldBreakAtSpace);

    ASSERT(&builder.setHeight(2.0f) == &builder);
    ASSERT(builder.height == 2.0f);
    ASSERT(&builder.setWidth(3.0f) == &builder);
    ASSERT(builder.width == 3.0f);
    ASSERT(&builder.setCutoff(4.0f) == &builder);
    ASSERT(builder.cutoff == 4.0f);
    ASSERT(&builder.setBounds(5.0f, 6.0f) == &builder);
    ASSERT(builder.width == 5.0f);
    ASSERT(builder.height == 6.0f);
    ASSERT(&builder.breakAtSpace() == &builder);
    ASSERT(builder.shouldBreakAtSpace);
    builder.breakAtSpace(false);
    ASSERT(!builder.shouldBreakAtSpace);
}

TEST(testTextBuilder2DOrientation)
{
    TextureAtlas atlas{};
    TextBuilder2D builder{atlas};

    builder.pos = Vec2{9.0f, 9.0f};

    builder.setTopLeft(Vec2{1.0f, 2.0f});
    ASSERT((builder.beginDir == Vec2{1.0f, 1.0f}));
    ASSERT((builder.pos == Vec2{1.0f, 2.0f}));

    builder.setCenter(Vec2{3.0f, 4.0f});
    ASSERT((builder.beginDir == Vec2{0.0f, 0.0f}));
    ASSERT((builder.pos == Vec2{3.0f, 4.0f}));

    builder.setCenterLeft(Vec2{5.0f, 6.0f});
    ASSERT((builder.beginDir == Vec2{1.0f, 0.0f}));

    builder.setCenterRight(Vec2{7.0f, 8.0f});
    ASSERT((builder.beginDir == Vec2{-1.0f, 0.0f}));

    builder.setTopCenter(Vec2{1.0f, 1.0f});
    ASSERT((builder.beginDir == Vec2{0.0f, 1.0f}));

    builder.setTopRight(Vec2{1.0f, 1.0f});
    ASSERT((builder.beginDir == Vec2{-1.0f, 1.0f}));

    builder.setBottomLeft(Vec2{1.0f, 1.0f});
    ASSERT((builder.beginDir == Vec2{1.0f, -1.0f}));

    builder.setBottomCenter(Vec2{1.0f, 1.0f});
    ASSERT((builder.beginDir == Vec2{0.0f, -1.0f}));

    builder.setBottomRight(Vec2{1.0f, 1.0f});
    ASSERT((builder.beginDir == Vec2{-1.0f, -1.0f}));
}

// ---- offscreen rendering ----

static constexpr u32 render2dImgSize = 8;

static std::unique_ptr<GpuImage> render2dTarget(u32 size)
{
    return testGpu->createImage(size, size, Format::rgba8_unorm,
        GpuImageUsage::colorAttachment | GpuImageUsage::transferSrc);
}

TEST(testRenderer2DDrawRect)
{
    static constexpr u32 size = render2dImgSize;

    Renderer2D renderer{testGpu, Format::rgba8_unorm};

    Layer2D layer{};
    layer.drawRect(Vec4{0.0f, 1.0f, 0.0f, 1.0f}, Rect{{-1.0f, -1.0f}, {1.0f, 1.0f}});

    std::unique_ptr<GpuImage> img = render2dTarget(size);
    std::unique_ptr<GpuView> view = img->createView();

    Camera camera{};
    camera.projection = Orthographic{{{-1.0f, -1.0f}, {1.0f, 1.0f}}};

    GpuAttachment att{};
    att.image = view.get();
    att.loadOp = GpuLoadOp::clear;
    att.storeOp = GpuStoreOp::store;
    att.clearColor[0] = 0.0f;
    att.clearColor[1] = 0.0f;
    att.clearColor[2] = 0.0f;
    att.clearColor[3] = 1.0f;

    GpuPass pass{};
    pass.colorAttachments = {&att, 1};

    renderer.drawLayer(layer);

    {
        std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
        cmd->beginRenderPass(pass);
        renderer.render(cmd.get(), camera);
        cmd->endRenderPass();
        testGpu->endCommands(std::move(cmd));
    }

    u32 pixels[size * size] = {};
    img->readWhole(std::as_writable_bytes(std::span{pixels}));
    // Full coverage green rect over black clear: rgba8 packed 0xFF00FF00
    for (u32 i = 0; i < size * size; ++i)
        ASSERT(pixels[i] == 0xFF00FF00);
}

TEST(testRenderer2DClearAndSecondFrame)
{
    static constexpr u32 size = render2dImgSize;

    Renderer2D renderer{testGpu, Format::rgba8_unorm};

    std::unique_ptr<GpuImage> img = render2dTarget(size);
    std::unique_ptr<GpuView> view = img->createView();

    Camera camera{};
    camera.projection = Orthographic{{{-1.0f, -1.0f}, {1.0f, 1.0f}}};

    GpuAttachment att{};
    att.image = view.get();
    att.loadOp = GpuLoadOp::clear;
    att.storeOp = GpuStoreOp::store;
    att.clearColor[0] = 0.0f;
    att.clearColor[1] = 0.0f;
    att.clearColor[2] = 0.0f;
    att.clearColor[3] = 1.0f;

    GpuPass pass{};
    pass.colorAttachments = {&att, 1};

    Layer2D layer{};
    layer.drawRect(Vec4{1.0f, 0.0f, 0.0f, 1.0f}, Rect{{-1.0f, -1.0f}, {1.0f, 1.0f}});
    renderer.drawLayer(layer);

    {
        std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
        cmd->beginRenderPass(pass);
        renderer.render(cmd.get(), camera);
        cmd->endRenderPass();
        testGpu->endCommands(std::move(cmd));
    }

    u32 pixels[size * size] = {};
    img->readWhole(std::as_writable_bytes(std::span{pixels}));
    for (u32 i = 0; i < size * size; ++i)
        ASSERT(pixels[i] == 0xFF0000FF);

    // After clear the layer draws nothing: second frame keeps the clear color
    layer.clear();
    {
        std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();
        cmd->beginRenderPass(pass);
        renderer.render(cmd.get(), camera);
        cmd->endRenderPass();
        testGpu->endCommands(std::move(cmd));
    }

    img->readWhole(std::as_writable_bytes(std::span{pixels}));
    for (u32 i = 0; i < size * size; ++i)
        ASSERT(pixels[i] == 0xFF000000);
}

TEST(testRenderer2DDrawSprite)
{
    static constexpr u32 size = render2dImgSize;

    // A texture whose top-left half is white and bottom half black
    TextureData data{};
    data.width = 2;
    data.height = 2;
    data.depth = 1;
    data.format = Format::rgba8_unorm;
    u32 texPixels[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0xFF000000};
    data.pixels.assign(reinterpret_cast<const byte*>(texPixels),
        reinterpret_cast<const byte*>(texPixels) + sizeof(texPixels));
    Texture tex = uploadTextureToGpu(testGpu, data);
    ASSERT(tex.image != nullptr);

    Renderer2D renderer{testGpu, Format::rgba8_unorm};

    Layer2D layer{};
    // Draw the top texel row across the whole screen
    layer.drawSprite(tex, Rect{{0.0f, 0.0f}, {0.5f, 0.5f}}, Rect{{-1.0f, -1.0f}, {1.0f, 1.0f}});
    renderer.drawLayer(layer);

    std::unique_ptr<GpuImage> img = render2dTarget(size);
    std::unique_ptr<GpuView> view = img->createView();

    Camera camera{};
    camera.projection = Orthographic{{{-1.0f, -1.0f}, {1.0f, 1.0f}}};

    std::unique_ptr<GpuCommandBuffer> cmd = testGpu->beginCommands();

    GpuAttachment att{};
    att.image = view.get();
    att.loadOp = GpuLoadOp::clear;
    att.storeOp = GpuStoreOp::store;
    att.clearColor[0] = 0.0f;
    att.clearColor[1] = 0.0f;
    att.clearColor[2] = 1.0f;
    att.clearColor[3] = 1.0f;

    GpuPass pass{};
    pass.colorAttachments = {&att, 1};

    cmd->beginRenderPass(pass);
    renderer.render(cmd.get(), camera);
    cmd->endRenderPass();

    testGpu->endCommands(std::move(cmd));

    u32 pixels[size * size] = {};
    img->readWhole(std::as_writable_bytes(std::span{pixels}));
    for (u32 i = 0; i < size * size; ++i)
        ASSERT(pixels[i] == 0xFFFFFFFF);
}

TEST(testRenderer2DDrawTextReturn)
{
    Renderer2D renderer{testGpu, Format::rgba8_unorm};

    const TextureAtlas& font = getDefaultFont();
    ASSERT(font.texture != nullptr);
    ASSERT(font.sprites.size() >= 127);

    Layer2D layer{};

    // Text that fits is fully consumed: empty remainder
    {
        TextBuilder2D box{font};
        box.setHeight(1.0f);
        std::string_view remaining = layer.drawText("Hello", Vec4{1.0f}, box);
        ASSERT(remaining.empty());
    }

    // Tiny cutoff cuts the text: the remainder is non-empty and is a suffix
    {
        TextBuilder2D box{font};
        box.setHeight(1.0f);
        box.setCutoff(0.001f);
        std::string_view text = "Hello";
        std::string_view remaining = layer.drawText(text, Vec4{1.0f}, box);
        // The cutoff is below the first glyph width: nothing is drawn
        ASSERT(remaining == text);
        ASSERT((remaining == std::string_view{text.end() - remaining.size(), text.end()}));
    }

    // Nothing should have thrown; the layer now contains sprite instances
    renderer.drawLayer(layer);
}

TEST(testRenderer2DMoveAndRefcount)
{
    {
        Renderer2D a{testGpu, Format::rgba8_unorm};
        Renderer2D b{std::move(a)};

        b = Renderer2D{testGpu, Format::rgba8_unorm};
    }

    // The default font must still be usable after the refcount drops to zero
    // and rises again
    Renderer2D c{testGpu, Format::rgba8_unorm};
    const TextureAtlas& font = getDefaultFont();
    ASSERT(font.texture != nullptr);
    ASSERT(font.texture->image != nullptr);
}


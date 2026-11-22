#include "hg/render2d.hpp"

#include "hg/utility.hpp"
#include "hg/memory.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

namespace hg {

template<>
bool serialize(Serializer& s, Camera* camera)
{
    return serializeObject(s,
        &camera->projection,
        &camera->position,
        &camera->rotation);
}

template<>
bool serialize(Serializer& s, TextureData* tex)
{
    return serializeObject(s,
        &tex->width,
        &tex->height,
        &tex->depth,
        &tex->format,
        &tex->pixels);
}

std::optional<TextureData> textureLoad(std::string_view path)
{
    std::optional<std::vector<byte>> bin = binaryLoad(path);
    if (!bin.has_value())
        return {};

    std::optional tex = std::make_optional<TextureData>();

    if (isSerialData(*bin))
    {
        Serializer s = Serializer::reader(*bin);
        if (serialize(s, &tex))
            return tex;
    }

    int x, y, channels;
    byte* pixels = reinterpret_cast<byte*>(stbi_load_from_memory(
        reinterpret_cast<stbi_uc*>(bin->data()),
        assertCast<int>(bin->size()),
        &x,
        &y,
        &channels,
        4));

    if (pixels == nullptr)
    {
        setError("Could not decode image: {}", path);
        return {};
    }
    HG_DEFER(std::free(pixels));

    tex->pixels = std::vector<byte>(pixels, pixels + x * y * 4);
    tex->width = assertCast<u32>(x);
    tex->height = assertCast<u32>(y);
    tex->depth = 1;
    tex->format = Format::rgba8_srgb;
    return tex;
}

bool textureStorePng(const TextureData& texture, std::string_view path)
{
    ArenaScope scratch = getScratch();
    StringTemp p{path, scratch};

    if (!stbi_write_png(
         p.c_str(),
         assertCast<int>(texture.width),
         assertCast<int>(texture.height),
         4,
         texture.pixels.data(),
         assertCast<int>(texture.width * sizeof(u32))))
    {
        setError("Could not store image: {}", p.c_str());
        return false;
    }
    return true;
}

bool textureStoreJpg(const TextureData& texture, u32 quality, std::string_view path)
{
    ArenaScope scratch = getScratch();
    StringTemp p{path, scratch};

    if (!stbi_write_jpg(
         p.c_str(),
         assertCast<int>(texture.width),
         assertCast<int>(texture.height),
         4,
         texture.pixels.data(),
         assertCast<int>(quality)))
    {
        setError("Could not store image: {}", p.c_str());
        return false;
    }
    return true;
}

bool textureStoreHdr(const TextureData& texture, std::string_view path)
{
    ArenaScope scratch = getScratch();
    StringTemp p{path, scratch};

    if (!stbi_write_hdr(
         p.c_str(),
         assertCast<int>(texture.width),
         assertCast<int>(texture.height),
         4,
         reinterpret_cast<const f32*>(texture.pixels.data())))
    {
        setError("Could not store image: {}", p.c_str());
        return false;
    }
    return true;
}

Texture uploadTextureToGpu(Gpu* gpu, const TextureData& data)
{
    if (data.pixels.empty())
        return {};

    GpuImageCreateInfo imageInfo{};
    imageInfo.format = data.format;
    imageInfo.width = data.width;
    imageInfo.height = data.height;
    imageInfo.depth = data.depth;
    imageInfo.usage = GpuImageUsage::transferDst | GpuImageUsage::sampled;

    Texture tex{};
    tex.image = gpu->createImage(imageInfo);
    tex.view = tex.image->createView(GpuFilter::nearest);

    tex.image->writeWhole(data.pixels);

    return tex;
}

TextureData downloadTextureFromGpu(const Texture& tex)
{
    TextureData data{};

    data.width = tex.view->width();
    data.height = tex.view->height();
    data.depth = tex.view->depth();
    data.format = tex.view->format();

    data.pixels.resize(data.width * data.height * data.depth * formatToSize(data.format));
    tex.image->readWhole(data.pixels);

    return data;
}

std::optional<Texture> textureLoadToGpu(Gpu* gpu, std::string_view path)
{
    std::optional<TextureData> data = textureLoad(path);
    if (!data.has_value())
        return std::nullopt;
    return uploadTextureToGpu(gpu, *data);
}

struct RenderState2D {
    TextureAtlas defaultFont{};
};

static RenderState2D render2D;

#include "resources/pixel_font.hg.h"

namespace internal {

void convertPixelFont()
{
    log("converting pixel font file\n");

    std::optional<TextureData> fontData = textureLoad("resources/pixel_font.png");
    if (!fontData.has_value())
    {
        warn("pixel_font.png could not be read: {}", getError());
        return;
    }

    serialStore(*fontData, "resources/pixel_font.hg");
}

} // namespace internal

static void initPixelFont(Gpu* gpu)
{
    std::optional fontData = serialLoadFromBinary<TextureData>(std::as_bytes(std::span{resources_pixel_font_hg}));
    if (!fontData.has_value())
        warn("Could not deserialize pixel font: {}", getError());

    render2D.defaultFont.texture = uploadTextureToGpu(gpu, *fontData);
    render2D.defaultFont.addEmpty(32);
    render2D.defaultFont.addGrid({{0, 0}, {1, 1}}, 12, 8, 95);
}

static void deinitPixelFont()
{
    render2D = {};
}

u32 TextureAtlas::addEmpty(u32 count)
{
    u32 idx = assertCast<u32>(sprites.size());
    for (u32 i = 0; i < count; ++i)
    {
        sprites.emplace_back(emptyRect());
    }
    return idx;
}

u32 TextureAtlas::add(Rect sprite)
{
    u32 idx = assertCast<u32>(sprites.size());
    sprites.emplace_back(sprite);
    return idx;
}

u32 TextureAtlas::addGrid(Rect grid, u32 width, u32 height, u32 count, f32 marginX, f32 marginY)
{
    u32 idx = assertCast<u32>(sprites.size());

    Vec2 subdivSize = (grid.end - grid.begin) / Vec2{assertCast<f32>(width), assertCast<f32>(height)};
    Vec2 spriteSize = subdivSize * (Vec2{1} - Vec2{marginX, marginY});

    count = std::min(count, width * height);
    u32 i = 0;
    Vec2 pos = grid.begin;
    for (u32 y = 0; y < height; ++y)
    {
        pos.x() = grid.begin.x();
        for (u32 x = 0; x < width; ++x)
        {
            if (i++ == count)
                goto done;

            sprites.emplace_back(pos, pos + spriteSize);
            pos.x() += subdivSize.x();
        }
        pos.y() += subdivSize.y();
    }

done:
    return idx;
}

Rect TextureAtlas::get(u32 idx) const
{
    return sprites[idx];
}

const TextureAtlas& getDefaultFont()
{
    return render2D.defaultFont;
}

template<>
bool serialize(Serializer& s, TextureAtlas* atlas)
{
    return serializeObject(s,
        &atlas->texture,
        &atlas->sprites);
}

void Layer2D::clear()
{
    instances.clear();
    changed = true;
}

void Layer2D::drawRect(Vec4 color, Rect dst)
{
    Instance instance{};
    instance.color = color;
    instance.pos = dst.begin;
    instance.size = dst.end - dst.begin;

    instances.emplace_back(instance);
    changed = true;
}

void Layer2D::drawRectRot(Vec4 color, Rect dst, Vec2 origin, f32 rotation)
{
    Instance instance{};
    instance.color = color;
    instance.pos = dst.begin;
    instance.size = dst.end - dst.begin;
    instance.origin = origin;
    instance.rotation = rotation;

    instances.emplace_back(instance);
    changed = true;
}

void Layer2D::drawSprite(const Texture& texture, Rect src, Rect dst, Vec4 tint)
{
    Instance instance{};
    instance.color = tint;
    instance.pos = dst.begin;
    instance.size = dst.end - dst.begin;
    instance.uvPos = src.begin;
    instance.uvSize = src.end - src.begin;
    instance.texIdx = texture.view->samplerDescriptor();

    instances.emplace_back(instance);
    changed = true;
}

void Layer2D::drawSpriteRot(const Texture& texture, Rect src, Rect dst, Vec2 origin, f32 rotation, Vec4 tint)
{
    Instance instance{};
    instance.color = tint;
    instance.pos = dst.begin;
    instance.size = dst.end - dst.begin;
    instance.uvPos = src.begin;
    instance.uvSize = src.end - src.begin;
    instance.origin = origin;
    instance.rotation = rotation;
    instance.texIdx = texture.view->samplerDescriptor();

    instances.emplace_back(instance);
    changed = true;
}

std::string_view Layer2D::drawText(std::string_view text, Vec4 color, const TextBuilder2D& box)
{
    Rect bounds{};
    f32 factor = 1.0f;

    HG_ASSERT(box.width != INFINITY || box.height != INFINITY);

    if (box.width == INFINITY)
    {
        Rect firstRect = box.font->sprites[assertCast<u32>(text[0])];
        Vec2 firstSize = firstRect.end - firstRect.begin;

        factor = box.height / firstSize.y();
        for (char c : text)
        {
            Rect cRect = box.font->sprites[assertCast<u32>(c)];
            Vec2 size = cRect.end - cRect.begin;
            bounds.end.x() += size.x() * factor;
        }
        bounds.end.y() = box.height;
    }
    else if (box.height == INFINITY)
    {
        Rect firstRect = box.font->sprites[assertCast<u32>(text[0])];
        Vec2 firstSize = firstRect.end - firstRect.begin;

        f32 width = 0;
        for (char c : text)
        {
            Rect cRect = box.font->sprites[assertCast<u32>(c)];
            Vec2 size = cRect.end - cRect.begin;
            width += size.x();
        }
        factor = box.width / width;

        bounds.end.x() = box.width;
        bounds.end.y() = firstSize.y() * box.width / width;
    }
    else
    {
        Rect firstRect = box.font->sprites[assertCast<u32>(text[0])];
        Vec2 firstSize = firstRect.end - firstRect.begin;

        bounds.end.y() = firstSize.y();

        for (char c : text)
        {
            Rect cRect = box.font->sprites[assertCast<u32>(c)];
            Vec2 size = cRect.end - cRect.begin;
            bounds.end.x() += size.x();
        }

        Rect hBounds = {{}, {bounds.end.x() * box.height / bounds.end.y(), box.height}};
        Rect wBounds = {{}, {box.width, bounds.end.y() * box.width / bounds.end.x()}};

        if (hBounds.end.x() <= box.width)
        {
            factor = box.height / bounds.end.y();
            bounds = hBounds;
        }
        else if (wBounds.end.y() <= box.height)
        {
            factor = box.width / bounds.end.x();
            bounds = wBounds;
        }
    }

    std::string_view toDraw = text;
    if (box.cutoff < bounds.end.x())
    {
        const char* lastSpace = nullptr;
        const char* c = text.data();
        const char* end = text.data() + text.size();
        f32 width = 0;
        for (; c != end; ++c)
        {
            if (*c == ' ')
                lastSpace = c;

            Rect cRect = box.font->sprites[assertCast<u32>(*c)];
            Vec2 size = cRect.end - cRect.begin;

            if (width + size.x() > box.cutoff)
            {
                if (box.shouldBreakAtSpace && lastSpace != nullptr)
                    c = lastSpace;
                break;
            }
            width += size.x();
        }
        toDraw = std::string_view{text.data(), assertCast<usize>(c - text.data())};
    }

    Vec2 size = bounds.end - bounds.begin;
    Vec2 pos = box.pos + (size / 2) * (box.beginDir - Vec2{1});
    bounds.begin = pos;
    bounds.end += pos;
    for (char c : toDraw)
    {
        if (pos.x() > bounds.end.x())
            break;

        Rect sprite = box.font->get(assertCast<u32>(c));
        Vec2 spriteSize = (sprite.end - sprite.begin) * factor;

        Rect dst = {pos, pos + spriteSize};
        drawSprite(box.font->texture, sprite, dst, color);

        pos.x() += spriteSize.x();
    }

    return {text.begin() + toDraw.length(), text.end()};
}

namespace {

struct ViewProj {
    Mat4 proj{};
    Mat4 view{};
};

struct Push {
    Mat4 model{};
    u32 vpIdx = 0;
    u32 instIdx = 0;
};

} // namespace

#include "shaders/render2d.vert.spv.h"
#include "shaders/render2d.frag.spv.h"

static u32 pixelFontCount = 0;

Renderer2D::Renderer2D(Gpu* gpu, Format colorFormat)
    : gpu{gpu}
{
    if (pixelFontCount++ == 0)
        initPixelFont(gpu);

    GpuGraphicsPipelineCreateInfo pipelineConfig{};
    pipelineConfig.vertexShader = std::as_bytes(std::span{render2d_vert_spv});
    pipelineConfig.fragmentShader = std::as_bytes(std::span{render2d_frag_spv});
    pipelineConfig.pushConstantSize = sizeof(Push);
    pipelineConfig.colorAttachmentFormats = {&colorFormat, 1};
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = {&enableColorBlend, 1};

    pipeline = gpu->createGraphicsPipeline(pipelineConfig);

    vpBuffer = gpu->createBuffer(sizeof(ViewProj),
        GpuBufferUsage::uniformBuffer, GpuMemoryUsage::frequentUpdate);

    ViewProj vp{
        matView(Vec3{0.0f}, Vec3{1.0f}, 1.0f),
        matOrthographic(-1, 1, -1, 1, 0, 1),
    };
    vpBuffer->write(std::as_bytes(std::span{&vp, 1}), 0);
}

Renderer2D::Renderer2D(Renderer2D&& other) noexcept
    : gpu{std::exchange(other.gpu, nullptr)}
    , pipeline{std::move(other.pipeline)}
    , vpBuffer{std::move(other.vpBuffer)}
    , layerQueue{std::move(other.layerQueue)}
{}

Renderer2D& Renderer2D::operator=(Renderer2D&& other) noexcept
{
    if (this != &other)
    {
        this->~Renderer2D();
        new (this) Renderer2D{std::move(other)};
    }
    return *this;
}

Renderer2D::~Renderer2D() noexcept
{
    if (gpu != nullptr && --pixelFontCount == 0)
        deinitPixelFont();
}

void Renderer2D::drawLayer(Layer2D& layer)
{
    layerQueue.emplace_back(&layer);
}

void Renderer2D::render(GpuCommandBuffer* cmd, const Camera& camera)
{
    HG_ASSERT(cmd != nullptr);

    ViewProj vp{};
    match(camera.projection,
        [&](const Perspective& persp)
        {
            vp.proj = matPerspective(
                persp.fov,
                persp.aspect,
                persp.near,
                persp.far);
        },
        [&](const Orthographic& ortho)
        {
            vp.proj = matOrthographic(
                ortho.left,
                ortho.right,
                ortho.top,
                ortho.bottom,
                ortho.near,
                ortho.far);
        });
    vp.view = matView(camera.position, Vec3{1.0f}, camera.rotation);
    vpBuffer->write(std::as_bytes(std::span{&vp, 1}), 0);

    for (Layer2D* layer : layerQueue)
    {
        if (layer->instances.size() == 0)
            continue;

        if (layer->changed)
        {
            if (layer->instances.capacity() > layer->instanceCapacity)
            {
                gpu->waitIdle();

                layer->instanceBuffer = gpu->createBuffer(layer->instances.capacity() * sizeof(Layer2D::Instance),
                    GpuBufferUsage::transferDst | GpuBufferUsage::storageBuffer, GpuMemoryUsage::frequentUpdate);
                layer->instanceCapacity = assertCast<u32>(layer->instances.capacity());
            }

            layer->instanceBuffer->write(std::as_bytes(std::span{layer->instances}), 0);

            layer->changed = false;
        }

        cmd->bindPipeline(pipeline.get());

        Push push{};
        push.model = layer->transform;
        push.vpIdx = vpBuffer->uniformDescriptor();
        push.instIdx = layer->instanceBuffer->storageDescriptor();

        cmd->pushConstants(pipeline.get(), push);

        cmd->draw(0, 6, 0, assertCast<u32>(layer->instances.size()));
    }

    layerQueue.clear();
}

} // namespace hg

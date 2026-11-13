#include "hg/render2d.hpp"

#include "hg/utility.hpp"
#include "hg/error.hpp"
#include "hg/memory.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

namespace hg {

void Camera::update()
{
    if (vpBuffer.data == nullptr)
    {
        vpBuffer = GpuBuffer::create(sizeof(VPBuffer),
            GpuBufferUsage_uniformBuffer, GpuMemoryUsage_frequentUpdate);
    }

    VPBuffer vp{};
    match(projection,
        [&](Perspective& persp)
        {
            vp.proj = matPerspective(
                persp.fov,
                persp.aspect,
                persp.near,
                persp.far);
        },
        [&](Orthographic& ortho)
        {
            vp.proj = matOrthographic(
                ortho.left,
                ortho.right,
                ortho.top,
                ortho.bottom,
                ortho.near,
                ortho.far);
        });
    vp.view = matView(position, Vec3{1.0f}, rotation);

    vpBuffer.write(std::as_bytes(std::span{&vp, 1}), 0);
}

template<>
bool serialize(Serializer* s, Orthographic* ortho)
{
    return serializeObject(s,
        &ortho->left,
        &ortho->right,
        &ortho->top,
        &ortho->bottom,
        &ortho->near,
        &ortho->far);
}

template<>
bool serialize(Serializer* s, Perspective* persp)
{
    return serializeObject(s,
        &persp->aspect,
        &persp->fov,
        &persp->near,
        &persp->far);
}

template<>
bool serialize(Serializer* s, Camera* camera)
{
    return serializeObject(s,
        &camera->projection,
        &camera->position,
        &camera->rotation);
}

TextureData::~TextureData() noexcept
{
    std::free(pixels);
}

template<>
bool serialize(Serializer* s, TextureData* tex)
{
    serializeBegin(s);
    HG_DEFER(serializeEnd(s));

    if (!serialize(s, &tex->width))
        return false;
    if (!serialize(s, &tex->height))
        return false;
    if (!serialize(s, &tex->depth))
        return false;
    if (!serialize(s, &tex->format))
        return false;

    u64 size = tex->width * tex->height * tex->depth * formatToSize(tex->format);
    if (!s->writing)
        tex->pixels = std::malloc(size);
    return serializeVoid(s, {static_cast<u8*>(tex->pixels), size});
}

template<>
bool assetLoadImpl(AssetData<TextureData>* data)
{
    std::optional<std::vector<u8>> bin = loadFile(data->path);
    if (!bin.has_value())
    {
        setError("Could not load asset file: {}", data->path.c_str());
        return false;
    }

    if (isSerialData(*bin))
    {
        std::optional<Serializer> s = readSerialBinary(*bin);
        if (!s.has_value() || !serialize(&*s, &data->asset))
        {
            setError("Could not deserialize serial data from file: {}", data->path.c_str());
            return false;
        }
        return true;
    }

    int x, y, channels;
    data->asset.pixels = stbi_load_from_memory(
        bin->data(),
        static_cast<int>(bin->size()),
        &x,
        &y,
        &channels,
        4);

    if (data->asset.pixels == nullptr)
    {
        setError("Could not decode image: {}", data->path.c_str());
        return false;
    }
    data->asset.width = static_cast<u32>(x);
    data->asset.height = static_cast<u32>(y);
    data->asset.depth = 1;
    data->asset.format = Format_rgba8_srgb;
    return true;
}

bool storeTexturePng(const TextureData& texture, std::string_view path)
{
    ArenaScope scratch = getScratch();
    string_temp p{path, scratch};

    if (!stbi_write_png(
         p.c_str(),
         static_cast<int>(texture.width),
         static_cast<int>(texture.height),
         4,
         texture.pixels,
         static_cast<int>(texture.width * sizeof(u32))))
    {
        setError("Could not store image: {}", p.c_str());
        return false;
    }
    return true;
}

bool storeTextureJpg(const TextureData& texture, u32 quality, std::string_view path)
{
    ArenaScope scratch = getScratch();
    string_temp p{path, scratch};

    if (!stbi_write_jpg(
         p.c_str(),
         static_cast<int>(texture.width),
         static_cast<int>(texture.height),
         4,
         texture.pixels,
         static_cast<int>(quality)))
    {
        setError("Could not store image: {}", p.c_str());
        return false;
    }
    return true;
}

bool storeTextureHdr(const TextureData& texture, std::string_view path)
{
    ArenaScope scratch = getScratch();
    string_temp p{path, scratch};

    if (!stbi_write_hdr(
         p.c_str(),
         static_cast<int>(texture.width),
         static_cast<int>(texture.height),
         4,
         static_cast<f32*>(texture.pixels)))
    {
        setError("Could not store image: {}", p.c_str());
        return false;
    }
    return true;
}

Texture uploadTextureToGpu(const TextureData& data)
{
    if (data.pixels == nullptr)
        return {};

    GpuImageCreateInfo imageInfo{};
    imageInfo.format = data.format;
    imageInfo.width = data.width;
    imageInfo.height = data.height;
    imageInfo.depth = data.depth;
    imageInfo.usage = GpuImageUsage_transferDst | GpuImageUsage_sampled;

    Texture tex{};
    tex.image = GpuImage::createEx(imageInfo);
    tex.view = GpuView::create(tex.image, GpuAspect_color, GpuFilter_nearest);

    u64 size = static_cast<u64>(data.width) * data.height * data.depth * formatToSize(data.format);
    tex.view.write(std::span<const std::byte>{static_cast<const std::byte*>(data.pixels), size});

    return tex;
}

TextureData downloadTextureFromGpu(const Texture& tex)
{
    TextureData data{};
    data.width = tex.view.width();
    data.height = tex.view.height();
    data.depth = tex.view.depth();
    data.format = tex.view.format();

    u64 size = static_cast<u64>(data.width) * data.height * data.depth * formatToSize(data.format);
    data.pixels = std::malloc(size);

    tex.view.read(std::span{static_cast<std::byte*>(data.pixels), size});

    return data;
}

template<>
bool assetLoadImpl(AssetData<Texture>* data)
{
    Asset<TextureData> tex = load<TextureData>(data->path);
    if (tex->pixels == nullptr)
        return false;

    data->asset = uploadTextureToGpu(*tex);
    return true;
}

struct RenderState2D {
    TextureAtlas defaultFont{};
};

static RenderState2D render2D;

#include "resources/pixel_font.hg.h"

namespace internal {

void convertPixelFont()
{
    HG_LOG("converting pixel font file\n");

    Asset<TextureData> fontData = load<TextureData>("resources/pixel_font.png");

    Serializer s = serialWriter();
    serialize(&s, &*fontData);

    std::vector<u8> bin = writeSerialBinary(&s);
    storeFile(bin, "resources/pixel_font.hg");
}

void initPixelFont()
{
    std::optional<Serializer> s = readSerialBinary(resources_pixel_font_hg);

    TextureData fontData{};
    if (!s.has_value() || !serialize(&*s, &fontData))
    {
        HG_WARN("Could not deserialize pixel font: {}", getError());
    }

    render2D.defaultFont = newAsset<Texture>(uploadTextureToGpu(fontData));
    render2D.defaultFont.addEmpty(32);
    render2D.defaultFont.addGrid({{0, 0}, {1, 1}}, 12, 8, 95);
}

void deinitPixelFont()
{
    render2D = {};
}

} // namespace internal

u32 TextureAtlas::addEmpty(u32 count)
{
    u32 idx = static_cast<u32>(sprites.size());
    for (u32 i = 0; i < count; ++i)
    {
        sprites.emplace_back(rectEmpty());
    }
    return idx;
}

u32 TextureAtlas::add(Rect sprite)
{
    u32 idx = static_cast<u32>(sprites.size());
    sprites.emplace_back(sprite);
    return idx;
}

u32 TextureAtlas::addGrid(Rect grid, u32 width, u32 height, u32 count, f32 marginX, f32 marginY)
{
    u32 idx = static_cast<u32>(sprites.size());

    Vec2 subdivSize = (grid.end - grid.begin) / Vec2{static_cast<f32>(width), static_cast<f32>(height)};
    Vec2 spriteSize = subdivSize * (Vec2{1} - Vec2{marginX, marginY});

    count = std::min(count, width * height);
    u32 i = 0;
    Vec2 pos = grid.begin;
    for (u32 y = 0; y < height; ++y)
    {
        pos.x = grid.begin.x;
        for (u32 x = 0; x < width; ++x)
        {
            if (i++ == count)
                goto done;

            sprites.emplace_back(pos, pos + spriteSize);
            pos.x += subdivSize.x;
        }
        pos.y += subdivSize.y;
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
bool serialize(Serializer* s, TextureAtlas* atlas)
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
    instance.texIdx = texture.view.samplerDescriptor();

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
    instance.texIdx = texture.view.samplerDescriptor();

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
        Rect firstRect = box.font->sprites[static_cast<u32>(text[0])];
        Vec2 firstSize = firstRect.end - firstRect.begin;

        factor = box.height / firstSize.y;
        for (char c : text)
        {
            Rect cRect = box.font->sprites[static_cast<u32>(c)];
            Vec2 size = cRect.end - cRect.begin;
            bounds.end.x += size.x * factor;
        }
        bounds.end.y = box.height;
    }
    else if (box.height == INFINITY)
    {
        Rect firstRect = box.font->sprites[static_cast<u32>(text[0])];
        Vec2 firstSize = firstRect.end - firstRect.begin;

        f32 width = 0;
        for (char c : text)
        {
            Rect cRect = box.font->sprites[static_cast<u32>(c)];
            Vec2 size = cRect.end - cRect.begin;
            width += size.x;
        }
        factor = box.width / width;

        bounds.end.x = box.width;
        bounds.end.y = firstSize.y * box.width / width;
    }
    else
    {
        Rect firstRect = box.font->sprites[static_cast<u32>(text[0])];
        Vec2 firstSize = firstRect.end - firstRect.begin;

        bounds.end.y = firstSize.y;

        for (char c : text)
        {
            Rect cRect = box.font->sprites[static_cast<u32>(c)];
            Vec2 size = cRect.end - cRect.begin;
            bounds.end.x += size.x;
        }

        Rect hBounds = {{}, {bounds.end.x * box.height / bounds.end.y, box.height}};
        Rect wBounds = {{}, {box.width, bounds.end.y * box.width / bounds.end.x}};

        if (hBounds.end.x <= box.width)
        {
            factor = box.height / bounds.end.y;
            bounds = hBounds;
        }
        else if (wBounds.end.y <= box.height)
        {
            factor = box.width / bounds.end.x;
            bounds = wBounds;
        }
    }

    std::string_view toDraw = text;
    if (box.cutoff < bounds.end.x)
    {
        const char* lastSpace = nullptr;
        const char* c = text.begin();
        f32 width = 0;
        for (; c != text.end(); ++c)
        {
            if (*c == ' ')
                lastSpace = c;

            Rect cRect = box.font->sprites[static_cast<u32>(*c)];
            Vec2 size = cRect.end - cRect.begin;

            if (width + size.x > box.cutoff)
            {
                if (box.shouldBreakAtSpace && lastSpace != nullptr)
                    c = lastSpace;
                break;
            }
            width += size.x;
        }
        toDraw = {text.begin(), c};
    }

    Vec2 size = bounds.end - bounds.begin;
    Vec2 pos = box.pos + (size / 2) * (box.beginDir - Vec2{1});
    bounds.begin = pos;
    bounds.end += pos;
    for (char c : toDraw)
    {
        if (pos.x > bounds.end.x)
            break;

        Rect sprite = box.font->get(static_cast<u32>(c));
        Vec2 spriteSize = (sprite.end - sprite.begin) * factor;

        Rect dst = {pos, pos + spriteSize};
        drawSprite(*box.font->texture, sprite, dst, color);

        pos.x += spriteSize.x;
    }

    return {text.begin() + toDraw.length(), text.end()};
}

struct RenderPush2D {
    Mat4 model = {};
    u32 vpIdx = 0;
    u32 instIdx = 0;
};

#include "shaders/render2d.vert.spv.h"
#include "shaders/render2d.frag.spv.h"

Renderer2D::Renderer2D(Format colorFormat)
{
    GpuGraphicsPipelineCreateInfo pipelineConfig{};
    pipelineConfig.vertexShader = shaders_render2d_vert_spv;
    pipelineConfig.fragmentShader = shaders_render2d_frag_spv;
    pipelineConfig.pushConstantSize = sizeof(RenderPush2D);
    pipelineConfig.colorAttachmentFormats = {&colorFormat, 1};
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = {&enableColorBlend, 1};

    pipeline = GpuPipeline::graphics(pipelineConfig);
}

void Renderer2D::drawLayer(Layer2D& layer)
{
    layerQueue.emplace_back(&layer);
}

void Renderer2D::render(GpuCmd* cmd, const Camera& camera)
{
    HG_ASSERT(cmd != nullptr);

    for (Layer2D* layer : layerQueue)
    {
        if (layer->instances.size() == 0)
            continue;

        if (layer->changed)
        {
            if (layer->instances.capacity() > layer->instanceCapacity)
            {
                gpuWaitIdle();

                layer->instanceBuffer = GpuBuffer::create(layer->instances.capacity() * sizeof(Layer2D::Instance),
                    GpuBufferUsage_transferDst | GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate);
                layer->instanceCapacity = static_cast<u32>(layer->instances.capacity());
            }

            layer->instanceBuffer.write(std::as_bytes(std::span{layer->instances}), 0);

            layer->changed = false;
        }

        gpuBindPipeline(cmd, pipeline);

        RenderPush2D push{};
        push.model = layer->transform;
        push.vpIdx = camera.vpBuffer.uniformDescriptor();
        push.instIdx = layer->instanceBuffer.storageDescriptor();

        gpuPushConstants(cmd, pipeline, push);

        gpuDraw(cmd, 0, 6, 0, static_cast<u32>(layer->instances.size()));
    }

    layerQueue.clear();
}

} // namespace hg

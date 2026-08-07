#include "hg/render2d.hpp"
#include "hg/error.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

namespace hg {

struct RenderState2D {
    Texture defaultTex{};
    Atlas2D defaultFont{};
};

static RenderState2D render2D;

#include "pixel_font.h"

namespace internal {

void initRender2D()
{
    ArenaScope scratch = getScratch();

    struct Color {
        u8 r, g, b, a;
    };
    Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    render2D.defaultTex.image = GpuImage::create(2, 2, Format_r8g8b8a8_srgb,
        GpuImageUsage_sampled | GpuImageUsage_transferDst);
    render2D.defaultTex.view = GpuView::create(render2D.defaultTex.image, GpuAspect_color, GpuFilter_nearest);
    render2D.defaultTex.view.write(defaultColors);

    TextureData fontData{};
    Serializer s = readSerialBinary(scratch, {pixel_font, sizeof(pixel_font)});
    serializeBegin(&s);
    serializeObject(&s, &fontData.width, &fontData.height, &fontData.format);
    u64 size = fontData.width * fontData.height * formatToSize(fontData.format);
    fontData.pixels = heapAlloc(size, formatToSize(fontData.format));
    serializeVoid(&s, {fontData.pixels, size});
    serializeEnd(&s);

    render2D.defaultFont.texture = newAsset<Texture>();
    *render2D.defaultFont.texture = createTextureFromData(fontData);

    render2D.defaultFont.addEmpty(32);
    render2D.defaultFont.addGrid({{0, 0}, {1, 1}}, 12, 8, 95, 1.0f / 6.0f, 1.0f / 8.0f);
}

void deinitRender2D()
{
    render2D = {};
}

} // namespace internal

template<>
void assetLoadImpl(AssetData<TextureData>* data)
{
    ArenaScope scratch = getScratch();
    char* cpath = cString(scratch, data->path);

    int x, y, channels;
    data->asset.pixels = stbi_load(cpath, &x, &y, &channels, 4);
    if (data->asset.pixels == nullptr)
    {
        setError("Could not load image: %s", cpath);
        return;
    }
    data->asset.width = static_cast<u32>(x);
    data->asset.height = static_cast<u32>(y);
    data->asset.depth = 1;
    data->asset.format = Format_r8g8b8a8_srgb;
}

TextureData::~TextureData() noexcept
{
    std::free(pixels);
}

bool textureStorePng(TextureData* texture, StringView path)
{
    ArenaScope scratch = getScratch();

    const char* cpath = cString(scratch, path);

    if (!stbi_write_png(
         cpath,
         static_cast<int>(texture->width),
         static_cast<int>(texture->height),
         4,
         texture->pixels,
         static_cast<int>(texture->width * sizeof(u32))))
    {
        setError("Could not store image: %s", cpath);
        return false;
    }
    return true;
}

Texture createTextureFromData(const TextureData& data)
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
    tex.view.write(data.pixels);

    return tex;
}

template<>
void assetLoadImpl(AssetData<Texture>* data)
{
    Asset<TextureData> tex = load<TextureData>(data->path);
    if (tex->pixels == nullptr)
        return;

    data->asset = createTextureFromData(*tex);
}

template<>
void assetLoadImpl(AssetData<MeshData>* data)
{
    static_cast<void>(data);
    HG_PANIC("load gltf file : TODO\n");
}

void meshStoreGltf(MeshData* data, StringView path)
{
    static_cast<void>(data);
    static_cast<void>(path);
    HG_PANIC("store gltf file : TODO\n");
}

template<>
void assetLoadImpl(AssetData<Mesh>* data)
{
    Asset<MeshData> mesh = load<MeshData>(data->path);

    data->asset.vertexCount = static_cast<u32>(mesh->vertices.count);
    data->asset.vertexWidth = sizeof(MeshVertex);
    data->asset.indexCount = static_cast<u32>(mesh->indices.count);

    data->asset.vertexBuffer = GpuBuffer::create(
        data->asset.vertexCount * data->asset.vertexWidth, GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);

    data->asset.indexBuffer = GpuBuffer::create(
        data->asset.indexCount * sizeof(u32), GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);

    data->asset.vertexBuffer.write(mesh->vertices.vals, 0, data->asset.vertexCount * data->asset.vertexWidth);
    data->asset.indexBuffer.write(mesh->indices.vals, 0, data->asset.indexCount * sizeof(u32));
}

struct VPUniform {
    Mat4 proj = {};
    Mat4 view = {};
};

void Camera::setPerspective(f32 aspect, f32 fov, f32 near, f32 far)
{
    projection = CameraPerspective{aspect, fov, near, far};
}

void Camera::setOrthographic(f32 width, f32 height, f32 actualAspect)
{
    projection = CameraOrthographic{0, width, 0, height, 0, 1};

    if (actualAspect != 0.0)
    {
        CameraOrthographic& ortho = projection.get<CameraOrthographic>();
        if (actualAspect > static_cast<f32>(width) / static_cast<f32>(height))
        {
            f32 margin = actualAspect - static_cast<f32>(width) / static_cast<f32>(height);
            ortho.left -= margin * width / 2.0f;
            ortho.right += margin * width / 2.0f;
        }
        else
        {
            f32 margin = 1.0f / actualAspect - static_cast<f32>(height) / static_cast<f32>(width);
            ortho.top -= margin * height / 2.0f;
            ortho.bottom += margin * height / 2.0f;
        }
    }
}

void Camera::update()
{
    if (vpBuffer.data == nullptr)
    {
        vpBuffer = GpuBuffer::create(sizeof(VPUniform),
            GpuBufferUsage_uniformBuffer, GpuMemoryUsage_frequentUpdate);
    }

    VPUniform vp{};
    vp.view = matView(position, Vec3{1.0f}, rotation);
    projection.match(
        [&](CameraPerspective& persp)
        {
            vp.proj = matPerspective(
                persp.fov,
                persp.aspect,
                persp.near,
                persp.far);
        },
        [&](CameraOrthographic& ortho)
        {
            vp.proj = matOrthographic(
                ortho.left,
                ortho.right,
                ortho.top,
                ortho.bottom,
                ortho.near,
                ortho.far);
        });

    vpBuffer.write(&vp, 0, sizeof(vp));
}

template<>
void serialize(Serializer* s, CameraPerspective* camera)
{
    serializeObject(s,
        &camera->aspect,
        &camera->fov,
        &camera->near,
        &camera->far);
}

template<>
void serialize(Serializer* s, CameraOrthographic* camera)
{
    serializeObject(s,
        &camera->left,
        &camera->right,
        &camera->top,
        &camera->bottom,
        &camera->near,
        &camera->far);
}

template<>
void serialize(Serializer* s, Camera* camera)
{
    serializeBegin(s);
    serialize(s, &camera->rotation);
    serialize(s, &camera->position);
    serialize(s, &camera->projection);
    serializeEnd(s);
}

Atlas2D Atlas2D::create(const Asset<Texture>& texture)
{
    Atlas2D atlas{};
    atlas.texture = texture.clone();
    atlas.sprites = Array<Rect>{0, 1024};
    return atlas;
}

u32 Atlas2D::addEmpty(u32 count)
{
    u32 idx = static_cast<u32>(sprites.count);
    for (u32 i = 0; i < count; ++i)
    {
        sprites.push(rectEmpty());
    }
    return idx;
}

u32 Atlas2D::add(Rect sprite)
{
    u32 idx = static_cast<u32>(sprites.count);
    sprites.push(sprite);
    return idx;
}

u32 Atlas2D::addGrid(Rect grid, u32 width, u32 height, u32 count, f32 marginX, f32 marginY)
{
    u32 idx = static_cast<u32>(sprites.count);

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

            sprites.push({pos, pos + spriteSize});
            pos.x += subdivSize.x;
        }
        pos.y += subdivSize.y;
    }

done:
    return idx;
}

Sprite2D Atlas2D::get(u32 idx) const
{
    return {texture.clone(), sprites[idx]};
}

const Atlas2D& getDefaultFont()
{
    return render2D.defaultFont;
}

template<>
void serialize(Serializer* s, Atlas2D* atlas)
{
    serializeObject(s, &atlas->texture, &atlas->sprites);
}

template<>
void assetLoadImpl(AssetData<Atlas2D>* data)
{
    ArenaScope scratch = getScratch();

    Serializer s = readSerialBinary(scratch, *load<Binary>(data->path));
    serialize(&s, &data->asset);
}

Tilemap2D Tilemap2D::create(u32 width, u32 height, const Asset<Atlas2D>& atlas)
{
    Tilemap2D tilemap{};

    tilemap.atlas = atlas.clone();
    tilemap.tiles = Array<u32>{width * height, width * height};
    tilemap.width = width;
    tilemap.height = height;

    for (u32 i = 0; i < width * height; ++i)
    {
        tilemap.tiles[i] = static_cast<u32>(-1);
    }

    return tilemap;
}

u32 Tilemap2D::get(u32 x, u32 y) const
{
    return tiles[y * width + x];
}

u32 Tilemap2D::set(u32 x, u32 y, u32 tile)
{
    return tiles[y * width + x] = tile;
}

template<>
void serialize(Serializer* s, Tilemap2D* tilemap)
{
    serializeObject(s, &tilemap->atlas, &tilemap->tiles, &tilemap->width, &tilemap->height);
}

void Layer2D::clear()
{
    instances.count = 0;
    changed = true;
}

void Layer2D::drawRect(Vec4 color, Rect dst)
{
    using internal::Render2DInstance;

    Render2DInstance instance{};
    instance.color = color;
    instance.pos = dst.begin;
    instance.size = dst.end - dst.begin;

    instances.push(instance);
    changed = true;
}

void Layer2D::drawSprite(const Sprite2D& sprite, Rect dst, Vec4 tint)
{
    using internal::Render2DInstance;

    Texture* texture = sprite.texture == nullptr
        ? &render2D.defaultTex
        : &*sprite.texture;

    Render2DInstance instance{};
    instance.color = tint;
    instance.pos = dst.begin;
    instance.size = dst.end - dst.begin;
    instance.uvPos = sprite.uv.begin;
    instance.uvSize = sprite.uv.end - sprite.uv.begin;
    instance.origin = {};
    instance.rotation = 0;
    instance.texIdx = texture->view.samplerDescriptor();

    instances.push(instance);
    changed = true;
}

void Layer2D::drawSpriteRot(const Sprite2D& sprite, Rect dst, Vec2 origin, f32 rotation, Vec4 tint)
{
    using internal::Render2DInstance;

    Texture* texture = sprite.texture == nullptr
        ? &render2D.defaultTex
        : &*sprite.texture;

    Render2DInstance instance{};
    instance.color = tint;
    instance.pos = dst.begin;
    instance.size = dst.end - dst.begin;
    instance.uvPos = sprite.uv.begin;
    instance.uvSize = sprite.uv.end - sprite.uv.begin;
    instance.origin = origin;
    instance.rotation = rotation;
    instance.texIdx = texture->view.samplerDescriptor();

    instances.push(instance);
    changed = true;
}

void Layer2D::drawTilemap(const Tilemap2D& tilemap, Rect dst)
{
    Vec2 pos = dst.begin;
    Vec2 size = (dst.end - dst.begin) / Vec2{static_cast<f32>(tilemap.width), static_cast<f32>(tilemap.height)};
    for (u32 y = 0; y < tilemap.height; ++y)
    {
        pos.x = dst.begin.x;
        for (u32 x = 0; x < tilemap.width; ++x)
        {
            drawSprite(tilemap.atlas->get(tilemap.get(x, y)), {pos, pos + size});
            pos.x += size.x;
        }
        pos.y += size.y;
    }
}

StringView Layer2D::drawText(StringView text, const Atlas2D& font, Vec4 color, Rect bounds, f32 spacing, bool breakAtSpace)
{
    Vec2 dst = bounds.begin;
    Vec2 boundSize = bounds.end - bounds.begin;

    const char* c = text.begin();
    const char* end = text.end();
    while (c != end)
    {
        Sprite2D sprite = font.get((u32)*c);
        Vec2 spriteSize = sprite.uv.end - sprite.uv.begin;

        Vec2 dstSize = {boundSize.y * spriteSize.x / spriteSize.y, boundSize.y};
        Vec2 dstEnd = dst + dstSize;
        if (dstEnd.x > bounds.end.x)
        {
            if (breakAtSpace)
            {
                while (c > text.chars && *c != ' ')
                {
                    --c;
                    instances.pop();
                }
                if (*c == ' ')
                    ++c;
            }
            break;
        }

        drawSprite(sprite, {dst, dstEnd}, color);
        dst.x += dstSize.x + spacing;
        ++c;
    }
    return {c, end};
}

struct RenderPush2D {
    Mat4 model = {};
    u32 vpIdx = 0;
    u32 instIdx = 0;
};

#include "shaders/render2d.vert.spv.h"
#include "shaders/render2d.frag.spv.h"
#include "shaders/debug2d.frag.spv.h"

Renderer2D::Renderer2D(Format colorFormat)
{
    GpuGraphicsPipelineCreateInfo pipelineConfig{};
    pipelineConfig.vertexShader = {shaders_render2d_vert_spv, sizeof(shaders_render2d_vert_spv)};
    pipelineConfig.fragmentShader = {shaders_render2d_frag_spv, sizeof(shaders_render2d_frag_spv)};
    pipelineConfig.pushConstantSize = sizeof(RenderPush2D);
    pipelineConfig.colorAttachmentFormats = {&colorFormat, 1};
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = {&enableColorBlend, 1};

    pipeline = GpuPipeline::graphics(pipelineConfig);
}

DebugRenderer2D::DebugRenderer2D(Format colorFormat)
{
    GpuGraphicsPipelineCreateInfo pipelineConfig{};
    pipelineConfig.vertexShader = {shaders_render2d_vert_spv, sizeof(shaders_render2d_vert_spv)};
    pipelineConfig.fragmentShader = {shaders_debug2d_frag_spv, sizeof(shaders_debug2d_frag_spv)};
    pipelineConfig.pushConstantSize = sizeof(RenderPush2D);
    pipelineConfig.colorAttachmentFormats = {&colorFormat, 1};
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = {&enableColorBlend, 1};
    pipelineConfig.topology = GpuTopology_lineStrip;

    pipeline = GpuPipeline::graphics(pipelineConfig);
}

static void renderRenderer2D(GpuCmd* cmd, Span<Layer2D*> layers, const Camera& camera, const GpuPipeline& pipeline)
{
    using internal::Render2DInstance;

    HG_ASSERT(cmd != nullptr);

    for (Layer2D* layer : layers)
    {
        if (layer->instances.count == 0)
            return;

        if (layer->changed)
        {
            if (layer->instances.capacity > layer->instanceCapacity)
            {
                gpuWaitIdle();

                layer->instanceBuffer = GpuBuffer::create(layer->instances.capacity * sizeof(Render2DInstance),
                    GpuBufferUsage_transferDst | GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate);
                layer->instanceCapacity = static_cast<u32>(layer->instances.capacity);
            }

            layer->instanceBuffer.write(layer->instances.vals, 0, layer->instances.count * sizeof(Render2DInstance));

            layer->changed = false;
        }

        gpuBindPipeline(cmd, pipeline);

        RenderPush2D push{};
        push.model = layer->transform;
        push.vpIdx = camera.vpBuffer.uniformDescriptor();
        push.instIdx = layer->instanceBuffer.storageDescriptor();

        gpuPushConstants(cmd, pipeline, &push, sizeof(push));

        gpuDraw(cmd, 0, 6, 0, static_cast<u32>(layer->instances.count));
    }
}

void Renderer2D::queueLayer(Layer2D& layer)
{
    layerQueue.push(&layer);
}

void DebugRenderer2D::queueLayer(Layer2D& layer)
{
    layerQueue.push(&layer);
}

void Renderer2D::render(GpuCmd* cmd, const Camera& camera)
{
    renderRenderer2D(cmd, layerQueue, camera, pipeline);
    layerQueue.reset();
}

void DebugRenderer2D::render(GpuCmd* cmd, const Camera& camera)
{
    renderRenderer2D(cmd, layerQueue, camera, pipeline);
    layerQueue.reset();
}

} // namespace hg

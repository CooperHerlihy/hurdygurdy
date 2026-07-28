#include "hg_render2d.hpp"

namespace hg {

struct RenderState2D {
    GpuPipeline pipeline{};
    GpuPipeline debugPipeline{};
    Texture defaultTex{};
};

static RenderState2D render2D;

struct RenderPush2D {
    Mat4 model = {};
    u32 vpIdx = 0;
    u32 instIdx = 0;
};

#include "shaders/render2d.vert.spv.h"
#include "shaders/render2d.frag.spv.h"
#include "shaders/debug2d.frag.spv.h"

void initRenderer2D(Format colorFormat)
{
    GpuGraphicsPipelineCreateInfo pipelineConfig{};
    pipelineConfig.vertexShader = {shaders_render2d_vert_spv, sizeof(shaders_render2d_vert_spv)};
    pipelineConfig.fragmentShader = {shaders_render2d_frag_spv, sizeof(shaders_render2d_frag_spv)};
    pipelineConfig.pushConstantSize = sizeof(RenderPush2D);
    pipelineConfig.colorAttachmentFormats = {&colorFormat, 1};
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = {&enableColorBlend, 1};

    render2D.pipeline = GpuPipeline::graphics(pipelineConfig);

    pipelineConfig.fragmentShader = {shaders_debug2d_frag_spv, sizeof(shaders_debug2d_frag_spv)};
    pipelineConfig.topology = GpuTopology_lineStrip;

    render2D.debugPipeline = GpuPipeline::graphics(pipelineConfig);

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
}

void deinitRenderer2D()
{
    render2D = {};
}

Atlas2D Atlas2D::create(const Asset<Texture>& texture)
{
    Atlas2D atlas{};
    atlas.texture = texture.clone();
    atlas.sprites = Array<Rect>{0, 1024};
    return atlas;
}

u32 Atlas2D::add(Rect sprite)
{
    u32 idx = static_cast<u32>(sprites.count);
    sprites.push(sprite);
    return idx;
}

u32 Atlas2D::addGrid(Rect grid, u32 width, u32 height)
{
    u32 idx = static_cast<u32>(sprites.count);

    Vec2 spriteSize = (grid.end - grid.begin) / Vec2{static_cast<f32>(width), static_cast<f32>(height)};
    Vec2 pos = grid.begin;
    for (u32 y = 0; y < height; ++y)
    {
        pos.x = grid.begin.x;
        for (u32 x = 0; x < width; ++x)
        {
            sprites.push({pos, pos + spriteSize});
            pos.x += spriteSize.x;
        }
        pos.y += spriteSize.y;
    }

    return idx;
}

Sprite2D Atlas2D::get(u32 idx)
{
    return {texture.clone(), sprites[idx]};
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

Layer2D Layer2D::create()
{
    using internal::Render2DInstance;

    Layer2D layer{};
    layer.instances = Array<Render2DInstance>{0, 1024};
    layer.instanceBuffer = GpuBuffer::create(layer.instances.capacity * sizeof(Render2DInstance),
        GpuBufferUsage_transferDst | GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate);
    layer.instanceCapacity = static_cast<u32>(layer.instances.capacity);
    layer.transform = Mat4{1.0f};
    layer.changed = true;

    return layer;
}

void Layer2D::clear()
{
    instances.count = 0;
    changed = true;
}

static void renderLayer2D(GpuCmd* cmd, Camera* camera, Layer2D* layer, const GpuPipeline& pipeline)
{
    using internal::Render2DInstance;

    HG_ASSERT(cmd != nullptr);
    HG_ASSERT(camera != nullptr);
    HG_ASSERT(layer != nullptr);

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
    push.vpIdx = camera->vpBuffer.uniformDescriptor();
    push.instIdx = layer->instanceBuffer.storageDescriptor();

    gpuPushConstants(cmd, pipeline, &push, sizeof(push));

    gpuDraw(cmd, 0, 6, 0, static_cast<u32>(layer->instances.count));
}

void Layer2D::render(GpuCmd* cmd, Camera* camera)
{
    renderLayer2D(cmd, camera, this, render2D.pipeline);
}

void Layer2D::renderDebug(GpuCmd* cmd, Camera* camera)
{
    renderLayer2D(cmd, camera, this, render2D.debugPipeline);
}

void Layer2D::drawRect(Vec4 color, Rect dst)
{
    using internal::Render2DInstance;
    using internal::Render2DInstanceType_color;

    Render2DInstance instance{};
    instance.rect.pos = dst.begin;
    instance.rect.size = dst.end - dst.begin;
    instance.rect.type = Render2DInstanceType_color;
    instance.rect.color = color;

    instances.push(instance);
    changed = true;
}

void Layer2D::drawSprite(const Sprite2D& sprite, Rect dst)
{
    using internal::Render2DInstance;
    using internal::Render2DInstanceType_sprite;

    Texture* texture = sprite.texture == nullptr
        ? &render2D.defaultTex
        : &*sprite.texture;

    Render2DInstance instance{};
    instance.sprite.pos = dst.begin;
    instance.sprite.size = dst.end - dst.begin;
    instance.sprite.type = Render2DInstanceType_sprite;
    instance.sprite.tex = texture->view.samplerDescriptor();
    instance.sprite.uvPos = sprite.uv.begin;
    instance.sprite.uvSize = sprite.uv.end - sprite.uv.begin;

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

} // namespace hg

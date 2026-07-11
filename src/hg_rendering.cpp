#include "hg_core.hpp"
#include "hg_ecs.hpp"
#include "hg_rendering.hpp"
#include "hg_templates.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

namespace hg {

template<>
void assetLoadImpl(Asset<TextureData>* data)
{
    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);
    char* cpath = cString(sc, data->path);

    int x, y, channels;
    data->asset.pixels = stbi_load(cpath, &x, &y, &channels, 4);
    if (data->asset.pixels == nullptr)
    {
        errorFormat("Could not load image: %s", cpath);
        return;
    }
    data->asset.width = (u32)x;
    data->asset.height = (u32)y;
    data->asset.depth = 1;
    data->asset.format = Format_r8g8b8a8_srgb;
}

template<>
void assetUnloadImpl(Asset<TextureData>* data)
{
    free(data->asset.pixels);
}

bool textureStorePng(TextureData* texture, String path)
{
    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    const char* cpath = cString(sc, path);

    if (!stbi_write_png(
         cpath,
         (int)texture->width,
         (int)texture->height,
         4,
         texture->pixels,
         (int)(texture->width * sizeof(u32))))
    {
        errorFormat("Could not store image: %s", cpath);
        return false;
    }
    return true;
}

template<>
void assetLoadImpl(Asset<Texture>* data)
{
    TextureDataAsset* tex = assetLoad<TextureData>(data->path);
    HG_DEFER(assetUnload(tex));
    if (tex->asset.pixels == nullptr)
        return;

    GpuImageCreateEx imageInfo{};
    imageInfo.format = tex->asset.format;
    imageInfo.width = tex->asset.width;
    imageInfo.height = tex->asset.height;
    imageInfo.depth = tex->asset.depth;
    imageInfo.usage = GpuImageUsage_transferDst | GpuImageUsage_sampled;

    data->asset.image = gpuImageCreateEx(&imageInfo);
    data->asset.view = gpuViewCreate(data->asset.image, GpuAspect_color, GpuFilter_nearest);

    gpuImageWrite(data->asset.view, tex->asset.pixels);
}

template<>
void assetUnloadImpl(Asset<Texture>* data)
{
    gpuViewDestroy(data->asset.view);
    gpuImageDestroy(data->asset.image);
}

template<>
void assetLoadImpl(Asset<MeshData>* data)
{
    (void)data;
    HG_PANIC("load gltf file : TODO\n");
}

template<>
void assetUnloadImpl(Asset<MeshData>* data)
{
    gpaFree(data->asset.indices, data->asset.indexCount);
    gpaFree(data->asset.vertices, data->asset.vertexCount);
}

void meshStoreGltf(MeshData* data, String path, Fence* fence)
{
    (void)data;
    (void)path;
    (void)fence;
    HG_PANIC("store gltf file : TODO\n");
}

template<>
void assetLoadImpl(Asset<Mesh>* data)
{
    MeshDataAsset* mesh = assetLoad<MeshData>(data->path);
    HG_DEFER(assetUnload(mesh));

    if (mesh->asset.vertices == nullptr || mesh->asset.indices == nullptr)
        return;

    data->asset.vertexCount = mesh->asset.vertexCount;
    data->asset.vertexWidth = mesh->asset.vertexWidth;
    data->asset.indexCount = mesh->asset.indexCount;

    data->asset.vertexBuffer = gpuBufferCreate(
        data->asset.vertexCount * data->asset.vertexWidth,
        GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);

    data->asset.indexBuffer = gpuBufferCreate(
        data->asset.indexCount * sizeof(u32),
        GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);

    gpuBufferWrite(
        data->asset.vertexBuffer,
        0,
        mesh->asset.vertices,
        data->asset.vertexCount * data->asset.vertexWidth);

    gpuBufferWrite(
        data->asset.indexBuffer,
        0,
        mesh->asset.indices,
        data->asset.indexCount * sizeof(u32));
}

template<>
void assetUnloadImpl(Asset<Mesh>* data)
{
    gpuBufferDestroy(data->asset.vertexBuffer);
    gpuBufferDestroy(data->asset.indexBuffer);
}

template<>
void serialize(Serializer* s, Camera* camera)
{
    serializeBegin(s);
    serialize(s, &camera->rotation);
    serialize(s, &camera->position);
    serialize(s, &camera->type);
    if (camera->type == CameraType_perspective)
    {
        serializeObject(s,
            &camera->perspective.aspect,
            &camera->perspective.fov,
            &camera->perspective.near,
            &camera->perspective.far);
    }
    else
    {
        serializeObject(s,
            &camera->orthographic.left,
            &camera->orthographic.right,
            &camera->orthographic.top,
            &camera->orthographic.bottom,
            &camera->orthographic.near,
            &camera->orthographic.far);
    }
    serializeEnd(s);
}

struct VPUniform {
    Mat4 proj;
    Mat4 view;
};

Camera cameraCreate()
{
    Camera camera{};

    camera.vpBuffer = gpuBufferCreate(
        sizeof(VPUniform),
        GpuBufferUsage_uniformBuffer,
        GpuMemoryUsage_frequentUpdate);

    camera.rotation = Quat{1.0f};
    camera.position = Vec3{0.0f};

    return camera;
}

void cameraDestroy(Camera* camera)
{
    gpuBufferDestroy(camera->vpBuffer);
}

void cameraSetPerspective(Camera* camera, f32 aspect, f32 fov, f32 near, f32 far)
{
    camera->type = CameraType_perspective;
    camera->perspective.aspect = aspect;
    camera->perspective.fov = fov;
    camera->perspective.near = near;
    camera->perspective.far = far;
}

void cameraSetOrthographic(Camera* camera, f32 width, f32 height, f32 actualAspect)
{
    camera->type = CameraType_orthographic;
    camera->orthographic.left = 0;
    camera->orthographic.right = width;
    camera->orthographic.top = 0;
    camera->orthographic.bottom = height;
    camera->orthographic.near = 0;
    camera->orthographic.far = 1;

    if (actualAspect != 0.0)
    {
        if (actualAspect > (f32)width / (f32)height)
        {
            f32 margin = actualAspect - (f32)width / (f32)height;
            camera->orthographic.left -= margin * width / 2.0f;
            camera->orthographic.right += margin * width / 2.0f;
        }
        else
        {
            f32 margin = 1.0f / actualAspect - (f32)height / (f32)width;
            camera->orthographic.top -= margin * height / 2.0f;
            camera->orthographic.bottom += margin * height / 2.0f;
        }
    }
}

void cameraUpdate(Camera* camera)
{
    VPUniform vp{};
    vp.view = matView(camera->position, Vec3{1.0f}, camera->rotation);
    if (camera->type == CameraType_perspective)
    {
        vp.proj = matPerspective(
            camera->perspective.fov,
            camera->perspective.aspect,
            camera->perspective.near,
            camera->perspective.far);
    }
    else
    {
        vp.proj = matOrthographic(
            camera->orthographic.left,
            camera->orthographic.right,
            camera->orthographic.top,
            camera->orthographic.bottom,
            camera->orthographic.near,
            camera->orthographic.far);
    }

    gpuBufferWrite(camera->vpBuffer, 0, &vp, sizeof(vp));
}

Camera* cameraAdd(Ecs* ecs, Entity e)
{
    Camera* camera = ecsAdd<Camera>(ecs, e);
    *camera = cameraCreate();
    return camera;
}

template<>
void ecsDtor(Camera* camera)
{
    cameraDestroy(camera);
}

void cameraUpdateEcs(Ecs* ecs, Entity e)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsHas<Camera>(ecs, e));
    HG_ASSERT(ecsHas<Transform>(ecs, e));

    Camera* camera = ecsGet<Camera>(ecs, e);
    Transform* tf = ecsGet<Transform>(ecs, e);
    HG_ASSERT(camera->type == CameraType_perspective || camera->type == CameraType_orthographic);

    VPUniform vp{};
    vp.view = matModelToView(tf->mat);
    if (camera->type == CameraType_perspective)
    {
        vp.proj = matPerspective(
            camera->perspective.fov,
            camera->perspective.aspect,
            camera->perspective.near,
            camera->perspective.far);
    }
    else
    {
        vp.proj = matOrthographic(
            camera->orthographic.left,
            camera->orthographic.right,
            camera->orthographic.top,
            camera->orthographic.bottom,
            camera->orthographic.near,
            camera->orthographic.far);
    }

    gpuBufferWrite(camera->vpBuffer, 0, &vp, sizeof(vp));
}

struct RenderState2D {
    GpuPipeline* pipeline;
    GpuPipeline* debugPipeline;
    Texture defaultTex;
};

static RenderState2D render2D;

struct RenderPush2D {
    Mat4 model;
    u32 vpIdx;
    u32 instIdx;
};

#include "render2d.vert.spv.h"
#include "render2d.frag.spv.h"
#include "debug2d.frag.spv.h"

void rendererInit2D(Format colorFormat)
{
    CreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = render2d_vert_spv;
    pipelineConfig.vertexShaderSize = sizeof(render2d_vert_spv);
    pipelineConfig.fragmentShader = render2d_frag_spv;
    pipelineConfig.fragmentShaderSize = sizeof(render2d_frag_spv);
    pipelineConfig.pushConstantSize = sizeof(RenderPush2D);
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    render2D.pipeline = gpuPipelineCreateGraphics(&pipelineConfig);

    pipelineConfig.fragmentShader = debug2d_frag_spv;
    pipelineConfig.fragmentShaderSize = sizeof(debug2d_frag_spv);
    pipelineConfig.topology = GpuTopology_lineStrip;

    render2D.debugPipeline = gpuPipelineCreateGraphics(&pipelineConfig);

    struct Color {
        u8 r, g, b, a;
    };
    Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    render2D.defaultTex.image = gpuImageCreate(2, 2, Format_r8g8b8a8_srgb,
        GpuImageUsage_sampled | GpuImageUsage_transferDst);

    render2D.defaultTex.view = gpuViewCreate(
        render2D.defaultTex.image, GpuAspect_color, GpuFilter_nearest);

    gpuImageWrite(render2D.defaultTex.view, defaultColors);
}

void rendererDeinit2D()
{
    gpuViewDestroy(render2D.defaultTex.view);
    gpuImageDestroy(render2D.defaultTex.image);
    gpuPipelineDestroy(render2D.debugPipeline);
    gpuPipelineDestroy(render2D.pipeline);
}

Layer2D layerCreate2D()
{
    Layer2D layer{};

    layer.instances = arrayCreate<Render2DInstance>();
    layer.instanceBuffer = gpuBufferCreate(layer.instances.capacity * sizeof(Render2DInstance),
        GpuBufferUsage_transferDst | GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate);
    layer.instanceCapacity = layer.instances.capacity;
    layer.transform = Mat4{1.0f};
    layer.changed = true;

    return layer;
}

void layerDestroy2D(Layer2D* layer)
{
    HG_ASSERT(layer != nullptr);

    gpuBufferDestroy(layer->instanceBuffer);
    arrayDestroy(&layer->instances);
}

void layerClear2D(Layer2D* layer)
{
    HG_ASSERT(layer != nullptr);

    layer->instances.count = 0;
    layer->changed = true;
}

static void renderLayer2D(GpuCmd* cmd, Camera* camera, Layer2D* layer, GpuPipeline* pipeline)
{
    HG_ASSERT(cmd != nullptr);
    HG_ASSERT(camera != nullptr);
    HG_ASSERT(layer != nullptr);

    if (layer->changed)
    {
        if (layer->instances.capacity > layer->instanceCapacity)
        {
            gpuWaitIdle();
            gpuBufferDestroy(layer->instanceBuffer);

            layer->instanceBuffer = gpuBufferCreate(layer->instances.capacity * sizeof(Render2DInstance),
                GpuBufferUsage_transferDst | GpuBufferUsage_storageBuffer, GpuMemoryUsage_frequentUpdate);
            layer->instanceCapacity = layer->instances.capacity;
        }

        gpuBufferWrite(layer->instanceBuffer, 0, layer->instances.vals, layer->instances.count * sizeof(Render2DInstance));

        layer->changed = false;
    }

    gpuBindPipeline(cmd, pipeline);

    RenderPush2D push{};
    push.model = layer->transform;
    push.vpIdx = gpuBufferUniformDescriptor(camera->vpBuffer);
    push.instIdx = gpuBufferStorageDescriptor(layer->instanceBuffer);

    gpuPushConstants(cmd, pipeline, &push, sizeof(push));

    gpuDraw(cmd, 0, 6, 0, layer->instances.count);
}

void renderLayer2D(GpuCmd* cmd, Camera* camera, Layer2D* layer)
{
    renderLayer2D(cmd, camera, layer, render2D.pipeline);
}

void renderDebug2D(GpuCmd* cmd, Camera* camera, Layer2D* layer)
{
    renderLayer2D(cmd, camera, layer, render2D.debugPipeline);
}

void drawRect2D(Layer2D* layer, Vec4 color, Rect dst)
{
    HG_ASSERT(layer != nullptr);

    Render2DInstance instance{};
    instance.rect.pos = dst.pos;
    instance.rect.size = dst.size;
    instance.rect.type = Render2DInstanceType_color;
    instance.rect.color = color;

    *arrayPush(&layer->instances) = instance;

    layer->changed = true;
}

void drawSprite2D(Layer2D* layer, Sprite2D* sprite, Rect dst)
{
    HG_ASSERT(layer != nullptr);
    HG_ASSERT(sprite != nullptr);

    Texture* texture = sprite->texture == nullptr
        ? &render2D.defaultTex
        : &sprite->texture->asset;

    Render2DInstance instance{};
    instance.sprite.pos = dst.pos;
    instance.sprite.size = dst.size;
    instance.sprite.type = Render2DInstanceType_sprite;
    instance.sprite.tex = gpuImageSamplerDescriptor(texture->view);
    instance.sprite.uvPos = sprite->uv.pos;
    instance.sprite.uvSize = sprite->uv.size;

    *arrayPush(&layer->instances) = instance;

    layer->changed = true;
}

Atlas2D atlasCreate2D(TextureAsset* texture)
{
    HG_ASSERT(texture != nullptr);

    Atlas2D atlas{};
    atlas.texture = texture;
    atlas.sprites = arrayCreate<Rect>();
    return atlas;
}

void atlasDestroy2D(Atlas2D* atlas)
{
    HG_ASSERT(atlas != nullptr);
    arrayDestroy(&atlas->sprites);
}

u32 atlasAdd2D(Atlas2D* atlas, Rect sprite)
{
    HG_ASSERT(atlas != nullptr);

    u32 idx = atlas->sprites.count;
    *arrayPush(&atlas->sprites) = sprite;
    return idx;
}

u32 atlasAddGrid2D(Atlas2D* atlas, Rect grid, u32 width, u32 height)
{
    HG_ASSERT(atlas != nullptr);

    u32 idx = atlas->sprites.count;

    Vec2 spriteSize = grid.size / Vec2{(f32)width, (f32)height};
    Vec2 pos = grid.pos;
    for (u32 y = 0; y < height; ++y)
    {
        pos.x = grid.pos.x;
        for (u32 x = 0; x < width; ++x)
        {
            *arrayPush(&atlas->sprites) = {pos, spriteSize};
            pos.x += spriteSize.x;
        }
        pos.y += spriteSize.y;
    }

    return idx;
}

Sprite2D atlasGet2D(Atlas2D* atlas, u32 idx)
{
    HG_ASSERT(atlas != nullptr);

    return {atlas->texture, atlas->sprites[idx]};
}

Tilemap2D tilemapCreate2D(u32 width, u32 height)
{
    Tilemap2D tilemap{};
    tilemap.tiles = gpaAlloc<u32>(width * height);
    tilemap.width = width;
    tilemap.height = height;
    for (u32 i = 0; i < width * height; ++i)
    {
        tilemap.tiles[i] = (u32)-1;
    }

    return tilemap;
}

void tilemapDestroy2D(Tilemap2D* tilemap)
{
    HG_ASSERT(tilemap != nullptr);
    gpaFree(tilemap->tiles, tilemap->width * tilemap->height);
}

u32 tilemapGet2D(Tilemap2D* tilemap, u32 x, u32 y)
{
    HG_ASSERT(tilemap != nullptr);
    return tilemap->tiles[y * tilemap->width + x];
}

void tilemapSet2D(Tilemap2D* tilemap, u32 x, u32 y, u32 tile)
{
    HG_ASSERT(tilemap != nullptr);
    tilemap->tiles[y * tilemap->width + x] = tile;
}

void drawTilemap2D(Layer2D* layer, Atlas2D* atlas, Tilemap2D* tilemap, Rect dst)
{
    HG_ASSERT(layer != nullptr);
    HG_ASSERT(tilemap != nullptr);

    Vec2 pos = dst.pos;
    Vec2 size = dst.size / Vec2{(f32)tilemap->width, (f32)tilemap->height};
    for (u32 y = 0; y < tilemap->width; ++y)
    {
        pos.x = dst.pos.x;
        for (u32 x = 0; x < tilemap->height; ++x)
        {
            u32 tile = tilemapGet2D(tilemap, x, y);
            Sprite2D sprite = atlasGet2D(atlas, tile);
            drawSprite2D(layer, &sprite, {pos, size});
            pos.x += size.x;
        }
        pos.y += size.y;
    }
}

struct SpritePipelinePush {
    Mat4 model;
    Vec2 uvPos;
    Vec2 uvSize;
    u32 viewProj;
    u32 texture;
};

struct SpritePipelineState {
    GpuPipeline* pipeline;
    Texture defaultTex;
};

static SpritePipelineState spritePipeline{};

#include "sprite.vert.spv.h"
#include "sprite.frag.spv.h"

void spritesInit(
    Format colorFormat,
    Format depthFormat)
{
    HG_ASSERT(colorFormat != Format_undefined);
    HG_ASSERT(depthFormat != Format_undefined);

    CreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = sprite_vert_spv;
    pipelineConfig.vertexShaderSize = sizeof(sprite_vert_spv);
    pipelineConfig.fragmentShader = sprite_frag_spv;
    pipelineConfig.fragmentShaderSize = sizeof(sprite_frag_spv);
    pipelineConfig.pushConstantSize = sizeof(SpritePipelinePush);
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    pipelineConfig.enableDepthRead = true;
    pipelineConfig.enableDepthWrite = true;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    spritePipeline.pipeline = gpuPipelineCreateGraphics(&pipelineConfig);

    struct Color {
        u8 r, g, b, a;
    };
    Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    spritePipeline.defaultTex.image = gpuImageCreate(2, 2, Format_r8g8b8a8_srgb,
        GpuImageUsage_sampled | GpuImageUsage_transferDst);

    spritePipeline.defaultTex.view = gpuViewCreate(
        spritePipeline.defaultTex.image, GpuAspect_color, GpuFilter_nearest);

    gpuImageWrite(spritePipeline.defaultTex.view, defaultColors);
}

void spritesDeinit()
{
    gpuPipelineDestroy(spritePipeline.pipeline);

    gpuViewDestroy(spritePipeline.defaultTex.view);
    gpuImageDestroy(spritePipeline.defaultTex.image);
}

template<>
void serialize(Serializer* s, Sprite* sprite)
{
    serializeObject(s,
        &sprite->texture,
        &sprite->uvPos,
        &sprite->uvSize);
}

Sprite* spriteAdd(Ecs* ecs, Entity e, TextureAsset* texture, Vec2 uvPos, Vec2 uvSize)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));

    Sprite* sprite = ecsAdd<Sprite>(ecs, e);
    *sprite = {};
    sprite->texture = texture;
    sprite->uvPos = uvPos;
    sprite->uvSize = uvSize;

    return sprite;
}

template<>
void ecsDtor(Sprite* sprite)
{
    assetUnload(sprite->texture);
}

void spritesDraw(Ecs* ecs, Entity camera, GpuCmd* cmd)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(cmd != nullptr);

    gpuBindPipeline(cmd, spritePipeline.pipeline);

    ecsForEach<Sprite, Transform>(ecs, [&](Entity, Sprite* sprite, Transform* tf)
    {
        Texture* texture = sprite->texture == nullptr
            ? &spritePipeline.defaultTex
            : &sprite->texture->asset;

        SpritePipelinePush push{};
        push.model = tf->mat;
        push.uvPos = sprite->uvPos;
        push.uvSize = sprite->uvSize;
        push.viewProj = gpuBufferUniformDescriptor(ecsGet<Camera>(ecs, camera)->vpBuffer);
        push.texture = gpuImageSamplerDescriptor(texture->view);

        gpuPushConstants(cmd, spritePipeline.pipeline, &push, sizeof(push));

        gpuDraw(cmd, 0, 6, 0, 1);
    });
}

struct SkyboxPipelinePush {
    u32 viewProj;
    u32 texture;
};

struct SkyboxPipelineState {
    GpuPipeline* pipeline;
    Texture defaultTex;
};

static SkyboxPipelineState skyboxPipeline{};

#include "skybox.vert.spv.h"
#include "skybox.frag.spv.h"

void skyboxInit(Format colorFormat, Format depthFormat)
{
    HG_ASSERT(colorFormat != Format_undefined);

    CreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = skybox_vert_spv;
    pipelineConfig.vertexShaderSize = sizeof(skybox_vert_spv);
    pipelineConfig.fragmentShader = skybox_frag_spv;
    pipelineConfig.fragmentShaderSize = sizeof(skybox_frag_spv);
    pipelineConfig.pushConstantSize = sizeof(SkyboxPipelinePush);
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    skyboxPipeline.pipeline = gpuPipelineCreateGraphics(&pipelineConfig);

    struct Color {
        u8 r, g, b, a;
    };
    Color top = {0x00, 0x22, 0x44, 0xff};
    Color mid = {0x00, 0x11, 0x33, 0xff};
    Color bot = {0x00, 0x00, 0x00, 0xff};
    Color nul = {};
    Color defaultColors[]{
        nul, top, nul, nul,
        mid, mid, mid, mid,
        nul, bot, nul, nul,
    };

    GpuImageCreateEx imageConfig{};
    imageConfig.width = 1;
    imageConfig.height = 1;
    imageConfig.format = Format_r8g8b8a8_srgb;
    imageConfig.arrayLayers = 6;
    imageConfig.usage = GpuImageUsage_sampled | GpuImageUsage_transferDst;
    imageConfig.flags = GpuImageConfig_cubeCompatible;

    skyboxPipeline.defaultTex.image = gpuImageCreateEx(&imageConfig);

    GpuViewCreateEx viewConfig{};
    viewConfig.image = skyboxPipeline.defaultTex.image;
    viewConfig.baseMipLevel = 0;
    viewConfig.levelCount = 1;
    viewConfig.baseArrayLayer = 0;
    viewConfig.layerCount = 6;
    viewConfig.aspectFlags = GpuAspect_color;
    viewConfig.type = GpuViewType_cube;
    viewConfig.filter = GpuFilter_nearest;
    viewConfig.edgeMode = GpuSamplerEdgeMode_repeat;
    viewConfig.border = GpuSamplerBorder_floatTransparentBlack;

    skyboxPipeline.defaultTex.view = gpuViewCreateEx(&viewConfig);

    gpuImageWriteCubemap(skyboxPipeline.defaultTex.view, defaultColors);
}

void skyboxDeinit()
{
    gpuPipelineDestroy(skyboxPipeline.pipeline);

    gpuViewDestroy(skyboxPipeline.defaultTex.view);
    gpuImageDestroy(skyboxPipeline.defaultTex.image);
}

template<>
void serialize(Serializer* s, Skybox* skybox)
{
    serializeObject(s,
        &skybox->texture);
}

Skybox* skyboxAdd(Ecs* ecs, Entity e, TextureAsset* texture)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));

    Skybox* skybox = ecsAdd<Skybox>(ecs, e);
    *skybox = {texture};

    return skybox;
}

template<>
void ecsDtor(Skybox* skybox)
{
    assetUnload(skybox->texture);
}

void skyboxDraw(Ecs* ecs, Entity camera, GpuCmd* cmd)
{
    gpuBindPipeline(cmd, skyboxPipeline.pipeline);

    ecsForEach<Skybox>(ecs, [&](Entity, Skybox* skybox)
    {
        Texture* texture = skybox->texture == nullptr
            ? &skyboxPipeline.defaultTex
            : &skybox->texture->asset;

        SkyboxPipelinePush push{};
        push.viewProj = gpuBufferUniformDescriptor(ecsGet<Camera>(ecs, camera)->vpBuffer);
        push.texture = gpuImageSamplerDescriptor(texture->view);

        gpuPushConstants(cmd, skyboxPipeline.pipeline, &push, sizeof(push));

        gpuDraw(cmd, 0, 36, 0, 1);
    });
}

template<>
void serialize(Serializer* s, DirLight* light)
{
    serializeObject(s,
        &light->dir,
        &light->color);
}

DirLight* dirLightAdd(Ecs* ecs, Entity e, Vec3 dir, Vec4 color)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));

    DirLight* light = ecsAdd<DirLight>(ecs, e);
    *light = {dir, color};

    return light;
}

template<>
void serialize(Serializer* s, PointLight* light)
{
    serializeObject(s,
        &light->color);
}

PointLight* pointLightAdd(Ecs* ecs, Entity e, Vec4 color)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));

    PointLight* light = ecsAdd<PointLight>(ecs, e);
    *light = {color};

    return light;
}

struct ModelPipelineDirLightData {
    Vec4 dir;
    Vec4 color;
};

struct ModelPipelinePointLightData {
    Vec4 pos;
    Vec4 color;
};

struct ModelPipelinePush {
    Mat4 model;
    u32 indices;
    u32 vertices;
    u32 viewProj;
    u32 normalMap;
    u32 colorMap;
    u32 dirLights;
    u32 dirLightCount;
    u32 pointLights;
    u32 pointLightCount;
};

struct ModelPipelineState {
    GpuPipeline* pipeline;

    GpuBuffer* dirLightBuffer;
    u32 dirLightCapacity;

    GpuBuffer* pointLightBuffer;
    u32 pointLightCapacity;

    Mesh defaultModel;
    Texture defaultColorMap;
    Texture defaultNormalMap;
};

static ModelPipelineState modelPipeline{};

#include "model.vert.spv.h"
#include "model.frag.spv.h"

void modelsInit(
    Format colorFormat,
    Format depthFormat)
{
    HG_ASSERT(colorFormat != Format_undefined);
    HG_ASSERT(depthFormat != Format_undefined);

    CreateGpuGraphicsPipeline pipelineConfig{};
    pipelineConfig.vertexShader = model_vert_spv;
    pipelineConfig.vertexShaderSize = sizeof(model_vert_spv);
    pipelineConfig.fragmentShader = model_frag_spv;
    pipelineConfig.fragmentShaderSize = sizeof(model_frag_spv);
    pipelineConfig.pushConstantSize = sizeof(ModelPipelinePush);
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    pipelineConfig.cullMode = GpuCull_back;
    pipelineConfig.enableDepthRead = true;
    pipelineConfig.enableDepthWrite = true;

    modelPipeline.pipeline = gpuPipelineCreateGraphics(&pipelineConfig);

    modelPipeline.dirLightCapacity = 16;
    modelPipeline.dirLightBuffer = gpuBufferCreate(
        sizeof(ModelPipelineDirLightData) * modelPipeline.dirLightCapacity,
        GpuBufferUsage_storageBuffer,
        GpuMemoryUsage_frequentUpdate);

    modelPipeline.pointLightCapacity = 64;
    modelPipeline.pointLightBuffer = gpuBufferCreate(
        sizeof(ModelPipelinePointLightData) * modelPipeline.dirLightCapacity,
        GpuBufferUsage_storageBuffer,
        GpuMemoryUsage_frequentUpdate);

    MeshVertex cubeVertices[]{
        {{ 0.5f,-0.5f,-0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {0,0}},
        {{ 0.5f, 0.5f,-0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {1,0}},
        {{ 0.5f, 0.5f, 0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {1,1}},
        {{ 0.5f,-0.5f, 0.5f}, { 1, 0, 0}, { 0, 0, 1, 1}, {0,1}},

        {{-0.5f,-0.5f, 0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {0,0}},
        {{-0.5f, 0.5f, 0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {1,0}},
        {{-0.5f, 0.5f,-0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {1,1}},
        {{-0.5f,-0.5f,-0.5f}, {-1, 0, 0}, { 0, 0,-1, 1}, {0,1}},

        {{-0.5f, 0.5f,-0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {0,0}},
        {{-0.5f, 0.5f, 0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {1,0}},
        {{ 0.5f, 0.5f, 0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {1,1}},
        {{ 0.5f, 0.5f,-0.5f}, { 0, 1, 0}, { 1, 0, 0, 1}, {0,1}},

        {{-0.5f,-0.5f, 0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {0,0}},
        {{-0.5f,-0.5f,-0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {1,0}},
        {{ 0.5f,-0.5f,-0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {1,1}},
        {{ 0.5f,-0.5f, 0.5f}, { 0,-1, 0}, { 1, 0, 0, 1}, {0,1}},

        {{-0.5f,-0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {0,0}},
        {{ 0.5f,-0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {1,0}},
        {{ 0.5f, 0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {1,1}},
        {{-0.5f, 0.5f, 0.5f}, { 0, 0, 1}, { 1, 0, 0, 1}, {0,1}},

        {{ 0.5f,-0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {0,0}},
        {{-0.5f,-0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {1,0}},
        {{-0.5f, 0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {1,1}},
        {{ 0.5f, 0.5f,-0.5f}, { 0, 0,-1}, {-1, 0, 0, 1}, {0,1}},
    };

    u32 cubeIndices[]{
         0,  1,  2,  0,  2,  3,
         4,  5,  6,  4,  6,  7,
         8,  9, 10,  8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };

    modelPipeline.defaultModel.vertexBuffer = gpuBufferCreate(
        sizeof(cubeVertices),
        GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);

    modelPipeline.defaultModel.indexBuffer = gpuBufferCreate(
        sizeof(cubeIndices),
        GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);

    gpuBufferWrite(modelPipeline.defaultModel.vertexBuffer, 0, cubeVertices, sizeof(cubeVertices));
    gpuBufferWrite(modelPipeline.defaultModel.indexBuffer, 0, cubeIndices, sizeof(cubeIndices));

    modelPipeline.defaultModel.vertexCount = (u32)arrayCount(cubeVertices);
    modelPipeline.defaultModel.vertexWidth = (u32)sizeof(MeshVertex);
    modelPipeline.defaultModel.indexCount = (u32)arrayCount(cubeIndices);

    struct Color {
        u8 r, g, b, a;
    };
    Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    Vec4 defaultNormal{0, 0, -1, 0};

    modelPipeline.defaultColorMap.image = gpuImageCreate(3, 3, Format_r8g8b8a8_srgb,
        GpuImageUsage_sampled | GpuImageUsage_transferDst);
    modelPipeline.defaultNormalMap.image = gpuImageCreate(1, 1, Format_r32g32b32a32_sfloat,
        GpuImageUsage_sampled | GpuImageUsage_transferDst);

    modelPipeline.defaultColorMap.view = gpuViewCreate(
        modelPipeline.defaultColorMap.image, GpuAspect_color, GpuFilter_nearest);
    modelPipeline.defaultNormalMap.view = gpuViewCreate(
        modelPipeline.defaultNormalMap.image, GpuAspect_color, GpuFilter_nearest);

    gpuImageWrite(modelPipeline.defaultColorMap.view, defaultColors);
    gpuImageWrite(modelPipeline.defaultNormalMap.view, &defaultNormal);
}

void modelsDeinit()
{
    gpuPipelineDestroy(modelPipeline.pipeline);

    gpuViewDestroy(modelPipeline.defaultNormalMap.view);
    gpuImageDestroy(modelPipeline.defaultNormalMap.image);

    gpuViewDestroy(modelPipeline.defaultColorMap.view);
    gpuImageDestroy(modelPipeline.defaultColorMap.image);

    gpuBufferDestroy(modelPipeline.defaultModel.indexBuffer);
    gpuBufferDestroy(modelPipeline.defaultModel.vertexBuffer);

    gpuBufferDestroy(modelPipeline.pointLightBuffer);
    gpuBufferDestroy(modelPipeline.dirLightBuffer);
}

template<>
void serialize(Serializer* s, Model* model)
{
    serializeObject(s,
        &model->mesh,
        &model->colorMap,
        &model->normalMap);
}

Model* modelAdd(
    Ecs* ecs,
    Entity e,
    MeshAsset* mesh,
    TextureAsset* colorMap,
    TextureAsset* normalMap)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(ecsAlive(ecs, e));

    Model* model = ecsAdd<Model>(ecs, e);
    *model = {};
    model->mesh = mesh;
    model->colorMap = colorMap;
    model->normalMap = normalMap;

    return model;
}

template<>
void ecsDtor(Model* model)
{
    assetUnload(model->normalMap);
    assetUnload(model->colorMap);
    assetUnload(model->mesh);
}

void modelsDraw(Ecs* ecs, Entity camera, GpuCmd* cmd)
{
    HG_ASSERT(ecs != nullptr);
    HG_ASSERT(cmd != nullptr);

    Arena* sc = scratch();
    HG_ARENA_SCOPE(sc);

    Camera* cameraC = ecsGet<Camera>(ecs, camera);
    Transform* cameraTf = ecsGet<Transform>(ecs, camera);

    Mat4 view = matModelToView(cameraTf->mat);

    u32 dirLightCount = ecsCount<DirLight>(ecs);
    u32 pointLightCount = ecsCount<PointLight>(ecs);

    if (dirLightCount > modelPipeline.dirLightCapacity)
    {
        if (modelPipeline.dirLightCapacity == 0)
            modelPipeline.dirLightCapacity = 1;
        while (modelPipeline.dirLightCapacity < dirLightCount)
        {
            modelPipeline.dirLightCapacity *= 2;
        }

        gpuWaitIdle();

        gpuBufferDestroy(modelPipeline.dirLightBuffer);
        modelPipeline.dirLightBuffer = gpuBufferCreate(
            sizeof(ModelPipelineDirLightData) * modelPipeline.dirLightCapacity,
            GpuBufferUsage_storageBuffer,
            GpuMemoryUsage_frequentUpdate);
    }

    if (pointLightCount > modelPipeline.pointLightCapacity)
    {
        if (modelPipeline.pointLightCapacity == 0)
            modelPipeline.pointLightCapacity = 1;
        while (modelPipeline.pointLightCapacity < pointLightCount)
        {
            modelPipeline.pointLightCapacity *= 2;
        }

        gpuWaitIdle();

        gpuBufferDestroy(modelPipeline.pointLightBuffer);
        modelPipeline.pointLightBuffer = gpuBufferCreate(
            sizeof(ModelPipelinePointLightData) * modelPipeline.pointLightCapacity,
            GpuBufferUsage_storageBuffer,
            GpuMemoryUsage_frequentUpdate);
    }

    ModelPipelineDirLightData* dirLights = arenaAlloc<ModelPipelineDirLightData>(sc, dirLightCount);
    ModelPipelinePointLightData* pointLights = arenaAlloc<ModelPipelinePointLightData>(sc, pointLightCount);

    u32 i = 0;
    ecsForEach<DirLight>(ecs, [&](Entity, DirLight* light)
    {
        dirLights[i].dir = Vec4{Mat3{view} * light->dir, 0.0};
        dirLights[i].color = light->color;
        ++i;
    });

    i = 0;
    ecsForEach<PointLight, Transform>(ecs, [&](Entity, PointLight* light, Transform* tf)
    {
        pointLights[i].pos = view * Vec4{transformWorldPos(*tf), 1.0};
        pointLights[i].color = light->color;
        ++i;
    });

    if (dirLightCount > 0)
        gpuBufferWrite(modelPipeline.dirLightBuffer, 0, dirLights, sizeof(*dirLights) * dirLightCount);

    if (pointLightCount > 0)
        gpuBufferWrite(modelPipeline.pointLightBuffer, 0, pointLights, sizeof(*pointLights) * pointLightCount);

    gpuBindPipeline(cmd, modelPipeline.pipeline);

    ecsForEach<Model, Transform>(ecs, [&](Entity, Model* model, Transform* tf)
    {
        Texture* colorMap = model->colorMap == nullptr
            ? &modelPipeline.defaultColorMap
            : &model->colorMap->asset;

        Texture* normalMap = model->normalMap == nullptr
            ? &modelPipeline.defaultNormalMap
            : &model->normalMap->asset;

        Mesh* gpuModel = model->mesh == nullptr
            ? &modelPipeline.defaultModel
            : &model->mesh->asset;

        ModelPipelinePush push{};
        push.model = tf->mat;
        push.vertices = gpuBufferStorageDescriptor(gpuModel->vertexBuffer);
        push.indices = gpuBufferStorageDescriptor(gpuModel->indexBuffer);
        push.viewProj = gpuBufferUniformDescriptor(cameraC->vpBuffer);
        push.normalMap = gpuImageSamplerDescriptor(normalMap->view);
        push.colorMap = gpuImageSamplerDescriptor(colorMap->view);
        push.dirLights = gpuBufferStorageDescriptor(modelPipeline.dirLightBuffer);
        push.dirLightCount = dirLightCount;
        push.pointLights = gpuBufferStorageDescriptor(modelPipeline.pointLightBuffer);
        push.pointLightCount = pointLightCount;

        gpuPushConstants(cmd, modelPipeline.pipeline, &push, sizeof(push));

        gpuDraw(cmd, 0, gpuModel->indexCount, 0, 1);
    });
}

} // namespace hg

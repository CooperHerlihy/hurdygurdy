#include "hurdygurdy.hpp"

#include "model.vert.spv.h"
#include "model.frag.spv.h"

struct GlobalUniform {
    HgMat4 proj;
    HgMat4 view;
    u32 dirLightCount;
    u32 pointLightCount;
};

struct Push {
    HgMat4 model;
};

struct DirLightData {
    HgVec4 dir;
    HgVec4 color;
};

struct PointLightData {
    HgVec4 pos;
    HgVec4 color;
};

static VkDescriptorSetLayout globalSetLayout;
static VkDescriptorSetLayout textureSetLayout;
static VkPipelineLayout pipelineLayout;
static VkPipeline pipeline;

static HgBuffer* globalBuffer;
static GlobalUniform globalData;
static HgBuffer* dirLightBuffer;
static u32 dirLightCapacity;
static HgBuffer* pointLightBuffer;
static u32 pointLightCapacity;
static VkDescriptorSet globalSet;

static HgModelResource defaultModel;
static HgTextureResource defaultColorMap;
static HgTextureResource defaultNormalMap;
static VkSampler defaultMapSampler;
static VkDescriptorSet defaultModelSet;

static HgHashMap<HgResource, VkDescriptorSet> modelSets;

void hgInitPipeline3D(
    HgArena* arena,
    u32 maxModels,
    VkFormat colorFormat,
    VkFormat depthFormat)
{
    hgAssert(colorFormat != VK_FORMAT_UNDEFINED);
    hgAssert(depthFormat != VK_FORMAT_UNDEFINED);

    VkDescriptorSetLayoutBinding globalSetBindings[] = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
        {2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    };
    globalSetLayout = hgCreateVkDescriptorSetLayout(
        globalSetBindings, sizeof(globalSetBindings) / sizeof(*globalSetBindings));

    VkDescriptorSetLayoutBinding textureSetBindings[] = {
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr},
    };
    textureSetLayout = hgCreateVkDescriptorSetLayout(
        textureSetBindings, sizeof(textureSetBindings) / sizeof(*textureSetBindings));

    VkDescriptorSetLayout setLayouts[] = {globalSetLayout, textureSetLayout};
    VkPushConstantRange pushRange = {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push)};
    pipelineLayout = hgCreateVkPipelineLayout(
        setLayouts, sizeof(setLayouts) / sizeof(*setLayouts),
        &pushRange, 1);

    VkVertexInputBindingDescription vertexBindings[] = {
        {0, sizeof(HgModelVertex), VK_VERTEX_INPUT_RATE_VERTEX},
    };
    VkVertexInputAttributeDescription vertexAttributes[] = {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(HgModelVertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(HgModelVertex, norm)},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(HgModelVertex, tan)},
        {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(HgModelVertex, uv)},
    };
    HgCreateVkGraphicsPipeline pipelineConfig{};
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    pipelineConfig.vertexShader = model_vert_spv;
    pipelineConfig.vertexShaderSize = model_vert_spv_size;
    pipelineConfig.fragmentShader = model_frag_spv;
    pipelineConfig.fragmentShaderSize = model_frag_spv_size;
    pipelineConfig.layout = pipelineLayout;
    pipelineConfig.vertexBindings = vertexBindings;
    pipelineConfig.vertexBindingCount = sizeof(vertexBindings) / sizeof(*vertexBindings);
    pipelineConfig.vertexAttributes = vertexAttributes;
    pipelineConfig.vertexAttributeCount = sizeof(vertexAttributes) / sizeof(*vertexAttributes);
    pipelineConfig.cullMode = VK_CULL_MODE_BACK_BIT;

    pipeline = hgCreateVkGraphicsPipeline(pipelineConfig);

    globalSet = hgCreateVkDescriptorSet(globalSetLayout);
    hgAssert(globalSet != nullptr);

    globalData.proj = HgMat4{1.0f};
    globalData.view = HgMat4{1.0f};
    globalData.dirLightCount = 0;
    globalData.pointLightCount = 0;

    globalBuffer = hgCreateBuffer(
        sizeof(GlobalUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        HgBufferMemoryUsage_frequentUpdate);

    hgWriteBuffer(globalBuffer, 0, &globalData, sizeof(globalData));

    VkDescriptorBufferInfo bufferInfo = {globalBuffer->buffer, 0, sizeof(GlobalUniform)};
    hgUpdateVkDescriptorSetBinding(
        globalSet,
        0,
        0,
        1,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        &bufferInfo,
        nullptr);

    dirLightCapacity = 4;
    dirLightBuffer = hgCreateBuffer(
        sizeof(DirLightData) * dirLightCapacity,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        HgBufferMemoryUsage_frequentUpdate);

    VkDescriptorBufferInfo dirLightBufferInfo = {dirLightBuffer->buffer, 0, VK_WHOLE_SIZE};
    hgUpdateVkDescriptorSetBinding(
        globalSet,
        1,
        0,
        1,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        &dirLightBufferInfo,
        nullptr);

    pointLightCapacity = 4;
    pointLightBuffer = hgCreateBuffer(
        sizeof(PointLightData) * dirLightCapacity,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        HgBufferMemoryUsage_frequentUpdate);

    VkDescriptorBufferInfo pointLightBufferInfo = {pointLightBuffer->buffer, 0, VK_WHOLE_SIZE};
    hgUpdateVkDescriptorSetBinding(
        globalSet,
        2,
        0,
        1,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        &pointLightBufferInfo,
        nullptr);

    static const HgModelVertex cubeVertices[] = {
        {HgVec3{ 0.5f,-0.5f,-0.5f}, HgVec3{ 1, 0, 0}, HgVec4{ 0, 0, 1, 1}, HgVec2{0,0}},
        {HgVec3{ 0.5f, 0.5f,-0.5f}, HgVec3{ 1, 0, 0}, HgVec4{ 0, 0, 1, 1}, HgVec2{1,0}},
        {HgVec3{ 0.5f, 0.5f, 0.5f}, HgVec3{ 1, 0, 0}, HgVec4{ 0, 0, 1, 1}, HgVec2{1,1}},
        {HgVec3{ 0.5f,-0.5f, 0.5f}, HgVec3{ 1, 0, 0}, HgVec4{ 0, 0, 1, 1}, HgVec2{0,1}},

        {HgVec3{-0.5f,-0.5f, 0.5f}, HgVec3{-1, 0, 0}, HgVec4{ 0, 0,-1, 1}, HgVec2{0,0}},
        {HgVec3{-0.5f, 0.5f, 0.5f}, HgVec3{-1, 0, 0}, HgVec4{ 0, 0,-1, 1}, HgVec2{1,0}},
        {HgVec3{-0.5f, 0.5f,-0.5f}, HgVec3{-1, 0, 0}, HgVec4{ 0, 0,-1, 1}, HgVec2{1,1}},
        {HgVec3{-0.5f,-0.5f,-0.5f}, HgVec3{-1, 0, 0}, HgVec4{ 0, 0,-1, 1}, HgVec2{0,1}},

        {HgVec3{-0.5f, 0.5f,-0.5f}, HgVec3{ 0, 1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,0}},
        {HgVec3{-0.5f, 0.5f, 0.5f}, HgVec3{ 0, 1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,0}},
        {HgVec3{ 0.5f, 0.5f, 0.5f}, HgVec3{ 0, 1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,1}},
        {HgVec3{ 0.5f, 0.5f,-0.5f}, HgVec3{ 0, 1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,1}},

        {HgVec3{-0.5f,-0.5f, 0.5f}, HgVec3{ 0,-1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,0}},
        {HgVec3{-0.5f,-0.5f,-0.5f}, HgVec3{ 0,-1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,0}},
        {HgVec3{ 0.5f,-0.5f,-0.5f}, HgVec3{ 0,-1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,1}},
        {HgVec3{ 0.5f,-0.5f, 0.5f}, HgVec3{ 0,-1, 0}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,1}},

        {HgVec3{-0.5f,-0.5f, 0.5f}, HgVec3{ 0, 0, 1}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,0}},
        {HgVec3{ 0.5f,-0.5f, 0.5f}, HgVec3{ 0, 0, 1}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,0}},
        {HgVec3{ 0.5f, 0.5f, 0.5f}, HgVec3{ 0, 0, 1}, HgVec4{ 1, 0, 0, 1}, HgVec2{1,1}},
        {HgVec3{-0.5f, 0.5f, 0.5f}, HgVec3{ 0, 0, 1}, HgVec4{ 1, 0, 0, 1}, HgVec2{0,1}},

        {HgVec3{ 0.5f,-0.5f,-0.5f}, HgVec3{ 0, 0,-1}, HgVec4{-1, 0, 0, 1}, HgVec2{0,0}},
        {HgVec3{-0.5f,-0.5f,-0.5f}, HgVec3{ 0, 0,-1}, HgVec4{-1, 0, 0, 1}, HgVec2{1,0}},
        {HgVec3{-0.5f, 0.5f,-0.5f}, HgVec3{ 0, 0,-1}, HgVec4{-1, 0, 0, 1}, HgVec2{1,1}},
        {HgVec3{ 0.5f, 0.5f,-0.5f}, HgVec3{ 0, 0,-1}, HgVec4{-1, 0, 0, 1}, HgVec2{0,1}},
    };

    static u32 cubeIndices[] = {
         0,  1,  2,  0,  2,  3,
         4,  5,  6,  4,  6,  7,
         8,  9, 10,  8, 10, 11,
        12, 13, 14, 12, 14, 15,
        16, 17, 18, 16, 18, 19,
        20, 21, 22, 20, 22, 23
    };

    defaultModel.vertexBuffer = hgCreateBuffer(
        sizeof(cubeVertices),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    defaultModel.indexBuffer = hgCreateBuffer(
        sizeof(cubeIndices),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    hgWriteBuffer(defaultModel.vertexBuffer, 0, cubeVertices, sizeof(cubeVertices));
    hgWriteBuffer(defaultModel.indexBuffer, 0, cubeIndices, sizeof(cubeIndices));

    defaultModel.vertexCount = sizeof(cubeVertices) / sizeof(*cubeVertices);
    defaultModel.vertexWidth = sizeof(HgModelVertex);
    defaultModel.indexCount = sizeof(cubeIndices) / sizeof(*cubeIndices);

    struct Color
    {
        u8 r, g, b, a;
    };
    static const Color defaultColors[] = {
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    static const HgVec4 defaultNormals[] = {
        HgVec4{0, 0, -1, 0}, HgVec4{0, 0, -1, 0},
        HgVec4{0, 0, -1, 0}, HgVec4{0, 0, -1, 0},
    };

    defaultColorMap.image = hgCreateImage(3, 3, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
    defaultNormalMap.image = hgCreateImage(2, 2, VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    defaultColorMap.view = hgCreateImageView(defaultColorMap.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});
    defaultNormalMap.view = hgCreateImageView(defaultNormalMap.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    hgWriteImage(
        defaultColorMap.image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        defaultColors,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    hgWriteImage(
        defaultNormalMap.image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        defaultNormals,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    HgCreateVkSampler samplerInfo{};
    defaultMapSampler = hgCreateVkSampler(samplerInfo);

    defaultColorMap.sampler = defaultMapSampler;
    defaultNormalMap.sampler = defaultMapSampler;

    defaultModelSet = hgCreateVkDescriptorSet(textureSetLayout);

    VkDescriptorImageInfo imageInfos[2]{
        {defaultMapSampler, defaultColorMap.view->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
        {defaultMapSampler, defaultNormalMap.view->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
    };
    hgUpdateVkDescriptorSetBinding(
        defaultModelSet,
        0,
        0,
        2,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        nullptr,
        imageInfos);

    modelSets = modelSets.create(arena, maxModels);
}

void hgDeinitPipeline3D()
{
    modelSets.forEach([](HgResource, VkDescriptorSet set)
    {
        hgDestroyVkDescriptorSet(set);
    });
    hgDestroyVkDescriptorSet(defaultModelSet);
    hgDestroyBuffer(defaultModel.indexBuffer);
    hgDestroyBuffer(defaultModel.vertexBuffer);
    vkDestroySampler(hgVkDevice, defaultMapSampler, nullptr);
    hgDestroyImageView(defaultNormalMap.view);
    hgDestroyImage(defaultNormalMap.image);
    hgDestroyImageView(defaultColorMap.view);
    hgDestroyImage(defaultColorMap.image);
    hgDestroyVkDescriptorSet(globalSet);
    hgDestroyBuffer(pointLightBuffer);
    hgDestroyBuffer(dirLightBuffer);
    hgDestroyBuffer(globalBuffer);
    vkDestroyPipeline(hgVkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(hgVkDevice, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(hgVkDevice, textureSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(hgVkDevice, globalSetLayout, nullptr);
}

void hgAddModel3D(HgResource modelID, HgResource colorID, HgResource normalID)
{
    HgTextureResource* colorMap = hgGetTexture(colorID);
    if (colorMap == nullptr)
        colorMap = &defaultColorMap;

    HgTextureResource* normalMap = hgGetTexture(normalID);
    if (normalMap == nullptr)
        normalMap = &defaultNormalMap;

    VkDescriptorSet set = hgCreateVkDescriptorSet(textureSetLayout);

    VkDescriptorImageInfo imageInfos[2]{};
    imageInfos[0].sampler = colorMap->sampler;
    imageInfos[0].imageView = colorMap->view->view;
    imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfos[1].sampler = normalMap->sampler;
    imageInfos[1].imageView = normalMap->view->view;
    imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hgUpdateVkDescriptorSetBinding(
        set,
        0,
        0,
        2,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        nullptr,
        imageInfos);

    modelSets.add(modelID) = set;
}

void hgRemoveModel3D(HgResource modelID)
{
    VkDescriptorSet set;
    if (modelSets.remove(modelID, &set))
        hgDestroyVkDescriptorSet(set);
}

void hgUpdateProjection3D(const HgMat4& projection)
{
    globalData.proj = projection;
}

void hgUpdateView3D(const HgMat4& view)
{
    globalData.view = view;
}

void hgDraw3D(HgECS* ecs, VkCommandBuffer cmd)
{
    hgAssert(cmd != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    globalData.dirLightCount = ecs->count<HgDirLight3D>();
    globalData.pointLightCount = ecs->count<HgPointLight3D>();

    if (globalData.dirLightCount > dirLightCapacity)
    {
        vkQueueWaitIdle(hgVkQueue);

        u32 newCapacity = dirLightCapacity == 0 ? 1 : dirLightCapacity * 2;
        while (newCapacity < dirLightCapacity)
        {
            newCapacity *= 2;
        }

        hgDestroyBuffer(dirLightBuffer);
        dirLightBuffer = hgCreateBuffer(
            sizeof(DirLightData) * newCapacity,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            HgBufferMemoryUsage_frequentUpdate);
        dirLightCapacity = newCapacity;

        VkDescriptorBufferInfo dirLightBufferInfo = {dirLightBuffer->buffer, 0, VK_WHOLE_SIZE};
        hgUpdateVkDescriptorSetBinding(
            globalSet,
            1,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            &dirLightBufferInfo,
            nullptr);
    }

    if (globalData.pointLightCount > pointLightCapacity)
    {
        vkQueueWaitIdle(hgVkQueue);

        u32 newCapacity = pointLightCapacity == 0 ? 1 : pointLightCapacity * 2;
        while (newCapacity < dirLightCapacity)
        {
            newCapacity *= 2;
        }

        hgDestroyBuffer(pointLightBuffer);
        pointLightBuffer = hgCreateBuffer(
            sizeof(PointLightData) * newCapacity,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            HgBufferMemoryUsage_frequentUpdate);
        pointLightCapacity = newCapacity;

        VkDescriptorBufferInfo pointLightBufferInfo = {pointLightBuffer->buffer, 0, VK_WHOLE_SIZE};
        hgUpdateVkDescriptorSetBinding(
            globalSet,
            1,
            0,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            &pointLightBufferInfo,
            nullptr);
    }

    DirLightData* dirLights = hgAlloc<DirLightData>(scratch, globalData.dirLightCount);
    PointLightData* pointLights = hgAlloc<PointLightData>(scratch, globalData.pointLightCount);

    u32 i = 0;
    ecs->forEach<HgDirLight3D>([&](HgEntity, HgDirLight3D& light)
    {
        dirLights[i].dir = HgVec4{HgMat3{globalData.view} * light.dir, 0.0};
        dirLights[i].color = light.color;
        ++i;
    });

    i = 0;
    ecs->forEach<HgPointLight3D, HgTransform>([&](HgEntity, HgPointLight3D& light, HgTransform& transform)
    {
        pointLights[i].pos = globalData.view * HgVec4{transform.position, 1.0};
        pointLights[i].color = light.color;
        ++i;
    });

    if (globalData.dirLightCount > 0)
        hgWriteBuffer(dirLightBuffer, 0, dirLights, sizeof(*dirLights) * globalData.dirLightCount);

    if (globalData.pointLightCount > 0)
        hgWriteBuffer(pointLightBuffer, 0, pointLights, sizeof(*pointLights) * globalData.pointLightCount);

    hgWriteBuffer(globalBuffer, 0, &globalData, sizeof(globalData));

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &globalSet, 0, nullptr);

    ecs->forEach<HgModel3D, HgTransform>([&](HgEntity, HgModel3D& model, HgTransform& transform)
    {
        VkDescriptorSet modelSet = modelSets[model.modelResource];
        if (modelSet == nullptr)
            modelSet = defaultModelSet;

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &modelSet, 0, nullptr);

        Push push{};
        push.model = hgModelMatrix3D(transform.position, transform.scale, transform.rotation);

        HgModelResource* gpuModel = hgGetModel(model.modelResource);
        if (gpuModel == nullptr)
            gpuModel = &defaultModel;

        vkCmdBindIndexBuffer(cmd, gpuModel->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);

        VkBuffer buffers[] = {gpuModel->vertexBuffer->buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);

        vkCmdDrawIndexed(cmd, gpuModel->indexCount, 1, 0, 0, 0);
    });
}


#include "hurdygurdy.hpp"

struct VPUniform {
    HgMat4 proj;
    HgMat4 view;
};

struct DirLightData {
    HgVec4 dir;
    HgVec4 color;
};

struct PointLightData {
    HgVec4 pos;
    HgVec4 color;
};

struct Push {
    HgMat4 model;
    u32 vpIdx;
    u32 dirLightIdx;
    u32 dirLightCount;
    u32 pointLightIdx;
    u32 pointLightCount;
    u32 colorMapIdx;
    u32 normalMapIdx;
};

static HgBuffer* vpBuffer;
static VPUniform vpData;
static HgDescriptor vpDesc;

static HgBuffer* dirLightBuffer;
static u32 dirLightCapacity;
static HgDescriptor dirLightDesc;

static HgBuffer* pointLightBuffer;
static u32 pointLightCapacity;
static HgDescriptor pointLightDesc;

static HgModelResource defaultModel;
static VkSampler defaultMapSampler;
static HgTextureResource defaultColorMap;
static HgTextureResource defaultNormalMap;

static VkPipelineLayout pipelineLayout;
static VkPipeline pipeline;

void hgInitPipeline3D(
    VkFormat colorFormat,
    VkFormat depthFormat)
{
    hgAssert(colorFormat != VK_FORMAT_UNDEFINED);
    hgAssert(depthFormat != VK_FORMAT_UNDEFINED);

    HgFence fence;
    const char* modelVertSpvName = "build/model.vert.spv";
    const char* modelFragSpvName = "build/model.frag.spv";
    HgResource modelVertSpvID = hgResourceID(modelVertSpvName);
    HgResource modelFragSpvID = hgResourceID(modelFragSpvName);
    hgLoadResource(&fence, 1, modelVertSpvID, modelVertSpvName);
    hgLoadResource(&fence, 1, modelFragSpvID, modelFragSpvName);
    hgDefer(hgUnloadResource(nullptr, 0, modelVertSpvID));
    hgDefer(hgUnloadResource(nullptr, 0, modelFragSpvID));

    HgBinary* modelVertSpv = hgGetResource(modelVertSpvID);
    HgBinary* modelFragSpv = hgGetResource(modelFragSpvID);

    vpData.proj = HgMat4{1.0f};
    vpData.view = HgMat4{1.0f};

    vpBuffer = hgCreateBuffer(
        sizeof(VPUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        HgBufferMemoryUsage_frequentUpdate);

    hgWriteBuffer(vpBuffer, 0, &vpData, sizeof(vpData));

    vpDesc = hgCreateDescriptor(HgDescriptorType_uniformBuffer);

    VkDescriptorBufferInfo bufferInfo{vpBuffer->buffer, 0, sizeof(VPUniform)};
    hgUpdateDescriptor(vpDesc, &bufferInfo, nullptr);

    dirLightCapacity = 4;
    dirLightBuffer = hgCreateBuffer(
        sizeof(DirLightData) * dirLightCapacity,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        HgBufferMemoryUsage_frequentUpdate);

    pointLightCapacity = 4;
    pointLightBuffer = hgCreateBuffer(
        sizeof(PointLightData) * dirLightCapacity,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        HgBufferMemoryUsage_frequentUpdate);

    dirLightDesc = hgCreateDescriptor(HgDescriptorType_storageBuffer);
    pointLightDesc = hgCreateDescriptor(HgDescriptorType_storageBuffer);

    VkDescriptorBufferInfo dirLightBufferInfo{dirLightBuffer->buffer, 0, VK_WHOLE_SIZE};
    hgUpdateDescriptor(dirLightDesc, &dirLightBufferInfo, nullptr);

    VkDescriptorBufferInfo pointLightBufferInfo{pointLightBuffer->buffer, 0, VK_WHOLE_SIZE};
    hgUpdateDescriptor(pointLightDesc, &pointLightBufferInfo, nullptr);

    static const HgModelVertex cubeVertices[]{
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

    static u32 cubeIndices[]{
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

    defaultMapSampler = hgCreateSampler(VK_FILTER_NEAREST);

    defaultColorMap.sampler = defaultMapSampler;
    defaultNormalMap.sampler = defaultMapSampler;

    struct Color
    {
        u8 r, g, b, a;
    };
    static const Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    static const HgVec4 defaultNormals[]{
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

    defaultColorMap.descriptor = hgCreateDescriptor(HgDescriptorType_combinedImageSampler);
    defaultNormalMap.descriptor = hgCreateDescriptor(HgDescriptorType_combinedImageSampler);

    VkDescriptorImageInfo colorInfo =
        {defaultMapSampler, defaultColorMap.view->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    hgUpdateDescriptor(defaultColorMap.descriptor, nullptr, &colorInfo);

    VkDescriptorImageInfo normalInfo =
        {defaultMapSampler, defaultNormalMap.view->view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
    hgUpdateDescriptor(defaultNormalMap.descriptor, nullptr, &normalInfo);

    VkPushConstantRange push{VK_SHADER_STAGE_ALL, 0, sizeof(Push)};
    pipelineLayout = hgCreatePipelineLayout(&push);

    VkVertexInputBindingDescription vertexBindings[]{
        {0, sizeof(HgModelVertex), VK_VERTEX_INPUT_RATE_VERTEX},
    };
    VkVertexInputAttributeDescription vertexAttributes[]{
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(HgModelVertex, pos)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(HgModelVertex, norm)},
        {2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(HgModelVertex, tan)},
        {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(HgModelVertex, uv)},
    };
    HgCreateGraphicsPipeline pipelineConfig{};
    pipelineConfig.layout = pipelineLayout;
    pipelineConfig.vertexShader = (u8*)modelVertSpv->data;
    pipelineConfig.vertexShaderSize = modelVertSpv->size;
    pipelineConfig.fragmentShader = (u8*)modelFragSpv->data;
    pipelineConfig.fragmentShaderSize = modelFragSpv->size;
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    pipelineConfig.vertexBindings = vertexBindings;
    pipelineConfig.vertexBindingCount = sizeof(vertexBindings) / sizeof(*vertexBindings);
    pipelineConfig.vertexAttributes = vertexAttributes;
    pipelineConfig.vertexAttributeCount = sizeof(vertexAttributes) / sizeof(*vertexAttributes);
    pipelineConfig.cullMode = VK_CULL_MODE_BACK_BIT;
    pipelineConfig.enableDepthRead = true;
    pipelineConfig.enableDepthWrite = true;

    hgWaitForFenceIndefinite(&fence);
    pipeline = hgCreateGraphicsPipeline(&pipelineConfig);
}

void hgDeinitPipeline3D()
{
    vkDestroyPipeline(hgVkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(hgVkDevice, pipelineLayout, nullptr);

    hgDestroyDescriptor(defaultNormalMap.descriptor);
    hgDestroyImageView(defaultNormalMap.view);
    hgDestroyImage(defaultNormalMap.image);

    hgDestroyDescriptor(defaultColorMap.descriptor);
    hgDestroyImageView(defaultColorMap.view);
    hgDestroyImage(defaultColorMap.image);

    vkDestroySampler(hgVkDevice, defaultMapSampler, nullptr);

    hgDestroyBuffer(defaultModel.indexBuffer);
    hgDestroyBuffer(defaultModel.vertexBuffer);

    hgDestroyDescriptor(pointLightDesc);
    hgDestroyDescriptor(dirLightDesc);
    hgDestroyBuffer(pointLightBuffer);
    hgDestroyBuffer(dirLightBuffer);

    hgDestroyDescriptor(vpDesc);
    hgDestroyBuffer(vpBuffer);
}

void hgUpdateProjection3D(const HgMat4* projection)
{
    vpData.proj = *projection;
}

void hgUpdateView3D(const HgMat4* view)
{
    vpData.view = *view;
}

void hgDraw3D(HgECS* ecs, VkCommandBuffer cmd)
{
    hgAssert(cmd != nullptr);

    HgArena* scratch = hgGetScratch();
    HgArenaScope scratchScope{scratch};

    hgWriteBuffer(vpBuffer, 0, &vpData, sizeof(vpData));

    u32 dirLightCount = ecs->count<HgDirLight3D>();
    u32 pointLightCount = ecs->count<HgPointLight3D>();

    if (dirLightCount > dirLightCapacity)
    {
        vkQueueWaitIdle(hgVkQueue);

        dirLightCapacity = dirLightCapacity == 0 ? 1 : dirLightCapacity * 2;
        while (dirLightCapacity < dirLightCount)
        {
            dirLightCapacity *= 2;
        }

        hgDestroyBuffer(dirLightBuffer);
        dirLightBuffer = hgCreateBuffer(
            sizeof(DirLightData) * dirLightCapacity,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            HgBufferMemoryUsage_frequentUpdate);

        VkDescriptorBufferInfo dirLightBufferInfo{dirLightBuffer->buffer, 0, VK_WHOLE_SIZE};
        hgUpdateDescriptor(dirLightDesc, &dirLightBufferInfo, nullptr);
    }

    if (pointLightCount > pointLightCapacity)
    {
        vkQueueWaitIdle(hgVkQueue);

        pointLightCapacity = pointLightCapacity == 0 ? 1 : pointLightCapacity * 2;
        while (pointLightCapacity < pointLightCount)
        {
            pointLightCapacity *= 2;
        }

        hgDestroyBuffer(pointLightBuffer);
        pointLightBuffer = hgCreateBuffer(
            sizeof(PointLightData) * pointLightCapacity,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            HgBufferMemoryUsage_frequentUpdate);

        VkDescriptorBufferInfo pointLightBufferInfo{pointLightBuffer->buffer, 0, VK_WHOLE_SIZE};
        hgUpdateDescriptor(pointLightDesc, &pointLightBufferInfo, nullptr);
    }

    DirLightData* dirLights = hgAlloc<DirLightData>(scratch, dirLightCount);
    PointLightData* pointLights = hgAlloc<PointLightData>(scratch, pointLightCount);

    u32 i = 0;
    ecs->forEach<HgDirLight3D>([&](HgEntity, HgDirLight3D& light)
    {
        dirLights[i].dir = HgVec4{HgMat3{vpData.view} * light.dir, 0.0};
        dirLights[i].color = light.color;
        ++i;
    });

    i = 0;
    ecs->forEach<HgPointLight3D, HgTransform>([&](HgEntity, HgPointLight3D& light, HgTransform& transform)
    {
        pointLights[i].pos = vpData.view * HgVec4{transform.position, 1.0};
        pointLights[i].color = light.color;
        ++i;
    });

    if (dirLightCount > 0)
        hgWriteBuffer(dirLightBuffer, 0, dirLights, sizeof(*dirLights) * dirLightCount);

    if (pointLightCount > 0)
        hgWriteBuffer(pointLightBuffer, 0, pointLights, sizeof(*pointLights) * pointLightCount);

    hgBindGraphicsPipeline(cmd, pipeline, pipelineLayout);

    ecs->forEach<HgModel3D, HgTransform>([&](HgEntity, HgModel3D& model, HgTransform& transform)
    {
        HgTextureResource* colorMap = hgGetTexture(model.colorMap);
        if (colorMap == nullptr)
            colorMap = &defaultColorMap;

        HgTextureResource* normalMap = hgGetTexture(model.normalMap);
        if (normalMap == nullptr)
            normalMap = &defaultNormalMap;

        Push push{};
        push.model = hgModelMatrix3D(transform.position, transform.scale, transform.rotation);
        push.dirLightIdx = hgDescriptorIdx(dirLightDesc);
        push.dirLightCount = dirLightCount;
        push.pointLightIdx = hgDescriptorIdx(pointLightDesc);
        push.pointLightCount = pointLightCount;
        push.vpIdx = hgDescriptorIdx(vpDesc);
        push.colorMapIdx = hgDescriptorIdx(colorMap->descriptor);
        push.normalMapIdx = hgDescriptorIdx(normalMap->descriptor);

        HgModelResource* gpuModel = hgGetModel(model.modelResource);
        if (gpuModel == nullptr)
            gpuModel = &defaultModel;

        vkCmdBindIndexBuffer(cmd, gpuModel->indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT32);

        VkBuffer buffers[]{gpuModel->vertexBuffer->buffer};
        VkDeviceSize offsets[]{0};
        vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);

        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);

        vkCmdDrawIndexed(cmd, gpuModel->indexCount, 1, 0, 0, 0);
    });
}


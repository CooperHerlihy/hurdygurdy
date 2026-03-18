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

static VkDescriptorPool descriptorPool;
static VkDescriptorSet globalSet;

static GlobalUniform globalData;

static VkBuffer globalBuffer;
static VmaAllocation globalAlloc;

static HgHashMap<HgResource, VkDescriptorSet> modelMapSets;

static u32 dirLightCapacity;
static u32 pointLightCapacity;
static VkBuffer dirLightBuffer;
static VkBuffer pointLightBuffer;
static VmaAllocation dirLightAlloc;
static VmaAllocation pointLightAlloc;

static VkImage defaultColorMapImage;
static VmaAllocation defaultColorMapAlloc;
static VkImageView defaultColorMapView;

static VkImage defaultNormalMapImage;
static VkImageView defaultNormalMapView;
static VmaAllocation defaultNormalMapAlloc;

static VkSampler defaultMapSampler;

static VkBuffer defaultModelVertexBuffer;
static VmaAllocation defaultModelVertexAlloc;

static VkBuffer defaultModelIndexBuffer;
static VmaAllocation defaultModelIndexAlloc;

void hgInitPipeline3D(
    HgArena* arena,
    u32 maxModels,
    VkFormat colorFormat,
    VkFormat depthFormat
)
{
    hgAssert(colorFormat != VK_FORMAT_UNDEFINED);
    hgAssert(depthFormat != VK_FORMAT_UNDEFINED);

    modelMapSets = modelMapSets.create(arena, maxModels);

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

    VkShaderModule vertexShader = hgCreateVkShaderModule(model_vert_spv, model_vert_spv_size);
    VkShaderModule fragmentShader = hgCreateVkShaderModule(model_frag_spv, model_frag_spv_size);
    hgDefer(vkDestroyShaderModule(hgVkDevice, vertexShader, nullptr));
    hgDefer(vkDestroyShaderModule(hgVkDevice, fragmentShader, nullptr));

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
    pipelineConfig.vertexShader = vertexShader;
    pipelineConfig.fragmentShader = fragmentShader;
    pipelineConfig.layout = pipelineLayout;
    pipelineConfig.vertexBindings = vertexBindings;
    pipelineConfig.vertexBindingCount = sizeof(vertexBindings) / sizeof(*vertexBindings);
    pipelineConfig.vertexAttributes = vertexAttributes;
    pipelineConfig.vertexAttributeCount = sizeof(vertexAttributes) / sizeof(*vertexAttributes);
    pipelineConfig.cullMode = VK_CULL_MODE_BACK_BIT;

    pipeline = hgCreateVkGraphicsPipeline(pipelineConfig);

    VkDescriptorPoolSize descriptorSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxModels * 2}
    };
    descriptorPool = hgCreateVkDescriptorPool(
        maxModels + 1,
        descriptorSizes,
        sizeof(descriptorSizes) / sizeof(*descriptorSizes),
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

    globalSet = hgCreateVkDescriptorSet(descriptorPool, globalSetLayout);
    hgAssert(globalSet != nullptr);

    hgCreateVkBuffer(&globalBuffer, &globalAlloc, sizeof(GlobalUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    globalData.proj = HgMat4{1.0f};
    globalData.view = HgMat4{1.0f};
    globalData.dirLightCount = 0;
    globalData.pointLightCount = 0;

    vmaCopyMemoryToAllocation(hgVkVma, &globalData, globalAlloc, 0, sizeof(globalData));

    dirLightCapacity = 1;
    hgCreateVkBuffer(&dirLightBuffer, &dirLightAlloc, sizeof(DirLightData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    pointLightCapacity = 1;
    hgCreateVkBuffer(&pointLightBuffer, &pointLightAlloc, sizeof(PointLightData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    VkDescriptorBufferInfo dirLightBufferInfo = {dirLightBuffer, 0, VK_WHOLE_SIZE};
    hgUpdateVkDescriptorSet(
        globalSet,
        1,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        &dirLightBufferInfo,
        nullptr,
        1);

    VkDescriptorBufferInfo pointLightBufferInfo = {pointLightBuffer, 0, VK_WHOLE_SIZE};
    hgUpdateVkDescriptorSet(
        globalSet,
        2,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        &pointLightBufferInfo,
        nullptr,
        1);

    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = globalBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(GlobalUniform);

    hgUpdateVkDescriptorSet(
        globalSet,
        0,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        &bufferInfo,
        nullptr,
        1);

    struct Color {
        u8 r, g, b, a;
    };

    static const Color defaultColors[] = {
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    HgCreateVkImage colorImageConfig{};
    colorImageConfig.width = 3;
    colorImageConfig.height = 3;
    colorImageConfig.format = VK_FORMAT_R8G8B8A8_SRGB;
    colorImageConfig.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    hgCreateVkImage(&defaultColorMapImage, &defaultColorMapAlloc, colorImageConfig);

    HgCreateVkImageView colorViewConfig{};
    colorViewConfig.image = defaultColorMapImage;
    colorViewConfig.format = colorImageConfig.format;
    colorViewConfig.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    defaultColorMapView = hgCreateVkImageView(colorViewConfig);

    HgWriteVkImage colorWrite{};
    colorWrite.dstImage = defaultColorMapImage;
    colorWrite.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorWrite.srcData = defaultColors;
    colorWrite.width = colorImageConfig.width;
    colorWrite.height = colorImageConfig.height;
    colorWrite.format = colorImageConfig.format;
    colorWrite.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hgWriteVkImage(colorWrite);

    static const HgVec4 defaultNormals[] = {
        HgVec4{0, 0, -1, 0}, HgVec4{0, 0, -1, 0},
        HgVec4{0, 0, -1, 0}, HgVec4{0, 0, -1, 0},
    };

    HgCreateVkImage normalImageConfig{};
    normalImageConfig.width = 2;
    normalImageConfig.height = 2;
    normalImageConfig.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    normalImageConfig.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    hgCreateVkImage(&defaultNormalMapImage, &defaultNormalMapAlloc, normalImageConfig);

    HgCreateVkImageView normalViewConfig{};
    normalViewConfig.image = defaultNormalMapImage;
    normalViewConfig.format = normalImageConfig.format;
    normalViewConfig.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    defaultNormalMapView = hgCreateVkImageView(normalViewConfig);

    HgWriteVkImage normalWrite{};
    normalWrite.dstImage = defaultNormalMapImage;
    normalWrite.subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    normalWrite.srcData = defaultNormals;
    normalWrite.width = normalImageConfig.width;
    normalWrite.height = normalImageConfig.height;
    normalWrite.format = normalImageConfig.format;
    normalWrite.layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hgWriteVkImage(normalWrite);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

    VkResult samplerResult = vkCreateSampler(hgVkDevice, &samplerInfo, nullptr, &defaultMapSampler);
    if (samplerResult != VK_SUCCESS)
        hgError("Could not create sampler: %s\n", hgVkResultToStr(samplerResult));

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

    hgCreateVkBuffer(&defaultModelVertexBuffer, &defaultModelVertexAlloc, sizeof(cubeVertices),
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    hgCreateVkBuffer(&defaultModelIndexBuffer, &defaultModelIndexAlloc, sizeof(cubeIndices),
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    hgWriteVkBuffer(defaultModelVertexBuffer, 0, cubeVertices, sizeof(cubeVertices));
    hgWriteVkBuffer(defaultModelIndexBuffer, 0, cubeIndices, sizeof(cubeIndices));
}

void hgDeinitPipeline3D()
{
    vmaDestroyBuffer(hgVkVma, defaultModelIndexBuffer, defaultModelIndexAlloc);
    vmaDestroyBuffer(hgVkVma, defaultModelVertexBuffer, defaultModelVertexAlloc);
    vkDestroySampler(hgVkDevice, defaultMapSampler, nullptr);
    vkDestroyImageView(hgVkDevice, defaultNormalMapView, nullptr);
    vmaDestroyImage(hgVkVma, defaultNormalMapImage, defaultNormalMapAlloc);
    vkDestroyImageView(hgVkDevice, defaultColorMapView, nullptr);
    vmaDestroyImage(hgVkVma, defaultColorMapImage, defaultColorMapAlloc);
    vmaDestroyBuffer(hgVkVma, pointLightBuffer, pointLightAlloc);
    vmaDestroyBuffer(hgVkVma, dirLightBuffer, dirLightAlloc);
    vmaDestroyBuffer(hgVkVma, globalBuffer, globalAlloc);
    vkDestroyDescriptorPool(hgVkDevice, descriptorPool, nullptr);
    vkDestroyPipeline(hgVkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(hgVkDevice, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(hgVkDevice, textureSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(hgVkDevice, globalSetLayout, nullptr);
}

void hgAddModel3D(HgResource modelID, HgResource colorID, HgResource normalID)
{
    HgTextureResource* colorMap = hgGetTexture(colorID);
    HgTextureResource* normalMap = hgGetTexture(normalID);

    VkDescriptorSet set = hgCreateVkDescriptorSet(descriptorPool, textureSetLayout);

    VkDescriptorImageInfo imageInfos[2]{};
    imageInfos[0].sampler = colorMap != nullptr ? colorMap->sampler : defaultMapSampler;
    imageInfos[0].imageView = colorMap != nullptr ? colorMap->view : defaultColorMapView;
    imageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfos[1].sampler = normalMap != nullptr ? normalMap->sampler : defaultMapSampler;
    imageInfos[1].imageView = normalMap != nullptr ? normalMap->view : defaultNormalMapView;
    imageInfos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hgUpdateVkDescriptorSet(
        set,
        0,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        nullptr,
        imageInfos,
        2);

    modelMapSets.add(modelID) = set;
}

void hgRemoveModel3D(HgResource modelID)
{
    VkDescriptorSet set;
    if (modelMapSets.remove(modelID, &set))
        vkFreeDescriptorSets(hgVkDevice, descriptorPool, 1, &set);
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

        vmaDestroyBuffer(hgVkVma, dirLightBuffer, dirLightAlloc);
        hgCreateVkBuffer(&dirLightBuffer, &dirLightAlloc, sizeof(DirLightData) * newCapacity,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        dirLightCapacity = newCapacity;

        VkDescriptorBufferInfo dirLightBufferInfo = {dirLightBuffer, 0, VK_WHOLE_SIZE};
        hgUpdateVkDescriptorSet(
            globalSet,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            &dirLightBufferInfo,
            nullptr,
            1);
    }

    if (globalData.pointLightCount > pointLightCapacity)
    {
        vkQueueWaitIdle(hgVkQueue);

        u32 newCapacity = pointLightCapacity == 0 ? 1 : pointLightCapacity * 2;
        while (newCapacity < dirLightCapacity)
        {
            newCapacity *= 2;
        }

        vmaDestroyBuffer(hgVkVma, pointLightBuffer, pointLightAlloc);
        hgCreateVkBuffer(&pointLightBuffer, &pointLightAlloc, sizeof(PointLightData) * newCapacity,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        pointLightCapacity = newCapacity;

        VkDescriptorBufferInfo pointLightBufferInfo = {pointLightBuffer, 0, VK_WHOLE_SIZE};
        hgUpdateVkDescriptorSet(
            globalSet,
            1,
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            &pointLightBufferInfo,
            nullptr,
            1);
    }

    DirLightData* dirLights = hgAlloc<DirLightData>(scratch, globalData.dirLightCount);
    PointLightData* pointlights = hgAlloc<PointLightData>(scratch, globalData.pointLightCount);

    u32 i = 0;
    ecs->forEach<HgDirLight3D>([&](HgEntity, HgDirLight3D& light)
    {
        dirLights[i].dir = HgVec4{HgMat3{globalData.view} * light.dir, 0.0};
        dirLights[i].dir.w = 0.0f;
        dirLights[i].color = light.color;
        ++i;
    });

    i = 0;
    ecs->forEach<HgPointLight3D, HgTransform>([&](HgEntity, HgPointLight3D& light, HgTransform& transform)
    {
        pointlights[i].pos = globalData.view * HgVec4{transform.position, 1.0};
        pointlights[i].pos.w = 0.0f;
        pointlights[i].color = light.color;
        ++i;
    });

    if (globalData.dirLightCount > 0)
        vmaCopyMemoryToAllocation(
            hgVkVma,
            dirLights,
            dirLightAlloc,
            0,
            sizeof(*dirLights) * globalData.dirLightCount);

    if (globalData.pointLightCount > 0)
        vmaCopyMemoryToAllocation(
            hgVkVma,
            pointlights,
            pointLightAlloc,
            0,
            sizeof(*pointlights) * globalData.pointLightCount);

    vmaCopyMemoryToAllocation(
        hgVkVma,
        &globalData,
        globalAlloc,
        0,
        sizeof(globalData));

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &globalSet, 0, nullptr);

    ecs->forEach<HgModel3D, HgTransform>([&](HgEntity, HgModel3D& model, HgTransform& transform)
    {
        hgAssert(modelMapSets.has(model.modelResource));

        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipelineLayout,
            1,
            1,
            &modelMapSets[model.modelResource],
            0,
            nullptr);

        Push push{};
        push.model = hgModelMatrix3D(transform.position, transform.scale, transform.rotation);

        HgModelResource* gpuModel = hgGetModel(model.modelResource);
        if (gpuModel != nullptr)
        {
            vkCmdBindIndexBuffer(cmd, gpuModel->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            VkBuffer buffers[] = {gpuModel->vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        } else {
            vkCmdBindIndexBuffer(cmd, defaultModelIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

            VkBuffer buffers[] = {defaultModelVertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
        }

        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);

        vkCmdDrawIndexed(cmd, 36, 1, 0, 0, 0);
    });
}


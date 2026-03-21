#include "hurdygurdy.hpp"

struct VPUniform {
    HgMat4 proj;
    HgMat4 view;
};

struct Push {
    HgMat4 model;
    HgVec2 uvPos;
    HgVec2 uvSize;
    u32 vpIdx;
    u32 texIdx;
};

static HgBuffer* vpBuffer;
static HgDescriptor vpDesc;

static HgTextureResource defaultTex;

static VkPipelineLayout pipelineLayout;
static VkPipeline pipeline;

void hgInitPipeline2D(
    VkFormat colorFormat,
    VkFormat depthFormat)
{
    hgAssert(hgVkDevice != nullptr);
    hgAssert(colorFormat != VK_FORMAT_UNDEFINED);

    HgFence fence;
    const char* spriteVertSpvName = "build/sprite.vert.spv";
    const char* spriteFragSpvName = "build/sprite.frag.spv";
    HgResource spriteVertSpvID = hgResourceID(spriteVertSpvName);
    HgResource spriteFragSpvID = hgResourceID(spriteFragSpvName);
    hgLoadResource(&fence, 1, spriteVertSpvID, spriteVertSpvName);
    hgLoadResource(&fence, 1, spriteFragSpvID, spriteFragSpvName);
    hgDefer(hgUnloadResource(nullptr, 0, spriteVertSpvID));
    hgDefer(hgUnloadResource(nullptr, 0, spriteFragSpvID));

    HgBinary* spriteVertSpv = hgGetResource(spriteVertSpvID);
    HgBinary* spriteFragSpv = hgGetResource(spriteFragSpvID);

    vpBuffer = hgCreateBuffer(
        sizeof(VPUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        HgBufferMemoryUsage_frequentUpdate);

    VPUniform vpData{};
    vpData.proj = HgMat4{1.0f};
    vpData.view = HgMat4{1.0f};

    hgWriteBuffer(vpBuffer, 0, &vpData, sizeof(vpData));

    vpDesc = hgCreateDescriptor(HgDescriptorType_uniformBuffer);

    VkDescriptorBufferInfo bufferInfo{vpBuffer->buffer, 0, sizeof(VPUniform)};
    hgUpdateDescriptor(vpDesc, &bufferInfo, nullptr);

    struct Color
    {
        u8 r, g, b, a;
    };
    static const Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    defaultTex.image = hgCreateImage(2, 2, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    defaultTex.view = hgCreateImageView(defaultTex.image, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    hgWriteImage(
        defaultTex.image,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        defaultColors,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    defaultTex.sampler = hgCreateSampler(VK_FILTER_NEAREST);

    defaultTex.descriptor = hgCreateDescriptor(HgDescriptorType_combinedImageSampler);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = defaultTex.sampler;
    imageInfo.imageView = defaultTex.view->view;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hgUpdateDescriptor(defaultTex.descriptor, nullptr, &imageInfo);

    VkPushConstantRange push{VK_SHADER_STAGE_ALL, 0, sizeof(Push)};
    pipelineLayout = hgCreatePipelineLayout(&push);

    HgCreateGraphicsPipeline pipelineConfig{};
    pipelineConfig.layout = pipelineLayout;
    pipelineConfig.vertexShader = (u8*)spriteVertSpv->data;
    pipelineConfig.vertexShaderSize = spriteVertSpv->size;
    pipelineConfig.fragmentShader = (u8*)spriteFragSpv->data;
    pipelineConfig.fragmentShaderSize = spriteFragSpv->size;
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    pipelineConfig.enableDepthRead = depthFormat != VK_FORMAT_UNDEFINED;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    pipeline = hgCreateGraphicsPipeline(&pipelineConfig);
}

void hgDeinitPipeline2D()
{
    vkDestroyPipeline(hgVkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(hgVkDevice, pipelineLayout, nullptr);

    hgDestroyDescriptor(defaultTex.descriptor);
    vkDestroySampler(hgVkDevice, defaultTex.sampler, nullptr);
    hgDestroyImageView(defaultTex.view);
    hgDestroyImage(defaultTex.image);

    hgDestroyDescriptor(vpDesc);
    hgDestroyBuffer(vpBuffer);
}

void hgUpdateProjection2D(const HgMat4* projection)
{
    hgWriteBuffer(vpBuffer, offsetof(VPUniform, proj), projection, sizeof(*projection));
}

void hgUpdateView2D(const HgMat4* view)
{
    hgWriteBuffer(vpBuffer, offsetof(VPUniform, view), view, sizeof(*view));
}

void hgDraw2D(HgECS* ecs, VkCommandBuffer cmd)
{
    hgAssert(cmd != nullptr);

    ecs->sort<HgSprite2D>(nullptr, [](void*, HgECS* ecs, HgEntity lhs, HgEntity rhs) -> bool {
        hgAssert(ecs->has<HgTransform>(lhs));
        hgAssert(ecs->has<HgTransform>(rhs));
        return ecs->get<HgTransform>(lhs).position.z > ecs->get<HgTransform>(rhs).position.z;
    });

    hgBindGraphicsPipeline(cmd, pipeline, pipelineLayout);

    ecs->forEach<HgSprite2D, HgTransform>([&](HgEntity, HgSprite2D& sprite, HgTransform& transform) 
    {
        HgTextureResource* texture = hgGetTexture(sprite.texture);
        if (texture == nullptr)
            texture = &defaultTex;

        Push push{};
        push.model = hgModelMatrix3D(transform.position, transform.scale, transform.rotation);
        push.uvPos = sprite.uvPos;
        push.uvSize = sprite.uvSize;
        push.vpIdx = hgDescriptorIdx(vpDesc);
        push.texIdx = hgDescriptorIdx(texture->descriptor);

        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);

        vkCmdDraw(cmd, 6, 1, 0, 0);
    });
}


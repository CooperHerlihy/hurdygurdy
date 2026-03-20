#include "hurdygurdy.hpp"

#include "sprite.vert.spv.h"
#include "sprite.frag.spv.h"

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

static VkPipelineLayout pipelineLayout;
static VkPipeline pipeline;

static HgBuffer* vpBuffer;
static HgDescriptor vpDesc;

static HgTextureResource defaultTex;

void hgInitPipeline2D(
    VkFormat colorFormat,
    VkFormat depthFormat)
{
    hgAssert(hgVkDevice != nullptr);
    hgAssert(colorFormat != VK_FORMAT_UNDEFINED);

    VkPushConstantRange push{VK_SHADER_STAGE_ALL, 0, sizeof(Push)};
    pipelineLayout = hgCreatePipelineLayout(push);

    HgCreateGraphicsPipeline pipelineConfig{};
    pipelineConfig.layout = pipelineLayout;
    pipelineConfig.vertexShader = sprite_vert_spv;
    pipelineConfig.vertexShaderSize = sprite_vert_spv_size;
    pipelineConfig.fragmentShader = sprite_frag_spv;
    pipelineConfig.fragmentShaderSize = sprite_frag_spv_size;
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    pipelineConfig.enableDepthRead = depthFormat != VK_FORMAT_UNDEFINED;
    bool enableColorBlend = true;
    pipelineConfig.colorBlendEnables = &enableColorBlend;

    pipeline = hgCreateGraphicsPipeline(pipelineConfig);

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

    defaultTex.sampler = hgCreateVkSampler(VK_FILTER_NEAREST);

    defaultTex.descriptor = hgCreateDescriptor(HgDescriptorType_combinedImageSampler);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = defaultTex.sampler;
    imageInfo.imageView = defaultTex.view->view;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hgUpdateDescriptor(defaultTex.descriptor, nullptr, &imageInfo);
}

void hgDeinitPipeline2D()
{
    hgDestroyDescriptor(defaultTex.descriptor);
    vkDestroySampler(hgVkDevice, defaultTex.sampler, nullptr);
    hgDestroyImageView(defaultTex.view);
    hgDestroyImage(defaultTex.image);
    hgDestroyDescriptor(vpDesc);
    hgDestroyBuffer(vpBuffer);
    vkDestroyPipeline(hgVkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(hgVkDevice, pipelineLayout, nullptr);
}

void hgUpdateProjection2D(const HgMat4& projection)
{
    hgWriteBuffer(vpBuffer, offsetof(VPUniform, proj), &projection, sizeof(projection));
}

void hgUpdateView2D(const HgMat4& view)
{
    hgWriteBuffer(vpBuffer, offsetof(VPUniform, view), &view, sizeof(view));
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
        push.vpIdx = vpDesc.idx();
        push.texIdx = texture->descriptor.idx();

        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_ALL, 0, sizeof(push), &push);

        vkCmdDraw(cmd, 6, 1, 0, 0);
    });
}


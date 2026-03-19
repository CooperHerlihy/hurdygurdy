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
};

static VkDescriptorSetLayout vpSetLayout;
static VkDescriptorSetLayout textureSetLayout;
static VkPipelineLayout pipelineLayout;
static VkPipeline pipeline;

static HgBuffer* vpBuffer;
static VkDescriptorSet vpSet;

static HgTextureResource defaultTex;
static VkDescriptorSet defaultTexSet;

static HgHashMap<HgResource, VkDescriptorSet> textureSets;

void hgInitPipeline2D(
    HgArena* arena,
    u32 maxTextures,
    VkFormat colorFormat,
    VkFormat depthFormat)
{
    hgAssert(hgVkDevice != nullptr);
    hgAssert(colorFormat != VK_FORMAT_UNDEFINED);

    VkDescriptorSetLayoutBinding vpBindings[] = {
        {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT, nullptr}
    };
    vpSetLayout = hgCreateVkDescriptorSetLayout(
        vpBindings, sizeof(vpBindings) / sizeof(*vpBindings));

    VkDescriptorSetLayoutBinding textureBindings[] = {
        {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}
    };
    textureSetLayout = hgCreateVkDescriptorSetLayout(
        textureBindings, sizeof(textureBindings) / sizeof(*textureBindings));

    VkDescriptorSetLayout setLayouts[] = {vpSetLayout, textureSetLayout};
    VkPushConstantRange push = {VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(Push)};
    pipelineLayout = hgCreateVkPipelineLayout(
        setLayouts,
        sizeof(setLayouts) / sizeof(*setLayouts),
        &push,
        1);

    HgCreateVkGraphicsPipeline pipelineConfig{};
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    pipelineConfig.vertexShader = sprite_vert_spv;
    pipelineConfig.vertexShaderSize = sprite_vert_spv_size;
    pipelineConfig.fragmentShader = sprite_frag_spv;
    pipelineConfig.fragmentShaderSize = sprite_frag_spv_size;
    pipelineConfig.layout = pipelineLayout;
    pipelineConfig.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    pipelineConfig.enableColorBlend = true;

    pipeline = hgCreateVkGraphicsPipeline(pipelineConfig);

    vpSet = hgCreateVkDescriptorSet(vpSetLayout);
    hgAssert(vpSet != nullptr);

    vpBuffer = hgCreateBuffer(
        sizeof(VPUniform),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        HgBufferMemoryUsage_frequentUpdate);

    VPUniform vpData{};
    vpData.proj = HgMat4{1.0f};
    vpData.view = HgMat4{1.0f};

    hgWriteBuffer(vpBuffer, 0, &vpData, sizeof(vpData));

    VkDescriptorBufferInfo bufferInfo = {vpBuffer->buffer, 0, sizeof(VPUniform)};
    hgUpdateVkDescriptorSetBinding(
        vpSet,
        0,
        0,
        1,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        &bufferInfo,
        nullptr);

    struct Color
    {
        u8 r, g, b, a;
    };
    static const Color defaultColors[] = {
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

    HgCreateVkSampler defaultTexSamplerInfo{};
    defaultTex.sampler = hgCreateVkSampler(defaultTexSamplerInfo);

    defaultTexSet = hgCreateVkDescriptorSet(textureSetLayout);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = defaultTex.sampler;
    imageInfo.imageView = defaultTex.view->view;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hgUpdateVkDescriptorSetBinding(
        defaultTexSet,
        0,
        0,
        1,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        nullptr,
        &imageInfo);

    textureSets = textureSets.create(arena, maxTextures);
}

void hgDeinitPipeline2D()
{
    textureSets.forEach([](HgResource, VkDescriptorSet set)
    {
        hgDestroyVkDescriptorSet(set);
    });
    hgDestroyVkDescriptorSet(defaultTexSet);
    vkDestroySampler(hgVkDevice, defaultTex.sampler, nullptr);
    hgDestroyImageView(defaultTex.view);
    hgDestroyImage(defaultTex.image);
    hgDestroyBuffer(vpBuffer);
    vkDestroyPipeline(hgVkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(hgVkDevice, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(hgVkDevice, textureSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(hgVkDevice, vpSetLayout, nullptr);
}

void hgAddTexture2D(HgResource textureID)
{
    if (textureSets.get(textureID) != nullptr)
        return;

    hgAssert(hgGetTexture(textureID) != nullptr);
    HgTextureResource& texture = *hgGetTexture(textureID);

    VkDescriptorSet set = hgCreateVkDescriptorSet(textureSetLayout);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = texture.sampler;
    imageInfo.imageView = texture.view->view;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hgUpdateVkDescriptorSetBinding(
        set,
        0,
        0,
        1,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        nullptr,
        &imageInfo);

    textureSets.add(textureID, set);
}

void hgRemoveTexture2D(HgResource textureID)
{
    VkDescriptorSet set;
    if (textureSets.remove(textureID, &set))
        hgDestroyVkDescriptorSet(set);
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

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &vpSet, 0, nullptr);

    ecs->forEach<HgSprite2D, HgTransform>([&](HgEntity, HgSprite2D& sprite, HgTransform& transform) 
    {
        VkDescriptorSet* texSet = textureSets.get(sprite.texture);
        if (texSet == nullptr)
            texSet = &defaultTexSet;

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, texSet, 0, nullptr);

        Push push{};
        push.model = hgModelMatrix3D(transform.position, transform.scale, transform.rotation);
        push.uvPos = sprite.uvPos;
        push.uvSize = sprite.uvSize;

        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);

        vkCmdDraw(cmd, 4, 1, 0, 0);
    });
}


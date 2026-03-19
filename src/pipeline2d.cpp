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

static VkDescriptorPool descriptorPool;
static VkDescriptorSet vpSet;

static HgBuffer* vpBuffer;

static HgHashMap<HgResource, VkDescriptorSet> textureSets;

static HgImage* defaultTex;
static HgImageView* defaultTexView;
static VkSampler defaultTexSampler;

static VkDescriptorSet defaultTexSet;

void hgInitPipeline2D(
    HgArena* arena,
    u32 maxTextures,
    VkFormat colorFormat,
    VkFormat depthFormat
)
{
    hgAssert(hgVkDevice != nullptr);
    hgAssert(colorFormat != VK_FORMAT_UNDEFINED);

    textureSets = textureSets.create(arena, maxTextures);

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

    VkShaderModule vertexShader = hgCreateVkShaderModule(sprite_vert_spv, sprite_vert_spv_size);
    VkShaderModule fragmentShader = hgCreateVkShaderModule(sprite_frag_spv, sprite_frag_spv_size);
    hgDefer(vkDestroyShaderModule(hgVkDevice, vertexShader, nullptr));
    hgDefer(vkDestroyShaderModule(hgVkDevice, fragmentShader, nullptr));

    HgCreateVkGraphicsPipeline pipelineConfig{};
    pipelineConfig.colorAttachmentFormats = &colorFormat;
    pipelineConfig.colorAttachmentCount = 1;
    pipelineConfig.depthAttachmentFormat = depthFormat;
    pipelineConfig.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    pipelineConfig.vertexShader = vertexShader;
    pipelineConfig.fragmentShader = fragmentShader;
    pipelineConfig.layout = pipelineLayout;
    pipelineConfig.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
    pipelineConfig.enableColorBlend = true;

    pipeline = hgCreateVkGraphicsPipeline(pipelineConfig);

    VkDescriptorPoolSize descriptorSizes[] = {
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxTextures}
    };
    descriptorPool = hgCreateVkDescriptorPool(
        maxTextures + 1,
        descriptorSizes,
        sizeof(descriptorSizes) / sizeof(*descriptorSizes),
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT);

    vpSet = hgCreateVkDescriptorSet(descriptorPool, vpSetLayout);
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
    hgUpdateVkDescriptorSet(vpSet, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &bufferInfo, nullptr, 1);

    struct Color
    {
        u8 r, g, b, a;
    };
    static const Color defaultColors[] = {
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    defaultTex = hgCreateImage(2, 2, VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    defaultTexView = hgCreateImageView(defaultTex, {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1});

    hgWriteImage(
        defaultTex,
        {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1},
        defaultColors,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    HgCreateVkSampler defaultTexSamplerInfo{};
    defaultTexSampler = hgCreateVkSampler(defaultTexSamplerInfo);

    defaultTexSet = hgCreateVkDescriptorSet(descriptorPool, textureSetLayout);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = defaultTexSampler;
    imageInfo.imageView = defaultTexView->view;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hgUpdateVkDescriptorSet(
        defaultTexSet,
        0,
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        nullptr,
        &imageInfo,
        1);
}

void hgDeinitPipeline2D()
{
    vkDestroySampler(hgVkDevice, defaultTexSampler, nullptr);
    hgDestroyImageView(defaultTexView);
    hgDestroyImage(defaultTex);
    hgDestroyBuffer(vpBuffer);
    vkDestroyDescriptorPool(hgVkDevice, descriptorPool, nullptr);
    vkDestroyPipeline(hgVkDevice, pipeline, nullptr);
    vkDestroyPipelineLayout(hgVkDevice, pipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(hgVkDevice, textureSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(hgVkDevice, vpSetLayout, nullptr);
}

void hgAddTexture2D(HgResource textureID)
{
    if (textureSets.has(textureID))
        return;

    hgAssert(hgGetTexture(textureID) != nullptr);
    HgTextureResource& texture = *hgGetTexture(textureID);

    VkDescriptorSet set = hgCreateVkDescriptorSet(descriptorPool, textureSetLayout);

    VkDescriptorImageInfo imageInfo{};
    imageInfo.sampler = texture.sampler;
    imageInfo.imageView = texture.view->view;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    hgUpdateVkDescriptorSet(set, 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nullptr, &imageInfo, 1);

    textureSets.add(textureID) = set;
}

void hgRemoveTexture2D(HgResource textureID)
{
    VkDescriptorSet set;
    if (textureSets.remove(textureID, &set))
        vkFreeDescriptorSets(hgVkDevice, descriptorPool, 1, &set);
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
        VkDescriptorSet set = textureSets[sprite.texture];
        if (set == nullptr)
            set = defaultTexSet;

        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &set, 0, nullptr);

        Push push{};
        push.model = hgModelMatrix3D(transform.position, transform.scale, transform.rotation);
        push.uvPos = sprite.uvPos;
        push.uvSize = sprite.uvSize;

        vkCmdPushConstants(cmd, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(push), &push);

        vkCmdDraw(cmd, 4, 1, 0, 0);
    });
}


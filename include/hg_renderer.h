#pragma once

#include "hg_math.h"
#include "hg_vulkan.h"
#include "hg_window.h"

namespace hg {

struct PbrRenderer {
    struct ViewProjectionUniform {
        glm::mat4 projection{1.0f};
        glm::mat4 view{1.0f};
    };

    static constexpr usize MaxLights = 10;
    struct Light {
        glm::vec4 position{};
        glm::vec4 color{};
    };
    struct LightUniform {
        alignas(16) usize count = 0;
        alignas(16) Light vals[MaxLights]{};
    };

    struct SkyboxPush {
        Pool<Texture>::Handle cubemap{};
    };
    struct Skybox {
        Pool<Texture>::Handle cubemap{};
        GpuBuffer index_buffer{};
        GpuBuffer vertex_buffer{};
    };

    struct ModelPush {
        glm::mat4 model{1.0f};
        u32 normal_map_index = UINT32_MAX;
        u32 texture_index = UINT32_MAX;
        float roughness = 0.0f;
        float metalness = 0.0f;
    };
    struct Model {
        u32 index_count = 0;
        GpuBuffer index_buffer{};
        GpuBuffer vertex_buffer{};
        Pool<Texture>::Handle normal_map{};
        Pool<Texture>::Handle color_map{};
        float roughness = 0.0;
        float metalness = 0.0;
    };
    struct ModelTicket {
        Pool<Model>::Handle model{};
        Transform3Df transform{};
    };

    GpuImageAndView buffer_image{};
    GpuImageAndView depth_image{};

    GraphicsPipeline skybox_pipeline{};
    GraphicsPipeline model_pipeline{};

    VkDescriptorPool descriptor_pool{};
    VkDescriptorSetLayout descriptor_layout{};
    VkDescriptorSet descriptor_set{};

    GpuBuffer vp_buffer{};
    GpuBuffer light_buffer{};
    Skybox skybox{};

    Pool<Texture> textures{};
    Pool<Model> models{};
};

using PbrTextureHandle = Pool<Texture>::Handle;
using PbrModelHandle = Pool<PbrRenderer::Model>::Handle;

struct PbrRendererConfig {
    const Window& window;
    u32 max_textures = 256;
    u32 max_models = 256;
};
PbrRenderer create_pbr_renderer(Vk& vk, const PbrRendererConfig& config);
void resize_pbr_renderer(Vk& vk, PbrRenderer& renderer, const Window& window);
void destroy_pbr_renderer(Vk& vk, PbrRenderer& renderer);

Result<void> draw_pbr(Vk& vk, Window& window, PbrRenderer& renderer, Slice<const PbrRenderer::ModelTicket> models);

void update_projection(Vk& vk, const PbrRenderer& renderer, const glm::mat4& projection);

PbrRenderer::Light make_light(glm::vec3 position, glm::vec3 color, f32 intensity);
struct PbrLightData {
    const Cameraf& camera;
    Slice<const PbrRenderer::Light> lights;
};
void update_camera_and_lights(Vk& vk, PbrRenderer& renderer, const PbrLightData& config);

PbrTextureHandle load_texture(Vk& vk, PbrRenderer& renderer, const ImageData& data, const VkFormat format);
void unload_texture(Vk& vk, PbrRenderer& renderer, const PbrTextureHandle texture);

void load_skybox(Vk& vk, PbrRenderer& renderer, const ImageData& cubemap);
void unload_skybox(Vk& vk, PbrRenderer& renderer);

struct PbrModelConfig {
    const GltfData& data;
    PbrTextureHandle normal_map;
    PbrTextureHandle color_map;
};
PbrModelHandle load_model(Vk& vk, PbrRenderer& renderer, const PbrModelConfig& config);
void unload_model(Vk& vk, PbrRenderer& renderer, const PbrModelHandle model);

} // namespace hg

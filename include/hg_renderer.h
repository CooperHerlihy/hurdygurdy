#pragma once

#include "hg_math.h"
#include "hg_vulkan.h"
#include "hg_assets.h"

namespace hg {

struct Window {
    SDL_Window* window{};
    VkSurfaceKHR surface{};
    Swapchain swapchain{};
};

struct WindowConfig {
    bool windowed = false;
    glm::ivec2 size{};
};
[[nodiscard]] Result<Window> create_window(Vk& vk, const WindowConfig& config);
[[nodiscard]] Result<void> resize_window(Vk& vk, Window& window);
void destroy_window(Vk& vk, Window& window);

struct PbrRenderer {
    static constexpr usize MaxLights = 10;
    struct Light {
        glm::vec4 position{};
        glm::vec4 color{};
    };

    struct Texture {
        GpuImage image;
        VkImageView view;
        VkSampler sampler;
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

    struct Skybox {
        Pool<Texture>::Handle cubemap{};
        GpuBuffer index_buffer{};
        GpuBuffer vertex_buffer{};
    };

    Vk* vk = nullptr;

    VkDescriptorSetLayout descriptor_layout{};
    VkDescriptorPool descriptor_pool{};
    VkDescriptorSet descriptor_set{};

    GraphicsPipeline skybox_pipeline{};
    GraphicsPipeline model_pipeline{};
    GraphicsPipeline tonemap_pipeline{};
    GraphicsPipeline antialias_pipeline{};

    Pool<Texture> textures{};
    Pool<Model> models{};

    Pool<Texture>::Handle depth_image{};
    std::array<Pool<Texture>::Handle, 2> color_images{};

    GpuBuffer vp_buffer{};
    GpuBuffer light_buffer{};
    Skybox skybox{};
};

using PbrTextureHandle = Pool<PbrRenderer::Texture>::Handle;
using PbrModelHandle = Pool<PbrRenderer::Model>::Handle;

struct PbrRendererConfig {
    const Window& window;
    u32 max_textures = 256;
    u32 max_models = 256;
};
PbrRenderer create_pbr_renderer(Vk& vk, const PbrRendererConfig& config);
void resize_pbr_renderer(PbrRenderer& renderer, VkExtent2D extent);
void destroy_pbr_renderer(PbrRenderer& renderer);

struct Scene {
    const Cameraf* camera;
    Slice<const PbrRenderer::Light> lights;
    Slice<const PbrRenderer::ModelTicket> models;
};
Result<void> draw_pbr(PbrRenderer& renderer, Window& window, const Scene& scene);

PbrRenderer::Light make_light(glm::vec3 position, glm::vec3 color, f32 intensity);

void update_projection(PbrRenderer& renderer, const glm::mat4& projection);

struct PbrTextureConfig {
    ImageHandle<void> data{};
    VkFormat format = VK_FORMAT_UNDEFINED;
};
PbrTextureHandle create_texture(PbrRenderer& renderer, AssetManager& assets, const PbrTextureConfig& config);
void destroy_texture(PbrRenderer& renderer, const PbrTextureHandle texture);

void load_skybox(PbrRenderer& renderer, AssetManager& assets, const ImageHandle<u32> cubemap);
void unload_skybox(PbrRenderer& renderer);

struct PbrModelConfig {
    const GltfData& data;
    PbrTextureHandle normal_map;
    PbrTextureHandle color_map;
};
PbrModelHandle create_model(PbrRenderer& renderer, AssetManager& assets, const PbrModelConfig& config);
void destroy_model(PbrRenderer& renderer, PbrModelHandle model);

} // namespace hg

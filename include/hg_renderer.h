#pragma once

#include "hg_math.h"
#include "hg_vulkan.h"
#include "hg_assets.h"

namespace hg {

struct Window {
    SDL_Window* window;
    VkSurfaceKHR surface;
    Swapchain swapchain;
};

struct WindowConfig {
    bool windowed;
    Vec2i size;
};
[[nodiscard]] Result<Window> create_window(Vk& vk, const WindowConfig& config);
[[nodiscard]] Result<void> resize_window(Vk& vk, Window& window);
void destroy_window(Vk& vk, Window& window);

static constexpr usize PbrMaxLights = 10;
struct PbrLight {
    Vec4f position;
    Vec4f color;
};

struct PbrTexture {
    GpuImage image;
    VkImageView view;
    VkSampler sampler;
};

struct PbrModel {
    u32 index_count;
    GpuBuffer index_buffer;
    GpuBuffer vertex_buffer;
    Pool<PbrTexture>::Handle normal_map;
    Pool<PbrTexture>::Handle color_map;
    float roughness;
    float metalness;
};

struct PbrModelTicket {
    Pool<PbrModel>::Handle model;
    Transform3Df transform;
};

struct PbrSkybox {
    Pool<PbrTexture>::Handle cubemap;
    GpuBuffer index_buffer;
    GpuBuffer vertex_buffer;
};

struct PbrRenderer {
    Vk* vk;

    VkDescriptorSetLayout descriptor_layout;
    VkDescriptorPool descriptor_pool;
    VkDescriptorSet descriptor_set;

    GraphicsPipeline skybox_pipeline;
    GraphicsPipeline model_pipeline;
    GraphicsPipeline color_grading_pipeline;
    GraphicsPipeline antialias_pipeline;

    Pool<PbrTexture> textures;
    Pool<PbrModel> models;

    Pool<PbrTexture>::Handle depth_image;
    Pool<PbrTexture>::Handle color_images[2];

    GpuBuffer vp_buffer;
    GpuBuffer light_buffer;
    PbrSkybox skybox;
};

using PbrTextureHandle = Pool<PbrTexture>::Handle;
using PbrModelHandle = Pool<PbrModel>::Handle;

struct PbrRendererConfig {
    const Window& window;
    u32 max_textures = 256;
    u32 max_models = 256;
};
[[nodiscard]] PbrRenderer create_pbr_renderer(Vk& vk, const PbrRendererConfig& config);
void resize_pbr_renderer(PbrRenderer& renderer, VkExtent2D extent);
void destroy_pbr_renderer(PbrRenderer& renderer);

struct Scene {
    const Cameraf* camera;
    Slice<const PbrLight> lights;
    Slice<const PbrModelTicket> models;
};
[[nodiscard]] Result<void> draw_pbr(PbrRenderer& renderer, Window& window, const Scene& scene);

[[nodiscard]] PbrLight make_light(const Vec3f position, const Vec3f color, f32 intensity);

void update_projection(PbrRenderer& renderer, const Mat4f& projection);

struct PbrTextureConfig {
    ImageHandle<void> data;
    VkFormat format;
};
[[nodiscard]] PbrTextureHandle create_texture(
    PbrRenderer& renderer, AssetManager& assets, const PbrTextureConfig& config
);
void destroy_texture(PbrRenderer& renderer, const PbrTextureHandle texture);

void load_skybox(PbrRenderer& renderer, AssetManager& assets, const ImageHandle<u32> cubemap);
void unload_skybox(PbrRenderer& renderer);

struct PbrModelConfig {
    const GltfData& data;
    PbrTextureHandle normal_map;
    PbrTextureHandle color_map;
};
[[nodiscard]] PbrModelHandle create_model(PbrRenderer& renderer, AssetManager& assets, const PbrModelConfig& config);
void destroy_model(PbrRenderer& renderer, PbrModelHandle model);

} // namespace hg


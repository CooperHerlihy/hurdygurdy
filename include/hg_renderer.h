#pragma once

#include "hg_math.h"
#include "hg_engine.h"

namespace hg {

class DefaultRenderer {
public:
    DefaultRenderer() = default;

    class Pipeline {
    public:
        virtual ~Pipeline() = default;

        virtual void draw(const DefaultRenderer& renderer, const vk::CommandBuffer cmd) = 0;
    };

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

    [[nodiscard]] SDL_Window* get_window() const { return m_window.get(); }
    [[nodiscard]] vk::Extent2D get_extent() const { return m_window.get_extent(); }

    [[nodiscard]] vk::DescriptorSetLayout get_global_set_layout() const { return m_set_layout.get(); }
    [[nodiscard]] vk::DescriptorSet get_global_set() const { return m_global_set; }

    struct Config {
        bool fullscreen = false;
        glm::uvec2 window_size{1920, 1080};
    };
    [[nodiscard]] static Result<DefaultRenderer> create(Engine& engine, const Config& config);
    Result<void> resize(Engine& engine);
    void destroy(Engine& engine) const;

    Result<void> draw(Engine& engine, Slice<Pipeline*> pipelines);

    void update_projection(Engine& engine, const glm::mat4& projection) const {
        m_vp_buffer.write(engine.vk, projection, offsetof(ViewProjectionUniform, projection));
    }

    void update_camera_and_lights(Engine& engine, const Cameraf& camera);

    void queue_light(const glm::vec3 position, const glm::vec3 color) {
        ASSERT(m_light_queue.size() <= MaxLights);
        m_light_queue.emplace_back(glm::vec4{position, 1.0f}, glm::vec4{color, 1.0});
    }

private:
    Window m_window{};
    Surface m_surface{};
    Swapchain m_swapchain{};

    GpuImageAndView m_color_image{};
    GpuImageAndView m_depth_image{};

    DescriptorSetLayout m_set_layout{};
    DescriptorPool m_descriptor_pool{};
    vk::DescriptorSet m_global_set{};
    GpuBuffer m_vp_buffer{};
    GpuBuffer m_light_buffer{};
    std::vector<Light> m_light_queue{};
};

class SkyboxPipeline : public DefaultRenderer::Pipeline {
public:
    [[nodiscard]] static SkyboxPipeline create(Engine& engine, const DefaultRenderer& pipeline);
    void destroy(Engine& engine) const;
    void draw(const DefaultRenderer& renderer, const vk::CommandBuffer cmd) override;

    void load_skybox(Engine& engine, const ImageData& data);
    Result<void> load_skybox(Engine& engine, const std::filesystem::path path);

private:
    DescriptorSetLayout m_set_layout{};
    GraphicsPipeline m_pipeline{};

    DescriptorPool m_descriptor_pool{};
    vk::DescriptorSet m_set{};

    Texture m_cubemap{};
    GpuBuffer m_index_buffer{};
    GpuBuffer m_vertex_buffer{};
};

class PbrPipeline : public DefaultRenderer::Pipeline {
public:
    [[nodiscard]] static PbrPipeline create(Engine& engine, const DefaultRenderer& pipeline);
    void destroy(Engine& engine) const;

    struct PushConstant {
        glm::mat4 model{1.0f};
        u32 normal_map_index = UINT32_MAX;
        u32 texture_index = UINT32_MAX;
        float roughness = 0.0f;
        float metalness = 0.0f;
    };

    void draw(const DefaultRenderer& renderer, const vk::CommandBuffer cmd) override;

    static constexpr usize MaxTextures = 256;
    struct TextureHandle {
        usize index = SIZE_MAX;
    };

    [[nodiscard]] TextureHandle load_texture(
        Engine& engine, const ImageData& data, vk::Format format = vk::Format::eR8G8B8A8Srgb
    );
    [[nodiscard]] Result<TextureHandle> load_texture(
        Engine& engine, const std::filesystem::path path, vk::Format format = vk::Format::eR8G8B8A8Srgb
    );

    struct Model {
        u32 index_count = 0;
        GpuBuffer index_buffer{};
        GpuBuffer vertex_buffer{};
        TextureHandle normal_map{};
        TextureHandle texture{};
        float roughness = 0.0;
        float metalness = 0.0;

        void destroy(Engine& engine) const {
            vertex_buffer.destroy(engine.vk);
            index_buffer.destroy(engine.vk);
        }
    };

    struct ModelHandle {
        usize index = SIZE_MAX;
    };

    [[nodiscard]] ModelHandle load_model(
        Engine& engine,
        const GltfData& data,
        TextureHandle normal_map,
        TextureHandle texture
    );
    [[nodiscard]] Result<ModelHandle> load_model(
        Engine& engine,
        const std::filesystem::path path,
        TextureHandle normal_map,
        TextureHandle texture
    );

    struct RenderTicket {
        ModelHandle model{};
        Transform3Df transform{};
    };

    void queue_model(const ModelHandle model, const Transform3Df& transform) {
        ASSERT(model.index < m_models.size());
        m_render_queue.emplace_back(model, transform);
    }

    void clear_queue() { m_render_queue.clear(); }

private:
    DescriptorSetLayout m_set_layout{};
    GraphicsPipeline m_pipeline{};

    DescriptorPool m_descriptor_pool{};
    vk::DescriptorSet m_texture_set{};

    std::vector<Texture> m_textures{};
    std::vector<Model> m_models{};
    std::vector<RenderTicket> m_render_queue{};
};

} // namespace hg

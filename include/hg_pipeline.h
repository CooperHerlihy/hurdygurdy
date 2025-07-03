#pragma once

#include "hg_utils.h"
#include "hg_math.h"
#include "hg_generate.h"
#include "hg_vulkan_engine.h"

namespace hg {

class DefaultPipeline : public Pipeline {
public:
    DefaultPipeline() = default;

    class RenderSystem {
    public:
        virtual ~RenderSystem() = default;

        virtual void cmd_draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const = 0;
    };

    struct ViewProjectionUniform {
        glm::mat4 projection = {1.0f};
        glm::mat4 view = {1.0f};
    };

    static constexpr usize MaxLights = 10;
    struct Light {
        glm::vec4 position = {};
        glm::vec4 color = {};
    };

    struct LightUniform {
        alignas(16) usize count = 0;
        alignas(16) Light vals[MaxLights] = {};
    };

    [[nodiscard]] static Result<DefaultPipeline> create(const Engine& engine, vk::Extent2D window_size);
    void destroy(const Engine& engine) const;
    void resize(const Engine& engine, const vk::Extent2D window_size);

    void cmd_draw(vk::CommandBuffer cmd, vk::Image render_target, vk::Extent2D window_size) const override;

    vk::DescriptorSetLayout get_global_set_layout() const { return m_set_layout; }

    void add_render_system(const RenderSystem& system) {
        m_render_systems.emplace_back(&system);
    }

    void update_projection(const Engine& engine, const glm::mat4& projection) const {
        m_vp_buffer.write(engine, projection, offsetof(ViewProjectionUniform, projection));
    }

    void update_camera(const Engine& engine, const Cameraf& camera);

    void add_light(const glm::vec3 position, const glm::vec3 color) {
        ASSERT(m_lights.size() <= MaxLights);
        m_lights.emplace_back(glm::vec4{position, 1.0f}, glm::vec4{color, 1.0});
    }

    void clear_lights() {
        m_lights.clear();
    }

private:
    GpuImageAndView m_color_image = {};
    GpuImageAndView m_depth_image = {};

    vk::DescriptorPool m_descriptor_pool = {};
    vk::DescriptorSetLayout m_set_layout = {};
    vk::DescriptorSet m_global_set = {};
    GpuBuffer m_vp_buffer = {};
    GpuBuffer m_light_buffer = {};
    std::vector<Light> m_lights = {};

    std::vector<const RenderSystem*> m_render_systems = {};
};

class SkyboxRenderer : public DefaultPipeline::RenderSystem {
public:
    [[nodiscard]] static Result<SkyboxRenderer> create(const Engine& engine, const DefaultPipeline& pipeline);
    void destroy(const Engine& engine) const;
    void cmd_draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const override;

    [[nodiscard]] Result<void> load_skybox(
        const Engine& engine, const std::filesystem::path path
    );

private:
    vk::DescriptorSetLayout m_set_layout = {};
    vk::PipelineLayout m_pipeline_layout = {};
    std::array<vk::ShaderEXT, 2> m_shaders = {};

    vk::DescriptorPool m_descriptor_pool = {};
    vk::DescriptorSet m_set = {};

    GpuImageAndView m_cubemap = {};
    Sampler m_sampler = {};

    GpuBuffer m_index_buffer = {};
    GpuBuffer m_vertex_buffer = {};
};

class PbrRenderer : public DefaultPipeline::RenderSystem {
public:
    [[nodiscard]] static Result<PbrRenderer> create(const Engine& engine, const DefaultPipeline& pipeline);
    void destroy(const Engine& engine) const;

    struct PushConstant {
        glm::mat4 model = {1.0f};
        u32 normal_map_index = UINT32_MAX;
        u32 texture_index = UINT32_MAX;
        float roughness = 0.0f;
        float metalness = 0.0f;
    };

    void cmd_draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const override;

    static constexpr usize MaxTextures = 256;
    struct Texture {
        GpuImageAndView image = {};
        Sampler sampler = {};

        void destroy(const Engine& engine) const {
            sampler.destroy(engine);
            image.destroy(engine);
        }
    };

    struct TextureHandle {
        usize index = SIZE_MAX;
    };

    [[nodiscard]] Result<TextureHandle> load_texture(const Engine& engine, std::filesystem::path path);
    [[nodiscard]] TextureHandle load_texture_from_data(
        const Engine& engine, const GpuImageAndView::Data& data, vk::Format format = vk::Format::eR8G8B8A8Srgb
    );

    struct Model {
        u32 index_count = 0;
        GpuBuffer index_buffer = {};
        GpuBuffer vertex_buffer = {};
        TextureHandle normal_map = {};
        TextureHandle texture = {};
        float roughness = 0.0;
        float metalness = 0.0;

        void destroy(const Engine& engine) const {
            vertex_buffer.destroy(engine);
            index_buffer.destroy(engine);
        }
    };

    struct ModelHandle {
        usize index = SIZE_MAX;
    };

    [[nodiscard]] Result<ModelHandle> load_model(
        const Engine& engine,
        std::filesystem::path path,
        TextureHandle normal_map,
        TextureHandle texture
    );
    [[nodiscard]] ModelHandle load_model_from_data(
        const Engine& engine,
        const Mesh& data,
        TextureHandle normal_map,
        TextureHandle texture,
        float roughness, float metalness
    );

    struct RenderTicket {
        ModelHandle model = {};
        Transform3Df transform = {};
    };

    void queue_model(const ModelHandle model, const Transform3Df& transform) {
        ASSERT(model.index < m_models.size());
        m_render_queue.emplace_back(model, transform);
    }

    void clear_queue() {
        m_render_queue.clear();
    }

private:
    vk::DescriptorSetLayout m_set_layout = {};
    vk::PipelineLayout m_pipeline_layout = {};
    std::array<vk::ShaderEXT, 2> m_shaders = {};

    vk::DescriptorPool m_descriptor_pool = {};
    vk::DescriptorSet m_texture_set = {};

    std::vector<Texture> m_textures = {};
    std::vector<Model> m_models = {};
    std::vector<RenderTicket> m_render_queue = {};
};

} // namespace hg

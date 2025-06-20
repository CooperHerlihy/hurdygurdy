#pragma once

#include "hg_utils.h"
#include "hg_math.h"
#include "hg_mesh.h"
#include "hg_vulkan_engine.h"
#include <vulkan/vulkan_handles.hpp>

namespace hg {

class PbrPipeline : public Pipeline {
public:
    PbrPipeline() = default;

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
        Light vals[MaxLights];
        alignas(16) usize count;
    };

    struct Texture {
        GpuImage image = {};
        vk::Sampler sampler = {};

        void destroy(const Engine& engine) const {
            debug_assert(engine.device != nullptr);
            debug_assert(sampler != nullptr);

            engine.device.destroySampler(sampler);
            image.destroy(engine);
        }
    };

    struct Model {
        u32 index_count = 0;
        GpuBuffer index_buffer = {};
        GpuBuffer vertex_buffer = {};
        GpuBuffer material_buffer = {};
        vk::DescriptorSet set = {};

        void destroy(const Engine& engine) const {
            material_buffer.destroy(engine);
            vertex_buffer.destroy(engine);
            index_buffer.destroy(engine);
        }
    };

    struct Vertex {
        glm::vec3 position = {};
        glm::vec3 normal = {};
        glm::vec2 tex_coord = {};
    };

    struct VertexData {
        std::vector<u32> indices = {};
        std::vector<Vertex> vertices = {};

        static VertexData from_mesh(const Mesh& mesh);
    };

    struct PushConstant {
        glm::mat4 model = {1.0f};
    };

    struct MaterialUniform {
        float roughness = 0.0f;
        float metalness = 0.0f;
    };

    struct TextureHandle {
        usize index = UINT32_MAX;
    };

    struct ModelHandle {
        usize index = UINT32_MAX;
    };

    struct RenderTicket {
        ModelHandle model = {};
        Transform3Df transform = {};
    };

    [[nodiscard]] static Result<PbrPipeline> create(const Engine& engine, const Window& window, const vk::DescriptorPool descriptor_pool);
    void destroy(const Engine& engine) const;
    void resize(const Engine& engine, const Window& window);

    void update_lights(const Engine& engine, const Cameraf& camera);
    void cmd_draw(const Window& window, const vk::CommandBuffer cmd) const override;

    void update_projection(const Engine& engine, const glm::mat4& projection) const {
        m_vp_buffer.write(engine, projection, offsetof(ViewProjectionUniform, projection));
    }

    void add_render_system(const RenderSystem& system) {
        m_render_systems.emplace_back(&system);
    }

    [[nodiscard]] Result<TextureHandle> load_texture(const Engine& engine, std::filesystem::path path);
    [[nodiscard]] TextureHandle load_texture_from_data(const Engine& engine, const void* data, const vk::Extent3D extent, const vk::Format format, const u32 pixel_alignment);
    void add_texture(const Texture& texture) {
        debug_assert(texture.image.image != nullptr);
        debug_assert(texture.sampler != nullptr);
        m_textures.push_back(texture);
    }

    [[nodiscard]] Result<ModelHandle> load_model(const Engine& engine, const vk::DescriptorPool descriptor_pool, std::filesystem::path path, TextureHandle texture);
    [[nodiscard]] ModelHandle load_model_from_data(const Engine& engine, const vk::DescriptorPool descriptor_pool, std::span<const u32> indices, std::span<const Vertex> vertices, TextureHandle texture, float roughness, float metalness);
    void add_model(const Model& model) {
        debug_assert(model.index_count > 0);
        debug_assert(model.index_buffer.buffer != nullptr);
        debug_assert(model.vertex_buffer.buffer != nullptr);
        debug_assert(model.set != nullptr);
        m_models.push_back(model);
    }

    void queue_light(const glm::vec3 position, const glm::vec3 color) {
        debug_assert(m_lights.size() <= MaxLights);
        m_lights.emplace_back(glm::vec4{position, 1.0f}, glm::vec4{color, 1.0});
    }
    void queue_model(const ModelHandle model, const Transform3Df& transform) {
        debug_assert(model.index < m_models.size());
        m_render_queue.emplace_back(model, transform);
    }

private:
    GpuImage m_color_image = {};
    GpuImage m_depth_image = {};

    vk::DescriptorSet m_global_set = {};
    GpuBuffer m_vp_buffer = {};
    GpuBuffer m_light_buffer = {};

    std::vector<Light> m_lights = {};

    std::vector<const RenderSystem*> m_render_systems = {};

    std::array<vk::ShaderEXT, 2> m_pbr_shaders = {};
    vk::PipelineLayout m_pbr_layout = {};
    std::array<vk::DescriptorSetLayout, 2> m_pbr_set_layouts = {};

    std::vector<Texture> m_textures = {};
    std::vector<Model> m_models = {};
    std::vector<RenderTicket> m_render_queue = {};

};

class SkyboxSystem : public PbrPipeline::RenderSystem {
public:
    [[nodiscard]] static Result<SkyboxSystem> create(const Engine& engine, const vk::DescriptorPool pool, std::filesystem::path cubemap);
    void destroy(const Engine& engine) const;

    void cmd_draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const;

private:
    std::array<vk::DescriptorSetLayout, 2> m_set_layouts = {};
    vk::PipelineLayout m_pipeline_layout = {};
    std::array<vk::ShaderEXT, 2> m_shaders = {};

    GpuImage m_cubemap = {};
    vk::Sampler m_sampler = {};
    vk::DescriptorSet m_set = {};

    GpuBuffer m_index_buffer = {};
    GpuBuffer m_vertex_buffer = {};
};

} // namespace hg

#pragma once

#include "hg_load.h"
#include "hg_math.h"
#include "hg_utils.h"
#include "hg_vulkan_engine.h"
#include <glm/ext/vector_float2.hpp>
#include <vulkan/vulkan_handles.hpp>

namespace hg {

class PbrPipeline {
public:
    PbrPipeline() = default;

    struct PushConstant {
        glm::mat4 model = {1.0f};
    };

    struct ViewProjectionUniform {
        glm::mat4 projection = {1.0f};
        glm::mat4 view = {1.0f};
    };

    struct MaterialUniform {
        float roughness = 0.0f;
        float metalness = 0.0f;
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

    struct RenderTicket {
        usize model_index;
        Transform3Df transform;
    };

    [[nodiscard]] static PbrPipeline create(const Engine& engine, const Window& window);
    void destroy(const Engine& engine) const;
    void resize(const Engine& engine, const Window& window);
    void render(vk::CommandBuffer cmd, const Engine& engine, Window& window, const Cameraf& camera);

    void update_projection(const Engine& engine, const glm::mat4& projection) const { m_vp_buffer.write(engine, projection, offsetof(ViewProjectionUniform, projection)); }

    void add_texture(const Texture& texture) {
        debug_assert(texture.image.image != nullptr);
        debug_assert(texture.sampler != nullptr);
        m_textures.push_back(texture);
    }

    void add_model(const Model& model) {
        debug_assert(model.index_count > 0);
        debug_assert(model.index_buffer.buffer != nullptr);
        debug_assert(model.vertex_buffer.buffer != nullptr);
        debug_assert(model.set != nullptr);
        m_models.push_back(model);
    }

    void load_texture(const Engine& engine, std::filesystem::path path);
    void load_texture_from_data(const Engine& engine, const void* data, const vk::Extent3D extent, const vk::Format format, const u32 pixel_alignment);

    void load_model(const Engine& engine, std::filesystem::path path, usize texture_index);
    void load_model_from_data(const Engine& engine, std::span<const u32> indices, std::span<const ModelVertex> vertices, usize texture_index, float roughness, float metalness);

    void queue_light(const glm::vec3 position, const glm::vec3 color) {
        debug_assert(m_lights.size() <= MaxLights);
        m_lights.emplace_back(glm::vec4{position, 1.0f}, glm::vec4{color, 1.0});
    }

    void queue_model(const usize model_index, const Transform3Df& transform) {
        debug_assert(model_index < m_models.size());
        m_render_queue.emplace_back(model_index, transform);
    }

private:
    GpuImage m_color_image = {};
    GpuImage m_depth_image = {};
    Pipeline m_model_pipeline = {};
    vk::DescriptorPool m_descriptor_pool = {};
    vk::DescriptorSet m_global_set = {};
    GpuBuffer m_vp_buffer = {};
    GpuBuffer m_light_buffer = {};

    std::vector<Texture> m_textures = {};
    std::vector<Model> m_models = {};
    std::vector<RenderTicket> m_render_queue = {};
    std::vector<Light> m_lights = {};

    vk::DescriptorSetLayout test_set_layout = {};
    vk::PipelineLayout test_pipeline_layout = {};
    vk::ShaderEXT test_vert_shader = {};
    vk::ShaderEXT test_frag_shader = {};
    vk::DescriptorSet test_set = {};
    GpuBuffer test_vertices = {};
    GpuImage test_texture = {};
    vk::Sampler test_sampler = {};
};

} // namespace hg

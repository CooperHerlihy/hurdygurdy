#pragma once

#include "hg_math.h"
#include "hg_mesh.h"
#include "hg_utils.h"
#include "hg_vulkan_engine.h"

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

    struct Skybox {
        static constexpr u32 IndexCount = 36;
        vk::DescriptorSet set = {};
        vk::Sampler sampler = {};
        GpuImage cubemap = {};
        GpuBuffer index_buffer = {};
        GpuBuffer vertex_buffer = {};

        void destroy(const Engine& engine) const {
            engine.device.destroySampler(sampler);
            cubemap.destroy(engine);
            index_buffer.destroy(engine);
            vertex_buffer.destroy(engine);
        }
    };

    struct RenderTicket {
        usize model_index = {};
        Transform3Df transform = {};
    };

    struct Vertex {
        glm::vec3 position = {};
        glm::vec3 normal = {};
        glm::vec2 tex_coord = {};
    };

    struct VertexData {
        Vec<u32> indices = {};
        Vec<Vertex> vertices = {};

        static VertexData from_mesh(const Mesh& mesh);
    };

    [[nodiscard]] static PbrPipeline create(const Engine& engine, const Window& window);
    void destroy(const Engine& engine) const;
    void resize(const Engine& engine, const Window& window);
    void render(vk::CommandBuffer cmd, const Engine& engine, Window& window, const Cameraf& camera);

    void update_projection(const Engine& engine, const glm::mat4& projection) const { m_vp_buffer.write(engine, projection, offsetof(ViewProjectionUniform, projection)); }

    void load_skybox(const Engine& engine, std::filesystem::path path);

    void load_texture(const Engine& engine, std::filesystem::path path);
    void load_texture_from_data(const Engine& engine, const void* data, const vk::Extent3D extent, const vk::Format format, const u32 pixel_alignment);
    void add_texture(const Texture& texture) {
        debug_assert(texture.image.image != nullptr);
        debug_assert(texture.sampler != nullptr);
        m_textures.push_back(texture);
    }

    void load_model(const Engine& engine, std::filesystem::path path, usize texture_index);
    void load_model_from_data(const Engine& engine, std::span<const u32> indices, std::span<const Vertex> vertices, usize texture_index, float roughness, float metalness);
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

    void queue_model(const usize model_index, const Transform3Df& transform) {
        debug_assert(model_index < m_models.size());
        m_render_queue.emplace_back(model_index, transform);
    }

private:
    GpuImage m_color_image = {};
    GpuImage m_depth_image = {};

    std::array<vk::ShaderEXT, 2> m_pbr_shaders = {};
    vk::PipelineLayout m_pbr_layout = {};
    std::array<vk::DescriptorSetLayout, 2> m_pbr_set_layouts = {};

    std::array<vk::ShaderEXT, 2> m_skybox_shaders = {};
    vk::PipelineLayout m_skybox_layout = {};
    std::array<vk::DescriptorSetLayout, 2> m_skybox_set_layouts = {};

    vk::DescriptorPool m_descriptor_pool = {};
    vk::DescriptorSet m_global_set = {};
    GpuBuffer m_vp_buffer = {};
    GpuBuffer m_light_buffer = {};

    Skybox m_skybox = {};
    Vec<Texture> m_textures = {};
    Vec<Model> m_models = {};
    Vec<RenderTicket> m_render_queue = {};
    Vec<Light> m_lights = {};
};

} // namespace hg

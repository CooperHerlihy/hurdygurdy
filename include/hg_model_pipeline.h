#pragma once

#include "hg_math.h"
#include "hg_utils.h"
#include "hg_vulkan_utils.h"

namespace hg {

class ModelPipeline {
public:
    struct ViewProjectionUniform {
        glm::mat4 view = {1.0f};
        glm::mat4 projection = {1.0f};
    };

    struct Texture {
        GpuImage image;
        vk::Sampler sampler;
        vk::DescriptorSet set;

        void destroy(const Engine& engine) const {
            debug_assert(sampler != nullptr);
            engine.device.destroySampler(sampler);
            image.destroy(engine);
        }
    };

    struct Model {
        u32 index_count;
        GpuBuffer index_buffer;
        GpuBuffer vertex_buffer;
        usize texture_index;
        // material data?

        void destroy(const Engine& engine) const {
            index_buffer.destroy(engine);
            vertex_buffer.destroy(engine);
        }
    };

    struct RenderTicket {
        usize model_index;
        Transform3Df transform;
    };

    [[nodiscard]] static ModelPipeline create(const Engine& engine, const Window& window);
    void destroy(const Engine& engine) const;
    void resize(const Engine& engine, const Window& window);
    void render(const vk::CommandBuffer cmd, Window& window);

    void update_projection(const Engine& engine, const glm::mat4& projection_matrix) const {
        m_vp_buffer.write(engine, projection_matrix, offsetof(ViewProjectionUniform, projection));
    }
    void update_camera(const Engine& engine, const Cameraf& camera) const { m_vp_buffer.write(engine, camera.view(), offsetof(ViewProjectionUniform, view)); }

    void queue_model(const usize model_index, const Transform3Df& transform) {
        debug_assert(model_index < m_models.size());
        m_render_queue.emplace_back(model_index, transform);
    }

    void load_texture(const Engine& engine, const std::filesystem::path path);
    void load_model(const Engine& engine, const std::filesystem::path path, const usize texture_index);

private:
    ModelPipeline(const GpuImage color_image, const GpuImage depth_image, const Pipeline render_pipeline, const vk::DescriptorPool descriptor_pool, const vk::DescriptorSet vp_set,
                  const GpuBuffer vp_buffer)
        : m_color_image(color_image), m_depth_image(depth_image), m_render_pipeline(render_pipeline), m_descriptor_pool(descriptor_pool), m_vp_set(vp_set), m_vp_buffer(vp_buffer),
          m_textures(), m_models(), m_render_queue() {};

    GpuImage m_color_image = {};
    GpuImage m_depth_image = {};
    Pipeline m_render_pipeline = {};
    vk::DescriptorPool m_descriptor_pool = {};
    vk::DescriptorSet m_vp_set = {};
    GpuBuffer m_vp_buffer = {};

    std::vector<Texture> m_textures = {};
    std::vector<Model> m_models = {};
    std::vector<RenderTicket> m_render_queue = {};
};

} // namespace hg

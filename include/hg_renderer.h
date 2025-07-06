#pragma once

#include "hg_utils.h"
#include "hg_math.h"
#include "hg_generate.h"
#include "hg_engine.h"

namespace hg {

class DefaultRenderer {
public:
    DefaultRenderer() = default;

    class Pipeline {
    public:
        virtual ~Pipeline() = default;

        virtual void draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const = 0;
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

    void draw(const Window::DrawInfo& info) const;

    [[nodiscard]] static DefaultRenderer create(vk::Extent2D window_size);
    void destroy() const;
    void resize(const vk::Extent2D window_size);

    vk::DescriptorSetLayout get_global_set_layout() const { return m_set_layout.get(); }

    void add_pipeline(const Pipeline& system) {
        m_pipelines.emplace_back(&system);
    }

    void update_projection(const glm::mat4& projection) const {
        m_vp_buffer.write(projection, offsetof(ViewProjectionUniform, projection));
    }

    void update_camera(const Cameraf& camera);

    void add_light(const glm::vec3 position, const glm::vec3 color) {
        ASSERT(m_lights.size() <= MaxLights);
        m_lights.emplace_back(glm::vec4{position, 1.0f}, glm::vec4{color, 1.0});
    }

    void clear_lights() {
        m_lights.clear();
    }

private:
    GpuImageAndView m_color_image{};
    GpuImageAndView m_depth_image{};

    DescriptorSetLayout m_set_layout{};
    DescriptorPool m_descriptor_pool{};
    vk::DescriptorSet m_global_set{};
    GpuBuffer m_vp_buffer{};
    GpuBuffer m_light_buffer{};
    std::vector<Light> m_lights{};

    std::vector<const Pipeline*> m_pipelines{};
};

class SkyboxPipeline : public DefaultRenderer::Pipeline {
public:
    [[nodiscard]] static SkyboxPipeline create(const DefaultRenderer& pipeline);
    void destroy() const;
    void draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const override;

    [[nodiscard]] Result<void> load_skybox(const std::filesystem::path path);

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
    [[nodiscard]] static PbrPipeline create(const DefaultRenderer& pipeline);
    void destroy() const;

    struct PushConstant {
        glm::mat4 model{1.0f};
        u32 normal_map_index = UINT32_MAX;
        u32 texture_index = UINT32_MAX;
        float roughness = 0.0f;
        float metalness = 0.0f;
    };

    void draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) const override;

    static constexpr usize MaxTextures = 256;
    struct TextureHandle {
        usize index = SIZE_MAX;
    };

    [[nodiscard]] Result<TextureHandle> load_texture(
        std::filesystem::path path, vk::Format format = vk::Format::eR8G8B8A8Srgb
    );
    [[nodiscard]] TextureHandle load_texture_from_data(
        const GpuImage::Data& data, vk::Format format = vk::Format::eR8G8B8A8Srgb
    );

    struct Model {
        u32 index_count = 0;
        GpuBuffer index_buffer{};
        GpuBuffer vertex_buffer{};
        TextureHandle normal_map{};
        TextureHandle texture{};
        float roughness = 0.0;
        float metalness = 0.0;

        void destroy() const {
            vertex_buffer.destroy();
            index_buffer.destroy();
        }
    };

    struct ModelHandle {
        usize index = SIZE_MAX;
    };

    [[nodiscard]] Result<ModelHandle> load_model(
        std::filesystem::path path,
        TextureHandle normal_map,
        TextureHandle texture
    );
    [[nodiscard]] ModelHandle load_model_from_data(
        const Mesh& data,
        TextureHandle normal_map,
        TextureHandle texture,
        float roughness, float metalness
    );

    struct RenderTicket {
        ModelHandle model{};
        Transform3Df transform{};
    };

    void queue_model(const ModelHandle model, const Transform3Df& transform) {
        ASSERT(model.index < m_models.size());
        m_render_queue.emplace_back(model, transform);
    }

    void clear_queue() {
        m_render_queue.clear();
    }

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

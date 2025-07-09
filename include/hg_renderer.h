#pragma once

#include "hg_utils.h"
#include "hg_math.h"
#include "hg_generate.h"
#include "hg_vulkan.h"

namespace hg {

class Window {
public:
    class Renderer {
    public:
        virtual ~Renderer() = default;

        virtual void draw(const Swapchain::DrawInfo& info) = 0;

        virtual void resize(const Vk& vk, const Window& window) = 0;
    };

    [[nodiscard]] GLFWwindow* get() const {
        ASSERT(m_window != nullptr);
        return m_window;
    }
    [[nodiscard]] vk::SurfaceKHR get_surface() const { return m_surface.get(); }
    [[nodiscard]] vk::Extent2D get_extent() const {
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        return {to_u32(width), to_u32(height)};
    }

    [[nodiscard]] static Result<Window> create(const Vk& vk, bool fullscreen, i32 width, i32 height);
    void destroy(const Vk& vk) const {
        CONTEXT("Destroying window");

        m_swapchain.destroy(vk);
        m_surface.destroy(vk);

        ASSERT(m_window != nullptr);
        glfwDestroyWindow(m_window);
    }

    [[nodiscard]] Result<void> resize(const Vk& vk) {
        CONTEXT("Resizing window");

        const auto res = m_swapchain.resize(vk, m_surface.get());
        if (res.has_err())
            return res.err();

        return ok();
    }

    [[nodiscard]] Result<void> draw(const Vk& vk, Renderer& renderer);

private:
    GLFWwindow* m_window = nullptr;
    Surface m_surface{};
    Swapchain m_swapchain{};
};

class DefaultRenderer : public Window::Renderer {
public:
    DefaultRenderer() = default;

    class Pipeline {
    public:
        virtual ~Pipeline() = default;

        virtual void draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) = 0;
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

    void draw(const Swapchain::DrawInfo& info) override;

    [[nodiscard]] static DefaultRenderer create(const Vk& vk, const Window& window);
    void destroy(const Vk& vk) const;
    void resize(const Vk& vk, const Window& window) override;

    vk::DescriptorSetLayout get_global_set_layout() const { return m_set_layout.get(); }

    void add_pipeline(Pipeline& system) {
        m_pipelines.emplace_back(&system);
    }

    void update_projection(const Vk& vk, const glm::mat4& projection) const {
        m_vp_buffer.write(vk, projection, offsetof(ViewProjectionUniform, projection));
    }

    void update_camera_and_lights(const Vk& vk, const Cameraf& camera);

    void queue_light(const glm::vec3 position, const glm::vec3 color) {
        ASSERT(m_light_queue.size() <= MaxLights);
        m_light_queue.emplace_back(glm::vec4{position, 1.0f}, glm::vec4{color, 1.0});
    }

private:
    GpuImageAndView m_color_image{};
    GpuImageAndView m_depth_image{};

    DescriptorSetLayout m_set_layout{};
    DescriptorPool m_descriptor_pool{};
    vk::DescriptorSet m_global_set{};
    GpuBuffer m_vp_buffer{};
    GpuBuffer m_light_buffer{};
    std::vector<Light> m_light_queue{};

    std::vector<Pipeline*> m_pipelines{};
};

class SkyboxPipeline : public DefaultRenderer::Pipeline {
public:
    [[nodiscard]] static SkyboxPipeline create(const Vk& vk, const DefaultRenderer& pipeline);
    void destroy(const Vk& vk) const;
    void draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) override;

    [[nodiscard]] Result<void> load_skybox(const Vk& vk, const std::filesystem::path path);

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
    [[nodiscard]] static PbrPipeline create(const Vk& vk, const DefaultRenderer& pipeline);
    void destroy(const Vk& vk) const;

    struct PushConstant {
        glm::mat4 model{1.0f};
        u32 normal_map_index = UINT32_MAX;
        u32 texture_index = UINT32_MAX;
        float roughness = 0.0f;
        float metalness = 0.0f;
    };

    void draw(const vk::CommandBuffer cmd, const vk::DescriptorSet global_set) override;

    static constexpr usize MaxTextures = 256;
    struct TextureHandle {
        usize index = SIZE_MAX;
    };

    [[nodiscard]] Result<TextureHandle> load_texture(
        const Vk& vk, std::filesystem::path path, vk::Format format = vk::Format::eR8G8B8A8Srgb
    );
    [[nodiscard]] TextureHandle load_texture_from_data(
        const Vk& vk, const GpuImage::Data& data, vk::Format format = vk::Format::eR8G8B8A8Srgb
    );

    struct Model {
        u32 index_count = 0;
        GpuBuffer index_buffer{};
        GpuBuffer vertex_buffer{};
        TextureHandle normal_map{};
        TextureHandle texture{};
        float roughness = 0.0;
        float metalness = 0.0;

        void destroy(const Vk& vk) const {
            vertex_buffer.destroy(vk);
            index_buffer.destroy(vk);
        }
    };

    struct ModelHandle {
        usize index = SIZE_MAX;
    };

    [[nodiscard]] Result<ModelHandle> load_model(
        const Vk& vk, std::filesystem::path path,
        TextureHandle normal_map, TextureHandle texture
    );
    [[nodiscard]] ModelHandle load_model_from_data(
        const Vk& vk,
        const Mesh& data,
        TextureHandle normal_map,
        TextureHandle texture,
        float roughness,
        float metalness
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

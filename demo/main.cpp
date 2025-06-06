#include "hurdy_gurdy.h"

struct ViewProjectionUniform {
    glm::mat4 view = {1.0f};
    glm::mat4 projection = {1.0f};
};

int main() {
    const auto engine = hg::Engine::create();
    defer(engine.destroy());

    auto window = hg::Window::create(engine, 1920, 1080);
    defer(window.destroy(engine));

    auto color_image = hg::GpuImage::create(engine, {.extent = {window.extent.width, window.extent.height, 1},
                                                     .format = window.image_format,
                                                     .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                                                     .sample_count = vk::SampleCountFlagBits::e4});
    defer(color_image.destroy(engine));
    auto depth_image = hg::GpuImage::create(engine, {.extent = {window.extent.width, window.extent.height, 1},
                                                     .format = vk::Format::eD32Sfloat,
                                                     .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                     .aspect_flags = vk::ImageAspectFlagBits::eDepth,
                                                     .sample_count = vk::SampleCountFlagBits::e4});
    defer(depth_image.destroy(engine));

    const auto model_pipeline =
        hg::GraphicsPipelineBuilder()
            .set_shaders("../shaders/model.vert.spv", "../shaders/model.frag.spv")
            .set_render_target(std::array{window.image_format}, vk::Format::eD32Sfloat)
            .add_descriptor_set_layout(std::array{vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eVertex}})
            .add_descriptor_set_layout(std::array{vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}})
            .add_push_constant_range(vk::ShaderStageFlagBits::eVertex, sizeof(hg::ModelPushConstant))
            .add_vertex_binding(std::array{hg::VertexAttribute{vk::Format::eR32G32B32Sfloat, offsetof(hg::ModelVertex, position)},
                                           hg::VertexAttribute{vk::Format::eR32G32B32Sfloat, offsetof(hg::ModelVertex, normal)},
                                           hg::VertexAttribute{vk::Format::eR32G32Sfloat, offsetof(hg::ModelVertex, uv)}},
                                sizeof(hg::ModelVertex))
            .set_MSAA(vk::SampleCountFlagBits::e4)
            .enable_depth_buffer()
            .enable_color_blend()
            .enable_culling()
            .build(engine);
    defer(model_pipeline.destroy(engine));

    constexpr std::array pool_sizes = {
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 1},
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1},
    };
    const auto [pool_result, pool] = engine.device.createDescriptorPool({.maxSets = 2, .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()), .pPoolSizes = pool_sizes.data()});
    critical_assert(pool_result == vk::Result::eSuccess);
    defer(engine.device.destroyDescriptorPool(pool));

    const auto vp_buffer = hg::GpuBuffer::create(engine, sizeof(ViewProjectionUniform), vk::BufferUsageFlagBits::eUniformBuffer, hg::GpuBuffer::RandomAccess);
    defer(vp_buffer.destroy(engine));

    hg::Cameraf camera = {};
    camera.translate({0.0f, -2.0f, -2.0f});
    vp_buffer.write(engine, camera.view(), offsetof(ViewProjectionUniform, view));

    const f32 aspect_ratio = static_cast<f32>(window.extent.width) / static_cast<f32>(window.extent.height);
    vp_buffer.write(engine, glm::perspective(glm::pi<f32>() / 4.0f, aspect_ratio, 0.1f, 100.f), offsetof(ViewProjectionUniform, projection));

    hg::Transform3Df barrels = {};
    barrels.position = {0.0f, 0.0f, 1.0f};

    const auto model_data = hg::ModelData::load_gltf("../assets/dungeon_models/Assets/gltf/barrel_small_stack.gltf").value();
    u32 index_count = static_cast<u32>(model_data.indices.size());
    const auto index_buffer =
        hg::GpuBuffer::create(engine, model_data.indices.size() * sizeof(model_data.indices[0]), vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    defer(index_buffer.destroy(engine));
    index_buffer.write(engine, model_data.indices.data(), model_data.indices.size() * sizeof(model_data.indices[0]), 0);
    const auto vertex_buffer =
        hg::GpuBuffer::create(engine, model_data.vertices.size() * sizeof(model_data.vertices[0]), vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst);
    defer(vertex_buffer.destroy(engine));
    vertex_buffer.write(engine, model_data.vertices.data(), model_data.vertices.size() * sizeof(model_data.vertices[0]), 0);

    const auto model_texture_data = hg::ImageData::load("../assets/dungeon_models/Assets/gltf/dungeon_texture.png").value();
    const vk::Extent3D model_texture_extent = {static_cast<u32>(model_texture_data.width), static_cast<u32>(model_texture_data.height), 1};
    const u32 model_texture_mips = hg::get_mip_count(model_texture_extent);
    const auto model_texture_image =
        hg::GpuImage::create(engine, {.extent = model_texture_extent,
                                      .format = vk::Format::eR8G8B8A8Srgb,
                                      .usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
                                      .mip_levels = model_texture_mips});
    defer(model_texture_image.destroy(engine));
    model_texture_image.write(engine, model_texture_data.pixels, model_texture_extent, 4, vk::ImageLayout::eShaderReadOnlyOptimal);
    model_texture_image.generate_mipmaps(engine, model_texture_mips, model_texture_extent, vk::Format::eR8G8B8A8Srgb, vk::ImageLayout::eShaderReadOnlyOptimal);
    model_texture_data.unload();

    const auto model_texture_sampler = hg::create_sampler(engine, {.type = hg::SamplerType::Linear, .mip_levels = hg::get_mip_count(model_texture_extent)});
    defer(engine.device.destroySampler(model_texture_sampler));

    std::array<vk::DescriptorSet, 2> model_sets = {};
    hg::allocate_descriptor_sets(engine, pool, model_pipeline.descriptor_layouts, model_sets);

    hg::write_uniform_buffer_descriptor(engine, model_sets[0], 0, vp_buffer.buffer, sizeof(ViewProjectionUniform));
    hg::write_image_sampler_descriptor(engine, model_sets[1], 0, model_texture_sampler, model_texture_image.view);

    glm::vec<2, f64> cursor_pos = {};
    glfwGetCursorPos(window.window, &cursor_pos.x, &cursor_pos.y);

    hg::Clock clock = {};
    f64 time_count = 0.0;
    i32 frame_count = 0;
    while (!glfwWindowShouldClose(window.window)) {
        clock.update();
        const f64 delta = clock.delta_sec();
        const f32 delta32 = static_cast<f32>(delta);
        if (time_count >= 1.0) {
            std::cout << std::format("avg: {}ms\n", 1000.0 / frame_count);
            frame_count = 0;
            time_count -= 1.0;
        }
        time_count += delta;
        ++frame_count;

        glfwPollEvents();

        if (glfwGetKey(window.window, GLFW_KEY_LEFT) == GLFW_PRESS) {
            barrels.translate({-delta32, 0.0f, 0.0f});
        }
        if (glfwGetKey(window.window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            barrels.translate({delta32, 0.0f, 0.0f});
        }
        if (glfwGetKey(window.window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            barrels.translate({0.0f, 0.0f, -delta32});
        }
        if (glfwGetKey(window.window, GLFW_KEY_UP) == GLFW_PRESS) {
            barrels.translate({0.0f, 0.0f, delta32});
        }

        bool moved = false;
        constexpr f32 speed = 2.0f;
        if (glfwGetKey(window.window, GLFW_KEY_A) == GLFW_PRESS) {
            camera.move({-1.0f, 0.0f, 0.0f}, speed * delta32);
            moved = true;
        }
        if (glfwGetKey(window.window, GLFW_KEY_D) == GLFW_PRESS) {
            camera.move({1.0f, 0.0f, 0.0f}, speed * delta32);
            moved = true;
        }
        if (glfwGetKey(window.window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            camera.move({0.0f, -1.0f, 0.0f}, speed * delta32);
            moved = true;
        }
        if (glfwGetKey(window.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            camera.move({0.0f, 1.0f, 0.0f}, speed * delta32);
            moved = true;
        }
        if (glfwGetKey(window.window, GLFW_KEY_S) == GLFW_PRESS) {
            camera.move({0.0f, 0.0f, -1.0f}, speed * delta32);
            moved = true;
        }
        if (glfwGetKey(window.window, GLFW_KEY_W) == GLFW_PRESS) {
            camera.move({0.0f, 0.0f, 1.0f}, speed * delta32);
            moved = true;
        }
        constexpr f32 turn_speed = 0.003f;
        glm::vec<2, f64> new_cursor_pos = {};
        glfwGetCursorPos(window.window, &new_cursor_pos.x, &new_cursor_pos.y);
        const glm::vec2 cursor_dif = new_cursor_pos - cursor_pos;
        cursor_pos = new_cursor_pos;
        if (cursor_dif.x != 0 && glfwGetMouseButton(window.window, GLFW_MOUSE_BUTTON_1)) {
            camera.rotate_external(glm::angleAxis<f32, glm::defaultp>(cursor_dif.x * turn_speed, {0.0f, 1.0f, 0.0f}));
            moved = true;
        }
        if (cursor_dif.y != 0 && glfwGetMouseButton(window.window, GLFW_MOUSE_BUTTON_1)) {
            camera.rotate_internal(glm::angleAxis<f32, glm::defaultp>(cursor_dif.y * turn_speed, {-1.0f, 0.0f, 0.0f}));
            moved = true;
        }
        if (moved) {
            vp_buffer.write(engine, camera.view(), offsetof(ViewProjectionUniform, view));
        }

        const auto cmd = window.begin_frame(engine);

        const vk::Viewport viewport = {0.0f, 0.0f, static_cast<f32>(window.extent.width), static_cast<f32>(window.extent.height), 0.0f, 1.0f};
        cmd.setViewport(0, {viewport});
        const vk::Rect2D scissor = {{0, 0}, window.extent};
        cmd.setScissor(0, {scissor});

        hg::BarrierBuilder(cmd)
            .add_image_barrier(color_image.image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_dst(vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal)
            .add_image_barrier(depth_image.image, {vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1})
            .set_image_dst(vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests, vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
                           vk::ImageLayout::eDepthStencilAttachmentOptimal)
            .build_and_run();

        const vk::RenderingAttachmentInfo color_attachment = {
            .imageView = color_image.view,
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = {{std::array{0.0f, 0.0f, 0.0f, 1.0f}}},
        };
        const vk::RenderingAttachmentInfo depth_attachment = {
            .imageView = depth_image.view,
            .imageLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eDontCare,
            .clearValue = {.depthStencil = {.depth = 1.0f, .stencil = 0}},
        };
        cmd.beginRendering({
            .renderArea = {{0, 0}, window.extent},
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &color_attachment,
            .pDepthAttachment = &depth_attachment,
        });

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, model_pipeline.pipeline);
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, model_pipeline.layout, 0, model_sets, {});

        hg::ModelPushConstant model_push = {barrels.matrix()};
        cmd.pushConstants(model_pipeline.layout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(model_push), &model_push);

        std::array vertex_buffers = {vertex_buffer.buffer};
        std::array offsets = {vk::DeviceSize{0}};
        cmd.bindVertexBuffers(0, vertex_buffers, offsets);
        cmd.bindIndexBuffer(index_buffer.buffer, 0, vk::IndexType::eUint32);
        cmd.drawIndexed(index_count, 1, 0, 0, 1);

        cmd.endRendering();

        hg::BarrierBuilder(cmd)
            .add_image_barrier(color_image.image, {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_src(vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite, vk::ImageLayout::eColorAttachmentOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferRead, vk::ImageLayout::eTransferSrcOptimal)
            .add_image_barrier(window.current_image(), {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_dst(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .build_and_run();

        const vk::ImageResolve2 resolve = {
            .srcSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            .dstSubresource = {vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            .extent = vk::Extent3D{window.extent.width, window.extent.height, 1},
        };
        cmd.resolveImage2({
            .srcImage = color_image.image,
            .srcImageLayout = vk::ImageLayout::eTransferSrcOptimal,
            .dstImage = window.current_image(),
            .dstImageLayout = vk::ImageLayout::eTransferDstOptimal,
            .regionCount = 1,
            .pRegions = &resolve,
        });

        hg::BarrierBuilder(cmd)
            .add_image_barrier(window.current_image(), {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1})
            .set_image_src(vk::PipelineStageFlagBits2::eTransfer, vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eTransferDstOptimal)
            .set_image_dst(vk::PipelineStageFlagBits2::eAllGraphics, vk::AccessFlagBits2::eNone, vk::ImageLayout::ePresentSrcKHR)
            .build_and_run();

        if (const bool present_success = window.end_frame(engine); !present_success) {
            const auto wait_result = engine.queue.waitIdle();
            critical_assert(wait_result == vk::Result::eSuccess);

            for (int width = 0, height = 0; width == 0 || height == 0;) {
                glfwGetWindowSize(window.window, &width, &height);
                glfwPollEvents();
            };

            window.resize(engine);
            color_image.destroy(engine);
            color_image = hg::GpuImage::create(engine, {.extent = {window.extent.width, window.extent.height, 1},
                                                        .format = window.image_format,
                                                        .usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                                                        .sample_count = vk::SampleCountFlagBits::e4});
            depth_image.destroy(engine);
            depth_image = hg::GpuImage::create(engine, {.extent = {window.extent.width, window.extent.height, 1},
                                                        .format = vk::Format::eD32Sfloat,
                                                        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
                                                        .aspect_flags = vk::ImageAspectFlagBits::eDepth,
                                                        .sample_count = vk::SampleCountFlagBits::e4});
        }
    }

    const auto wait_result = engine.device.waitIdle();
    critical_assert(wait_result == vk::Result::eSuccess);
}

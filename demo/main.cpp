#include <hurdy_gurdy.h>

using namespace hg;

constexpr double sqrt3 = 1.73205080757;

#define errf(e) std::format("{} error: {}", #e, to_string(e.err()))

int main() {
    const auto engine = Engine::create();
    if (engine.has_err())
        critical_error(errf(engine));
    defer(engine->destroy());

    auto window = Window::create(*engine, true, 0, 0);
    if (window.has_err())
        critical_error(errf(window));
    defer(window->destroy(*engine));

    constexpr std::array pool_sizes = {
        vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer, 256},
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 256},
    };
    const auto descriptor_pool = engine->device.createDescriptorPool({
        .maxSets = 256, 
        .poolSizeCount = to_u32(pool_sizes.size()), 
        .pPoolSizes = pool_sizes.data(),
    }).value;
    if (descriptor_pool == nullptr)
        critical_error("Could not create descriptor pool");
    defer(engine->device.destroyDescriptorPool(descriptor_pool));

    auto pbr_pipeline = DefaultPipeline::create(*engine, window->extent(), descriptor_pool);
    if (pbr_pipeline.has_err())
        critical_error(errf(pbr_pipeline));
    defer(pbr_pipeline->destroy(*engine));

    auto skybox_renderer = SkyboxRenderer::create(*engine, *pbr_pipeline);
    if (skybox_renderer.has_err())
        critical_error(errf(skybox_renderer));
    defer(skybox_renderer->destroy(*engine));
    const auto skybox = skybox_renderer->load_skybox(*engine, descriptor_pool, "../assets/cloudy_skyboxes/Cubemap/Cubemap_Sky_06-512x512.png");
    if (skybox.has_err())
        critical_error(errf(skybox));
    pbr_pipeline->add_render_system(*skybox_renderer);

    auto model_renderer = PbrRenderer::create(*engine, *pbr_pipeline);
    if (model_renderer.has_err())
        critical_error(errf(model_renderer));
    defer(model_renderer->destroy(*engine));
    pbr_pipeline->add_render_system(*model_renderer);

    std::array<u32, 4> gold_color = {};
    gold_color.fill(0xff44ccff);
    const auto gold_texture = model_renderer->load_texture_from_data(*engine, {gold_color.data(), 4, {2, 2, 1}});
    const auto hex_texture = *model_renderer->load_texture(*engine, "../assets/hexagon_models/Textures/hexagons_medieval.png");

    const auto sphere = model_renderer->load_model_from_data(*engine, descriptor_pool, PbrRenderer::VertexData::from_mesh(generate_sphere(32)), 0.1f, 1.0f, gold_texture);
    const auto cube = model_renderer->load_model_from_data(*engine, descriptor_pool, PbrRenderer::VertexData::from_mesh(generate_cube()), 0.1f, 1.0f, gold_texture);
    const auto grass = *model_renderer->load_model(*engine, descriptor_pool, "../assets/hexagon_models/Assets/gltf/tiles/base/hex_grass.gltf", hex_texture);
    const auto tree = *model_renderer->load_model(*engine, descriptor_pool, "../assets/hexagon_models/Assets/gltf/decoration/nature/tree_single_A.gltf", hex_texture);
    const auto building = *model_renderer->load_model(*engine, descriptor_pool, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_home_A_blue.gltf", hex_texture);
    const auto tower = *model_renderer->load_model(*engine, descriptor_pool, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_tower_A_blue.gltf", hex_texture);
    const auto blacksmith = *model_renderer->load_model(*engine, descriptor_pool, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_blacksmith_blue.gltf", hex_texture);
    const auto castle = *model_renderer->load_model(*engine, descriptor_pool, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_castle_blue.gltf", hex_texture);

    pbr_pipeline->queue_light({-2.0f, -3.0f, -2.0f}, {glm::vec3{1.0f, 1.0f, 1.0f} * 300.0f});
    pbr_pipeline->queue_light({-0.8f, -0.5f, 1.5}, {glm::vec3{1.0f, 0.2f, 0.0f} * 10.0f});

    model_renderer->queue_model(grass, {.position = {0.0f, 0.0f, 0.0f}});
    model_renderer->queue_model(sphere, {.position = {-0.5f, -0.5f, 0.0f}, .scale = {0.25f, 0.25f, 0.25f}});
    model_renderer->queue_model(cube, {.position = {0.5f, -0.5f, 0.0f}, .scale = {0.25f, 0.25f, 0.25f}});

    model_renderer->queue_model(grass, {.position = {-1.0f, -0.25f, sqrt3}});
    model_renderer->queue_model(blacksmith, {.position = {-1.0f, -0.25f, sqrt3}});

    model_renderer->queue_model(grass, {.position = {1.0f, -0.5f, sqrt3}});
    model_renderer->queue_model(castle, {.position = {1.0f, -0.5f, sqrt3}});

    model_renderer->queue_model(grass, {.position = {-2.0f, -0.1f, 0.0f}});
    model_renderer->queue_model(building, {.position = {-2.0f, -0.1f, 0.0f}});
    model_renderer->queue_model(tree, {.position = {-2.0f - 0.75f, -0.1f, 0.0f - 0.25f}});

    model_renderer->queue_model(grass, {.position = {2.0f, -0.25f, 0.0f}});
    model_renderer->queue_model(tower, {.position = {2.0f, -0.25f, 0.0f}});
    model_renderer->queue_model(tree, {.position = {2.0f - 0.75f, -0.25f, 0.0f + 0.25f}});
    model_renderer->queue_model(tree, {.position = {2.0f + 0.75f, -0.25f, 0.0f - 0.25f}});

    const f32 aspect_ratio = static_cast<f32>(window->extent().width) / static_cast<f32>(window->extent().height);
    pbr_pipeline->update_projection(*engine, glm::perspective(glm::pi<f32>() / 4.0f, aspect_ratio, 0.1f, 100.f));

    Cameraf camera = {};
    camera.translate({0.0f, -2.0f, -4.0f});

    glm::dvec2 cursor_pos = {};
    glfwGetCursorPos(window->window(), &cursor_pos.x, &cursor_pos.y);

    Clock clock = {};
    f64 time_count = 0.0;
    i32 frame_count = 0;
    while (!glfwWindowShouldClose(window->window())) {
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

        constexpr f32 speed = 2.0f;
        if (glfwGetKey(window->window(), GLFW_KEY_A) == GLFW_PRESS)
            camera.move({-1.0f, 0.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(window->window(), GLFW_KEY_D) == GLFW_PRESS)
            camera.move({1.0f, 0.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(window->window(), GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.move({0.0f, -1.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(window->window(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camera.move({0.0f, 1.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(window->window(), GLFW_KEY_S) == GLFW_PRESS)
            camera.move({0.0f, 0.0f, -1.0f}, speed * delta32);
        if (glfwGetKey(window->window(), GLFW_KEY_W) == GLFW_PRESS)
            camera.move({0.0f, 0.0f, 1.0f}, speed * delta32);

        constexpr f32 turn_speed = 0.003f;
        glm::vec<2, f64> new_cursor_pos = {};
        glfwGetCursorPos(window->window(), &new_cursor_pos.x, &new_cursor_pos.y);
        const glm::vec2 cursor_dif = new_cursor_pos - cursor_pos;
        cursor_pos = new_cursor_pos;
        if (cursor_dif.x != 0 && glfwGetMouseButton(window->window(), GLFW_MOUSE_BUTTON_1))
            camera.rotate_external(glm::angleAxis<f32, glm::defaultp>(cursor_dif.x * turn_speed, {0.0f, 1.0f, 0.0f}));
        if (cursor_dif.y != 0 && glfwGetMouseButton(window->window(), GLFW_MOUSE_BUTTON_1))
            camera.rotate_internal(glm::angleAxis<f32, glm::defaultp>(cursor_dif.y * turn_speed, {-1.0f, 0.0f, 0.0f}));

        pbr_pipeline->update_camera(*engine, camera);
        const auto frame_result = window->submit_frame(*engine, *pbr_pipeline);
        if (frame_result.has_err())
            critical_error(errf(frame_result));
    }

    (void)engine->device.waitIdle();
}

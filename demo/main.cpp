#include <hurdy_gurdy.h>

using namespace hg;

constexpr double sqrt3 = 1.73205080757;

#define errp(e) if (e.has_err()) critical_error(std::format("{} error: {}", #e, to_string(e.err())))

int main() {
    const auto engine = Engine::create();
    errp(engine);
    defer(engine->destroy());

    auto window = Window::create(*engine, 1920, 1080);
    errp(window);
    defer(window->destroy(*engine));

    auto pbr_pipeline = PbrPipeline::create(*engine, *window);
    defer(pbr_pipeline->destroy(*engine));

    const auto skybox = pbr_pipeline->load_skybox(*engine, "../assets/cloudy_skyboxes/Cubemap/Cubemap_Sky_06-512x512.png");
    errp(skybox);

    std::array<u32, 4> gold_color = {};
    gold_color.fill(0xff44ccff);
    const auto gold_texture = pbr_pipeline->load_texture_from_data(*engine, gold_color.data(), {2, 2, 1}, vk::Format::eR8G8B8A8Srgb, 4);
    const auto hex_texture = *pbr_pipeline->load_texture(*engine, "../assets/hexagon_models/Textures/hexagons_medieval.png");

    const auto sphere_model = PbrPipeline::VertexData::from_mesh(generate_sphere(32));
    const auto sphere = pbr_pipeline->load_model_from_data(*engine, sphere_model.indices, sphere_model.vertices, gold_texture, 0.1f, 1.0f);
    const auto cube_model = PbrPipeline::VertexData::from_mesh(generate_cube());
    const auto cube = pbr_pipeline->load_model_from_data(*engine, cube_model.indices, cube_model.vertices, gold_texture, 0.1f, 1.0f);

    const auto grass = *pbr_pipeline->load_model(*engine, "../assets/hexagon_models/Assets/gltf/tiles/base/hex_grass.gltf", hex_texture);
    const auto tree = *pbr_pipeline->load_model(*engine, "../assets/hexagon_models/Assets/gltf/decoration/nature/tree_single_A.gltf", hex_texture);
    const auto building = *pbr_pipeline->load_model(*engine, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_home_A_blue.gltf", hex_texture);
    const auto tower = *pbr_pipeline->load_model(*engine, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_tower_A_blue.gltf", hex_texture);
    const auto blacksmith = *pbr_pipeline->load_model(*engine, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_blacksmith_blue.gltf", hex_texture);
    const auto castle = *pbr_pipeline->load_model(*engine, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_castle_blue.gltf", hex_texture);

    pbr_pipeline->update_projection(*engine, glm::perspective(glm::pi<f32>() / 4.0f, static_cast<f32>(window->extent.width) / static_cast<f32>(window->extent.height), 0.1f, 100.f));

    Cameraf camera = {};
    camera.translate({0.0f, -2.0f, -4.0f});

    glm::dvec2 cursor_pos = {};
    glfwGetCursorPos(window->window, &cursor_pos.x, &cursor_pos.y);

    Clock clock = {};
    f64 time_count = 0.0;
    i32 frame_count = 0;
    while (!glfwWindowShouldClose(window->window)) {
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
        if (glfwGetKey(window->window, GLFW_KEY_A) == GLFW_PRESS)
            camera.move({-1.0f, 0.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(window->window, GLFW_KEY_D) == GLFW_PRESS)
            camera.move({1.0f, 0.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(window->window, GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.move({0.0f, -1.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(window->window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camera.move({0.0f, 1.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(window->window, GLFW_KEY_S) == GLFW_PRESS)
            camera.move({0.0f, 0.0f, -1.0f}, speed * delta32);
        if (glfwGetKey(window->window, GLFW_KEY_W) == GLFW_PRESS)
            camera.move({0.0f, 0.0f, 1.0f}, speed * delta32);

        constexpr f32 turn_speed = 0.003f;
        glm::vec<2, f64> new_cursor_pos = {};
        glfwGetCursorPos(window->window, &new_cursor_pos.x, &new_cursor_pos.y);
        const glm::vec2 cursor_dif = new_cursor_pos - cursor_pos;
        cursor_pos = new_cursor_pos;
        if (cursor_dif.x != 0 && glfwGetMouseButton(window->window, GLFW_MOUSE_BUTTON_1))
            camera.rotate_external(glm::angleAxis<f32, glm::defaultp>(cursor_dif.x * turn_speed, {0.0f, 1.0f, 0.0f}));
        if (cursor_dif.y != 0 && glfwGetMouseButton(window->window, GLFW_MOUSE_BUTTON_1))
            camera.rotate_internal(glm::angleAxis<f32, glm::defaultp>(cursor_dif.y * turn_speed, {-1.0f, 0.0f, 0.0f}));

        auto frame_result = window->submit_frame(*engine, [&](const vk::CommandBuffer cmd) {
            pbr_pipeline->queue_light({1.0f, -3.0f, -2.0f}, {glm::vec3{1.0f, 1.0f, 1.0f} * 300.0f});
            pbr_pipeline->queue_light({-0.8f, -0.5f, 1.5}, {glm::vec3{1.0f, 0.2f, 0.0f} * 10.0f});

            pbr_pipeline->queue_model(grass, {.position = {0.0f, 0.0f, 0.0f}});
            pbr_pipeline->queue_model(sphere, {.position = {-0.5f, -0.5f, 0.0f}, .scale = {0.25f, 0.25f, 0.25f}});
            pbr_pipeline->queue_model(cube, {.position = {0.5f, -0.5f, 0.0f}, .scale = {0.25f, 0.25f, 0.25f}});

            pbr_pipeline->queue_model(grass, {.position = {-1.0f, -0.25f, sqrt3}});
            pbr_pipeline->queue_model(blacksmith, {.position = {-1.0f, -0.25f, sqrt3}});

            pbr_pipeline->queue_model(grass, {.position = {1.0f, -0.5f, sqrt3}});
            pbr_pipeline->queue_model(castle, {.position = {1.0f, -0.5f, sqrt3}});

            pbr_pipeline->queue_model(grass, {.position = {-2.0f, -0.1f, 0.0f}});
            pbr_pipeline->queue_model(building, {.position = {-2.0f, -0.1f, 0.0f}});
            pbr_pipeline->queue_model(tree, {.position = {-2.0f - 0.75f, -0.1f, 0.0f - 0.25f}});

            pbr_pipeline->queue_model(grass, {.position = {2.0f, -0.25f, 0.0f}});
            pbr_pipeline->queue_model(tower, {.position = {2.0f, -0.25f, 0.0f}});
            pbr_pipeline->queue_model(tree, {.position = {2.0f - 0.75f, -0.25f, 0.0f + 0.25f}});
            pbr_pipeline->queue_model(tree, {.position = {2.0f + 0.75f, -0.25f, 0.0f - 0.25f}});

            pbr_pipeline->render(cmd, *engine, *window, camera);
        });

        if (frame_result.has_err()) {
            while (frame_result.has_err()) {
                if (frame_result.err() != Err::InvalidWindowSize) {
                    errp(frame_result);
                }

                for (int width = 0, height = 0; width == 0 || height == 0;) {
                    debug_assert(window->window != nullptr);
                    glfwGetWindowSize(window->window, &width, &height);
                    glfwPollEvents();
                };

                debug_assert(engine->queue != nullptr);
                const auto wait_result = engine->queue.waitIdle();
                if (wait_result != vk::Result::eSuccess)
                    std::terminate();

                frame_result = window->resize(*engine);
            }

            pbr_pipeline->resize(*engine, *window);
            pbr_pipeline->update_projection(*engine, glm::perspective(glm::pi<f32>() / 4.0f, static_cast<f32>(window->extent.width) / static_cast<f32>(window->extent.height), 0.1f, 100.f));
        }
    }

    (void)engine->device.waitIdle();
}

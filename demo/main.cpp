#include "hurdy_gurdy.h"

int main() {
    const auto engine = hg::Engine::create();
    defer(engine.destroy());

    auto window = hg::Window::create(engine, 1920, 1080);
    defer(window.destroy(engine));

    auto model_pipeline = hg::ModelPipeline::create(engine, window);
    defer(model_pipeline.destroy(engine));

    model_pipeline.load_texture(engine, "../assets/dungeon_models/Assets/gltf/dungeon_texture.png");
    model_pipeline.load_model(engine, "../assets/dungeon_models/Assets/gltf/barrel_small_stack.gltf", 0);
    model_pipeline.load_model(engine, "../assets/dungeon_models/Assets/gltf/bed_decorated.gltf", 0);

    model_pipeline.update_projection(engine, glm::perspective(glm::pi<f32>() / 4.0f, static_cast<f32>(window.extent.width) / static_cast<f32>(window.extent.height), 0.1f, 100.f));

    model_pipeline.add_light({2.5f, -2.0f, 1.75f}, {glm::vec3{1.0f, 0.2f, 0.0f} * 10.0f});
    model_pipeline.add_light({-3.0f, -3.0f, -3.0f}, {glm::vec3{1.0f, 0.9f, 0.7f} * 50.0f});

    hg::Cameraf camera = {};
    camera.translate({0.0f, -2.0f, -2.0f});

    hg::Transform3Df barrels = {};
    barrels.position = {-1.0f, 0.0f, 2.0f};

    hg::Transform3Df bed = {};
    bed.position = {1.0f, 0.0f, 1.0f};

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

        constexpr f32 speed = 2.0f;
        if (glfwGetKey(window.window, GLFW_KEY_A) == GLFW_PRESS) {
            camera.move({-1.0f, 0.0f, 0.0f}, speed * delta32);
        }
        if (glfwGetKey(window.window, GLFW_KEY_D) == GLFW_PRESS) {
            camera.move({1.0f, 0.0f, 0.0f}, speed * delta32);
        }
        if (glfwGetKey(window.window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            camera.move({0.0f, -1.0f, 0.0f}, speed * delta32);
        }
        if (glfwGetKey(window.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            camera.move({0.0f, 1.0f, 0.0f}, speed * delta32);
        }
        if (glfwGetKey(window.window, GLFW_KEY_S) == GLFW_PRESS) {
            camera.move({0.0f, 0.0f, -1.0f}, speed * delta32);
        }
        if (glfwGetKey(window.window, GLFW_KEY_W) == GLFW_PRESS) {
            camera.move({0.0f, 0.0f, 1.0f}, speed * delta32);
        }
        constexpr f32 turn_speed = 0.003f;
        glm::vec<2, f64> new_cursor_pos = {};
        glfwGetCursorPos(window.window, &new_cursor_pos.x, &new_cursor_pos.y);
        const glm::vec2 cursor_dif = new_cursor_pos - cursor_pos;
        cursor_pos = new_cursor_pos;
        if (cursor_dif.x != 0 && glfwGetMouseButton(window.window, GLFW_MOUSE_BUTTON_1)) {
            camera.rotate_external(glm::angleAxis<f32, glm::defaultp>(cursor_dif.x * turn_speed, {0.0f, 1.0f, 0.0f}));
        }
        if (cursor_dif.y != 0 && glfwGetMouseButton(window.window, GLFW_MOUSE_BUTTON_1)) {
            camera.rotate_internal(glm::angleAxis<f32, glm::defaultp>(cursor_dif.y * turn_speed, {-1.0f, 0.0f, 0.0f}));
        }

        const bool present_success = window.submit_frame(engine, [&](const vk::CommandBuffer cmd) {
            model_pipeline.queue_model(0, barrels);
            model_pipeline.queue_model(1, bed);
            model_pipeline.render(cmd, engine, window, camera);
        });

        if (!present_success) {
            debug_assert(engine.queue != nullptr);
            const auto wait_result = engine.queue.waitIdle();
            critical_assert(wait_result == vk::Result::eSuccess);

            for (int width = 0, height = 0; width == 0 || height == 0;) {
                debug_assert(window.window != nullptr);
                glfwGetWindowSize(window.window, &width, &height);
                glfwPollEvents();
            };

            window.resize(engine);
            model_pipeline.resize(engine, window);
            model_pipeline.update_projection(engine,
                                             glm::perspective(glm::pi<f32>() / 4.0f, static_cast<f32>(window.extent.width) / static_cast<f32>(window.extent.height), 0.1f, 100.f));
        }
    }

    const auto wait_result = engine.device.waitIdle();
    critical_assert(wait_result == vk::Result::eSuccess);
}

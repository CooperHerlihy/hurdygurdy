#include <hurdy_gurdy.h>

#include <array>

using namespace hg;

constexpr double sqrt3 = 1.73205080757;

#define errf(e) "{} error: {}", #e, to_string(e.err())

int main() {
    auto engine = Engine::create();
    if (engine.has_err())
        LOGF_ERROR(errf(engine));

    auto renderer = DefaultRenderer::create(*engine, {.fullscreen = true});
    defer(renderer->destroy(*engine));

    auto skybox_pipeline = SkyboxPipeline::create(*engine, *renderer);
    auto model_pipeline = PbrPipeline::create(*engine, *renderer);
    defer(skybox_pipeline.destroy(*engine));
    defer(model_pipeline.destroy(*engine));

    const auto skybox = skybox_pipeline.load_skybox(*engine, "../assets/cloudy_skyboxes/Cubemap/Cubemap_Sky_06-512x512.png");
    if (skybox.has_err())
        LOGF_ERROR(errf(skybox));

    std::array<glm::vec4, 4> default_normal_image{};
    default_normal_image.fill(glm::vec4{0.0f, 0.0f, -1.0f, 0.0f});
    const auto default_normal_texture = model_pipeline.load_texture(*engine, {default_normal_image.data(), sizeof(glm::vec4), {2, 2, 1}}, vk::Format::eR32G32B32A32Sfloat);

    auto perlin_normal_image = create_normals_from_heightmap(generate_fractal_perlin_noise({512, 512}, {16, 16}));
    const auto perlin_normal_texture = model_pipeline.load_texture(*engine, {perlin_normal_image.data(), sizeof(glm::vec4), {512, 512, 1}}, vk::Format::eR32G32B32A32Sfloat);

    std::array<u32, 4> gray_color{};
    gray_color.fill(0xff777777);
    const auto gray_texture = model_pipeline.load_texture(*engine, {gray_color.data(), 4, {2, 2, 1}});

    const auto hex_texture = *model_pipeline.load_texture(*engine, "../assets/hexagon_models/Textures/hexagons_medieval.png");

    const auto cube = model_pipeline.load_model(*engine, {generate_cube(), 0.2f, 0.0f}, perlin_normal_texture, gray_texture);
    const auto sphere = model_pipeline.load_model(*engine, {generate_sphere({64, 32}), 0.2f, 1.0f}, perlin_normal_texture, gray_texture);
    const auto grass = *model_pipeline.load_model(*engine, "../assets/hexagon_models/Assets/gltf/tiles/base/hex_grass.gltf", default_normal_texture, hex_texture);
    const auto building = *model_pipeline.load_model(*engine, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_home_A_blue.gltf", default_normal_texture, hex_texture);
    const auto tower = *model_pipeline.load_model(*engine, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_tower_A_blue.gltf", default_normal_texture, hex_texture);

    const auto extent = renderer->get_extent();
    const f32 aspect_ratio = static_cast<f32>(extent.width) / static_cast<f32>(extent.height);
    renderer->update_projection(*engine, glm::perspective(glm::pi<f32>() / 4.0f, aspect_ratio, 0.1f, 100.f));

    Cameraf camera{};
    camera.translate({0.0f, -2.0f, -4.0f});

    glm::dvec2 cursor_pos{};
    glfwGetCursorPos(renderer->get_window(), &cursor_pos.x, &cursor_pos.y);

    Clock clock{};
    f64 time_count = 0.0;
    i32 frame_count = 0;
    while (!glfwWindowShouldClose(renderer->get_window())) {
        clock.update();
        const f64 delta = clock.delta_sec();
        const f32 delta32 = static_cast<f32>(delta);
        if (time_count >= 1.0) {
            std::printf("avg: %fms\n", 1000.0 / frame_count);
            frame_count = 0;
            time_count -= 1.0;
        }
        time_count += delta;
        ++frame_count;

        glfwPollEvents();

        constexpr f32 speed = 2.0f;
        if (glfwGetKey(renderer->get_window(), GLFW_KEY_A) == GLFW_PRESS)
            camera.move({-1.0f, 0.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(renderer->get_window(), GLFW_KEY_D) == GLFW_PRESS)
            camera.move({1.0f, 0.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(renderer->get_window(), GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.move({0.0f, -1.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(renderer->get_window(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camera.move({0.0f, 1.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(renderer->get_window(), GLFW_KEY_S) == GLFW_PRESS)
            camera.move({0.0f, 0.0f, -1.0f}, speed * delta32);
        if (glfwGetKey(renderer->get_window(), GLFW_KEY_W) == GLFW_PRESS)
            camera.move({0.0f, 0.0f, 1.0f}, speed * delta32);

        constexpr f32 turn_speed = 0.003f;
        glm::vec<2, f64> new_cursor_pos{};
        glfwGetCursorPos(renderer->get_window(), &new_cursor_pos.x, &new_cursor_pos.y);
        const glm::vec2 cursor_dif{new_cursor_pos - cursor_pos};
        cursor_pos = new_cursor_pos;
        if (cursor_dif.x != 0 && glfwGetMouseButton(renderer->get_window(), GLFW_MOUSE_BUTTON_1))
            camera.rotate_external(glm::angleAxis<f32, glm::defaultp>(cursor_dif.x * turn_speed, {0.0f, 1.0f, 0.0f}));
        if (cursor_dif.y != 0 && glfwGetMouseButton(renderer->get_window(), GLFW_MOUSE_BUTTON_1))
            camera.rotate_internal(glm::angleAxis<f32, glm::defaultp>(cursor_dif.y * turn_speed, {-1.0f, 0.0f, 0.0f}));

        model_pipeline.queue_model(grass, {.position{0.0f, 0.0f, 0.0f}});
        model_pipeline.queue_model(sphere, {.position{-0.5f, -0.5f, 0.0f}, .scale{0.25f, 0.25f, 0.25f}});
        model_pipeline.queue_model(cube, {.position{0.5f, -0.5f, 0.0f}, .scale{0.25f, 0.25f, 0.25f}});

        model_pipeline.queue_model(grass, {.position{-1.0f, -0.25f, sqrt3}});
        model_pipeline.queue_model(building, {.position{-1.0f, -0.25f, sqrt3}});

        model_pipeline.queue_model(grass, {.position{1.0f, -0.5f, sqrt3}});
        model_pipeline.queue_model(tower, {.position{1.0f, -0.5f, sqrt3}});

        renderer->queue_light({-2.0f, -3.0f, -2.0f}, {glm::vec3{1.0f, 1.0f, 1.0f} * 300.0f});

        renderer->update_camera_and_lights(*engine, camera);

        std::array<DefaultRenderer::Pipeline*, 2> pipelines{&skybox_pipeline, &model_pipeline};
        (void)renderer->draw(*engine, pipelines);
    }
}

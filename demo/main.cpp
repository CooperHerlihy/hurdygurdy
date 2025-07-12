#include <hurdy_gurdy.h>

using namespace hg;

constexpr double sqrt3 = 1.73205080757;

#define errf(e) "{} error: {}", #e, to_string(e.err())

int main() {
    auto engine = Engine::create({.fullscreen = true});
    if (engine.has_err())
        LOGF_ERROR(errf(engine));

    auto renderer = DefaultRenderer::create(engine->vk(), engine->window());
    defer(renderer.destroy(engine->vk()));

    auto skybox_pipeline = SkyboxPipeline::create(engine->vk(), renderer);
    auto model_pipeline = PbrPipeline::create(engine->vk(), renderer);
    defer(skybox_pipeline.destroy(engine->vk()));
    defer(model_pipeline.destroy(engine->vk()));
    renderer.add_pipeline(skybox_pipeline);
    renderer.add_pipeline(model_pipeline);

    const auto skybox = skybox_pipeline.load_skybox(engine->vk(), "../assets/cloudy_skyboxes/Cubemap/Cubemap_Sky_06-512x512.png");
    if (skybox.has_err())
        LOGF_ERROR(errf(skybox));

    std::array<glm::vec4, 4> default_normal_image{};
    default_normal_image.fill(glm::vec4{0.0f, 0.0f, -1.0f, 0.0f});
    const auto default_normal_texture = model_pipeline.load_texture_from_data(engine->vk(), {default_normal_image.data(), sizeof(glm::vec4), {2, 2, 1}}, vk::Format::eR32G32B32A32Sfloat);

    const auto perlin_normal_image = create_normals_from_heightmap(generate_fractal_perlin_noise({512, 512}, {16, 16}));
    const auto perlin_normal_texture = model_pipeline.load_texture_from_data(engine->vk(), {perlin_normal_image.data(), sizeof(glm::vec4), {512, 512, 1}}, vk::Format::eR32G32B32A32Sfloat);

    // const auto perlin_noise_color_image = map_image<u32>(generate_fractal_perlin_noise({512, 512}, {8, 8}), [](const f32 val) -> u32 {
    //     return (static_cast<u32>(val * 255.0f) << 0) + (static_cast<u32>(val * 255.0f) << 8) + (static_cast<u32>(val * 255.0f) << 16) + 0xff000000;
    // });
    // const auto perlin_noise_texture = model_renderer->load_texture_from_data({perlin_noise_color_image.data(), sizeof(u32), {512, 512, 1}});

    // std::array<u32, 4> gold_color = {};
    // gold_color.fill(0xff44ccff);
    // const auto gold_texture = model_renderer->load_texture_from_data({gold_color.data(), 4, {2, 2, 1}});

    std::array<u32, 4> gray_color{};
    gray_color.fill(0xff777777);
    const auto gray_texture = model_pipeline.load_texture_from_data(engine->vk(), {gray_color.data(), 4, {2, 2, 1}});

    const auto hex_texture = *model_pipeline.load_texture(engine->vk(), "../assets/hexagon_models/Textures/hexagons_medieval.png");

    const auto cube = model_pipeline.load_model_from_data(engine->vk(), generate_cube(), perlin_normal_texture, gray_texture, 0.2f, 0.0f);
    const auto sphere = model_pipeline.load_model_from_data(engine->vk(), generate_sphere({64, 32}), perlin_normal_texture, gray_texture, 0.2f, 1.0f);
    const auto grass = *model_pipeline.load_model(engine->vk(), "../assets/hexagon_models/Assets/gltf/tiles/base/hex_grass.gltf", default_normal_texture, hex_texture);
    const auto building = *model_pipeline.load_model(engine->vk(), "../assets/hexagon_models/Assets/gltf/buildings/blue/building_home_A_blue.gltf", default_normal_texture, hex_texture);
    const auto tower = *model_pipeline.load_model(engine->vk(), "../assets/hexagon_models/Assets/gltf/buildings/blue/building_tower_A_blue.gltf", default_normal_texture, hex_texture);

    const auto extent = engine->window().get_extent();
    const f32 aspect_ratio = static_cast<f32>(extent.width) / static_cast<f32>(extent.height);
    renderer.update_projection(engine->vk(), glm::perspective(glm::pi<f32>() / 4.0f, aspect_ratio, 0.1f, 100.f));

    Cameraf camera{};
    camera.translate({0.0f, -2.0f, -4.0f});

    glm::dvec2 cursor_pos{};
    glfwGetCursorPos(engine->window().get(), &cursor_pos.x, &cursor_pos.y);

    Clock clock{};
    f64 time_count = 0.0;
    i32 frame_count = 0;
    while (!glfwWindowShouldClose(engine->window().get())) {
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
        if (glfwGetKey(engine->window().get(), GLFW_KEY_A) == GLFW_PRESS)
            camera.move({-1.0f, 0.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(engine->window().get(), GLFW_KEY_D) == GLFW_PRESS)
            camera.move({1.0f, 0.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(engine->window().get(), GLFW_KEY_SPACE) == GLFW_PRESS)
            camera.move({0.0f, -1.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(engine->window().get(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            camera.move({0.0f, 1.0f, 0.0f}, speed * delta32);
        if (glfwGetKey(engine->window().get(), GLFW_KEY_S) == GLFW_PRESS)
            camera.move({0.0f, 0.0f, -1.0f}, speed * delta32);
        if (glfwGetKey(engine->window().get(), GLFW_KEY_W) == GLFW_PRESS)
            camera.move({0.0f, 0.0f, 1.0f}, speed * delta32);

        constexpr f32 turn_speed = 0.003f;
        glm::vec<2, f64> new_cursor_pos{};
        glfwGetCursorPos(engine->window().get(), &new_cursor_pos.x, &new_cursor_pos.y);
        const glm::vec2 cursor_dif{new_cursor_pos - cursor_pos};
        cursor_pos = new_cursor_pos;
        if (cursor_dif.x != 0 && glfwGetMouseButton(engine->window().get(), GLFW_MOUSE_BUTTON_1))
            camera.rotate_external(glm::angleAxis<f32, glm::defaultp>(cursor_dif.x * turn_speed, {0.0f, 1.0f, 0.0f}));
        if (cursor_dif.y != 0 && glfwGetMouseButton(engine->window().get(), GLFW_MOUSE_BUTTON_1))
            camera.rotate_internal(glm::angleAxis<f32, glm::defaultp>(cursor_dif.y * turn_speed, {-1.0f, 0.0f, 0.0f}));

        model_pipeline.queue_model(grass, {.position{0.0f, 0.0f, 0.0f}});
        model_pipeline.queue_model(sphere, {.position{-0.5f, -0.5f, 0.0f}, .scale{0.25f, 0.25f, 0.25f}});
        model_pipeline.queue_model(cube, {.position{0.5f, -0.5f, 0.0f}, .scale{0.25f, 0.25f, 0.25f}});

        model_pipeline.queue_model(grass, {.position{-1.0f, -0.25f, sqrt3}});
        model_pipeline.queue_model(building, {.position{-1.0f, -0.25f, sqrt3}});

        model_pipeline.queue_model(grass, {.position{1.0f, -0.5f, sqrt3}});
        model_pipeline.queue_model(tower, {.position{1.0f, -0.5f, sqrt3}});

        renderer.queue_light({-2.0f, -3.0f, -2.0f}, {glm::vec3{1.0f, 1.0f, 1.0f} * 300.0f});

        renderer.update_camera_and_lights(engine->vk(), camera);

        (void)engine->draw(renderer);
    }
}

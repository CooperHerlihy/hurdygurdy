#include <hurdy_gurdy.h>

#include <array>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

using namespace hg;

constexpr double sqrt3 = 1.73205080757;

#define errf(e) "{} error: {}", #e, to_string(e.err())

static Engine engine{};
static Generator generator{};
static DefaultRenderer renderer{};
static SkyboxPipeline skybox_pipeline{};
static PbrPipeline model_pipeline{};

static PbrPipeline::TextureHandle default_normals{};
static PbrPipeline::TextureHandle perlin_normals{};
static PbrPipeline::TextureHandle gray_texture{};
static PbrPipeline::ModelHandle cube{};
static PbrPipeline::ModelHandle sphere{};

static PbrPipeline::TextureHandle hex_texture{};
static PbrPipeline::ModelHandle grass{};
static PbrPipeline::ModelHandle building{};
static PbrPipeline::ModelHandle tower{};

static Cameraf camera{};
static Clock game_clock{};
static f64 time_count = 0.0;
static i32 frame_count = 0;

struct InputState {
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    bool forward = false;
    bool backward = false;
    bool left_mouse = false;
    bool right_mouse = false;
};

static InputState input_state{};

SDL_AppResult SDL_AppInit(void**, int, char**) {
    auto engine_res = Engine::create();
    if (engine_res.has_err())
        LOGF_ERROR(errf(engine_res));
    engine = std::move(*engine_res);

    generator = Generator{{.max_meshes = 64, .max_images = 64}};

    auto renderer_res = DefaultRenderer::create(engine, {.fullscreen = true});
    if (renderer_res.has_err())
        LOGF_ERROR(errf(renderer_res));
    renderer = std::move(*renderer_res);

    skybox_pipeline = SkyboxPipeline::create(engine, renderer);
    model_pipeline = PbrPipeline::create(engine, renderer);

    auto skybox_res = skybox_pipeline.load_skybox(engine, "../assets/cloudy_skyboxes/Cubemap/Cubemap_Sky_06-512x512.png");
    if (skybox_res.has_err())
        LOGF_ERROR(errf(skybox_res));

    default_normals = [&] {
        auto default_normal_image = generator.alloc_image<glm::vec4>({2, 2}, [](...) {
            return glm::vec4{0.0f, 0.0f, -1.0f, 0.0f};
        });
        defer(generator.dealloc_image(default_normal_image));

        return model_pipeline.load_texture(engine, generator.get(default_normal_image), vk::Format::eR32G32B32A32Sfloat);
    }();

    perlin_normals = [&] {
        auto perlin_noise = generator.alloc_image<f32>({512, 512}, [&](const auto pos) {
            return get_fractal_noise(pos, 1.0f, 32.0f, get_perlin_noise);
        });
        defer(generator.dealloc_image(perlin_noise));

        auto perlin_normal_image = generator.alloc_image<glm::vec4>({512, 512}, [&](const auto pos) {
            return get_normal_from_heightmap(pos, generator.get(perlin_noise));
        });
        defer(generator.dealloc_image(perlin_normal_image));

        return model_pipeline.load_texture(engine, generator.get(perlin_normal_image), vk::Format::eR32G32B32A32Sfloat);
    }();

    gray_texture = [&] {
        auto gray_image = generator.alloc_image<u32>({2, 2}, [](...) { return 0xff777777; });
        defer(generator.dealloc_image(gray_image));

        return model_pipeline.load_texture(engine, generator.get(gray_image));
    }();

    cube = [&] {
        auto cube_mesh = generator.generate_cube(generator.alloc_mesh());
        defer(generator.dealloc_mesh(cube_mesh));

        return model_pipeline.load_model(engine, {generator.get(cube_mesh), 0.2f, 0.0f}, perlin_normals, gray_texture);
    }();

    sphere = [&] {
        auto sphere_mesh = generator.generate_sphere(generator.alloc_mesh(), {64, 32});
        defer(generator.dealloc_mesh(sphere_mesh));

        return model_pipeline.load_model(engine, {generator.get(sphere_mesh), 0.2f, 1.0f}, perlin_normals, gray_texture);
    }();

    hex_texture = *model_pipeline.load_texture(engine, "../assets/hexagon_models/Textures/hexagons_medieval.png");
    grass = *model_pipeline.load_model(engine, "../assets/hexagon_models/Assets/gltf/tiles/base/hex_grass.gltf", default_normals, hex_texture);
    building = *model_pipeline.load_model(engine, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_home_A_blue.gltf", default_normals, hex_texture);
    tower = *model_pipeline.load_model(engine, "../assets/hexagon_models/Assets/gltf/buildings/blue/building_tower_A_blue.gltf", default_normals, hex_texture);

    const auto extent = renderer.get_extent();
    const f32 aspect_ratio = static_cast<f32>(extent.width) / static_cast<f32>(extent.height);
    renderer.update_projection(engine, glm::perspective(glm::pi<f32>() / 4.0f, aspect_ratio, 0.1f, 100.f));

    camera.translate({0.0f, -2.0f, -4.0f});
    game_clock.update();

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void*, SDL_AppResult) {
    defer(generator.destroy());
    defer(renderer.destroy(engine));
    defer(skybox_pipeline.destroy(engine));
    defer(model_pipeline.destroy(engine));
}

SDL_AppResult SDL_AppIterate(void*) {
    game_clock.update();
    const f64 delta = game_clock.delta_sec();
    const f32 delta32 = static_cast<f32>(delta);
    if (time_count >= 1.0) {
        std::printf("avg: %fms\n", 1000.0 / frame_count);
        frame_count = 0;
        time_count -= 1.0;
    }
    time_count += delta;
    ++frame_count;

    constexpr f32 speed = 2.0f;
    if (input_state.up)
        camera.move({0.0f, -1.0f, 0.0f}, speed * delta32);
    if (input_state.down)
        camera.move({0.0f, 1.0f, 0.0f}, speed * delta32);
    if (input_state.left)
        camera.move({-1.0f, 0.0f, 0.0f}, speed * delta32);
    if (input_state.right)
        camera.move({1.0f, 0.0f, 0.0f}, speed * delta32);
    if (input_state.backward)
        camera.move({0.0f, 0.0f, -1.0f}, speed * delta32);
    if (input_state.forward)
        camera.move({0.0f, 0.0f, 1.0f}, speed * delta32);

    model_pipeline.queue_model(grass, {.position{0.0f, 0.0f, 0.0f}});
    model_pipeline.queue_model(sphere, {.position{-0.5f, -0.5f, 0.0f}, .scale{0.25f, 0.25f, 0.25f}});
    model_pipeline.queue_model(cube, {.position{0.5f, -0.5f, 0.0f}, .scale{0.25f, 0.25f, 0.25f}});

    model_pipeline.queue_model(grass, {.position{-1.0f, -0.25f, sqrt3}});
    model_pipeline.queue_model(building, {.position{-1.0f, -0.25f, sqrt3}});

    model_pipeline.queue_model(grass, {.position{1.0f, -0.5f, sqrt3}});
    model_pipeline.queue_model(tower, {.position{1.0f, -0.5f, sqrt3}});

    renderer.queue_light({-2.0f, -3.0f, -2.0f}, {glm::vec3{1.0f, 1.0f, 1.0f} * 300.0f});

    renderer.update_camera_and_lights(engine, camera);

    std::array<DefaultRenderer::Pipeline*, 2> pipelines{&skybox_pipeline, &model_pipeline};
    (void)renderer.draw(engine, pipelines);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void*, SDL_Event* event) {
    switch (event->type) {
        case SDL_EVENT_QUIT: return SDL_APP_SUCCESS;
        case SDL_EVENT_KEY_DOWN: {
            switch (event->key.key) {
                case SDLK_ESCAPE: return SDL_APP_SUCCESS;
                case SDLK_W: input_state.forward = true; break;
                case SDLK_S: input_state.backward = true; break;
                case SDLK_A: input_state.left = true; break;
                case SDLK_D: input_state.right = true; break;
                case SDLK_SPACE: input_state.up = true; break;
                case SDLK_LSHIFT: input_state.down = true; break;
            }
            break;
        }
        case SDL_EVENT_KEY_UP: {
            switch (event->key.key) {
                case SDLK_W: input_state.forward = false; break;
                case SDLK_S: input_state.backward = false; break;
                case SDLK_A: input_state.left = false; break;
                case SDLK_D: input_state.right = false; break;
                case SDLK_SPACE: input_state.up = false; break;
                case SDLK_LSHIFT: input_state.down = false; break;
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            switch (event->button.button) {
                case SDL_BUTTON_LEFT: input_state.left_mouse = true; break;
                case SDL_BUTTON_RIGHT: input_state.right_mouse = true; break;
            }
            break;
        }
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            switch (event->button.button) {
                case SDL_BUTTON_LEFT: input_state.left_mouse = false; break;
                case SDL_BUTTON_RIGHT: input_state.right_mouse = false; break;
            }
            break;
        }
        case SDL_EVENT_MOUSE_MOTION: {
            if (input_state.left_mouse) {
                constexpr f32 turn_speed = 0.003f;
                camera.rotate_external(glm::angleAxis<f32, glm::defaultp>(event->motion.xrel * turn_speed, {0.0f, 1.0f, 0.0f}));
                camera.rotate_internal(glm::angleAxis<f32, glm::defaultp>(event->motion.yrel * turn_speed, {-1.0f, 0.0f, 0.0f}));
            }
            break;
        }
    }

    return SDL_APP_CONTINUE;
}


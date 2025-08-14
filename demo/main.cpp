#include <hurdy_gurdy.h>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#define perr(e) LOG_ERROR(to_string(e.err()))

using namespace hg;

constexpr double sqrt3 = 1.73205080757;

static Vk vk{};
static AssetManager assets{};
static Window window{};
static PbrRenderer renderer{};

static PbrTextureHandle default_normals{};
static PbrTextureHandle perlin_normals{};
static PbrTextureHandle gray_texture{};
static PbrModelHandle cube{};
static PbrModelHandle sphere{};

static PbrTextureHandle hex_texture{};
static PbrModelHandle grass{};
static PbrModelHandle building{};
static PbrModelHandle tower{};

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
    bool sdl_success = SDL_Init(SDL_INIT_VIDEO);
    if (!sdl_success)
        ERRORF("Could not initialize SDL: {}", SDL_GetError());

    vk = [] {
        auto vk_res = create_vk();
        if (vk_res.has_err())
            ERRORF("Could not create Vulkan: {}", to_string(vk_res.err()));
        return std::move(*vk_res);
    }();

    assets = create_asset_manager({});

    window = [] {
        auto window_res = create_window(vk, {});
        if (window_res.has_err())
            perr(window_res);
        return *window_res;
    }();

    renderer = create_pbr_renderer(vk, {window});
    {
        auto cubemap = load_image(assets, "assets/cloudy_skyboxes/Cubemap/Cubemap_Sky_06-512x512.png");
        if (cubemap.has_err())
            perr(cubemap);
        DEFER(destroy_image(assets, *cubemap));

        load_skybox(renderer, assets, *cubemap);
    };

    default_normals = [&] {
        auto default_normal_image = generate_image<glm::vec4>(assets, {2, 2}, [](...) {
            return glm::vec4{0.0f, 0.0f, -1.0f, 0.0f};
        });
        DEFER(destroy_image(assets, default_normal_image));

        return create_texture(renderer, assets, {default_normal_image, VK_FORMAT_R32G32B32A32_SFLOAT});
    }();

    perlin_normals = [&] {
        auto perlin_noise = generate_image<f32>(assets, {512, 512}, [&](const auto pos) {
            return get_fractal_noise(pos, 1.0f, 32.0f, get_perlin_noise);
        });
        DEFER(destroy_image(assets, perlin_noise));

        auto perlin_normal_image = generate_image<glm::vec4>(assets, {512, 512}, [&](const auto pos) {
            return get_normal_from_heightmap(pos, assets.images[perlin_noise.handle]);
        });
        DEFER(destroy_image(assets, perlin_normal_image));

        return create_texture(renderer, assets, {perlin_normal_image, VK_FORMAT_R32G32B32A32_SFLOAT});
    }();

    gray_texture = [&] {
        auto gray_image = generate_image<u32>(assets, {2, 2}, [](...) { return 0xff777777; });
        DEFER(destroy_image(assets, gray_image));

        return create_texture(renderer, assets, {gray_image, VK_FORMAT_R8G8B8A8_SRGB});
    }();

    cube = [&] {
        auto cube_mesh = generate_cube(assets);
        DEFER(destroy_mesh(assets, cube_mesh));

        return create_model(renderer, assets, {{cube_mesh, 0.2f, 0.0f}, perlin_normals, gray_texture});
    }();

    sphere = [&] {
        auto sphere_mesh = generate_sphere(assets, {128, 64});
        DEFER(destroy_mesh(assets, sphere_mesh));

        return create_model(renderer, assets, {{sphere_mesh, 0.2f, 1.0f}, perlin_normals, gray_texture});
    }();

    hex_texture = [&] {
        auto hex_image = load_image(assets, "assets/hexagon_models/Textures/hexagons_medieval.png");
        if (hex_image.has_err())
            perr(hex_image);
        DEFER(destroy_image(assets, *hex_image));

        return create_texture(renderer, assets, {*hex_image, VK_FORMAT_R8G8B8A8_SRGB});
    }();

    grass = [&] {
        auto grass_mesh = load_gltf(assets, "assets/hexagon_models/Assets/gltf/tiles/base/hex_grass.gltf");
        if (grass_mesh.has_err())
            perr(grass_mesh);
        DEFER(unload_gltf(assets, *grass_mesh));

        return create_model(renderer, assets, {*grass_mesh, default_normals, hex_texture});
    }();

    building = [&] {
        auto building_mesh = load_gltf(assets,
            "assets/hexagon_models/Assets/gltf/buildings/blue/building_home_A_blue.gltf"
        );
        if (building_mesh.has_err())
            perr(building_mesh);
        DEFER(unload_gltf(assets, *building_mesh));

        return create_model(renderer, assets, {*building_mesh, default_normals, hex_texture});
    }();

    tower = [&] {
        auto tower_mesh = load_gltf(assets,
            "assets/hexagon_models/Assets/gltf/buildings/blue/building_tower_A_blue.gltf"
        );
        if (tower_mesh.has_err())
            perr(tower_mesh);
        DEFER(unload_gltf(assets, *tower_mesh));

        return create_model(renderer, assets, {*tower_mesh, default_normals, hex_texture});
    }();

    const auto window_size = window.swapchain.extent;
    const f32 aspect_ratio = static_cast<f32>(window_size.width) / static_cast<f32>(window_size.height);
    update_projection(renderer, glm::perspective(glm::pi<f32>() / 4.0f, aspect_ratio, 0.1f, 100.f));

    camera.translate({0.0f, -2.0f, -4.0f});
    game_clock.update();

    return SDL_APP_CONTINUE;
}

#ifndef NDEBUG
void SDL_AppQuit(void*, SDL_AppResult) {
    vkQueueWaitIdle(vk.queue);

    destroy_texture(renderer, default_normals);
    destroy_texture(renderer, perlin_normals);
    destroy_texture(renderer, gray_texture);
    destroy_model(renderer, cube);
    destroy_model(renderer, sphere);

    destroy_texture(renderer, hex_texture);
    destroy_model(renderer, grass);
    destroy_model(renderer, building);
    destroy_model(renderer, tower);

    destroy_pbr_renderer(renderer);
    destroy_window(vk, window);
    destroy_asset_manager(assets);
    destroy_vk(vk);

    SDL_Quit();
}
#endif

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

    constexpr f32 speed = 1.0f;
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

    std::array lights{
        make_light({-2.0f, -3.0f, -2.0f}, {1.0f, 1.0f, 1.0f}, 300.0f),
    };
    std::array models{
        PbrRenderer::ModelTicket{grass, {.position{0.0f, 0.0f, 0.0f}}},
        PbrRenderer::ModelTicket{sphere, {.position{-0.5f, -0.5f, 0.0f}, .scale{0.25f, 0.25f, 0.25f}}},
        PbrRenderer::ModelTicket{cube, {.position{0.5f, -0.5f, 0.0f}, .scale{0.25f, 0.25f, 0.25f}}},

        PbrRenderer::ModelTicket{grass, {.position{-1.0f, -0.25f, sqrt3}}},
        PbrRenderer::ModelTicket{building, {.position{-1.0f, -0.25f, sqrt3}}},

        PbrRenderer::ModelTicket{grass, {.position{1.0f, -0.5f, sqrt3}}},
        PbrRenderer::ModelTicket{tower, {.position{1.0f, -0.5f, sqrt3}}},
    };
    auto draw_res = draw_pbr(renderer, window, {&camera, lights, models});
    if (draw_res.has_err())
        perr(draw_res);

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


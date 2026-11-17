#include <hurdygurdy.hpp>

using namespace hg;

static volatile bool quit = false;

int main()
{
    std::unique_ptr<InputDevice> input = InputDevice::create();
    if (input == nullptr)
        HG_PANIC("Could not create input device: {}\n", std::string{getError()}.c_str());

    if (!initGpu(input->getVulkanExtensions()))
        HG_PANIC("Could not initialize GPU: {}\n", std::string{getError()}.c_str());
    HG_DEFER(deinitGpu());

    std::unique_ptr<Window> window = input->createWindow({});
    if (window == nullptr)
        HG_PANIC("Could not create window: {}\n", std::string{getError()}.c_str());
    window->setTitle("Hurdy Gurdy 3D Example");

    u32 width, height;
    window->size(&width, &height);
    f32 aspect = height == 0 ? 1 : static_cast<f32>(width) / static_cast<f32>(height);

    Renderer3D renderer{window->swapchainFormat(), Format_d32_sfloat};

    Camera camera{Perspective{aspect, pif / 2.0f, 0.1f, 100.0f}};
    camera.position = Vec3{0.0f, 1.5f, -5.0f};

    Mesh mesh{};

    MeshVertex vertices[] = {
        {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},

        {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 1.0f}, {0.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, 0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f, 1.0f}, {0.0f, 1.0f}},

        {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

        {{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},

        {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
        {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
    };

    u32 indices[] = {
        0,  2,  1,  0,  3,  2,
        4,  6,  5,  4,  7,  6,
        8,  10, 9,  8,  11, 10,
        12, 14, 13, 12, 15, 14,
        16, 18, 17, 16, 19, 18,
        20, 22, 21, 20, 23, 22,
    };

    mesh.vertexBuffer = GpuBuffer::create(
        sizeof(vertices), GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);
    mesh.indexBuffer = GpuBuffer::create(
        sizeof(indices), GpuBufferUsage_storageBuffer | GpuBufferUsage_transferDst);
    mesh.vertexBuffer.write(std::as_bytes(std::span{vertices}), 0);
    mesh.indexBuffer.write(std::as_bytes(std::span{indices}), 0);
    mesh.vertexCount = std::size(vertices);
    mesh.vertexWidth = sizeof(MeshVertex);
    mesh.indexCount = std::size(indices);

    Texture normalTex{};
    {
        normalTex.image = GpuImage::create(1, 1, Format_rgba32_sfloat,
            GpuImageUsage_sampled | GpuImageUsage_transferDst);
        normalTex.view = GpuView::create(normalTex.image, GpuAspect_color, GpuFilter_linear);

        Vec4 normalVals[1] = {{0, 0, 1, 0}};
        normalTex.view.write(std::as_bytes(std::span{normalVals}));
    }

    Texture whiteTex{};
    {
        whiteTex.image = GpuImage::create(1, 1, Format_rgba8_srgb,
            GpuImageUsage_sampled | GpuImageUsage_transferDst);
        whiteTex.view = GpuView::create(whiteTex.image, GpuAspect_color, GpuFilter_nearest);

        u8 whitePixel[4] = {255, 255, 255, 255};
        whiteTex.view.write(std::as_bytes(std::span{&whitePixel, 1}));
    }

    GpuImage depthImg = GpuImage::create(width, height, Format_d32_sfloat,
        GpuImageUsage_depthStencilAttachment);
    GpuView depthView = GpuView::create(depthImg, GpuAspect_depth);

    f32 modelRot = 0.0f;
    Vec3 modelPos{0.0f, 0.0f, 0.0f};

    Clock gameClock{};
    f64 elapsed = 0.0;
    for (;;)
    {
        f64 delta = gameClock.tick();
        elapsed += delta;

        input->processEvents();
        if (input->wasQuit() || window->wasClosed())
            goto quit;

        if (window->wasResized())
        {
            window->size(&width, &height);
            aspect = static_cast<f32>(width) / static_cast<f32>(height);
            camera.projection = Perspective{aspect, pif / 2.0f, 0.1f, 100.0f};

            depthImg = GpuImage::create(width, height, Format_d32_sfloat,
                GpuImageUsage_depthStencilAttachment);
            depthView = GpuView::create(depthImg, GpuAspect_depth);
        }

        if (input->isButtonDown(Button_lmouse))
        {
            f32 rotSpeed = 2.0f;
            Vec2 md = input->mouseDelta();
            Quat rotX = quatAxisAngle({ 0, 1, 0}, md.x() * rotSpeed);
            Quat rotY = quatAxisAngle({1, 0, 0}, md.y() * rotSpeed);
            camera.rotation = rotX * camera.rotation * rotY;
        }

        Vec3 movement = Vec3{0.0f};
        movement.y() += static_cast<f32>(input->isButtonDown(Button_space) - input->isButtonDown(Button_lshift));
        movement.x() += static_cast<f32>(input->isButtonDown(Button_d) - input->isButtonDown(Button_a));
        movement.z() += static_cast<f32>(input->isButtonDown(Button_w) - input->isButtonDown(Button_s));

        if (movement != Vec3{0.0f})
        {
            f32 moveSpeed = 1.5f * static_cast<f32>(delta);
            Vec3 rotated = rotate(camera.rotation, {movement.x(), 0.0f, movement.z()});
            camera.position += normalize(rotated) * moveSpeed;
        }

        modelRot += static_cast<f32>(delta) * 0.5f;
        camera.update();

        renderer.drawDirectionalLight(Vec3{0.03f}, {-0.3f, 0.8f, 0.5f});

        f32 time = static_cast<f32>(elapsed);
        Vec3 lightPos1{std::sin(time) * 3.0f, 1.5f, std::cos(time) * 3.0f};
        Vec3 lightPos2{std::sin(time + 2.094f) * 3.0f, 1.5f, std::cos(time + 2.094f) * 3.0f};
        Vec3 lightPos3{std::sin(time + 4.188f) * 3.0f, 1.5f, std::cos(time + 4.188f) * 3.0f};

        renderer.drawPointLight(Vec3{1.0f, 0.0f, 0.0f} * 3.0f, lightPos1);
        renderer.drawPointLight(Vec3{0.0f, 1.0f, 0.0f} * 3.0f, lightPos2);
        renderer.drawPointLight(Vec3{0.0f, 0.0f, 1.0f} * 3.0f, lightPos3);

        renderer.drawModel(mesh, whiteTex, normalTex, 1.0f, 1.0f, matModel3D(lightPos1, Vec3{0.15f}, 1.0f));
        renderer.drawModel(mesh, whiteTex, normalTex, 1.0f, 1.0f, matModel3D(lightPos2, Vec3{0.15f}, 1.0f));
        renderer.drawModel(mesh, whiteTex, normalTex, 1.0f, 1.0f, matModel3D(lightPos3, Vec3{0.15f}, 1.0f));

        Mat4 modelTransform = matModel3D(modelPos, Vec3{1.0f}, quatAxisAngle(Vec3{0.0f, 1.0f, 0.0f}, modelRot));
        renderer.drawModel(mesh, whiteTex, normalTex, 0.9f, 64.0f, modelTransform);

        for (i32 x = -2; x <= 2; ++x)
        {
            for (i32 z = -2; z <= 2; ++z)
            {
                if (x == 0 && z == 0)
                    continue;
                f32 hx = static_cast<f32>(x) * 2.0f;
                f32 hz = static_cast<f32>(z) * 2.0f;
                Mat4 t = matModel3D(Vec3{hx, 0.0f, hz}, Vec3{0.6f}, 1.0f);
                renderer.drawModel(mesh, whiteTex, normalTex, 0.7f, 32.0f, t);
            }
        }

        Window* windows[] = {window.get()};
        GpuCmd* cmd = gpuBeginFrame(windows);
        if (window->renderTarget() != nullptr)
        {
            GpuAttachment colorAttachment{};
            colorAttachment.image = window->renderTarget();
            colorAttachment.clearColor = Vec4{0.02f, 0.02f, 0.05f, 1.0f};

            GpuAttachment depthAttachment{};
            depthAttachment.image = &depthView;
            depthAttachment.clearDepthStencil = {1.0f, 0};

            GpuPass pass{};
            pass.colorAttachments = {&colorAttachment, 1};
            pass.depthAttachment = &depthAttachment;

            gpuBeginRenderPass(cmd, pass);

            renderer.renderScene(cmd, camera);

            gpuEndRenderPass(cmd);
        }
        gpuEndFrame(cmd);
    }

quit:
    quit = true;
    gpuWaitIdle();
}

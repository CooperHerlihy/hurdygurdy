#include <hurdygurdy.hpp>

using namespace hg;

static volatile bool quit = false;

int main()
{
    std::unique_ptr<InputDevice> input = InputDevice::create();
    if (input == nullptr)
        panic("Could not create input device: {}\n", std::string{getError()}.c_str());

    std::unique_ptr<Gpu> gpu = Gpu::create(input->getPlatformGraphicsExtensions());
    if (gpu == nullptr)
        panic("Could not initialize GPU: {}\n", std::string{getError()}.c_str());

    WindowConfig config{};
    config.imageUsage |= GpuImageUsage::transferSrc;

    std::unique_ptr<Window> window = input->createWindow(gpu.get(), config);
    if (window == nullptr)
        panic("Could not create window: {}\n", std::string{getError()}.c_str());
    window->setTitle("Hurdy Gurdy 3D Example");

    Renderer3D renderer{gpu.get(), window->swapchainFormat(), Format::d32_sfloat};

    f32 aspect = window->height() == 0 ? 1 : assertCast<f32>(window->width()) / assertCast<f32>(window->height());
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

    mesh.vertexBuffer = gpu->createBuffer(sizeof(vertices),
        GpuBufferUsage::storageBuffer | GpuBufferUsage::transferDst);
    mesh.indexBuffer = gpu->createBuffer(sizeof(indices),
        GpuBufferUsage::storageBuffer | GpuBufferUsage::transferDst);
    mesh.vertexBuffer->write(std::as_bytes(std::span{vertices}), 0);
    mesh.indexBuffer->write(std::as_bytes(std::span{indices}), 0);
    mesh.vertexCount = std::size(vertices);
    mesh.vertexWidth = sizeof(MeshVertex);
    mesh.indexCount = std::size(indices);

    Texture normalTex{};
    {
        normalTex.image = gpu->createImage(1, 1, Format::rgba32_sfloat,
            GpuImageUsage::sampled | GpuImageUsage::transferDst);
        normalTex.view = normalTex.image->createView(GpuFilter::linear);

        Vec4 normalVals[1] = {{0, 0, 1, 0}};
        normalTex.image->writeWhole(std::as_bytes(std::span{normalVals}));
    }

    Texture whiteTex{};
    {
        whiteTex.image = gpu->createImage(1, 1, Format::rgba8_srgb,
            GpuImageUsage::sampled | GpuImageUsage::transferDst);
        whiteTex.view = whiteTex.image->createView(GpuFilter::nearest);

        u8 whitePixel[4] = {255, 255, 255, 255};
        whiteTex.image->writeWhole(std::as_bytes(std::span{whitePixel}));
    }

    std::unique_ptr<GpuImage> depthImg = gpu->createImage(window->width(), window->height(), Format::d32_sfloat,
        GpuImageUsage::depthStencilAttachment);
    std::unique_ptr<GpuView> depthView = depthImg->createView();

    f32 modelRot = 0.0f;
    Vec3 modelPos{0.0f, 0.0f, 0.0f};

    Texture shot{};
    bool shotPending = false;

    Clock gameClock{};
    f64 elapsed = 0.0;
    bool wantScreenshot = false;
    for (;;)
    {
        f64 delta = gameClock.tick();
        elapsed += delta;

        input->processEvents();
        if (input->wasButtonPressed(Button::s)
            && (input->isButtonDown(Button::lctrl) || input->isButtonDown(Button::rctrl)))
            wantScreenshot = true;
        if (input->wasQuit() || window->wasClosed())
            goto quit;

        if (window->wasResized())
        {
            u32 width = window->width();
            u32 height = window->height();

            aspect = assertCast<f32>(width) / assertCast<f32>(height);
            camera.projection = Perspective{aspect, pif / 2.0f, 0.1f, 100.0f};

            depthImg = gpu->createImage(width, height, Format::d32_sfloat,
                GpuImageUsage::depthStencilAttachment);
            depthView = depthImg->createView();
        }

        if (input->isButtonDown(Button::lmouse))
        {
            f32 rotSpeed = 2.0f;
            Vec2 md = input->mouseDelta();
            Quat rotX = quatAxisAngle({ 0, 1, 0}, md.x() * rotSpeed);
            Quat rotY = quatAxisAngle({1, 0, 0}, md.y() * rotSpeed);
            camera.rotation = rotX * camera.rotation * rotY;
        }

        Vec3 movement = Vec3{0.0f};
        movement.y() += assertCast<f32>(input->isButtonDown(Button::space) - input->isButtonDown(Button::lshift));
        movement.x() += assertCast<f32>(input->isButtonDown(Button::d) - input->isButtonDown(Button::a));
        movement.z() += assertCast<f32>(input->isButtonDown(Button::w) - input->isButtonDown(Button::s));

        if (movement != Vec3{0.0f})
        {
            f32 moveSpeed = 1.5f * assertCast<f32>(delta);
            Vec3 rotated = rotate(camera.rotation, {movement.x(), 0.0f, movement.z()});;
            camera.position += normalize(Vec3{rotated.x(), movement.y(), rotated.z()}) * moveSpeed;
        }

        modelRot += assertCast<f32>(delta) * 0.5f;

        renderer.drawDirectionalLight(Vec3{0.03f}, {-0.3f, 0.8f, 0.5f});

        f32 time = assertCast<f32>(elapsed);
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
                f32 hx = assertCast<f32>(x) * 2.0f;
                f32 hz = assertCast<f32>(z) * 2.0f;
                Mat4 t = matModel3D(Vec3{hx, 0.0f, hz}, Vec3{0.6f}, 1.0f);
                renderer.drawModel(mesh, whiteTex, normalTex, 0.7f, 32.0f, t);
            }
        }

        Window* windows[] = {window.get()};
        std::unique_ptr<GpuCommandBuffer> cmd = gpu->beginFrame(windows);

        if (window->frameAvailable())
        {
            GpuAttachment colorAttachment{.clearColor{0.02f, 0.02f, 0.05f, 1.0f}};
            colorAttachment.image = window->currentView();

            GpuAttachment depthAttachment{};
            depthAttachment.image = depthView.get();
            depthAttachment.clearDepthStencil = {1.0f, 0};

            GpuPass pass{};
            pass.colorAttachments = {&colorAttachment, 1};
            pass.depthAttachment = &depthAttachment;

            cmd->beginRenderPass(pass);

            renderer.renderScene(cmd.get(), camera);

            cmd->endRenderPass();

            if (wantScreenshot)
            {
                wantScreenshot = false;
                if (shot.image == nullptr || shot.image->width() != window->currentImage()->width()
                    || shot.image->height() != window->currentImage()->height()
                    || shot.view->format() != window->currentImage()->format())
                {
                    shot.image = gpu->createImage(
                        window->currentImage()->width(),
                        window->currentImage()->height(),
                        Format::rgba8_srgb,
                        GpuImageUsage::sampled | GpuImageUsage::transferDst | GpuImageUsage::transferSrc);
                    shot.view = shot.image->createView();
                }
                cmd->blitImageWhole(shot.image.get(), window->currentImage());
                shotPending = true;
            }
        }

        gpu->endFrame(std::move(cmd));

        if (shotPending)
        {
            shotPending = false;
            gpu->waitIdle();
            if (!textureStorePng(downloadTextureFromGpu(shot), "screenshot3d.png"))
                panic("Could not save screenshot: {}\n", std::string{getError()}.c_str());
        }
    }

quit:
    quit = true;
    gpu->waitIdle();
}

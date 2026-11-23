#include <hurdygurdy.hpp>

using namespace hg;

static volatile bool quit = false;

int main()
{
    std::unique_ptr<InputDevice> input = InputDevice::create();
    if (input == nullptr)
        panic("Could not create input device: {}\n", std::string{getError()}.c_str());

    std::unique_ptr<Gpu> gpu = Gpu::create(input->getVulkanExtensions());
    if (gpu == nullptr)
        panic("Could not initialize GPU: {}\n", std::string{getError()}.c_str());

    WindowConfig config{};
    config.imageUsage |= GpuImageUsage::transferSrc;

    std::unique_ptr<Window> window = input->createWindow(gpu.get(), config);
    if (window == nullptr)
        panic("Could not create window: {}\n", std::string{getError()}.c_str());
    window->setTitle("Hurdy Gurdy 2D Example");

    Renderer2D renderer{gpu.get(), window->swapchainFormat()};

    f32 aspect = assertCast<f32>(window->width()) / assertCast<f32>(window->height());
    Camera camera{Orthographic{{{0, 0}, {aspect, 1.0f}}}};
    Layer2D layer{};

    struct Color {
        u8 r, g, b, a;
    };
    Color defaultColors[]{
        {0xff, 0x00, 0xff, 0xff}, {0x00, 0x00, 0x00, 0xff},
        {0x00, 0x00, 0x00, 0xff}, {0xff, 0x00, 0xff, 0xff},
    };

    Texture spriteTex{};
    spriteTex.image = gpu->createImage(2, 2, Format::rgba8_srgb,
        GpuImageUsage::sampled | GpuImageUsage::transferDst);
    spriteTex.image->writeWhole(std::as_bytes(std::span{defaultColors}));
    spriteTex.view = spriteTex.image->createView(GpuFilter::nearest);

    Vec2 spritePos{0.5f, 0.5f};
    f32 spriteRot = 0.0f;

    const TextureAtlas& font = getDefaultFont();

    Texture shot{};
    bool shotPending = false;

    Clock gameClock{};
    bool wantScreenshot = false;
    for (;;)
    {
        f64 delta = gameClock.tick();

        input->processEvents();
        if (input->wasButtonPressed(Button::s)
            && (input->isButtonDown(Button::lctrl) || input->isButtonDown(Button::rctrl)))
            wantScreenshot = true;
        if (input->wasQuit() || window->wasClosed())
            goto quit;

        if (window->wasResized())
        {
            aspect = assertCast<f32>(window->width()) / assertCast<f32>(window->height());
            camera.projection = Orthographic{{{0, 0}, {aspect, 1.0f}}};
        }

        if (input->isButtonDown(Button::lmouse))
        {
            Vec2 md = input->mouseDelta();
            camera.position.x() -= md.x();
            camera.position.y() -= md.y();
        }

        Vec2 spriteMove = {
            assertCast<f32>(input->isButtonDown(Button::d) - input->isButtonDown(Button::a)),
            assertCast<f32>(input->isButtonDown(Button::s) - input->isButtonDown(Button::w)),
        };
        if (spriteMove != Vec2{0.0f})
        {
            f32 moveSpeed = 0.3f;
            spritePos += normalize(spriteMove) * moveSpeed * assertCast<f32>(delta);
        }

        spriteRot += (input->isButtonDown(Button::e) - input->isButtonDown(Button::q)) * 2.0f * assertCast<f32>(delta);

        f32 sq = aspect < 1.0f ? aspect : 1.0f;
        f32 ox = (aspect - sq) / 2.0f;
        f32 oy = (1.0f - sq) / 2.0f;

        layer.clear();

        layer.drawRect({0.1f, 0.12f, 0.15f, 1.0f}, {Vec2{ox, oy}, Vec2{ox + sq, oy + sq}});

        Vec4 colors[] = {
            {0.9f, 0.2f, 0.2f, 1.0f},
            {0.2f, 0.9f, 0.2f, 1.0f},
            {0.2f, 0.2f, 0.9f, 1.0f},
            {0.9f, 0.9f, 0.2f, 1.0f},
            {0.9f, 0.2f, 0.9f, 1.0f},
            {0.2f, 0.9f, 0.9f, 1.0f},
        };
        f32 titleH = sq * 0.06f;
        f32 helpH = sq * 0.03f;
        f32 pad = sq * 0.01f;
        f32 gridTop = oy + titleH + helpH + pad * 3.0f;
        f32 gridH = oy + sq - gridTop;
        i32 cols = 5, rows = 4;
        f32 cellW = sq * 0.14f;
        f32 cellH = gridH * 0.2f;
        f32 gapX = (sq - assertCast<f32>(cols) * cellW) / assertCast<f32>(cols + 1);
        f32 gapY = (gridH - assertCast<f32>(rows) * cellH) / assertCast<f32>(rows + 1);
        for (i32 y = 0; y < rows; ++y)
        {
            for (i32 x = 0; x < cols; ++x)
            {
                Vec2 p = {
                    ox + gapX + assertCast<f32>(x) * (cellW + gapX),
                    gridTop + gapY + assertCast<f32>(y) * (cellH + gapY),
                };
                layer.drawRect(colors[(x + y * 3) % 6], {p, p + Vec2{cellW, cellH}});
            }
        }

        Vec2 spriteSize{sq * 0.08f, sq * 0.08f};
        Vec2 halfSprite = spriteSize / 2.0f;
        layer.drawSpriteRot(
            spriteTex,
            {Vec2{0}, Vec2{1}},
            {spritePos - halfSprite, spritePos + halfSprite},
            {0.5f, 0.5f},
            spriteRot);

        f32 textX = ox + sq / 2.0f;
        f32 titleY = oy + pad;
        TextBuilder2D titleBox(font);
        titleBox.setTopCenter({textX, titleY}).setHeight(titleH);
        layer.drawText("Hurdy Gurdy 2D", {1.0f, 1.0f, 1.0f, 1.0f}, titleBox);

        TextBuilder2D helpBox(font);
        helpBox
            .setTopCenter({textX, titleY + titleH + pad})
            .setHeight(helpH)
            .breakAtSpace();
        layer.drawText("WASD move   QE rotate   Mouse pan   Ctrl-S screenshot",
            {0.6f, 0.6f, 0.6f, 1.0f}, helpBox);

        renderer.drawLayer(layer);

        Window* windows[] = {window.get()};
        std::unique_ptr<GpuCommandBuffer> cmd = gpu->beginFrame(windows);

        if (window->currentView() != nullptr)
        {
            GpuAttachment colorAttachment{};
            colorAttachment.image = window->currentView();

            GpuPass pass{};
            pass.colorAttachments = {&colorAttachment, 1};

            cmd->beginRenderPass(pass);

            renderer.render(cmd.get(), camera);

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
            if (!textureStorePng(downloadTextureFromGpu(shot), "screenshot2d.png"))
                panic("Could not save screenshot: {}\n", std::string{getError()}.c_str());
        }
    }

quit:
    quit = true;
    gpu->waitIdle();
}

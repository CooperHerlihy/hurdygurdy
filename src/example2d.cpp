#include <hurdygurdy.hpp>

using namespace hg;

static volatile bool quit = false;

int main()
{
    HurdyGurdy hg = init().expect("Could not initialize Hurdy Gurdy\n");

    Window window = Window::create({});
    window.setTitle("Hurdy Gurdy 2D Example");

    Renderer2D renderer{window.swapchain().format()};

    u32 winW, winH;
    window.size(&winW, &winH);
    f32 aspect = static_cast<f32>(winW) / static_cast<f32>(winH);

    Camera camera{};
    camera.setOrthographic(aspect, 1.0f);

    Layer2D layer{};

    Sprite2D sprite = {nullptr, {Vec2{0}, Vec2{1}}};
    Vec2 spritePos{0.5f, 0.5f};
    f32 spriteRot = 0.0f;

    const Atlas2D& font = getDefaultFont();

    Clock gameClock{};
    for (;;)
    {
        f64 delta = gameClock.tick();

        processEvents();
        if (wasQuit() || window.wasClosed())
            goto quit;

        if (window.wasResized())
        {
            window.size(&winW, &winH);
            aspect = static_cast<f32>(winW) / static_cast<f32>(winH);
            camera.setOrthographic(aspect, 1.0f);
        }

        if (isButtonDown(Button_lmouse))
        {
            Vec2 md = mouseDelta();
            camera.position.x -= md.x;
            camera.position.y -= md.y;
        }
        camera.update();

        Vec2 spriteMove = {
            static_cast<f32>(isButtonDown(Button_d) - isButtonDown(Button_a)),
            static_cast<f32>(isButtonDown(Button_s) - isButtonDown(Button_w)),
        };
        if (spriteMove != Vec2{0.0f})
        {
            f32 moveSpeed = 0.3f;
            spritePos += vecNorm2(spriteMove) * moveSpeed * static_cast<f32>(delta);
        }

        spriteRot += (isButtonDown(Button_e) - isButtonDown(Button_q)) * 2.0f * static_cast<f32>(delta);

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
        f32 gapX = (sq - static_cast<f32>(cols) * cellW) / static_cast<f32>(cols + 1);
        f32 gapY = (gridH - static_cast<f32>(rows) * cellH) / static_cast<f32>(rows + 1);
        for (i32 y = 0; y < rows; ++y)
        {
            for (i32 x = 0; x < cols; ++x)
            {
                Vec2 p = {
                    ox + gapX + static_cast<f32>(x) * (cellW + gapX),
                    gridTop + gapY + static_cast<f32>(y) * (cellH + gapY),
                };
                layer.drawRect(colors[(x + y * 3) % 6], {p, p + Vec2{cellW, cellH}});
            }
        }

        Vec2 spriteSize{sq * 0.08f, sq * 0.08f};
        Vec2 halfSprite = spriteSize / 2.0f;
        layer.drawSpriteRot(sprite, {spritePos - halfSprite, spritePos + halfSprite}, {0.5f, 0.5f}, spriteRot);

        f32 textX = ox + sq / 2.0f;
        f32 titleY = oy + pad;
        TextBuilder titleBox(font);
        titleBox.setTopCenter({textX, titleY}).setHeight(titleH);
        layer.drawText("Hurdy Gurdy 2D", {1.0f, 1.0f, 1.0f, 1.0f}, titleBox);

        TextBuilder helpBox(font);
        helpBox.setTopCenter({textX, titleY + titleH + pad}).setHeight(helpH).breakAtSpace();
        layer.drawText("WASD move   QE rotate   Left mouse pan", {0.6f, 0.6f, 0.6f, 1.0f}, helpBox);

        renderer.queueLayer(layer);

        Window* windows[] = {&window};
        GpuCmd* cmd = gpuBeginFrame(windows);
        if (window.swapchain().renderTarget() != nullptr)
        {
            GpuAttachment colorAttachment{};
            colorAttachment.image = window.swapchain().renderTarget();

            GpuPass pass{};
            pass.colorAttachments = {&colorAttachment, 1};

            gpuBeginRenderPass(cmd, pass);

            renderer.render(cmd, camera);

            gpuEndRenderPass(cmd);
        }
        gpuEndFrame(cmd);
    }

quit:
    quit = true;
    gpuWaitIdle();
}

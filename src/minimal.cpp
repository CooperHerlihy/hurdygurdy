#include <hurdygurdy.hpp>

using namespace hg;

static volatile bool quit = false;

int main()
{
    HurdyGurdy hg = init().expect("Could not initialize Hurdy Gurdy\n");

    Window window = Window::create({
        // .preferredPresentMode = GpuPresentMode_mailbox,
    });
    window.setTitle("Hg Minimal Example");

    Renderer2D renderer{window.swapchain().format()};

    u32 width, height;
    window.size(&width, &height);

    Camera camera{};

    Layer2D layer{};

    Sprite2D sprite = {nullptr, {Vec2{0}, Vec2{1}}};
    Vec2 spriteSize{0.1f, 0.1f};
    Vec2 spritePos = (Vec2{static_cast<f32>(width) / static_cast<f32>(height), 1} - spriteSize) / 2.0f;

    Clock gameClock{};
    for (;;)
    {
        f64 delta = gameClock.tick();

        processEvents();
        if (wasQuit() || window.wasClosed())
            goto quit;

        if (window.wasResized())
        {
            window.size(&width, &height);
            camera.setOrthographic(static_cast<f32>(width) / static_cast<f32>(height), 1.0f);
        }

        if (isButtonDown(Button_lmouse))
        {
            Vec2 md = mouseDelta();
            f32 moveSpeed = 1.0f;
            camera.position.x -= md.x * moveSpeed;
            camera.position.y -= md.y * moveSpeed;
        }
        camera.update();

        Vec2 spriteMove = {
            static_cast<f32>(isButtonDown(Button_d) - isButtonDown(Button_a)),
            static_cast<f32>(isButtonDown(Button_s) - isButtonDown(Button_w)),
        };
        if (spriteMove != Vec2{0.0f})
        {
            f32 moveSpeed = 0.4f;
            spritePos += vecNorm2(spriteMove) * moveSpeed * static_cast<f32>(delta);
        }

        layer.clear();
        layer.drawRect({0.008f, 0.0f, 0.04f, 1}, {Vec2{0.25f}, Vec2{0.75f}});
        layer.drawSprite(sprite, {spritePos, spritePos + spriteSize});

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


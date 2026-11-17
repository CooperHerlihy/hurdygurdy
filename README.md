# Hurdy Gurdy

![License](https://img.shields.io/badge/license-MIT-blue)
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C)
![Platforms](https://img.shields.io/badge/platforms-Wayland%20%7C%20X11%20%7C%20Win32-lightgrey)
![Graphics](https://img.shields.io/badge/graphics-Vulkan-red)

Hurdy Gurdy is a modern C++20 game development library collection. I mostly
wrote it for fun and learning, but it should be usable to write real games. Most
subsystems are pretty modular, so use what you like, and not what you don't.

<p align="center">
  <img src="screenshots/example2d.png" width="49%" alt="Hurdy Gurdy 2D example">
  <img src="screenshots/example3d.png" width="49%" alt="Hurdy Gurdy 3D example">
</p>

## Feature List

TODO

## Quickstart

```cpp
#include <hurdygurdy.hpp>

using namespace hg;

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
    window->setTitle("Hello Hurdy Gurdy");

    Renderer2D renderer{window->swapchainFormat()};
    Layer2D layer{};

    Camera camera{Orthographic{{{0, 0}, {1.6f, 1.0f}}}};
    camera.update();

    for (;;)
    {
        input->processEvents();
        if (input->wasQuit() || window->wasClosed())
            break;

        layer.clear();
        layer.drawRect({0.1f, 0.12f, 0.15f, 1.0f}, {Vec2{0, 0}, Vec2{1.6f, 1.0f}});

        layer.drawText("Hello, Hurdy Gurdy!", {1.0f, 1.0f, 1.0f, 1.0f},
            TextBuilder2D{getDefaultFont()}
                .setTopCenter({0.8f, 0.4f})
                .setHeight(0.1f));

        renderer.drawLayer(layer);

        Window* windows[] = {window.get()};
        GpuCmd* cmd = gpuBeginFrame(windows);
        if (window->renderTarget() != nullptr)
        {
            GpuAttachment colorAttachment{};
            colorAttachment.image = window->renderTarget();

            GpuPass pass{};
            pass.colorAttachments = {&colorAttachment, 1};

            gpuBeginRenderPass(cmd, pass);
            renderer.render(cmd, camera);
            gpuEndRenderPass(cmd);
        }
        gpuEndFrame(cmd);
    }

    gpuWaitIdle();
}
```

Complete runnable versions live in [src/example2d.cpp](src/example2d.cpp) and
[src/example3d.cpp](src/example3d.cpp).

## Integration

With [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html):

```cmake
include(FetchContent)
FetchContent_Declare(hurdygurdy
    GIT_REPOSITORY https://github.com/CooperHerlihy/hurdygurdy
    GIT_TAG        main
)
FetchContent_MakeAvailable(hurdygurdy)

target_link_libraries(my_game hurdygurdy)
```

Or clone and use `add_subdirectory`:

```cmake
add_subdirectory(path/to/hurdygurdy)
target_link_libraries(my_game hurdygurdy)
```

Or link manually:

```bash
c++ -std=c++20 -Ipath/to/hurdygurdy/include \
    -Lpath/to/hurdygurdy/build -lhurdygurdy my_game.cpp
```

## Building

### Dependencies

- C++20 compiler (Clang 19+, GCC 15+, or MSVC)
- CMake (3.18+)
- SPIR-V compiler (glslc)
  - Windows: included in LunarG Vulkan SDK
  - Linux: install individually or in the LunarG Vulkan SDK
- Windows only: Windows SDK (installed with Visual Studio)

On Nix, `nix develop` provides all dependencies.

### Compilation

```bash
# Linux:
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Windows: open a Visual Studio 2022 x64 developer shell first, then:
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

On Nix, `nix build` compiles a standalone release build.

## Testing

Tests and examples are built together with the library:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Then run the tests and examples:

```bash
./build/tests
./build/example2d
./build/example3d
```

Both examples save a screenshot with Ctrl-S.

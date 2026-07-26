# Contents

Implementation files use the same order as header files.
Use `grep` to find specific symbols.

## include/hurdygurdy.hpp

Umbrella header. Includes all sub-headers in order.

## include/hurdygurdy/

- Config Macros
    - config.hpp - platform detection
- Core Types
    - core.hpp - i32, u8, f64, etc., StringView, BinaryView, Span, Product, Sum, Maybe
- Error Handling
    - error.hpp - getError, setError, logError
- Utility Macros
    - macros.hpp - HG_MACRO_CONCAT, HG_DEFER, HG_LOG, HG_WARN, HG_PANIC, HG_ASSERT
- Initialization
    - init.hpp - HurdyGurdy scope guard
- Core types (Implementations)
- Utility Functions
    - utility.hpp - isPowerOf2, align, endianReverse
- Memory
    - memory.hpp - heap alloc/realloc/free, Arena, ArenaScope, scratch
- Concurrency
    - concurrency.hpp - Spinlock, Fence, thread pool, parallel for
- Math
    - math.hpp - constants and util functions, Vec2/3/4, Mat2/3/4, Complex, Quat, model view projection matrices
- Geometry 2D
    - geometry_2d.hpp - Circle, Rect, Ray, Line
- Geometry 3D
    - geometry_3d.hpp - Sphere, Box, Tri, Plane, Ray, Line
- Noise & RNG
    - noise.hpp - white noise, value noise, perlin noise, true random, Rng
- Strings
    - strings.hpp - cString, StringBuilder, String, parsing
- Containers
    - containers.hpp - BinaryBuilder, Binary, UniquePtr, SharedPtr, Array, Queue, Set, Map, Pool, HandlePool
- Asset System
    - asset.hpp - Asset, AssetManager, load/unload templates
- Serialization
    - serialization.hpp - Serializer, binary format, JSON
- Timing
    - timing.hpp - Clock, Perf
- Dynamic Library
    - dynlib.hpp - Library
- GPU
    - gpu.hpp - Format enum, GpuBuffer, GpuImage, GpuView, GpuPipeline, GpuCmd, barriers and passes
- Windowing & Input
    - window.hpp - Button, WindowEvent, Window, gpuFrameBegin/End
- Audio
    - audio.hpp - AudioStream, Sound, AudioPlayer
- Rendering
    - rendering.hpp - Texture, Mesh, Camera, 2D renderer (Sprite2D, Atlas2D, Tilemap2D, Layer2D), ImGui
- ECS (commented out)
    - ecs.hpp - Entity, Ecs, Node, Transform, old 3D rendering
- Template Implementations
    - templates.hpp - out-of-line template bodies

## src/

- core.cpp - error handling, init, Arena, heap alloc, scratch
- concurrency.cpp - SpinLock, Fence, thread pool, forPar
- math.cpp - math ops, 2D/3D geometry, noise
- strings.cpp - StringBuilder, String, parsing
- containers.cpp - BinaryBuilder, Binary, HandlePool
- serialization.cpp - Serializer, binary serial
- timing.cpp - Clock, Perf, sleep
- audio.cpp - Sound asset loading, AudioPlayer
- asset_io.cpp - Texture, Mesh, Binary file I/O
- camera.cpp - Camera create/update
- render2d.cpp - Renderer 2D init, Atlas, Tilemap, Layer
- platform.cpp - Vulkan/SDL/platform init, GPU ops, windowing, ImGui
- vulkan_stubs.cpp - C-linkage Vulkan PFN stubs
- dynlib.cpp - Library::load/findFunction
- hurdygurdy.cpp - JSON parser (commented out), ECS (commented out)

## hurdygurdy.glsl

- bindless resource bindings
- math
    - constants
    - square
    - smoothing
- noise
    - white noise
    - unit vector
    - value noise
    - perlin noise
    - TODO: simplex noise
    - TODO: worley noise
    - fractal noise macro
- geometry
    - vertex transform
    - normal map transform
- post processing
    - color grading : TODO
    - tonemapping
        - Reinhard
        - Uncharted 2
        - ACES approx and fitted
        - PBR Neutral
- lighting
    - Lambertian diffuse
    - Blinn-Phong specular
    - TODO: Cooke-Torrence


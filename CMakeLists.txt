cmake_minimum_required(VERSION 3.18)

project(hurdygurdy VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

if(MSVC)
    add_compile_options(/permissive- /GR- /D_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/wd4702 /wd4127)
    set(STRICT_FLAGS /W4 /WX)
    set(TESTS_EXCEPTION_FLAG /EHsc)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wno-cpp")
    set(STRICT_FLAGS -Wall -Wextra -Wconversion -Wsign-conversion -pedantic -Werror)
    set(TESTS_EXCEPTION_FLAG -fexceptions)

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CLANG_VENDOR_FLAGS
            -Wno-nullability-extension
            -Wno-nullability-completeness
        )
    endif()

    find_program(MOLD_PROGRAM mold)
    if(MOLD_PROGRAM)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
    endif()
endif()

set(VulkanHeaders_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/Vulkan-Headers/include)

# Platform detection
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(HG_LINUX TRUE)
endif()
if(WIN32)
    set(HG_WINDOWS TRUE)
endif()

option(HG_USE_SDL "Use SDL backend" OFF)
if(HG_USE_SDL)
    find_package(SDL3 QUIET)
    if(SDL3_FOUND)
        message(STATUS "Using system SDL3")
        set(SDL_INCLUDE_DIR "")
    else()
        set(SDL_SHARED OFF CACHE BOOL "" FORCE)
        set(SDL_STATIC ON CACHE BOOL "" FORCE)
        set(SDL_TEST OFF CACHE BOOL "" FORCE)
        set(SDL_TESTS OFF CACHE BOOL "" FORCE)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL/CMakeLists.txt")
            message(STATUS "Using vendored SDL3")
            add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)
        else()
            message(STATUS "SDL3 not found, fetching from GitHub")
            include(FetchContent)
            FetchContent_Declare(SDL
                GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
                GIT_TAG release-3.4.10
            )
            FetchContent_MakeAvailable(SDL)
        endif()
        set(SDL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL/include)
    endif()
endif()

set(INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/wayland
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends
    ${SDL_INCLUDE_DIR}
)

# Shaders

add_executable(embed src/embed.c)

set(SHADERS
    shaders/render2d.vert
    shaders/render2d.frag
    shaders/debug2d.frag
    shaders/skybox.vert
    shaders/skybox.frag
    shaders/sprite.vert
    shaders/sprite.frag
    shaders/model.vert
    shaders/model.frag
    test/compute.comp
    test/tri.vert
    test/tri.frag
    test/sampler.frag
    test/mul.comp
    test/buf_to_img.comp
    test/uniform.vert
    test/uniform.frag
    test/storage_img.comp
    test/depth.vert
    test/depth.frag
    test/instance.vert
    test/instance.frag
    test/blur.comp
    test/invert.frag
)

set(SHADER_HEADERS)
foreach(shader ${SHADERS})
    set(spv ${CMAKE_CURRENT_BINARY_DIR}/${shader}.spv)
    set(spv_h ${CMAKE_CURRENT_BINARY_DIR}/${shader}.spv.h)
    get_filename_component(spv_dir ${spv} DIRECTORY)
    add_custom_command(
        OUTPUT ${spv_h}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${spv_dir}
        COMMAND glslc -o ${spv} ${CMAKE_CURRENT_SOURCE_DIR}/src/${shader} -I${CMAKE_CURRENT_SOURCE_DIR}/include
        COMMAND embed ${spv} ${shader}.spv ${spv_h}
        DEPENDS embed
            ${CMAKE_CURRENT_SOURCE_DIR}/src/${shader}
            ${CMAKE_CURRENT_SOURCE_DIR}/include/hurdygurdy.glsl
        COMMENT "Compiling ${shader}"
    )
    list(APPEND SHADER_HEADERS ${spv_h})
endforeach()

add_custom_target(shaders DEPENDS ${SHADER_HEADERS})

# Sources

file(GLOB IMGUI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/*.cpp)

set(VULKAN_SOURCES
    src/vulkan/loader.cpp
    src/vulkan/vulkan.cpp
    src/vulkan/gpu.cpp
)

set(PLATFORM_SOURCES)
if(HG_LINUX)
    set(PLATFORM_SOURCES
        src/wayland/wayland.cpp
        src/wayland/wayland-protocol.c
        src/x11/x11.cpp
        src/pipewire/pipewire.cpp
    )
elseif(HG_WINDOWS)
    set(PLATFORM_SOURCES
        src/win32/win32.cpp
        src/win32/window.cpp
        src/win32/audio.cpp
    )
endif()

if(HG_USE_SDL)
    set(SDL_SOURCES
        src/sdl/sdl.cpp
        src/sdl/window.cpp
        src/sdl/audio.cpp
    )
endif()

set(CORE_SOURCES
    src/error.cpp
    src/init.cpp
    src/memory.cpp
    src/concurrency.cpp
    src/math.cpp
    src/geometry2d.cpp
    src/geometry3d.cpp
    src/noise.cpp
    src/strings.cpp
    src/binary.cpp
    src/filesystem.cpp
    src/pool.cpp
    src/serialization.cpp
    src/time.cpp
    src/audio.cpp
    src/assets.cpp
    src/dynlib.cpp
    src/render.cpp
    src/imgui.cpp
    ${PLATFORM_SOURCES}
    ${VULKAN_SOURCES}
    ${SDL_SOURCES}
)

# Object libraries

add_library(hurdygurdy_imgui OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_vulkan.cpp
    ${IMGUI_SOURCES}
)
target_include_directories(hurdygurdy_imgui
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)
target_compile_definitions(hurdygurdy_imgui PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
target_compile_options(hurdygurdy_imgui PRIVATE ${CLANG_VENDOR_FLAGS})

add_library(hurdygurdy_misc OBJECT
    src/vk_mem_alloc.cpp
    src/stb.c
)
target_include_directories(hurdygurdy_misc
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)
target_compile_options(hurdygurdy_misc PRIVATE ${CLANG_VENDOR_FLAGS})

add_library(hurdygurdy_core OBJECT ${CORE_SOURCES})
add_dependencies(hurdygurdy_core shaders)
target_include_directories(hurdygurdy_core
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)

if(HG_LINUX)
    target_compile_definitions(hurdygurdy_core PRIVATE HG_USE_WAYLAND=1)
    target_include_directories(hurdygurdy_core SYSTEM PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/xorgproto/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libX11/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libXrandr/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libXrender/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/pipewire/src
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/pipewire/spa/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libxkbcommon/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libevdev
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/wayland/src
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/wayland/egl
    )
elseif(HG_WINDOWS)
    target_compile_definitions(hurdygurdy_core PRIVATE HG_USE_WIN32_BACKEND=1)
endif()

if(HG_USE_SDL)
    target_compile_definitions(hurdygurdy_core PRIVATE HG_USE_SDL=1)
endif()

if(MSVC)
    target_compile_options(hurdygurdy_core PRIVATE /EHs-c-)
endif()
target_compile_options(hurdygurdy_core PRIVATE ${STRICT_FLAGS} ${CLANG_VENDOR_FLAGS})
target_compile_definitions(hurdygurdy_core PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

# Static library

add_library(hurdygurdy STATIC)
target_link_libraries(hurdygurdy PUBLIC hurdygurdy_core hurdygurdy_imgui hurdygurdy_misc)
if(HG_USE_SDL)
    target_link_libraries(hurdygurdy PRIVATE SDL3::SDL3)
endif()
if(HG_WINDOWS)
    target_link_libraries(hurdygurdy PRIVATE ole32 xinput dwmapi)
endif()

# Executables

add_executable(tests
    src/test/tests.cpp
    src/test/span.cpp
    src/test/product.cpp
    src/test/sum.cpp
    src/test/maybe.cpp
    src/test/utility.cpp
    src/test/memory.cpp
    src/test/concurrency.cpp
    src/test/math.cpp
    src/test/geometry2d.cpp
    src/test/geometry3d.cpp
    src/test/noise.cpp
    src/test/strings.cpp
    src/test/binary.cpp
    src/test/filesystem.cpp
    src/test/smart_ptr.cpp
    src/test/array.cpp
    src/test/array_temp.cpp
    src/test/queue.cpp
    src/test/set.cpp
    src/test/map.cpp
    src/test/hash.cpp
    src/test/pool.cpp
    src/test/assets.cpp
    src/test/serialization.cpp
    src/test/gpu.cpp
    src/test/render.cpp
    src/test/ecs.cpp
    src/test/ecs_foreach.cpp
    src/test/ecs_serialize.cpp
)
target_link_libraries(tests hurdygurdy)
target_precompile_headers(tests PRIVATE <hurdygurdy.hpp>)
target_compile_options(tests PRIVATE ${STRICT_FLAGS} ${TESTS_EXCEPTION_FLAG})
target_compile_definitions(tests PRIVATE HG_LOGGING=1)

foreach(tgt minimal platform_test)
    add_executable(${tgt} src/${tgt}.cpp)
    target_link_libraries(${tgt} hurdygurdy)
    if(MSVC)
        target_compile_options(${tgt} PRIVATE /EHs-c-)
    endif()
    target_compile_options(${tgt} PRIVATE ${STRICT_FLAGS})
endforeach()

# Install

install(TARGETS hurdygurdy ARCHIVE DESTINATION lib)
install(TARGETS tests minimal platform_test RUNTIME DESTINATION bin)
install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.glsl"
)

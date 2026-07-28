cmake_minimum_required(VERSION 3.18)

project(hurdygurdy VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/hg_test_dir)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

if(MSVC)
    add_compile_options(/permissive- /GR- /D_CRT_SECURE_NO_WARNINGS)
    # C4702 fires spuriously in the Asset<T>::Asset constructor initializer on
    # optimized builds (MSVC proves pool.alloc() never returns null, then warns
    # about the "unreachable" else-branch that doesn't exist)
    # C4127 flags conditionals which could be compile time, but are at runtime
    add_compile_options(/wd4702 /wd4127)
    set(STRICT_FLAGS /W4 /WX)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -Wno-cpp")
    set(STRICT_FLAGS -Wall -Wextra -Wconversion -Wsign-conversion -pedantic -Werror)

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

set(VulkanHeaders_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/vendor/Vulkan-Headers/include)
set(SDL_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/vendor/SDL/include)

set(INCLUDE_DIRS
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
    ${SDL_INCLUDE_DIR}
)

add_executable(embed src/embed.c)

set(SHADERS
    render2d.vert
    render2d.frag
    debug2d.frag
    skybox.vert
    skybox.frag
    sprite.vert
    sprite.frag
    model.vert
    model.frag
    test_compute.comp
    test_tri.vert
    test_tri.frag
    test_sampler.frag
    test_mul.comp
    test_buf_to_img.comp
    test_uniform.vert
    test_uniform.frag
    test_storage_img.comp
    test_depth.vert
    test_depth.frag
    test_instance.vert
    test_instance.frag
    test_blur.comp
    test_invert.frag
)

set(SHADER_HEADERS)
foreach(shader ${SHADERS})
    set(spv ${CMAKE_BINARY_DIR}/${shader}.spv)
    set(spv_h ${CMAKE_BINARY_DIR}/${shader}.spv.h)

    add_custom_command(
        OUTPUT ${spv_h}
        COMMAND glslc -o ${spv} ${CMAKE_SOURCE_DIR}/src/${shader} -I${CMAKE_SOURCE_DIR}/include
        COMMAND embed ${spv} ${shader}.spv ${spv_h}
        DEPENDS embed
            ${CMAKE_SOURCE_DIR}/src/${shader}
            ${CMAKE_SOURCE_DIR}/include/hurdygurdy.glsl
        COMMENT "Compiling ${shader} -> header"
    )

    list(APPEND SHADER_HEADERS ${spv_h})
endforeach()

add_custom_target(shaders DEPENDS ${SHADER_HEADERS})

file(GLOB IMGUI_SOURCES ${CMAKE_SOURCE_DIR}/vendor/imgui/*.cpp)
set(IMGUI_BACKEND_SOURCES
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_sdl3.cpp
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_vulkan.cpp
)

set(VULKAN_SOURCES
    src/vulkan/loader.cpp
    src/vulkan/vulkan.cpp
    src/vulkan/gpu.cpp
)

set(SDL_SOURCES
    src/sdl/loader.cpp
    src/sdl/sdl.cpp
    src/sdl/window.cpp
    src/sdl/audio.cpp
)

set(CORE_SOURCES
    src/error.cpp
    src/init.cpp
    src/memory.cpp
    src/concurrency.cpp
    src/math.cpp
    src/geometry_2d.cpp
    src/geometry_3d.cpp
    src/noise.cpp
    src/strings.cpp
    src/containers.cpp
    src/serialization.cpp
    src/timing.cpp
    src/audio.cpp
    src/asset_io.cpp
    src/camera.cpp
    src/render2d.cpp
    src/dynlib.cpp
    src/imgui.cpp
    ${SDL_SOURCES}
    ${VULKAN_SOURCES}
)

set(MISC_VENDOR_SOURCES
    src/vk_mem_alloc.cpp
    src/stb.c
)

add_library(hurdygurdy_core OBJECT ${CORE_SOURCES})
add_dependencies(hurdygurdy_core shaders)
target_include_directories(hurdygurdy_core
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)

target_compile_options(hurdygurdy_core PRIVATE ${STRICT_FLAGS} ${CLANG_VENDOR_FLAGS})

add_library(hurdygurdy_imgui OBJECT ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})
target_include_directories(hurdygurdy_imgui
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)

target_compile_definitions(hurdygurdy_imgui PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
target_compile_options(hurdygurdy_imgui PRIVATE ${CLANG_VENDOR_FLAGS})

add_library(hurdygurdy_misc OBJECT ${MISC_VENDOR_SOURCES})
target_include_directories(hurdygurdy_misc
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)

target_compile_options(hurdygurdy_misc PRIVATE ${CLANG_VENDOR_FLAGS})

add_library(hurdygurdy STATIC)
target_link_libraries(hurdygurdy PUBLIC hurdygurdy_core hurdygurdy_imgui hurdygurdy_misc)

foreach(tgt test editor minimal)
    add_executable(${tgt} src/${tgt}.cpp)
    target_link_libraries(${tgt} hurdygurdy)
    target_include_directories(${tgt} PRIVATE ${CMAKE_BINARY_DIR})
    target_compile_options(${tgt} PRIVATE ${STRICT_FLAGS})
    if(tgt STREQUAL test)
        target_compile_definitions(${tgt} PRIVATE HG_LOGGING=1)
    endif()
endforeach()

install(TARGETS hurdygurdy
    ARCHIVE DESTINATION lib
)

install(TARGETS test editor minimal
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.glsl"
)


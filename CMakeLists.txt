cmake_minimum_required(VERSION 3.18)
project(hurdygurdy VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
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
    set(STRICT_FLAGS /W4 /WX)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fsanitize=undefined -Wno-cpp")
    set(STRICT_FLAGS -Wall -Wextra -Wconversion -Wsign-conversion -pedantic -Werror)

    find_program(MOLD_PROGRAM mold)
    if(MOLD_PROGRAM)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fuse-ld=mold")
    endif()
endif()

find_package(SDL3 QUIET)
if(SDL3_FOUND)
    set(SDL3_TARGET SDL3::SDL3)
    message(STATUS "SDL3 found via CMake config")
else()
    message(STATUS "SDL3 not found on system, downloading from GitHub...")
    include(FetchContent)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(sdl3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.10
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(sdl3)
    set(SDL3_TARGET SDL3::SDL3)
endif()

find_path(VulkanHeaders_INCLUDE_DIR vulkan/vulkan.h REQUIRED)

set(INCLUDE_DIRS
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/vendor/imgui
    ${CMAKE_SOURCE_DIR}/vendor/imgui/backends
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

set(CORE_SOURCES
    src/hurdygurdy.cpp
    src/platform.cpp
)

set(MISC_VENDOR_SOURCES
    src/vk_mem_alloc.cpp
    src/stb.c
)

# Core C++ library (with precompiled header for hurdygurdy.hpp)
add_library(hurdygurdy_core OBJECT ${CORE_SOURCES})
add_dependencies(hurdygurdy_core shaders)
target_include_directories(hurdygurdy_core
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)
target_link_libraries(hurdygurdy_core
    PUBLIC ${SDL3_TARGET}
)
target_precompile_headers(hurdygurdy_core PRIVATE include/hurdygurdy.hpp)
target_compile_options(hurdygurdy_core PRIVATE ${STRICT_FLAGS})

# ImGui library (with precompiled header for imgui.h)
add_library(hurdygurdy_imgui OBJECT ${IMGUI_SOURCES} ${IMGUI_BACKEND_SOURCES})
target_include_directories(hurdygurdy_imgui
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)
target_link_libraries(hurdygurdy_imgui
    PUBLIC ${SDL3_TARGET}
)
target_precompile_headers(hurdygurdy_imgui PRIVATE vendor/imgui/imgui.h)
target_compile_definitions(hurdygurdy_imgui PRIVATE IMGUI_DEFINE_MATH_OPERATORS)

# Misc vendor (C files — no precompiled header)
add_library(hurdygurdy_misc OBJECT ${MISC_VENDOR_SOURCES})
target_include_directories(hurdygurdy_misc
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)
target_link_libraries(hurdygurdy_misc
    PUBLIC ${SDL3_TARGET}
)

# Combined library
add_library(hurdygurdy STATIC)
target_link_libraries(hurdygurdy PUBLIC hurdygurdy_core hurdygurdy_imgui hurdygurdy_misc)

foreach(tgt test editor minimal)
    add_executable(${tgt} src/${tgt}.cpp)
    target_link_libraries(${tgt} hurdygurdy)
    target_precompile_headers(${tgt} REUSE_FROM hurdygurdy_core)
    target_compile_options(${tgt} PRIVATE ${STRICT_FLAGS})
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


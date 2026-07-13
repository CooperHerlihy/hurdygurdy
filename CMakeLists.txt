cmake_minimum_required(VERSION 3.16)
project(hurdygurdy VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/hg_test_dir)

if(MSVC)
    add_compile_options(/permissive- /GR- /D_CRT_SECURE_NO_WARNINGS)
    set(STRICT_FLAGS /W4 /WX)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-exceptions -fno-rtti")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -fsanitize=undefined -Wno-cpp")
    set(STRICT_FLAGS -Wall -Wextra -Wconversion -Wsign-conversion -pedantic -Werror)
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

set(VENDOR_SOURCES
    src/vk_mem_alloc.cpp
    src/stb.c
    ${IMGUI_SOURCES}
    ${IMGUI_BACKEND_SOURCES}
)

add_library(hurdygurdy STATIC
    ${CORE_SOURCES}
    ${VENDOR_SOURCES}
)

add_dependencies(hurdygurdy shaders)

target_include_directories(hurdygurdy
    PUBLIC
        ${INCLUDE_DIRS}
        ${VulkanHeaders_INCLUDE_DIR}
)

target_link_libraries(hurdygurdy
    PUBLIC ${SDL3_TARGET}
)

set_property(SOURCE ${CORE_SOURCES}
    APPEND PROPERTY COMPILE_OPTIONS ${STRICT_FLAGS}
)

foreach(tgt test editor minimal)
    add_executable(${tgt} src/${tgt}.cpp)
    target_link_libraries(${tgt} hurdygurdy)
    set_property(SOURCE src/${tgt}.cpp
        APPEND PROPERTY COMPILE_OPTIONS ${STRICT_FLAGS}
    )
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


cmake_minimum_required(VERSION 3.21)

project(hurdygurdy VERSION 0.1.0 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
include(cmake/Options.cmake)
include(cmake/Compiler.cmake)
include(cmake/Dependencies.cmake)
include(cmake/Embed.cmake)

# Resources

add_executable(embed src/embed.c)
target_compile_features(embed PRIVATE c_std_11)

set(RESOURCES
    resources/pixel_font.hg
)

set(RESOURCE_HEADERS)
foreach(resource ${RESOURCES})
    hg_embed_resource(${resource} header)
    list(APPEND RESOURCE_HEADERS ${header})
endforeach()

add_custom_target(resources DEPENDS ${RESOURCE_HEADERS})

# Shaders

set(LIBRARY_SHADERS
    src/shaders/render2d.vert
    src/shaders/render2d.frag
    src/shaders/render3d.vert
    src/shaders/render3d.frag
)

set(TEST_SHADERS
    test/shaders/compute.comp
    test/shaders/tri.vert
    test/shaders/tri.frag
    test/shaders/sampler.frag
    test/shaders/mul.comp
    test/shaders/buf_to_img.comp
    test/shaders/uniform.vert
    test/shaders/uniform.frag
    test/shaders/storage_img.comp
    test/shaders/depth.vert
    test/shaders/depth.frag
    test/shaders/instance.vert
    test/shaders/instance.frag
    test/shaders/blur.comp
    test/shaders/invert.frag
    test/shaders/noise.comp
)

set(LIBRARY_SHADER_HEADERS)
foreach(shader ${LIBRARY_SHADERS})
    hg_compile_shader(${shader} header)
    list(APPEND LIBRARY_SHADER_HEADERS ${header})
endforeach()

add_custom_target(shaders DEPENDS ${LIBRARY_SHADER_HEADERS})

# Sources

set(VULKAN_SOURCES
    src/vulkan/loader.cpp
    src/vulkan/vulkan.cpp
)

if(HG_LINUX)
    set(PLATFORM_SOURCES)
    if(HG_USE_WAYLAND)
        list(APPEND PLATFORM_SOURCES
            src/wayland/wayland.cpp
            src/wayland/wayland-protocol.c
        )
    endif()
    if(HG_USE_X11)
        list(APPEND PLATFORM_SOURCES
            src/x11/x11.cpp
        )
    endif()
    if(HG_USE_PIPEWIRE)
        list(APPEND PLATFORM_SOURCES
            src/pipewire/pipewire.cpp
        )
    endif()
elseif(HG_WINDOWS)
    set(PLATFORM_SOURCES
        src/win32/win32.cpp
    )
endif()

set(CORE_SOURCES
    src/utility.cpp
    src/memory.cpp
    src/concurrency.cpp
    src/math.cpp
    src/geometry.cpp
    src/noise.cpp
    src/serialization.cpp
    src/json.cpp
    src/time.cpp
    src/dynlib.cpp
    src/gpu.cpp
    src/window.cpp
    src/audio.cpp
    src/render2d.cpp
    src/render3d.cpp
    src/imgui.cpp
    ${VULKAN_SOURCES}
    ${PLATFORM_SOURCES}
)

if(HG_USE_SDL)
    list(APPEND CORE_SOURCES src/sdl/sdl.cpp)
endif()

file(GLOB IMGUI_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/*.cpp)

set(HG_PRIVATE_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/wayland
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends
    ${SDL_INCLUDE_DIR}
    ${VulkanHeaders_INCLUDE_DIR}
)

# Object libraries

add_library(hurdygurdy_imgui OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends/imgui_impl_vulkan.cpp
    ${IMGUI_SOURCES}
)
target_include_directories(hurdygurdy_imgui PRIVATE ${HG_PRIVATE_INCLUDE_DIRS})
target_compile_features(hurdygurdy_imgui PRIVATE cxx_std_20)
target_compile_definitions(hurdygurdy_imgui PRIVATE
    IMGUI_DEFINE_MATH_OPERATORS
    ${HG_CXX_DEFINITIONS}
)
target_compile_options(hurdygurdy_imgui PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${HG_CXX_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${HG_VENDOR_FLAGS}>
)

add_library(hurdygurdy_vendor OBJECT
    src/stb_image.c
    src/stb_image_write.c
    src/dr_flac.c
    src/dr_mp3.c
    src/dr_wav.c
    src/cppimpl.cpp
)
target_include_directories(hurdygurdy_vendor PRIVATE ${HG_PRIVATE_INCLUDE_DIRS})
target_compile_features(hurdygurdy_vendor PRIVATE cxx_std_20 c_std_11)
target_compile_definitions(hurdygurdy_vendor PRIVATE ${HG_CXX_DEFINITIONS})
target_compile_options(hurdygurdy_vendor PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${HG_CXX_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${HG_VENDOR_FLAGS}>
)

add_library(hurdygurdy_core OBJECT ${CORE_SOURCES})
add_dependencies(hurdygurdy_core shaders resources)
target_include_directories(hurdygurdy_core PRIVATE ${HG_PRIVATE_INCLUDE_DIRS})
target_compile_features(hurdygurdy_core PRIVATE cxx_std_20 c_std_11)
target_compile_definitions(hurdygurdy_core PRIVATE
    IMGUI_DEFINE_MATH_OPERATORS
    ${HG_CXX_DEFINITIONS}
)
target_compile_options(hurdygurdy_core PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${HG_CXX_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${HG_VENDOR_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${HG_STRICT_FLAGS}>
)
if(MSVC)
    target_compile_options(hurdygurdy_core PRIVATE /EHs-c-)
endif()

if(HG_USE_SDL)
    target_compile_definitions(hurdygurdy_core PRIVATE HG_USE_SDL=1)
    target_link_libraries(hurdygurdy_core PRIVATE ${SDL_TARGET})
endif()

if(HG_LINUX)
    if(HG_USE_WAYLAND)
        target_compile_definitions(hurdygurdy_core PRIVATE HG_USE_WAYLAND=1)
    endif()
    if(HG_USE_X11)
        target_compile_definitions(hurdygurdy_core PRIVATE HG_USE_X11=1)
    endif()
    if(HG_USE_PIPEWIRE)
        target_compile_definitions(hurdygurdy_core PRIVATE HG_USE_PIPEWIRE=1)
    endif()
    target_include_directories(hurdygurdy_core SYSTEM PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/xorgproto/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libX11/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libXrandr/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libXrender/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/pipewire/src
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/pipewire/spa/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libxkbcommon/include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/wayland/src
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/wayland/egl
    )
elseif(HG_WINDOWS)
    target_compile_definitions(hurdygurdy_core PRIVATE HG_USE_WIN32_BACKEND=1)
endif()

# Static library

add_library(hurdygurdy STATIC
    $<TARGET_OBJECTS:hurdygurdy_core>
    $<TARGET_OBJECTS:hurdygurdy_imgui>
    $<TARGET_OBJECTS:hurdygurdy_vendor>
)
add_library(hurdygurdy::hurdygurdy ALIAS hurdygurdy)

target_compile_features(hurdygurdy PUBLIC cxx_std_20)
target_include_directories(hurdygurdy
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/hurdygurdy>
)
target_link_options(hurdygurdy PRIVATE ${HG_LINK_OPTIONS})

if(HG_USE_SDL)
    # PUBLIC: consumers of the static library must link SDL3 themselves.
    # hurdygurdyConfig.cmake runs find_dependency(SDL3) for installed use.
    target_compile_definitions(hurdygurdy INTERFACE HG_USE_SDL=1)
    target_link_libraries(hurdygurdy PUBLIC ${SDL_TARGET})
endif()
if(HG_WINDOWS)
    target_link_libraries(hurdygurdy PRIVATE ole32 dwmapi)
endif()

# Tests

if(HG_BUILD_TESTS)
    set(TEST_SHADER_HEADERS)
    foreach(shader ${TEST_SHADERS})
        hg_compile_shader(${shader} header)
        list(APPEND TEST_SHADER_HEADERS ${header})
    endforeach()

    add_custom_target(test_shaders DEPENDS ${TEST_SHADER_HEADERS})

    add_executable(tests
        test/tests.cpp
        test/utility.cpp
        test/memory.cpp
        test/concurrency.cpp
        test/math.cpp
        test/geometry.cpp
        test/noise.cpp
        test/hash.cpp
        test/serialization.cpp
        test/json.cpp
        test/dynlib.cpp
        test/gpu.cpp
        test/audio.cpp
        test/texture.cpp
        test/ecs.cpp
        test/time.cpp
        test/inttypes.cpp
        test/render2d.cpp
        test/render3d.cpp
    )
    add_dependencies(tests test_shaders)
    target_link_libraries(tests PRIVATE hurdygurdy)
    target_link_options(tests PRIVATE ${HG_LINK_OPTIONS})
    target_include_directories(tests PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/test
        ${CMAKE_CURRENT_SOURCE_DIR}/test
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/wayland
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
    )
    if(HG_USE_PCH)
        target_precompile_headers(tests PRIVATE <tests.hpp>)
    endif()
    target_compile_features(tests PRIVATE cxx_std_20)
    target_compile_options(tests PRIVATE
        ${HG_CXX_FLAGS}
        ${HG_STRICT_FLAGS}
        ${HG_TEST_FLAGS}
    )
    target_compile_definitions(tests PRIVATE
        HG_LOGGING=1
        ${HG_CXX_DEFINITIONS}
    )
endif()

# Examples

if(HG_BUILD_EXAMPLES)
    foreach(example example2d example3d platform_test)
        add_executable(${example} examples/${example}.cpp)
        target_link_libraries(${example} PRIVATE hurdygurdy)
        target_link_options(${example} PRIVATE ${HG_LINK_OPTIONS})
        target_include_directories(${example} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
        )
        target_compile_features(${example} PRIVATE cxx_std_20)
        target_compile_options(${example} PRIVATE
            ${HG_CXX_FLAGS}
            ${HG_STRICT_FLAGS}
        )
        target_compile_definitions(${example} PRIVATE ${HG_CXX_DEFINITIONS})
    endforeach()
endif()

# Install and CMake package

if(HG_INSTALL)
    set(HG_CMAKE_INSTALL_DIR ${CMAKE_INSTALL_LIBDIR}/cmake/hurdygurdy)

    install(TARGETS hurdygurdy
        EXPORT hurdygurdyTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )

    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hurdygurdy
        FILES_MATCHING
            PATTERN "*.hpp"
            PATTERN "*.glsl"
    )

    install(EXPORT hurdygurdyTargets
        FILE hurdygurdyTargets.cmake
        NAMESPACE hurdygurdy::
        DESTINATION ${HG_CMAKE_INSTALL_DIR}
    )

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/hurdygurdyConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/hurdygurdyConfig.cmake
        INSTALL_DESTINATION ${HG_CMAKE_INSTALL_DIR}
    )

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/hurdygurdyConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/hurdygurdyConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/hurdygurdyConfigVersion.cmake
        DESTINATION ${HG_CMAKE_INSTALL_DIR}
    )
endif()

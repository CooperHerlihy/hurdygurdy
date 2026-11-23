# Build options. Defaults differ depending on whether hurdygurdy is the
# top-level project or consumed as a subproject.

if(PROJECT_IS_TOP_LEVEL)
    set(HG_BUILD_TESTS_DEFAULT ON)
    set(HG_BUILD_EXAMPLES_DEFAULT ON)
    set(HG_INSTALL_DEFAULT ON)
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
else()
    set(HG_BUILD_TESTS_DEFAULT OFF)
    set(HG_BUILD_EXAMPLES_DEFAULT OFF)
    set(HG_INSTALL_DEFAULT OFF)
endif()

option(HG_BUILD_TESTS "Build the test suite" ${HG_BUILD_TESTS_DEFAULT})
option(HG_BUILD_EXAMPLES "Build the examples" ${HG_BUILD_EXAMPLES_DEFAULT})
option(HG_INSTALL "Generate install and CMake package rules" ${HG_INSTALL_DEFAULT})
option(HG_USE_SDL "Use the SDL backend" OFF)
option(HG_USE_CCACHE "Use ccache as the compiler launcher" OFF)
option(HG_USE_MOLD "Use mold as the linker" OFF)

# Linux platform backends. Each can be disabled to cut dependencies; with all
# of them disabled, InputDevice::create() and AudioDevice::create() fall back
# to the dummy backends.
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(HG_USE_WAYLAND "Build the Wayland input backend" ON)
    option(HG_USE_X11 "Build the X11 input backend" ON)
    option(HG_USE_PIPEWIRE "Build the PipeWire audio backend" ON)
else()
    set(HG_USE_WAYLAND OFF)
    set(HG_USE_X11 OFF)
    set(HG_USE_PIPEWIRE OFF)
endif()

if(HG_USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    else()
        message(WARNING "HG_USE_CCACHE is ON but ccache was not found")
    endif()
endif()

# ccache cannot use precompiled headers, so enabling a PCH would make the test
# translation units uncacheable. Default the PCH off when ccache is on.
if(HG_USE_CCACHE)
    set(HG_USE_PCH_DEFAULT OFF)
else()
    set(HG_USE_PCH_DEFAULT ON)
endif()
option(HG_USE_PCH "Precompile the shared test header" ${HG_USE_PCH_DEFAULT})

set(HG_LINK_OPTIONS)
if(HG_USE_MOLD AND NOT MSVC)
    find_program(MOLD_PROGRAM mold)
    if(MOLD_PROGRAM)
        list(APPEND HG_LINK_OPTIONS -fuse-ld=mold)
    else()
        message(WARNING "HG_USE_MOLD is ON but mold was not found")
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(HG_LINUX TRUE)
endif()
if(WIN32)
    set(HG_WINDOWS TRUE)
endif()

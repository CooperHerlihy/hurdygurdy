# Third-party dependencies. Sets SDL_TARGET and SDL_INCLUDE_DIR.

set(VulkanHeaders_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/Vulkan-Headers/include)
set(SDL_INCLUDE_DIR "")
set(SDL_TARGET "")

if(HG_USE_SDL)
    find_package(SDL3 QUIET)
    if(SDL3_FOUND)
        message(STATUS "Using system SDL3")
        set(SDL_TARGET SDL3::SDL3)
    else()
        # Prefer static: linked into the static library and propagated to
        # consumers. SDL3::SDL3 resolves to whichever variant is available.
        set(SDL_SHARED OFF CACHE BOOL "Build SDL as a shared library")
        set(SDL_STATIC ON CACHE BOOL "Build SDL as a static library")
        set(SDL_TEST OFF CACHE BOOL "Build SDL tests")
        set(SDL_TESTS OFF CACHE BOOL "Build SDL tests")
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
        set(SDL_TARGET SDL3::SDL3)
    endif()
endif()

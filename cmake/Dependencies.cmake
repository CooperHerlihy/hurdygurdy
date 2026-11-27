# Third-party dependencies. Sets SDL_TARGET, SDL_INCLUDE_DIR and
# HG_SDL_BUILT_IN_TREE.

set(VulkanHeaders_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/Vulkan-Headers/include)
set(SDL_INCLUDE_DIR "")
set(SDL_TARGET "")
set(HG_SDL_BUILT_IN_TREE FALSE)

if(HG_USE_SDL)
    find_package(SDL3 QUIET)
    if(SDL3_FOUND)
        message(STATUS "Using system SDL3")
        set(SDL_TARGET SDL3::SDL3)
    else()
        # SDL is only used as a shared library, so it must never be built
        # or linked statically. FORCE so stale caches cannot resurrect a
        # static build.
        set(SDL_SHARED ON CACHE BOOL "Build SDL as a shared library" FORCE)
        set(SDL_STATIC OFF CACHE BOOL "Build SDL as a static library" FORCE)
        set(SDL_TEST OFF CACHE BOOL "Build SDL tests")
        set(SDL_TESTS OFF CACHE BOOL "Build SDL tests")
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/SDL/CMakeLists.txt")
            message(STATUS "Using vendored SDL3")
            set(HG_SDL_BUILT_IN_TREE TRUE)
            add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)
        else()
            message(STATUS "SDL3 not found, fetching from GitHub")
            set(HG_SDL_BUILT_IN_TREE TRUE)
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

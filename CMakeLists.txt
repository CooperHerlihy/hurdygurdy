cmake_minimum_required(VERSION 3.30)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_C_STANDARD 17)

project(HurdyGurdy LANGUAGES C CXX)

find_package(Vulkan REQUIRED)

set(SDL_STATIC ON)
set(SDL_SHARED OFF)
add_subdirectory(vendor/SDL EXCLUDE_FROM_ALL)
add_subdirectory(vendor/VulkanMemoryAllocator EXCLUDE_FROM_ALL)
add_subdirectory(vendor/glm EXCLUDE_FROM_ALL)
add_subdirectory(vendor/fastgltf)

add_library(mikktspace STATIC "vendor/mikktspace/mikktspace.c")
add_library(welder STATIC "vendor/welder/weldmesh.c")

file(GLOB_RECURSE HG_SRC "src/*.cpp" "include/*.h")
add_library(hurdy_gurdy STATIC ${HG_SRC})
target_include_directories(hurdy_gurdy PUBLIC include)
target_include_directories(hurdy_gurdy PUBLIC SYSTEM vendor)
target_link_libraries(hurdy_gurdy PUBLIC
    Vulkan::Vulkan
    SDL3::SDL3
    VulkanMemoryAllocator
    glm
    fastgltf
    mikktspace
    welder
)
target_precompile_headers(hurdy_gurdy PUBLIC "include/hg_pch.h")

file(GLOB_RECURSE DEMO_SRC "demo/*.cpp" "demo/*.h")
add_executable(demo ${DEMO_SRC})
target_link_libraries(demo PUBLIC hurdy_gurdy)
target_precompile_headers(demo REUSE_FROM hurdy_gurdy)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(STATUS "Configuring for GCC or Clang...")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-missing-field-initializers")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(STATUS "Configuring for MSVC...")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT demo)
else()
    message(WARNING "Unknown compiler ID: ${CMAKE_CXX_COMPILER_ID}. Using default flags.")
endif()

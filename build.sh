#!/bin/bash

START_TIME=$(date +%s.%N)

CFLAGS="-g -O1 -fsanitize=undefined -std=c11 -Werror -Wall -Wextra -Wconversion -Wshadow -pedantic"

LIBS="-Lbuild -Lbuild/SDL -lhurdy_gurdy -lSDL3 -lvulkan -lc -lm"
INCLUDES=" \
    -Iinclude \
    -Ivendor/SDL/include \
    -Ivendor/VulkanMemoryAllocator/include \
    -Ivendor/stb \
    -Ivendor/cgltf \
    -Ivendor/mikktspace \
    -Ivendor/welder \
    -Ibuild/shaders \
"

SHADERS=(
    # demo/test.comp
    # src/hg_depth.vert
    # src/hg_depth.frag
    # src/hg_sprite.vert
    # src/hg_sprite.frag
    src/hg_model.vert
    src/hg_model.frag
    # src/hg_ray_marcher.vert
    # src/hg_ray_marcher.frag
)

SRCS=(
    src/hurdy_gurdy.c
    src/hg_utils.c
    src/hg_math.c
    src/hg_graphics.c
    # src/hg_depth_renderer.c
    # src/hg_2d_renderer.c
    src/hg_3d_renderer.c
    # src/hg_ray_marcher.c
)

OBJS=()

mkdir -p build

echo "Compiling shaders..."

mkdir -p build/shaders

cc $CFLAGS $INCLUDES -o build/hg_embed_file src/hg_embed_file.c

for shader in "${SHADERS[@]}"; do
    echo "Compiling $shader..."
    glslc -o build/shaders/$(basename $shader).spv $shader
    build/hg_embed_file build/shaders/$(basename $shader).spv $(basename $shader)_spv > build/shaders/$(basename $shader).spv.h
done

echo "Building hurdy_gurdy..."

for file in "${SRCS[@]}"; do
    echo "Compiling $file..."
    cc $CFLAGS $INCLUDES -o "build/$(basename $file .c).o" -c $file
    OBJS+=" build/$(basename $file .c).o"
done

echo "Archiving..."
ar rcs build/libhurdy_gurdy.a build/vk_mem_alloc.o build/stb.o build/cgltf.o build/mikktspace.o build/weldmesh.o $OBJS

echo "Building demo..."

echo "Compiling demo/main.c..."
cc $CFLAGS -Iinclude -Ivendor/SDL/include $LIBS -o build/demo.o -c demo/main.c

echo "Linking..."
c++ build/demo.o $CFLAGS -std=c++20 $LIBS -o build/out -Wl,-rpath=./build/SDL

END_TIME=$(date +%s.%N)
printf "Build complete: %.6f seconds\n" "$(echo "$END_TIME - $START_TIME" | bc)"


#!/bin/bash

START_TIME=$(date +%s.%N)

if [ "$1" == "release" ]; then
    CFLAGS="-O3 -DNDEBUG -std=c11 -Werror -Wall -Wextra -Wconversion -Wshadow -pedantic"
else
    CFLAGS="-g -O0 -fsanitize=undefined -std=c11 -Werror -Wall -Wextra -Wconversion -Wshadow -pedantic"
fi

INCLUDES="-Iinclude -Ivendor/SDL/include -Ivendor/VulkanMemoryAllocator/include -Ivendor/fastgltf/include -Ivendor/stb -Ivendor/mikktspace -Ivendor/welder"
LIBS="-Lbuild -Lbuild/SDL -Lbuild/fastgltf -lhurdy_gurdy -lSDL3 -lfastgltf -lvulkan -lc -lm"

mkdir -p build
mkdir -p build/shaders

echo "Compiling shaders..."

SHADERS=(
    demo/test.vert
    demo/test.frag
)

for shader in "${SHADERS[@]}"; do
    echo "Compiling $shader..."
    glslc -o build/shaders/$(basename $shader).spv $shader
done

echo "Building hurdy_gurdy..."

SRCS=(
    src/hurdy_gurdy.c
    src/hg_utils.c
    src/hg_graphics.c
)
OBJS=()

for file in "${SRCS[@]}"; do
    echo "Compiling $file..."
    cc $CFLAGS $INCLUDES -o "build/$(basename $file .c).o" -c $file
    OBJS+=" build/$(basename $file .c).o"
done

echo "Archiving..."
ar rcs build/libhurdy_gurdy.a build/vk_mem_alloc.o build/mikktspace.o build/weldmesh.o $OBJS

echo "Building demo..."

echo "Compiling demo/main.c..."
cc $CFLAGS $INCLUDES $LIBS -o build/demo.o -c demo/main.c

echo "Linking..."
c++ build/demo.o $CFLAGS -std=c++20 $INCLUDES $LIBS -o build/out

END_TIME=$(date +%s.%N)
printf "Build complete: %.6f seconds\n" "$(echo "$END_TIME - $START_TIME" | bc)"


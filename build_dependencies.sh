#!/bin/bash

mkdir -p build

echo "Building SDL..."
cmake -S vendor/SDL -B build/SDL -DSDL_SHARED=OFF -DSDL_STATIC=ON
cmake --build build/SDL

echo "Building fastgltf..."
cmake -S vendor/fastgltf -B build/fastgltf
cmake --build build/fastgltf

echo "Building VulkanMemoryAllocator..."
c++ -std=c++20 -g -O0 -Ivendor/VulkanMemoryAllocator/include -o build/vk_mem_alloc.o -c src/vk_mem_alloc.cpp

echo "Building stb libraries..."
cc -std=c11 -g -O0 -Ivendor/stb -o build/stb.o -c src/stb.c

echo "Building MikkTSpace..."
cc -std=c11 -g -O0 -o build/mikktspace.o -c vendor/mikktspace/mikktspace.c

echo "Building Welder..."
cc -std=c11 -g -O0 -o build/weldmesh.o -c vendor/welder/weldmesh.c

echo "Dependencies complete."


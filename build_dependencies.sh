#!/bin/bash

echo "Building Dependencies..."
mkdir -p build

echo "Building SDL..."
cmake -S vendor/SDL -B build/SDL
cmake --build build/SDL

echo "Building VulkanMemoryAllocator..."
c++ -std=c++20 -g -O1 -Ivendor/VulkanMemoryAllocator/include -o build/vk_mem_alloc.o -c src/vk_mem_alloc.cpp

echo "Building stb libraries..."
cc -std=c11 -g -O1 -Ivendor/stb -o build/stb.o -c src/stb.c

echo "Building cgltf..."
cc -std=c11 -g -O1 -Ivendor/cgltf -o build/cgltf.o -c src/cgltf.c

echo "Building MikkTSpace..."
cc -std=c11 -g -O1 -o build/mikktspace.o -c vendor/mikktspace/mikktspace.c

echo "Building Welder..."
cc -std=c11 -g -O1 -o build/weldmesh.o -c vendor/welder/weldmesh.c

echo "Dependencies complete."


@echo off
setlocal

if not exist build mkdir build

echo Building SDL...
cmake -S vendor/SDL -B build/SDL
cmake --build build/SDL
copy build\SDL\Debug\SDL3.dll build\SDL3.dll

echo Building fastgltf...
cmake -S vendor/fastgltf -B build/fastgltf
cmake --build build/fastgltf

echo Building VulkanMemoryAllocator...
cl /std:c++20 /Zi /Od /I vendor\VulkanMemoryAllocator\include /I %VULKAN_SDK%\Include /c src\vk_mem_alloc.cpp /Fo:build\vk_mem_alloc.obj /Fd:build\vk_mem_alloc.pdb

echo Building stb libraries...
cl /std:c11 /Zi /Od /I vendor\stb /c src\stb.c /Fo:build\stb.obj /Fd:build\stb.pdb

echo Building MikkTSpace...
cl /std:c11 /Zi /Od /c vendor\mikktspace\mikktspace.c /Fo:build\mikktspace.obj /Fd:build\mikktspace.pdb

echo Building Welder...
cl /std:c11 /Zi /Od /c vendor\welder\weldmesh.c /Fo:build\weldmesh.obj /Fd:build\weldmesh.pdb

echo Building complete.

endlocal


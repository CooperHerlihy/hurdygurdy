@echo off
setlocal enabledelayedexpansion

set starttime=%time%

set CFLAGS=/nologo /Zi /Od /std:c11 /WX /W3 /experimental:c11atomics

set INCLUDES=^
    /I include^
    /I vendor\SDL\include^
    /I vendor\VulkanMemoryAllocator\include^
    /I vendor\stb^
    /I vendor\cgltf^
    /I vendor\mikktspace^
    /I vendor\welder^
    /I %VULKAN_SDK%\Include

set LIBS=^
    build\hurdy_gurdy.lib^
    build\SDL\Debug\SDL3.lib^
    %VULKAN_SDK%\Lib\vulkan-1.lib

if not exist build mkdir build

echo Compiling shaders...

set SHADERS=^
    demo\test.comp^
    src\hg_depth.vert^
    src\hg_depth.frag^
    src\hg_sprite.vert^
    src\hg_sprite.frag^
    src\hg_model.vert^
    src\hg_model.frag^
    src\hg_ray_marcher.vert^
    src\hg_ray_marcher.frag

for %%S in (%SHADERS%) do (
    echo Compiling %%S...
    glslc -o build\%%~nxS.spv %%S
)

echo Building hurdy_gurdy...

set SRCS=^
    src\hurdy_gurdy.c^
    src\hg_utils.c^
    src\hg_math.c^
    src\hg_graphics.c^
    src\hg_depth_renderer.c^
    src\hg_2d_renderer.c^
    src\hg_3d_renderer.c^
    src\hg_ray_marcher.c

set OBJS=

for %%F in (%SRCS%) do (
    cl %CFLAGS% %INCLUDES% /c %%F /Fo:build\%%~nF.obj /Fd:build\%%~nF.pdb
    set OBJS=!OBJS! build\%%~nF.obj
)

echo Archiving...
lib /nologo /OUT:build\hurdy_gurdy.lib build\vk_mem_alloc.obj build\stb.obj build\cgltf.obj build\mikktspace.obj build\weldmesh.obj %OBJS%

echo Building demo...

cl %CFLAGS% /I include /I vendor\SDL\include /c demo\main.c /Fo:build\demo.obj /Fd:build\demo.pdb

echo Linking...
cl %CFLAGS% %LIBS% build\demo.obj /Fe:build\out.exe /link /subsystem:console

set endtime=%time%

for /f "tokens=1-4 delims=:." %%a in ("%starttime%") do (
    set /a sh=%%a, sm=%%b, ss=%%c, sc=%%d
)
for /f "tokens=1-4 delims=:." %%a in ("%endtime%") do (
    set /a eh=%%a, em=%%b, es=%%c, ec=%%d
)

set /a startcs=(sh*3600 + sm*60 + ss)*100 + sc
set /a endcs=(eh*3600 + em*60 + es)*100 + ec
if %endcs% LSS %startcs% set /a endcs+=24*3600*100

set /a elapsedcs=endcs-startcs
set /a elapsedsec=elapsedcs/100
set /a elapsedms=(elapsedcs%%100)*10

echo Build complete: %elapsedsec%.%elapsedms%s

endlocal

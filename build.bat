@echo off
setlocal enabledelayedexpansion

set SRC_DIR=.
set BUILD_DIR=%SRC_DIR%\build
set TEST_DIR=%SRC_DIR%\hg_test_dir

set STD=/std:c++17
set WARNINGS=/W4 /WX /permissive- /D _CRT_SECURE_NO_WARNINGS
set CONFIG=/nologo /Zi /Od

for %%a in (%*) do (
    if "%%a"=="release" (
        set CONFIG=/nologo /O2 /D NDEBUG
    )
)

set INCLUDES= ^
    /I "%BUILD_DIR%" ^
    /I "%SRC_DIR%\include" ^
    /I "%SRC_DIR%\vendor\SDL\include" ^
    /I "%SRC_DIR%\vendor\imgui" ^
    /I "%SRC_DIR%\vendor\imgui\backends"

set IMGUI_SRC=vendor/imgui/*.cpp

set IMGUI_BACKENDS= ^
    imgui_impl_sdl3.cpp ^
    imgui_impl_vulkan.cpp

set SHADERS= ^
    noise.comp ^
    sprite.vert ^
    sprite.frag ^
    model.vert ^
    model.frag

set SRC= ^
    hurdygurdy.cpp ^
    vulkan.cpp ^
    test.cpp

set TARGETS= ^
    editor.exe ^
    minimal.exe

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%TEST_DIR%" mkdir "%TEST_DIR%"

if not exist "%BUILD_DIR%\SDL3.lib" (
    cmake -B"%BUILD_DIR%\SDL3" -S"%SRC_DIR%\vendor\SDL" -DSDL_TESTS=OFF
    cmake --build "%BUILD_DIR%\SDL3"
    copy "%BUILD_DIR%\SDL3\Debug\SDL3.lib" "%BUILD_DIR%\SDL3.lib"
    copy "%BUILD_DIR%\SDL3\Debug\SDL3.dll" "%BUILD_DIR%\SDL3.dll"
)

if not exist "%BUILD_DIR%\vk_mem_alloc.obj" (
    cl /c "%SRC_DIR%\src\vk_mem_alloc.cpp" ^
        /Fd:"%BUILD_DIR%\vk_mem_alloc.pdb" ^
        /Fo:"%BUILD_DIR%\vk_mem_alloc.obj" ^
        %STD% %CONFIG% %INCLUDES%
)

if not exist "%BUILD_DIR%\stb.obj" (
    cl /c "%SRC_DIR%\src\stb.c" ^
        /Fd:"%BUILD_DIR%\stb.pdb" ^
        /Fo:"%BUILD_DIR%\stb.obj" ^
        %STD% %CONFIG% %INCLUDES%
)

for %%F in (%IMGUI_SRC%) do (
    if not exist "%BUILD_DIR%\%%~nF.obj" (
        cl /c "%SRC_DIR%\vendor\imgui\%%F" ^
            /Fd:"%BUILD_DIR%\%%~nF.pdb" ^
            /Fo:"%BUILD_DIR%\%%~nF.obj" ^
            %STD% %WARNINGS% %CONFIG% %INCLUDES%
    )

    set OBJS=!OBJS! "%BUILD_DIR%\%%~nF.obj"
)

for %%F in (%IMGUI_BACKENDS%) do (
    if not exist "%BUILD_DIR%\%%~nF.obj" (
        cl /c "%SRC_DIR%\vendor\imgui\backends\%%F" ^
            /Fd:"%BUILD_DIR%\%%~nF.pdb" ^
            /Fo:"%BUILD_DIR%\%%~nF.obj" ^
            %STD% %WARNINGS% %CONFIG% %INCLUDES%
    )

    set OBJS=!OBJS! "%BUILD_DIR%\%%~nF.obj"
)

for %%F in (%SHADERS%) do (
    echo %%F
    glslc -o "%BUILD_DIR%\%%F.spv" "%SRC_DIR%\src\%%F" -I"%SRC_DIR%\include"
)

for %%F in (%SRC%) do (
    cl /c "%SRC_DIR%\src\%%F" ^
        /Fd:"%BUILD_DIR%\%%~nF.pdb" ^
        /Fo:"%BUILD_DIR%\%%~nF.obj" ^
        %STD% %WARNINGS% %CONFIG% %INCLUDES%

    set OBJS=!OBJS! "%BUILD_DIR%\%%~nF.obj"
)

lib /nologo /OUT:"%BUILD_DIR%\hurdygurdy.lib" ^
    "%BUILD_DIR%\vk_mem_alloc.obj" ^
    "%BUILD_DIR%\stb.obj" ^
    %OBJS%

for %%F in (%TARGETS%) do (
    cl "%SRC_DIR%\src\%%~nF.cpp" ^
        /Fd:"%BUILD_DIR%\%%~nF.pdb" ^
        /Fo:"%BUILD_DIR%\%%~nF.obj" ^
        /Fe:"%BUILD_DIR%\%%~nF.exe" ^
        %STD% %WARNINGS% %CONFIG% %INCLUDES% ^
        "%BUILD_DIR%\hurdygurdy.lib" "%BUILD_DIR%\SDL3.lib" User32.lib

    set OBJS=!OBJS! "%BUILD_DIR%\%%~nF.obj"
)

endlocal

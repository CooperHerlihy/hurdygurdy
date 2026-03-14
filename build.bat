@echo off
setlocal enabledelayedexpansion

set SRC_DIR=.
set BUILD_DIR=%SRC_DIR%\build
set TEST_DIR=%SRC_DIR%\hg_test_dir

set STD=/std:c++17
set WARNINGS=/W4 /WX /D _CRT_SECURE_NO_WARNINGS
set CONFIG=/nologo /Zi /Od

for %%a in (%*) do (
    if "%%a"=="release" (
        set CONFIG=/nologo /O2 /D NDEBUG
    )
)

set INCLUDES= ^
    /I "%BUILD_DIR%" ^
    /I "%SRC_DIR%\include" ^
    /I "%SRC_DIR%\vendor\libX11\include" ^
    /I "%SRC_DIR%\vendor\imgui" ^
    /I "%SRC_DIR%\vendor\imgui\backends"

set IMGUI_SRC=vendor/imgui/*.cpp

set IMGUI_BACKENDS= ^
    imgui_impl_win32.cpp ^
    imgui_impl_vulkan.cpp

set SRC= ^
    utils.cpp ^
    thread.cpp ^
    resources.cpp ^
    pipeline2d.cpp ^
    pipeline3d.cpp ^
    window_win32.cpp ^
    vulkan.cpp ^
    test.cpp

set SHADERS= ^
    sprite.vert ^
    sprite.frag ^
    model.vert ^
    model.frag

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%TEST_DIR%" mkdir "%TEST_DIR%"

cl "%SRC_DIR%\src\embed_file.cpp" ^
    /Fd:"%BUILD_DIR%\embed_file.pdb" ^
    /Fo:"%BUILD_DIR%\embed_file.obj" ^
    /Fe:"%BUILD_DIR%\embed_file.exe" ^
    %STD% %WARNINGS% %CONFIG% %INCLUDES%

for %%F in (%SHADERS%) do (
    echo %%F

    glslc -o "%BUILD_DIR%\%%F.spv" "%SRC_DIR%\src\%%F"

    "%BUILD_DIR%\embed_file.exe" ^
        "%BUILD_DIR%\%%F.spv" ^
        "%%F.spv" > "%BUILD_DIR%\%%F.spv.h"
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

cl "%SRC_DIR%\src\minimal.cpp" ^
    /Fd:"%BUILD_DIR%\minimal.pdb" ^
    /Fo:"%BUILD_DIR%\minimal.obj" ^
    /Fe:"%BUILD_DIR%\minimal.exe" ^
    %STD% %WARNINGS% %CONFIG% %INCLUDES% ^
    "%BUILD_DIR%\hurdygurdy.lib" User32.lib

cl "%SRC_DIR%\src\editor.cpp" ^
    /Fd:"%BUILD_DIR%\editor.pdb" ^
    /Fo:"%BUILD_DIR%\editor.obj" ^
    /Fe:"%BUILD_DIR%\editor.exe" ^
    %STD% %WARNINGS% %CONFIG% %INCLUDES% ^
    "%BUILD_DIR%\hurdygurdy.lib" User32.lib

endlocal

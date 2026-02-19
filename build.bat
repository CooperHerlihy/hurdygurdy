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
    /I "%SRC_DIR%\vendor\libX11\include"

set SRC= ^
    init.cpp ^
    test_utils.cpp ^
    math.cpp ^
    memory.cpp ^
    string.cpp ^
    time.cpp ^
    thread.cpp ^
    resources.cpp ^
    ecs.cpp ^
    pipeline2d.cpp ^
    windows.cpp ^
    vulkan.cpp

set SHADERS= ^
    sprite.vert ^
    sprite.frag

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

cl "%SRC_DIR%\src\tests.cpp" ^
    /Fd:"%BUILD_DIR%\tests.pdb" ^
    /Fo:"%BUILD_DIR%\tests.obj" ^
    /Fe:"%BUILD_DIR%\tests.exe" ^
    %STD% %WARNINGS% %CONFIG% %INCLUDES% ^
    "%BUILD_DIR%\hurdygurdy.lib" User32.lib

endlocal

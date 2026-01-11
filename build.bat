@echo off
setlocal enabledelayedexpansion

set SRC_DIR=.
set BUILD_DIR=build

set STD=/std:c++17
set WARNINGS=/W4 /WX /D _CRT_SECURE_NO_WARNINGS
set CONFIG=/nologo /Zi /Od

for %%a in (%*) do (
    if "%%a"=="release" (
        set CONFIG=/nologo /O2 /D NDEBUG
    )
)

set INCLUDES= ^
    /I "%BUILD_DIR%\shaders" ^
    /I "%SRC_DIR%\include" ^
    /I "%SRC_DIR%\vendor\libX11\include"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%BUILD_DIR%\shaders" mkdir "%BUILD_DIR%\shaders"

cl "%SRC_DIR%\src\embed_file.cpp" ^
    /Fd:"%BUILD_DIR%\embed_file.pdb" ^
    /Fo:"%BUILD_DIR%\embed_file.obj" ^
    /Fe:"%BUILD_DIR%\embed_file.exe" ^
    %STD% %WARNINGS% %CONFIG% %INCLUDES%

set SHADERS= ^
    %SRC_DIR%\src\sprite.vert ^
    %SRC_DIR%\src\sprite.frag

for %%S in (%SHADERS%) do (
    for %%N in (%%~nxS) do (
        echo %%N
        glslc -o "%BUILD_DIR%\shaders\%%N.spv" "%%S"

        "%BUILD_DIR%\embed_file.exe" ^
            "%BUILD_DIR%\shaders\%%N.spv" ^
            "%%N.spv" > "%BUILD_DIR%\shaders\%%N.spv.h"
    )
)

cl /c "%SRC_DIR%\src\vk_mem_alloc.cpp" ^
    /Fd:"%BUILD_DIR%\vk_mem_alloc.pdb" ^
    /Fo:"%BUILD_DIR%\vk_mem_alloc.obj" ^
    %STD% %CONFIG% %INCLUDES%

cl /c "%SRC_DIR%\src\stb.c" ^
    /Fd:"%BUILD_DIR%\stb.pdb" ^
    /Fo:"%BUILD_DIR%\stb.obj" ^
    %STD% %CONFIG% %INCLUDES%

cl /c "%SRC_DIR%\src\hurdygurdy.cpp" ^
    /Fd:"%BUILD_DIR%\hurdygurdy.pdb" ^
    /Fo:"%BUILD_DIR%\hurdygurdy.obj" ^
    %STD% %WARNINGS% %CONFIG% %INCLUDES%

lib /nologo /OUT:"%BUILD_DIR%\hurdygurdy.lib" ^
    "%BUILD_DIR%\hurdygurdy.obj" ^
    "%BUILD_DIR%\vk_mem_alloc.obj" ^
    "%BUILD_DIR%\stb.obj"

cl "%SRC_DIR%\src\test.cpp" ^
    /Fd:"%BUILD_DIR%\test.pdb" ^
    /Fo:"%BUILD_DIR%\test.obj" ^
    /Fe:"%BUILD_DIR%\test.exe" ^
    %STD% %WARNINGS% %CONFIG% %INCLUDES% ^
    "%BUILD_DIR%\hurdygurdy.lib" User32.lib

endlocal

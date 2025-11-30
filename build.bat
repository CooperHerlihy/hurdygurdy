@echo off
setlocal enabledelayedexpansion

set SRC_DIR=.
set BUILD_DIR=build

set STD=/std:c11
set WARNINGS=/W4 /WX
set CONFIG=/nologo /Zi /Od

set INCLUDES= ^
    /I "%BUILD_DIR%\shaders" ^
    /I "%SRC_DIR%\include" ^
    /I "%SRC_DIR%\vendor\libX11\include"

if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
if not exist "%BUILD_DIR%\shaders" mkdir "%BUILD_DIR%\shaders"

cl "%SRC_DIR%\src\embed_file.c" ^
    /Fd:"%BUILD_DIR%\embed_file.pdb" ^
    /Fo:"%BUILD_DIR%\embed_file.obj" ^
    /Fe:"%BUILD_DIR%\embed_file.exe" ^
    %STD% %WARNINGS% %CONFIG% %INCLUDES%

set SHADERS= ^
    %SRC_DIR%\src\test.vert ^
    %SRC_DIR%\src\test.frag

for %%S in (%SHADERS%) do (
    for %%N in (%%~nxS) do (
        echo %%N
        glslc -o "%BUILD_DIR%\shaders\%%N.spv" "%%S"

        "%BUILD_DIR%\embed_file.exe" ^
            "%BUILD_DIR%\shaders\%%N.spv" ^
            "%%N.spv" > "%BUILD_DIR%\shaders\%%N.spv.h"
    )
)

cl /c "%SRC_DIR%\src\hurdygurdy.c" ^
    /Fd:"%BUILD_DIR%\hurdygurdy.pdb" ^
    /Fo:"%BUILD_DIR%\hurdygurdy.obj" ^
    %STD% %WARNINGS% %CONFIG% %INCLUDES%

lib /nologo /OUT:"%BUILD_DIR%\hurdygurdy.lib" "%BUILD_DIR%\hurdygurdy.obj"

cl "%SRC_DIR%\src\test.c" ^
    /Fd:"%BUILD_DIR%\test.pdb" ^
    /Fo:"%BUILD_DIR%\test.obj" ^
    /Fe:"%BUILD_DIR%\test.exe" ^
    %STD% %WARNINGS% %CONFIG% %INCLUDES% ^
    "%BUILD_DIR%\hurdygurdy.lib" User32.lib

endlocal

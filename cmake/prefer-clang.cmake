# Prefer Clang if available, fall back to system default

find_program(HG_CLANGXX clang++)
find_program(HG_CLANG clang)

if(HG_CLANGXX AND HG_CLANG)
    set(CMAKE_C_COMPILER "${HG_CLANG}" CACHE FILEPATH "C compiler" FORCE)
    set(CMAKE_CXX_COMPILER "${HG_CLANGXX}" CACHE FILEPATH "C++ compiler" FORCE)
    message(STATUS "Using Clang: ${HG_CLANGXX}")
else()
    message(STATUS "Clang not found, falling back to system default compiler")
endif()


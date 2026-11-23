# Per-target compiler flags. Applied to hurdygurdy targets only; nothing here
# mutates global CMake variables.

if(MSVC)
    set(HG_CXX_FLAGS /permissive- /GR-)
    set(HG_CXX_DEFINITIONS _CRT_SECURE_NO_WARNINGS)
    set(HG_STRICT_FLAGS /W4 /WX /wd4702 /wd4127 /wd4530)
    set(HG_TEST_FLAGS /EHsc)
    set(HG_VENDOR_FLAGS)
else()
    set(HG_CXX_FLAGS -fno-exceptions -fno-rtti)
    set(HG_CXX_DEFINITIONS)
    set(HG_STRICT_FLAGS -Wall -Wextra -Wconversion -Wsign-conversion -pedantic -Werror)
    set(HG_TEST_FLAGS -fexceptions)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(HG_VENDOR_FLAGS
            -Wno-nullability-extension
            -Wno-nullability-completeness
        )
    else()
        set(HG_VENDOR_FLAGS)
    endif()
endif()

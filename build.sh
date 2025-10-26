#!/bin/bash

START_TIME=$(date +%s.%N)

EXIT_CODE=0

SRC_DIR=.
BUILD_DIR=build
INSTALL_DIR=build/install
CONFIG="debug"

while [[ $# -gt 0 ]]; do
    case $1 in
        "--source" | "-s")
            SRC_DIR=$2
            shift 2
            ;;
        "--build" | "-b")
            BUILD_DIR=$2
            shift 2
            ;;
        "--install" | "-i")
            INSTALL_DIR=$2
            shift 2
            ;;
        "--config" | "-c")
            CONFIG=$2
            shift 2
            ;;
        "--help" | "-h")
            echo "Usage: $0 [Options]"
            echo "Options:"
            echo "  --source | -s <dir>   Source directory"
            echo "  --build | -b <dir>    Build directory"
            echo "  --install | -i <dir>  Install directory"
            echo "  --config | -c <type>  Build type (debug, rel-with-debug-info, release)"
            echo "  --help | -h           Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

CVERSION="-std=c11"
CXXVERSION="-std=c++20"
CONFIG_FLAGS=""
WARNING_FLAGS="-Werror -Wall -Wextra -Wconversion -Wshadow -pedantic"

case "${CONFIG}" in
    "debug")
        CONFIG_FLAGS="-g -O0 -fsanitize=undefined"
        ;;
    "rel-with-debug-info")
        CONFIG_FLAGS="-g -O3"
        ;;
    "release")
        CONFIG_FLAGS="-O3 -DNDEBUG"
        ;;
    *)
        echo "Invalid config: ${CONFIG}"
        echo "Valid configs: debug, rel-with-debug-info, release"
        exit 1
        ;;
esac

INCLUDES=" \
    -I${SRC_DIR}/include \
    -I${SRC_DIR}/vendor/SDL/include \
    -I${SRC_DIR}/vendor/VulkanMemoryAllocator/include \
    -I${SRC_DIR}/vendor/stb \
    -I${SRC_DIR}/vendor/cgltf \
    -I${BUILD_DIR}/shaders \
"

SHADERS=(
    ${SRC_DIR}/src/hg_depth.vert
    ${SRC_DIR}/src/hg_depth.frag
    ${SRC_DIR}/src/hg_sprite.vert
    ${SRC_DIR}/src/hg_sprite.frag
    ${SRC_DIR}/src/hg_model.vert
    ${SRC_DIR}/src/hg_model.frag
    ${SRC_DIR}/src/hg_ray_marcher.vert
    ${SRC_DIR}/src/hg_ray_marcher.frag
    ${SRC_DIR}/src/hg_test.comp
)

SRCS=(
    ${SRC_DIR}/src/hurdygurdy.c
    ${SRC_DIR}/src/hg_utils.c
    ${SRC_DIR}/src/hg_math.c
    ${SRC_DIR}/src/hg_graphics.c
    ${SRC_DIR}/src/hg_depth_renderer.c
    ${SRC_DIR}/src/hg_2d_renderer.c
    ${SRC_DIR}/src/hg_3d_renderer.c
    ${SRC_DIR}/src/hg_ray_marcher.c
)

OBJS=""

mkdir -p ${BUILD_DIR}

echo "Compiling external libraries..."

mkdir -p ${BUILD_DIR}/obj

if [ ! -d "${BUILD_DIR}/SDL" ]; then
    echo "SDL..."
    cmake -S ${SRC_DIR}/vendor/SDL -B ${BUILD_DIR}/SDL
    if [ $? -ne 0 ]; then EXIT_CODE=1; fi
    if [ "${CONFIG}" == "debug" ]; then
        cmake --build ${BUILD_DIR}/SDL --config Debug
    elif [ "${CONFIG}" == "rel-with-debug-info" ]; then
        cmake --build ${BUILD_DIR}/SDL --config RelWithDebInfo
    elif [ "${CONFIG}" == "release" ]; then
        cmake --build ${BUILD_DIR}/SDL --config Release
    fi
    if [ $? -ne 0 ]; then EXIT_CODE=1; fi
fi

if [ ! -f "${BUILD_DIR}/obj/vk_mem_alloc.o" ]; then
    echo "VulkanMemoryAllocator"
    c++ ${CXXVERSION} ${CONFIG_FLAGS} \
        -I ${SRC_DIR}/vendor/VulkanMemoryAllocator/include \
        -o ${BUILD_DIR}/obj/vk_mem_alloc.o \
        -c ${SRC_DIR}/src/vk_mem_alloc.cpp
    if [ $? -ne 0 ]; then EXIT_CODE=1; fi
fi
OBJS+=" ${BUILD_DIR}/obj/vk_mem_alloc.o"

if [ ! -f "${BUILD_DIR}/obj/stb.o" ]; then
    echo "stb"
    cc ${CVERSION} ${CONFIG_FLAGS} \
        -I ${SRC_DIR}/vendor/stb \
        -o ${BUILD_DIR}/obj/stb.o \
        -c ${SRC_DIR}/src/stb.c
    if [ $? -ne 0 ]; then EXIT_CODE=1; fi
fi
OBJS+=" ${BUILD_DIR}/obj/stb.o"

if [ ! -f "${BUILD_DIR}/obj/cgltf.o" ]; then
    echo "cgltf"
    cc ${CVERSION} ${CONFIG_FLAGS} \
        -I ${SRC_DIR}/vendor/cgltf \
        -o ${BUILD_DIR}/obj/cgltf.o \
        -c ${SRC_DIR}/src/cgltf.c
    if [ $? -ne 0 ]; then EXIT_CODE=1; fi
fi
OBJS+=" ${BUILD_DIR}/obj/cgltf.o"

echo "Compiling..."

mkdir -p ${BUILD_DIR}/shaders

echo "hg_embed_file.c..."
cc ${CVERSION} ${CONFIG_FLAGS} ${WARNING_FLAGS} ${INCLUDES} \
    -o ${BUILD_DIR}/hg_embed_file \
    ${SRC_DIR}/src/hg_embed_file.c
if [ $? -ne 0 ]; then EXIT_CODE=1; fi

for shader in "${SHADERS[@]}"; do
    name=$(basename ${shader})
    echo "${name}"
    glslc -o ${BUILD_DIR}/shaders/${name}.spv ${shader}
    if [ $? -ne 0 ]; then EXIT_CODE=1; fi
    ${BUILD_DIR}/hg_embed_file \
        ${BUILD_DIR}/shaders/${name}.spv \
        ${name}.spv \
        > ${BUILD_DIR}/shaders/${name}.spv.h
    if [ $? -ne 0 ]; then EXIT_CODE=1; fi
done

for file in "${SRCS[@]}"; do
    name=$(basename ${file} .c)
    echo "${name}.c"
    cc ${CVERSION} ${CONFIG_FLAGS} ${WARNING_FLAGS} ${INCLUDES} \
        -o "${BUILD_DIR}/obj/${name}.o" \
        -c ${file}
    if [ $? -ne 0 ]; then EXIT_CODE=1; fi
    OBJS+=" ${BUILD_DIR}/obj/${name}.o"
done

echo "Archiving..."

echo "libhurdygurdy.a"
ar rcs ${BUILD_DIR}/libhurdygurdy.a $OBJS
if [ $? -ne 0 ]; then EXIT_CODE=1; fi

echo "Installing..."

mkdir -p ${INSTALL_DIR}

if [ ! -d "${INSTALL_DIR}/SDL" ]; then
    mkdir -p ${INSTALL_DIR}/SDL
    echo "SDL"
    if [ "${CONFIG}" == "debug" ]; then
        cmake --install ${BUILD_DIR}/SDL --config Debug --prefix ${INSTALL_DIR}/SDL
    elif [ "${CONFIG}" == "rel-with-debug-info" ]; then
        cmake --install ${BUILD_DIR}/SDL --config RelWithDebInfo --prefix ${INSTALL_DIR}/SDL
    elif [ "${CONFIG}" == "release" ]; then
        cmake --install ${BUILD_DIR}/SDL --config Release --prefix ${INSTALL_DIR}/SDL
    fi
    if [ $? -ne 0 ]; then EXIT_CODE=1; fi
fi

echo "libhurdygurdy.a"

mkdir -p ${INSTALL_DIR}/lib
cp ${BUILD_DIR}/libhurdygurdy.a ${INSTALL_DIR}/lib

echo "headers"

cp -r ${SRC_DIR}/include ${INSTALL_DIR}/

END_TIME=$(date +%s.%N)
printf "Build complete: %.6f seconds\n" "$(echo "$END_TIME - $START_TIME" | bc)"

if [ ${EXIT_CODE} -ne 0 ]; then
    echo "Build failed."
    exit ${EXIT_CODE}
fi
exit ${EXIT_CODE}

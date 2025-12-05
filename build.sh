#!/bin/bash

START_TIME=$(date +%s.%N)

SRC_DIR=.
BUILD_DIR=build

STD="-std=c11"
WARNINGS="-Werror -Wall -Wextra -Wconversion -Wshadow -pedantic"
CONFIG="-g -O0 -fsanitize=undefined"

INCLUDES=" \
    -I${BUILD_DIR}/shaders \
    -I${SRC_DIR}/include \
    -I${SRC_DIR}/vendor/libX11/include \
"

mkdir -p ${BUILD_DIR}

echo "embed_file.c"
cc ${STD} ${CONFIG} ${WARNINGS} ${INCLUDES} \
    ${SRC_DIR}/src/embed_file.c \
    -o ${BUILD_DIR}/embed_file

mkdir -p ${BUILD_DIR}/shaders

SHADERS=(
    ${SRC_DIR}/src/sprite.vert
    ${SRC_DIR}/src/sprite.frag
)

for shader in "${SHADERS[@]}"; do
    name=$(basename ${shader})
    echo "${name}"

    glslc -o ${BUILD_DIR}/shaders/${name}.spv ${shader}

    ${BUILD_DIR}/embed_file \
        ${BUILD_DIR}/shaders/${name}.spv \
        ${name}.spv > ${BUILD_DIR}/shaders/${name}.spv.h
done

echo "hurdygurdy.c"
cc ${STD} ${CONFIG} ${WARNINGS} ${INCLUDES} \
    -c ${SRC_DIR}/src/hurdygurdy.c \
    -o ${BUILD_DIR}/hurdygurdy.o

if [ ! -f "${BUILD_DIR}/vk_mem_alloc.o" ]; then
    echo "vk_mem_alloc.cpp"
    c++ -std=c++17 ${CONFIG} ${INCLUDES} \
        -c ${SRC_DIR}/src/vk_mem_alloc.cpp \
        -o ${BUILD_DIR}/vk_mem_alloc.o
fi

echo "libhurdygurdy.a"
ar rcs ${BUILD_DIR}/libhurdygurdy.a ${BUILD_DIR}/hurdygurdy.o ${BUILD_DIR}/vk_mem_alloc.o

echo "test.c"
cc ${SRC_DIR}/src/test.c \
    -o ${BUILD_DIR}/test \
    ${STD} ${CONFIG} ${WARNINGS} ${INCLUDES} \
    -lm -lc -lstdc++ -L"${BUILD_DIR}" -lhurdygurdy

END_TIME=$(date +%s.%N)
printf "Build complete: %.6f seconds\n" "$(echo "$END_TIME - $START_TIME" | bc)"

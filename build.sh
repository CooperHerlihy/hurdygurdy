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
)

for shader in "${SHADERS[@]}"; do
    name=$(basename ${shader})
    echo "${name}"

    glslc -o ${BUILD_DIR}/shaders/${name}.spv ${shader}

    ${BUILD_DIR}/hg_embed_file \
        ${BUILD_DIR}/shaders/${name}.spv \
        ${name}.spv > ${BUILD_DIR}/shaders/${name}.spv.h
done

echo "hurdygurdy.c"
cc ${STD} ${CONFIG} ${WARNINGS} ${INCLUDES} \
    -c ${SRC_DIR}/src/hurdygurdy.c \
    -o ${BUILD_DIR}/hurdygurdy.o

echo "libhurdygurdy.a"
ar rcs ${BUILD_DIR}/libhurdygurdy.a ${BUILD_DIR}/hurdygurdy.o

echo "test.c"
cc ${SRC_DIR}/src/test.c \
    ${STD} ${CONFIG} ${WARNINGS} ${INCLUDES} \
    -o ${BUILD_DIR}/test \
    -lc -lm -L"${BUILD_DIR}" -lhurdygurdy

END_TIME=$(date +%s.%N)
printf "Build complete: %.6f seconds\n" "$(echo "$END_TIME - $START_TIME" | bc)"

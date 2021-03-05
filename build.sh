#!/bin/bash

if [ -z "${GOBY3_COURSE_CMAKE_FLAGS}" ]; then
    GOBY3_COURSE_CMAKE_FLAGS=
fi

if [ -z "${GOBY3_COURSE_MAKE_FLAGS}" ]; then
    GOBY3_COURSE_MAKE_FLAGS=
fi

set -e -u
mkdir -p build

echo "Configuring..."
cd build
(set -x; cmake .. ${GOBY3_COURSE_CMAKE_FLAGS})
echo "Building..."
(set -x; cmake --build . -- -j`nproc` ${GOBY3_COURSE_MAKE_FLAGS} $@)

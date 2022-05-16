#!/bin/bash

if [ -z "${DCCL_CMAKE_FLAGS}" ]; then
    DCCL_CMAKE_FLAGS=
fi

if [ -z "${DCCL_MAKE_FLAGS}" ]; then
    DCCL_MAKE_FLAGS=
fi

set -e -u
echo "Configuring Dccl"
echo "cmake .. ${DCCL_CMAKE_FLAGS}"
mkdir -p build
pushd build >& /dev/null
cmake .. ${DCCL_CMAKE_FLAGS}
echo "Building Dccl"
echo "make ${DCCL_MAKE_FLAGS} $@"
make ${DCCL_MAKE_FLAGS} $@
popd >& /dev/null


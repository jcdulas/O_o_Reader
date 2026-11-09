#!/usr/bin/env bash
# Builds the WebAssembly package (64-bit WebAssembly, Memory64) in build/wasm/dist.
#
# Usage: scripts/build_wasm.sh <work folder of build_hdf5_wasm.sh>
#   (the static libhdf5 for 64-bit WebAssembly is in <work folder>/hdf5-wasm64)
#
# Needs emcmake and emcc on the PATH (the `emsdk_env` script of the Emscripten SDK), CMake and Ninja.
set -euo pipefail

WORK="${1:?usage: build_wasm.sh <work folder of build_hdf5_wasm.sh>}"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
HDF5="$(cd "$WORK" && pwd)/hdf5-wasm64"

# Warnings are not errors here: the code is kept free of them by the native build (GCC), and the Clang of Emscripten
# warns about other things.
emcmake cmake -S "$ROOT" -B "$ROOT/build/wasm" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DOOR_BUILD_TESTS=OFF \
    -DOOR_BUILD_WASM=ON \
    -DOOR_WARNINGS_AS_ERRORS=OFF \
    -DOOR_HDF5_WASM_ROOT="$HDF5"
cmake --build "$ROOT/build/wasm" -j "$(nproc 2>/dev/null || echo 4)"
echo "WebAssembly package in $ROOT/build/wasm/dist"

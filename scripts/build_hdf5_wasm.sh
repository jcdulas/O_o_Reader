#!/usr/bin/env bash
# Builds a static libhdf5 (C library only, with the deflate filter) for 64-bit WebAssembly (Memory64) with Emscripten.
#
# Usage: scripts/build_hdf5_wasm.sh <work folder>
#   The library is installed in <work folder>/hdf5-wasm64; pass that folder to CMake as OOR_HDF5_WASM_ROOT.
#
# The module of this project is built for Memory64 (wasm64: up to 16 GB of memory instead of 4 GB), and every object
# linked into it, libhdf5 and zlib included, must be built the same way.
#
# Needs: emcc, emcmake and embuilder on the PATH (the `emsdk_env` script of the Emscripten SDK), CMake and Ninja.
# HDF5 is distributed under a BSD-style licence (see COPYING in its sources); zlib comes from the Emscripten ports.
set -euo pipefail

HDF5_VERSION="1.14.6"
WORK="${1:?usage: build_hdf5_wasm.sh <work folder>}"
mkdir -p "$WORK"
WORK="$(cd "$WORK" && pwd)"
SOURCE="$WORK/hdf5-$HDF5_VERSION"
BUILD="$WORK/hdf5-build-wasm64"
INSTALL="$WORK/hdf5-wasm64"
FLAGS="-sMEMORY64=1"

if [ ! -d "$SOURCE" ]; then
    curl -sSL -o "$WORK/hdf5.tar.gz" \
        "https://github.com/HDFGroup/hdf5/releases/download/hdf5_${HDF5_VERSION}/hdf5-${HDF5_VERSION}.tar.gz"
    tar xzf "$WORK/hdf5.tar.gz" -C "$WORK"
fi

# WebAssembly has no floating-point exception flags: Emscripten's <fenv.h> does not define FE_INVALID. HDF5 only
# clears that flag after probing the float types, so the call is skipped when the flag does not exist.
FLOAT_INIT="$SOURCE/src/H5Tinit_float.c"
if ! grep -q "OOR_WASM_PATCH" "$FLOAT_INIT"; then
    sed -i 's|^    if (feclearexcept(FE_INVALID) != 0)|#ifdef FE_INVALID /* OOR_WASM_PATCH */\n    if (feclearexcept(FE_INVALID) != 0)|' "$FLOAT_INIT"
    sed -i 's|^        HSYS_GOTO_ERROR(H5E_DATATYPE, H5E_CANTSET, FAIL, "can.t clear floating-point exceptions");|&\n#endif|' "$FLOAT_INIT"
fi

# zlib from the Emscripten ports, built for Memory64
embuilder build zlib --wasm64
SYSROOT="$(dirname "$(which emcc)")/cache/sysroot"

emcmake cmake -S "$SOURCE" -B "$BUILD" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="$FLAGS" \
    -DCMAKE_EXE_LINKER_FLAGS="$FLAGS" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL" \
    -DBUILD_SHARED_LIBS=OFF -DBUILD_STATIC_LIBS=ON -DBUILD_TESTING=OFF \
    -DHDF5_BUILD_TOOLS=OFF -DHDF5_BUILD_EXAMPLES=OFF -DHDF5_BUILD_UTILS=OFF -DHDF5_BUILD_HL_LIB=OFF \
    -DHDF5_BUILD_CPP_LIB=OFF -DHDF5_BUILD_FORTRAN=OFF -DHDF5_BUILD_JAVA=OFF \
    -DHDF5_ENABLE_THREADSAFE=OFF -DHDF5_ENABLE_PARALLEL=OFF \
    -DHDF5_ENABLE_Z_LIB_SUPPORT=ON -DZLIB_USE_EXTERNAL=OFF -DHDF5_ENABLE_SZIP_SUPPORT=OFF \
    -DZLIB_INCLUDE_DIR="$SYSROOT/include" \
    -DZLIB_LIBRARY="$SYSROOT/lib/wasm64-emscripten/libz.a"
cmake --build "$BUILD" -j "$(nproc 2>/dev/null || echo 4)"
cmake --install "$BUILD"
echo "libhdf5 for 64-bit WebAssembly installed in $INSTALL"

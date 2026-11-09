# Third-party dependencies.
#
# - libhdf5 (BSD-style licence): from the system for a native build; for WebAssembly, a static libhdf5 built with
#   Emscripten for 64-bit WebAssembly (scripts/build_hdf5_wasm.sh), found through OOR_HDF5_WASM_ROOT.
# - zlib: the compression filter of HDF5 (deflate) and of the ZIP container of the Excel workbooks (src/io/zip).
#   Emscripten provides it as a port (-sUSE_ZLIB=1).
# - Catch2 v3.7.1 (Boost licence), fetched at a pinned version, for the native tests only.

include(FetchContent)

if(EMSCRIPTEN)
    set(OOR_HDF5_WASM_ROOT "$ENV{OOR_HDF5_WASM_ROOT}" CACHE PATH "Install folder of libhdf5 built with Emscripten")
    if(NOT EXISTS "${OOR_HDF5_WASM_ROOT}/lib/libhdf5.a")
        message(FATAL_ERROR "libhdf5.a not found under OOR_HDF5_WASM_ROOT='${OOR_HDF5_WASM_ROOT}'. "
                            "Build it with scripts/build_hdf5_wasm.sh and pass -DOOR_HDF5_WASM_ROOT=<install folder>.")
    endif()
    add_library(HDF5::HDF5 STATIC IMPORTED GLOBAL)
    set_target_properties(HDF5::HDF5 PROPERTIES
        IMPORTED_LOCATION "${OOR_HDF5_WASM_ROOT}/lib/libhdf5.a"
        INTERFACE_INCLUDE_DIRECTORIES "${OOR_HDF5_WASM_ROOT}/include"
        INTERFACE_COMPILE_OPTIONS "-sUSE_ZLIB=1"
        INTERFACE_LINK_OPTIONS "-sUSE_ZLIB=1")
    # zlib from the Emscripten ports (built for the memory model of the build: wasm64 here)
    add_library(ZLIB::ZLIB INTERFACE IMPORTED GLOBAL)
    set_target_properties(ZLIB::ZLIB PROPERTIES
        INTERFACE_COMPILE_OPTIONS "-sUSE_ZLIB=1"
        INTERFACE_LINK_OPTIONS "-sUSE_ZLIB=1")
else()
    find_package(HDF5 REQUIRED COMPONENTS C)
    find_package(ZLIB REQUIRED)
    find_package(Threads REQUIRED)
endif()

if(OOR_BUILD_TESTS AND NOT EMSCRIPTEN)
    set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
    set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.7.1
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(Catch2)
    list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")
endif()

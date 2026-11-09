# The WebAssembly module (64-bit WebAssembly, Memory64: see the top CMakeLists.txt): the library compiled with
# Emscripten, its embind interface (src/Bindings.cpp), and the JavaScript wrapper (js/). The package is assembled in
# <build>/dist:
#   oor_reader_core.js / .wasm   the module made by Emscripten
#   index.js, index.d.ts         the wrapper a web application imports
#   package.json
add_executable(oor_reader_core src/Bindings.cpp)
target_link_libraries(oor_reader_core PRIVATE oor_api)
set_target_properties(oor_reader_core PROPERTIES
    SUFFIX ".js"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dist")
target_link_options(oor_reader_core PRIVATE
    -lembind
    -sMODULARIZE=1
    -sEXPORT_ES6=1
    -sEXPORT_NAME=createOorReaderCore
    -sENVIRONMENT=web,worker,node
    -sALLOW_MEMORY_GROWTH=1
    -sMAXIMUM_MEMORY=16GB
    -sINITIAL_MEMORY=64MB
    -sSTACK_SIZE=4MB
    -sFORCE_FILESYSTEM=1
    "-sEXPORTED_RUNTIME_METHODS=['FS']")

# The wrapper, its types and the package description, next to the module.
foreach(file index.js index.d.ts)
    add_custom_command(TARGET oor_reader_core POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/js/${file}"
                "${CMAKE_BINARY_DIR}/dist/${file}")
endforeach()
add_custom_command(TARGET oor_reader_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/package.json"
            "${CMAKE_BINARY_DIR}/dist/package.json")

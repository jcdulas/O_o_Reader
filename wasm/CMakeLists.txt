# The WebAssembly module (64-bit WebAssembly, Memory64: see the top CMakeLists.txt): the library compiled with
# Emscripten, its embind interface (src/Bindings.cpp), and the JavaScript wrapper (js/). The package is assembled in
# <build>/dist:
#   oor_reader_core.js / .wasm   the module made by Emscripten
#   index.js, model-editing.js, index.d.ts   the wrapper a web application imports
#   package.json
add_executable(oor_reader_core src/Bindings.cpp src/ModelEditingBindings.cpp src/NamedResultsBindings.cpp)
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

# A standalone core target still assembles a runnable package. The ALL target also refreshes JS-only edits,
# without making them dependencies of the expensive WASM link.
function(oor_copy_wasm_package target)
    foreach(file index.js index.d.ts model-editing.js)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/js/${file}"
                    "${CMAKE_BINARY_DIR}/dist/${file}")
    endforeach()
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_CURRENT_SOURCE_DIR}/package.json"
                "${CMAKE_BINARY_DIR}/dist/package.json")
endfunction()
oor_copy_wasm_package(oor_reader_core)
add_custom_target(oor_reader_package ALL DEPENDS oor_reader_core)
oor_copy_wasm_package(oor_reader_package)

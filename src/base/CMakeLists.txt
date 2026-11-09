add_library(oor_base STATIC
    ModuleInfo.cpp
    diagnostics/Diagnostics.cpp
    ids/IdSpace.cpp
    platform/MemoryMapping.cpp
    text/Json.cpp
    text/MessageNumber.cpp
    text/NumberFormatter.cpp
    text/NumberParser.cpp
    threading/ParallelFor.cpp
    threading/ThreadPool.cpp)
add_library(oor::base ALIAS oor_base)
target_include_directories(oor_base PUBLIC ${PROJECT_SOURCE_DIR}/include)
# A native build runs its work on a thread pool; the WebAssembly build is single-threaded (a pool of zero workers).
if(NOT EMSCRIPTEN)
    target_link_libraries(oor_base PUBLIC Threads::Threads)
endif()
oor_enable_warnings(oor_base)

# The simple entry points of the library (Deck, Results, queries): what the WebAssembly module exposes to JavaScript.
add_library(oor_api STATIC
    Deck.cpp
    Results.cpp
    Queries.cpp
    CardEdits.cpp
    QueryArgumentsJson.cpp
    CardTables.cpp)
add_library(oor::api ALIAS oor_api)
target_include_directories(oor_api PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(oor_api PUBLIC oor_query)
oor_enable_warnings(oor_api)

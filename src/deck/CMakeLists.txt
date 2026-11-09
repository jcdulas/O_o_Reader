# Model record text, source metadata, Include limits and transactional file writes.
add_library(oor_deck STATIC
    ModuleInfo.cpp
    includes/IncludeBudget.cpp
    includes/IncludeTree.cpp
    raw/CardName.cpp
    records/ModelFileLexer.cpp
    records/RecordText.cpp
    writer/FileCommit.cpp
    writer/StagedFiles.cpp)
add_library(oor::deck ALIAS oor_deck)
target_include_directories(oor_deck PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(oor_deck PUBLIC oor_io)
oor_enable_warnings(oor_deck)

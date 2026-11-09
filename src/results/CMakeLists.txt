add_library(oor_results STATIC
    ModuleInfo.cpp
    # the model of results, whatever the file they come from
    core/ResultLayout.cpp
    core/ResultCatalogue.cpp
    core/ResultCatalogueData.cpp
    core/ReadResults.cpp
    core/ResultKind.cpp
    core/ResultFilter.cpp
    core/ResultData.cpp
    core/ResultDecodeStats.cpp
    core/ResultTable.cpp
    core/ResultTables.cpp
    core/NamedResult.cpp
    core/NamedResultKinds.cpp
    core/NamedResultBuilder.cpp
    core/NamedResultFilter.cpp
    core/MatrixResults.cpp
    core/MemoryResults.cpp
    core/MemoryTableIndex.cpp
    attach/ResultAttachment.cpp
    # the HDF5 results archive (docs/archive-format.md)
    archive/ArchiveResultFile.cpp
    archive/ArchiveWriter.cpp
    # the Excel workbook of results (an output only)
    xlsx/XlsxWriter.cpp)
add_library(oor::results ALIAS oor_results)
target_include_directories(oor_results PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(oor_results PUBLIC oor_model oor_io)
oor_enable_warnings(oor_results)

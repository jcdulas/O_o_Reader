add_library(oor_io STATIC
    ModuleInfo.cpp
    binary/Binary.cpp
    files/Files.cpp
    hdf5/Hdf5.cpp
    hdf5/Hdf5GlobalHeapCheck.cpp
    hdf5/Hdf5Internal.cpp
    hdf5/Hdf5Table.cpp
    hdf5/Hdf5Tree.cpp
    hdf5/Hdf5Writer.cpp
    hdf5/Hdf5StreamWriter.cpp
    zip/Zip.cpp)
add_library(oor::io ALIAS oor_io)
target_include_directories(oor_io PUBLIC ${PROJECT_SOURCE_DIR}/include)
# libhdf5 is a private dependency: its headers are included only by src/io/hdf5 (checked by tools/check_layering.py).
target_link_libraries(oor_io PUBLIC oor_base PRIVATE HDF5::HDF5 ZLIB::ZLIB)
oor_enable_warnings(oor_io)

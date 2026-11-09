#pragma once

#include <cstdint>

namespace oor::io
{

// Resource caps of the generic HDF5 layer. A file is hostile input: its metadata decides how many objects a walk
// meets, how deep the groups nest, how big a dataset claims to be. Every cap is checked before the work or the
// allocation it protects, and a file that exceeds one gives a LimitExceededError that names the cap. The defaults
// are far above any real the solver result file (a few thousand objects, a few hundred megabytes of tables) and far below
// what would exhaust the memory of a laptop.
struct Hdf5Limits
{
    // Groups, datasets and named types met by one tree walk.
    std::uint64_t maxObjects = 2'000'000;
    // Nesting of groups (the root is depth 0).
    std::uint32_t maxDepth = 128;
    // Longest path of an object, in bytes.
    std::uint32_t maxPathBytes = 65'535;
    // Attributes of one object.
    std::uint32_t maxAttributesPerObject = 10'000;
    // The value of an attribute is read into the tree up to this many bytes (a larger one is listed with its type and
    // shape and its value is left unread).
    std::uint64_t maxAttributeBytes = 1u << 20;
    // Whether a tree walk reads the value of an attribute made of variable-length strings. These values are stored in
    // the global heap of the file, and libhdf5 (1.14 and 2.1 alike) loops forever on some damaged heaps; every other
    // kind of damage met so far gives an error. Off by default: such an attribute is listed with its type and shape and
    // its value is left unread. Variable-length strings of a table are read when the column is read.
    bool readVarLenAttributes = false;
    // Nesting of a data type (compound in compound, array of ...) and members of one compound.
    std::uint32_t maxTypeDepth = 16;
    std::uint32_t maxCompoundMembers = 65'536;
    // Rows and bytes returned by one table read (the size of the arrays handed back).
    std::uint64_t maxReadRows = std::uint64_t{1} << 32;
    std::uint64_t maxReadBytes = std::uint64_t{8} << 30;
    // Bytes of one variable-length string element.
    std::uint64_t maxStringBytes = std::uint64_t{256} << 20;
    // Writing: objects and attributes of one file, and bytes of one attribute (the format that is written stores an
    // attribute in the object header, which holds at most 64 KiB).
    std::uint64_t maxWriteObjects = 2'000'000;
    std::uint64_t maxWriteAttributeBytes = 64'000;
};

} // namespace oor::io

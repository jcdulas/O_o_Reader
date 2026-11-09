#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <span>
#include <string>

namespace oor::io
{

// The check of the global heap of an HDF5 file that comes before libhdf5 reads variable-length data from it.
// Variable-length strings and sequences live in the global heap collections of the file (the blocks that start with
// "GCOL"); libhdf5 walks the objects of a collection by their sizes when it loads it, and a damaged size (a free-space
// object of size 0, a size that runs past its collection) makes that walk stand still or read out of the collection:
// H5Aread or H5Dread then never returns. The check walks every collection of the file the same way, on the bytes of the
// file, and refuses a collection whose objects do not tile it exactly.
//
// The collections are found by their signature: a "GCOL" followed by version 1 and three zero bytes (the layout of
// the HDF5 file format specification, section III.E). Four such bytes inside the data of a dataset would be read as
// a collection too, and refused if they do not tile one: a refusal of a sound file is possible, but needs that very
// pattern, where a missed damaged collection hangs the process.

// The first problem of the global heap collections in the bytes of a whole HDF5 file; nothing when they are sound or
// when there is none.
[[nodiscard]] std::optional<std::string> findGlobalHeapProblem(std::span<const std::byte> bytes);

// The same for a file on disk (mapped, read once). A file that cannot be mapped gives nothing: libhdf5 reports why
// it cannot be read.
[[nodiscard]] std::optional<std::string> findGlobalHeapProblem(const std::filesystem::path& file);

} // namespace oor::io

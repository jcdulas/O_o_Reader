#pragma once

#include <cstdint>
#include <string>

namespace oor::results
{

// What a result file says about itself, and the sizes of what was found in it.
struct ResultFileMetadata
{
    // The format of the file: "archive" (the HDF5 results archive of this library) or "memory" (a result set made by a
    // program, with no file).
    std::string format;
    // Where the results come from; the same as `format` for a file, "memory" for a result set made in memory.
    std::string origin;
    std::string path;
    std::uint64_t fileSize = 0;

    // The program that wrote the file, as the file says ("oor_reader 0.1.0"); empty when it does not say.
    std::string producer;
    // When the file was written, as the file says (ISO 8601); empty when it does not say.
    std::string createdAt;

    // The texts of the run (the first ones found).
    std::string title;
    std::string subtitle;
    std::string label;

    std::uint32_t tableCount = 0; // the result tables found
    std::uint32_t namedCount = 0; // the named results found (matrices, weights, ...)
    // The file was read to its end and nothing broke its structure on the way.
    bool complete = true;
};

} // namespace oor::results

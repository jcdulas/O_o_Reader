#pragma once

#include "oor/base/threading/CancellationToken.hpp"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace oor::deck
{

// A file to write: where, and what.
struct FileToWrite
{
    std::filesystem::path target;
    const std::string* text = nullptr;
};

// Where an included file goes when a deck is written with its INCLUDE structure: its path relative to the folder of
// the main source file, under the output folder. Throws IoError when the file lies outside the folder of the main file
// (".." or another drive) and `includeOutside` is false: a write must not go outside the target folder by accident.
[[nodiscard]] std::filesystem::path outputPathOfInclude(const std::filesystem::path& mainSourceDirectory,
                                                        const std::filesystem::path& includedSource,
                                                        const std::filesystem::path& outputDirectory,
                                                        bool includeOutside);

// Writes every file, all or nothing as far as the file system allows: each goes to a temporary file next to its
// target, then all are renamed over their targets. On any error (or cancellation) the temporary files are removed and
// no target is touched. Throws IoError, or OperationCancelled.
void commitFiles(const std::vector<FileToWrite>& files, const base::CancellationToken* token);

} // namespace oor::deck

#pragma once

#include "oor/base/threading/CancellationToken.hpp"
#include "oor/base/threading/ProgressCounter.hpp"
#include "oor/results/core/ResultFile.hpp"

#include <filesystem>
#include <memory>

namespace oor::results
{

struct ReadResultsOptions
{
    // Strict: a table the reader cannot understand raises an error; otherwise it is left out with a diagnostic.
    bool strict = false;
};

// Opens a file of results: an HDF5 results archive (ArchiveResultFile), recognised by its content, not by its
// extension. Throws IoError when the file cannot be read or is not a results archive.
[[nodiscard]] std::shared_ptr<ResultFile> readResults(const std::filesystem::path& path,
                                                      const ReadResultsOptions& options = {},
                                                      base::ProgressCounter* progress = nullptr,
                                                      const base::CancellationToken* token = nullptr);

} // namespace oor::results

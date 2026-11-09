#pragma once

#include "oor/model/core/Model.hpp"
#include "oor/results/core/ResultTables.hpp"
#include "oor/results/writers/ResultWriteReport.hpp"
#include "oor/results/writers/ResultWriteSession.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace oor::results
{

// How an archive is written.
struct ArchiveWriteOptions
{
    // The deflate level of the values and labels, 0 (no compression, contiguous datasets) to 9. 4 is a good balance:
    // most of the gain of 9 at a fraction of its time.
    int compressionLevel = 4;
    // The shuffle filter before deflate: it groups the bytes of the numbers by rank, which compresses floating-point
    // values much better.
    bool shuffle = true;
    // Raise (UsageError) instead of reporting when a table cannot be written.
    bool strict = false;
    // Written in the root attributes; the producer is "oor_reader <version>" when empty.
    std::string producer;
    std::string title;
    std::string subtitle;
    std::string label;
    // Optional model, serialized as native typed columns. An unrepresentable control entry raises UsageError.
    std::shared_ptr<const model::Model> model;
};

// Writes an archive by batches: `beginArchive`, then `push` tables and `pushNamed` named results as many times as
// needed, then `finish`. Each push writes its tables at once to a temporary file next to the target and keeps nothing
// of their values; `finish` writes the index and puts the file in place. `abort` (or the destruction of an unfinished
// session) removes the temporary file, so a failure never leaves a half-written archive at the target.
[[nodiscard]] std::unique_ptr<ResultWriteSession> beginArchive(const std::filesystem::path& target,
                                                               const ArchiveWriteOptions& options = {});

// Writes every table and named result of `tables` to an archive in one call.
ResultWriteReport writeArchive(const std::filesystem::path& target, const ResultTables& tables,
                               const ArchiveWriteOptions& options = {});

} // namespace oor::results

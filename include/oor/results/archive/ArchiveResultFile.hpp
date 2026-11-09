#pragma once

#include "oor/results/core/NamedResult.hpp"
#include "oor/results/core/ResultFile.hpp"
#include "oor/results/core/ResultTable.hpp"
#include "oor/results/core/ResultTables.hpp"

#include <filesystem>
#include <memory>
#include <vector>

namespace oor::results
{

// An HDF5 results archive (ArchiveFormat.hpp), opened. Opening reads the root attributes and the two index datasets;
// the values of a table are read when the table is first asked for them, step by step: a table restricted to some
// steps reads those steps only.
class ArchiveResultFile final : public ResultFile, public std::enable_shared_from_this<ArchiveResultFile>
{
public:
    // Throws IoError when the file cannot be read or is not an archive of a version this library reads.
    [[nodiscard]] static std::shared_ptr<ArchiveResultFile> open(const std::filesystem::path& path);

    // True when the file is an HDF5 file whose root says it is an archive (any version).
    [[nodiscard]] static bool isArchive(const std::filesystem::path& path);

    [[nodiscard]] std::string_view format() const noexcept override { return "archive"; }
    [[nodiscard]] const ResultFileMetadata& metadata() const noexcept override { return m_metadata; }
    [[nodiscard]] const std::vector<base::Diagnostic>& diagnostics() const noexcept override { return m_diagnostics; }
    [[nodiscard]] ResultTables tables(const ResultFilter& filter) const override;

    // The format version written in the file ("1").
    [[nodiscard]] const std::string& formatVersion() const noexcept { return m_formatVersion; }

private:
    ArchiveResultFile() = default;
    void readIndex();

    ResultFileMetadata m_metadata;
    std::vector<base::Diagnostic> m_diagnostics;
    std::string m_formatVersion;
    std::vector<std::shared_ptr<ResultTable>> m_tables;
    std::vector<std::shared_ptr<NamedResult>> m_named;
};

} // namespace oor::results

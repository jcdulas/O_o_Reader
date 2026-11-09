#include "oor/results/core/ReadResults.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/results/archive/ArchiveResultFile.hpp"

#include <filesystem>

namespace oor::results
{

std::shared_ptr<ResultFile> readResults(const std::filesystem::path& path, const ReadResultsOptions& options,
                                        base::ProgressCounter* progress, const base::CancellationToken* token)
{
    (void)options;
    (void)progress;
    if (token != nullptr)
        token->throwIfCancelled();
    if (!std::filesystem::exists(path))
        throw base::IoError("cannot open " + path.string() + ": no such file");
    if (!ArchiveResultFile::isArchive(path))
        throw base::IoError(path.string() +
                            " is not a results archive of this library (an HDF5 file whose root has "
                            "the attribute format = \"oor-results-archive\")");
    return ArchiveResultFile::open(path);
}

} // namespace oor::results

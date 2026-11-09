#pragma once

#include "oor/base/threading/CancellationToken.hpp"

#include <cstddef>
#include <filesystem>
#include <string_view>
#include <vector>

namespace oor::deck
{

// Files written piece by piece to temporary files next to their targets, then renamed over the targets all at once by
// commit(): a target is never left half written, and a large deck is written as it is formatted instead of being held
// whole in memory. Destroyed without a commit (an error, a cancellation), it removes its temporary files and no target
// is touched.
class StagedFiles
{
public:
    StagedFiles() = default;
    ~StagedFiles();
    StagedFiles(const StagedFiles&) = delete;
    StagedFiles& operator=(const StagedFiles&) = delete;

    // Creates the (empty) temporary file of `target`, and its folder when needed; returns its index. Throws IoError.
    std::size_t open(const std::filesystem::path& target);
    // Appends `text` to the temporary file `index`. Throws IoError.
    void append(std::size_t index, std::string_view text);
    // Renames every temporary file over its target, in the order they were opened. Throws OperationCancelled when
    // `token` is cancelled (before the first rename), IoError when a rename fails.
    void commit(const base::CancellationToken* token);

private:
    struct Staged
    {
        std::filesystem::path target;
        std::filesystem::path temporary;
    };

    [[noreturn]] static void throwCannotWrite(const std::filesystem::path& target);

    std::vector<Staged> m_files;
    std::size_t m_renamed = 0; // the files [0, m_renamed) are at their targets
};

} // namespace oor::deck

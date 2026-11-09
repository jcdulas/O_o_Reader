#include "oor/deck/writer/FileCommit.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"
#include "oor/deck/writer/StagedFiles.hpp"

namespace oor::deck
{

std::filesystem::path outputPathOfInclude(const std::filesystem::path& mainSourceDirectory,
                                          const std::filesystem::path& includedSource,
                                          const std::filesystem::path& outputDirectory, bool includeOutside)
{
    const std::filesystem::path relative = includedSource.lexically_relative(mainSourceDirectory);
    const bool outside = relative.empty() || relative.is_absolute() || *relative.begin() == "..";
    if (outside)
    {
        if (!includeOutside)
            throw base::IoError("the included file " + base::pathToUtf8(includedSource) +
                                " lies outside the folder of the main file; writing it would go outside the target "
                                "folder (allow it with includeOutside)");
        return includedSource.is_absolute() && relative.empty() ? includedSource
                                                                : (outputDirectory / relative).lexically_normal();
    }
    return (outputDirectory / relative).lexically_normal();
}

void commitFiles(const std::vector<FileToWrite>& files, const base::CancellationToken* token)
{
    StagedFiles staged;
    for (const FileToWrite& file : files)
        staged.append(staged.open(file.target), *file.text);
    staged.commit(token);
}

} // namespace oor::deck

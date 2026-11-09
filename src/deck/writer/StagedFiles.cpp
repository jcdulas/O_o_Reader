#include "oor/deck/writer/StagedFiles.hpp"

#include "oor/base/errors/Errors.hpp"
#include "oor/base/text/TextUtilities.hpp"

#include <fstream>
#include <system_error>

namespace oor::deck
{

StagedFiles::~StagedFiles()
{
    for (std::size_t index = m_renamed; index < m_files.size(); ++index)
    {
        std::error_code ignored;
        std::filesystem::remove(m_files[index].temporary, ignored);
    }
}

std::size_t StagedFiles::open(const std::filesystem::path& target)
{
    if (!target.parent_path().empty())
    {
        std::error_code ignored; // a folder that cannot be made shows as a file that cannot be written
        std::filesystem::create_directories(target.parent_path(), ignored);
    }
    std::filesystem::path temporary = target;
    temporary += ".oor_tmp";
    m_files.push_back(Staged{target, temporary});
    // The stream is opened for each append and closed after it, so that a deck of many included files never holds
    // more open files than one.
    std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
    stream.close();
    if (!stream)
        throwCannotWrite(target);
    return m_files.size() - 1;
}

void StagedFiles::append(std::size_t index, std::string_view text)
{
    if (text.empty())
        return;
    const Staged& file = m_files.at(index);
    std::ofstream stream(file.temporary, std::ios::binary | std::ios::app);
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    stream.close();
    if (!stream)
        throwCannotWrite(file.target);
}

void StagedFiles::commit(const base::CancellationToken* token)
{
    if (token != nullptr)
        token->throwIfCancelled();
    for (; m_renamed < m_files.size(); ++m_renamed)
    {
        const Staged& file = m_files[m_renamed];
        std::error_code error;
        std::filesystem::rename(file.temporary, file.target, error);
        if (error)
            throw base::IoError("cannot replace file: " + base::pathToUtf8(file.target) + " (" + error.message() + ")");
    }
}

void StagedFiles::throwCannotWrite(const std::filesystem::path& target)
{
    throw base::IoError("cannot write file: " + base::pathToUtf8(target));
}

} // namespace oor::deck

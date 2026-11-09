#include "oor/deck/includes/IncludeBudget.hpp"

#include "oor/base/text/TextUtilities.hpp"

#include <system_error>

namespace oor::deck
{

bool IncludeBudget::admit(const std::filesystem::path& path, std::uint64_t bytes)
{
    std::error_code error;
    std::filesystem::path canonical = std::filesystem::weakly_canonical(path, error);
    if (error)
        canonical = path.lexically_normal();
    const std::string key = base::pathToUtf8(canonical);
    if (!m_files.contains(key))
    {
        m_files.insert(key);
        m_distinctBytes += bytes;
        return true;
    }
    const std::uint64_t allowedRepeats = BASE_REPEATS + REPEATS_PER_FILE * m_files.size();
    const std::uint64_t allowedBytes = BASE_REPEATED_BYTES + REPEATED_BYTES_PER_BYTE * m_distinctBytes;
    if (m_repeats + 1 > allowedRepeats || m_repeatedBytes + bytes > allowedBytes)
        return false;
    ++m_repeats;
    m_repeatedBytes += bytes;
    return true;
}

std::string IncludeBudget::describe() const
{
    return std::to_string(m_files.size()) + " distinct files of " + std::to_string(m_distinctBytes) + " bytes, " +
           std::to_string(m_repeats) + " includes of a file already read (" + std::to_string(m_repeatedBytes) +
           " bytes read again); the bound is " + std::to_string(BASE_REPEATS + REPEATS_PER_FILE * m_files.size()) +
           " includes and " + std::to_string(BASE_REPEATED_BYTES + REPEATED_BYTES_PER_BYTE * m_distinctBytes) +
           " bytes";
}

} // namespace oor::deck

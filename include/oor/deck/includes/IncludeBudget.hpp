#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_set>

namespace oor::deck
{

// What the INCLUDE statements of one read may add. A file may be included again (the
// same load file in two subcases), but a few small files that include each other several times over (each including
// the next one twice: 2^31 copies at the depth limit of 32) must not make a read last forever or fill the memory. The
// repeated includes are bounded in number and in bytes, in proportion to the distinct files read: the cost of a read
// stays proportional to what is on the disk.
class IncludeBudget
{
public:
    // Includes again of a file already read: this many, plus REPEATS_PER_FILE for each distinct file.
    static constexpr std::uint64_t BASE_REPEATS = 1'000;
    static constexpr std::uint64_t REPEATS_PER_FILE = 10;
    // Bytes read again through those includes: this many, plus REPEATED_BYTES_PER_BYTE for each distinct byte.
    static constexpr std::uint64_t BASE_REPEATED_BYTES = std::uint64_t{256} << 20;
    static constexpr std::uint64_t REPEATED_BYTES_PER_BYTE = 16;

    // Records that `path`, of `bytes` bytes, is read (the main file, then each file an INCLUDE opens). Returns false,
    // recording nothing, when it is a file already read and reading it again would pass the bounds.
    [[nodiscard]] bool admit(const std::filesystem::path& path, std::uint64_t bytes);

    // The counts, for the message of a refused include.
    [[nodiscard]] std::string describe() const;

private:
    std::unordered_set<std::string> m_files; // the distinct files, by canonical path
    std::uint64_t m_distinctBytes = 0;
    std::uint64_t m_repeats = 0;
    std::uint64_t m_repeatedBytes = 0;
};

} // namespace oor::deck

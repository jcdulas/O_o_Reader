#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace oor::results
{

// What writing a result file did: every table written, and every table left out with the reason (a table a format
// cannot hold is reported, never dropped silently).

// A table written: what it was, and where it went in the file.
struct WrittenTable
{
    std::string kind;      // "displacements", "stress", "matrix", ...
    std::string container; // the element type or variant of a family kind ("ShellQuad4"), the name of a named result
    std::int32_t subcase = 0;
    std::string fileTable; // where it went: the group of an archive ("/data/tables/000001"), the sheet of a workbook
    std::string layout;    // the row layout of the table ("centre" or "corner"), empty for most tables
    std::size_t steps = 0;
    std::size_t rows = 0;
};

// Why a table was left out. The codes are stable: programs test them.
namespace write_reasons
{
// The format has no place for the table (an empty table, a kind the format does not hold).
inline constexpr std::string_view NO_FILE_LAYOUT = "no-file-layout";
} // namespace write_reasons

struct SkippedTable
{
    std::string kind;
    std::string container;
    std::int32_t subcase = 0;
    std::string reason;  // one of write_reasons
    std::string message; // what exactly, for a person
};

struct ResultWriteReport
{
    std::string path;
    std::string format; // "archive", "xlsx"
    bool finished = false;
    std::vector<WrittenTable> written;
    std::vector<SkippedTable> skipped;
    std::uint64_t bytes = 0;

    // Nothing was left out.
    [[nodiscard]] bool complete() const noexcept { return skipped.empty(); }
};

} // namespace oor::results

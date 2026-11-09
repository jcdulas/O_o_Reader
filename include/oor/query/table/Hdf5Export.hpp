#pragma once

#include "oor/query/table/QueryTable.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace oor::query
{

// One table to export, under a name ("maxStress", "elementMasses").
struct NamedQueryTable
{
    std::string name;
    std::shared_ptr<const QueryTable> table;
};

// Writes query tables (custom outputs: margins, envelopes, cuts, any named query) to an HDF5 file of this library's own
// layout, described in docs/archive-format.md ("query tables"):
//
//   /                              attributes: format = "oor-query-tables", formatVersion = "1", producer
//   /tables/<name>/columnNames    the columns, in order (variable-length strings)
//   /tables/<name>/columns/<column>          one value a row: int64 for whole numbers, float64, or strings
//   /tables/<name>/columns/<column>.offsets  for a list column: row i holds the values [offsets[i], offsets[i + 1])
//   /tables/<name>/columns/<column>.valid    when the column has nulls: 1 a row with a value, 0 a null
//
// A name may hold letters, digits, '_' and '-'; anything else is refused (UsageError), as is a name given twice.
// `compressionLevel` is the deflate level (0 to 9) of the columns of more than a thousand values.
void writeQueryTables(const std::filesystem::path& path, const std::vector<NamedQueryTable>& tables,
                      int compressionLevel = 4);

} // namespace oor::query

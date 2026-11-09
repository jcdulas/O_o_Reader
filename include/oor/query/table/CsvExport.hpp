#pragma once

#include "oor/query/table/QueryTable.hpp"

#include <iosfwd>

namespace oor::query
{

// Writes a table as CSV: a header with the names of the columns, then one line for each row, `\n` ending the lines. A
// number is written with the digits that give the same value back (a NaN as `nan`), a bool as `true` or `false`, a null
// as nothing, a text between quotes when it holds a comma, a quote, a line break or a space at either end (a quote
// doubled), and a list between quotes as `[1, 2, 3]`.
void writeCsv(const QueryTable& table, std::ostream& out);

} // namespace oor::query

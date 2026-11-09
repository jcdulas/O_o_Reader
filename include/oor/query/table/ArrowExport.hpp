#pragma once

#include "oor/base/arrow/ArrowAbi.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <memory>
#include <string>

namespace oor::query
{

// Hands a table to an Arrow consumer through the Arrow C Data Interface (and its stream form), without copying any
// data: the exported arrays point at the buffers of the columns, and the release callback of each structure holds a
// reference to those buffers, so they live as long as the consumer needs them, whatever becomes of the table and of the
// model it was made from. The memory layout is the one the table has (see ColumnType): nothing is converted. This
// module does not depend on the Arrow library, only on the format.
//
// A table is exported as one struct array (a record batch): one child per column.
class ArrowExport
{
public:
    // The Arrow type of a column as a format string of the C Data Interface ("i", "g", "u", "+w:4", ...).
    [[nodiscard]] static std::string formatOf(const QueryColumn& column);

    // The schema of a table: a struct with one field per column. `out` is filled; the caller (the consumer) releases
    // it.
    static void exportSchema(const QueryTable& table, ArrowSchema* out);
    // The schema of one column, named `name`.
    static void exportColumnSchema(const std::string& name, const QueryColumn& column, ArrowSchema* out);

    // The data of a table as one struct array (a record batch), and of one column as an array.
    static void exportArray(const QueryTable& table, ArrowArray* out);
    static void exportColumnArray(const QueryColumn& column, ArrowArray* out);

    // A stream that gives the table as one batch and then ends. It holds the table.
    static void exportStream(std::shared_ptr<const QueryTable> table, ArrowArrayStream* out);
};

} // namespace oor::query

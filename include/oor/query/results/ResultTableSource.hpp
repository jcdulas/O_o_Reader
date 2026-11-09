#pragma once

#include "oor/query/results/ResultSets.hpp"
#include "oor/query/table/QueryTable.hpp"
#include "oor/results/core/NamedResult.hpp"
#include "oor/results/core/ResultTable.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace oor::query
{

// A decoded result table as a QueryTable (what `ResultTable.to_table()` returns and what `results.query(...)` starts
// from): one row for each step and row of the table, steps first; the columns `subcase`, the step (`time`, `freq`,
// `mode` with `eigenvalue`, or `loadStep`), the IDs (`nodeId`, or `elementId` with `nodeId`, `layer` and `position` for
// element tables), the text labels and then one column for each component (a complex component is two: `<name>_real`
// and
// `<name>_imag`), in the precision of the data: float32 for a 32-bit file, nothing is widened. With `block` the
// components are one column `values`, a fixed list that shares the decoded block without copy. When the table is
// attached to a model the columns that link the rows to it are added (`modelNodeId`, or `modelElementId`, `propId` and
// `card`).
[[nodiscard]] std::shared_ptr<QueryTable> resultTableAsQuery(const std::shared_ptr<const results::ResultData>& decoded,
                                                             const results::ResultTable& table, bool block);

// The columns that link the rows of a table to the model it is attached to (the current IDs of the cards, the card
// type, the property); nothing when the table is not attached.
void addResultModelColumns(QueryTable& table, const results::ResultTable& source, const results::ResultData& data);

// The one-dimensional arrays and the texts of a named result as a table, a complex array as two columns. With `lists`
// an array of several dimensions whose first extent is the number of rows is a column of fixed lists in the precision
// of the array (the stability derivatives, the aerodynamic forces); the other results keep their one-dimensional arrays
// only.
[[nodiscard]] std::shared_ptr<QueryTable> namedResultAsQuery(const results::NamedResultData& data, bool lists = false);

// Tables put end to end: the union of their columns in order of first appearance, nulls where a table lacks one; a
// column of two types is an error that says so (`what` names the tables in the message).
[[nodiscard]] std::shared_ptr<const QueryTable> concatenateResultTables(
    const std::vector<std::shared_ptr<const QueryTable>>& parts, const std::string& what);

// The table that `results.query(name)` starts from: every table of the container `name` of the sets, one after the
// other
// ("displacements", "stress.ShellQuad4", "psd.displacements", "rms.stress.ShellQuad4"; also the named results
// "aeroFlutterSummary", "aeroTrimVariables", ...). Tables that do not have the same columns are put together with nulls
// where a column is missing; a column of two types is an error that says so. Throws QueryError for a name that is none
// of these, with the names the sets do have.
[[nodiscard]] std::shared_ptr<const QueryTable> resultSourceTable(const std::vector<ResultSet>& sets,
                                                                  std::string_view name,
                                                                  const results::DecodeContext& context);

// The names `resultSourceTable` accepts for the sets (the containers that hold at least one table, and the named
// results), sorted.
[[nodiscard]] std::vector<std::string> resultSourceNames(const std::vector<ResultSet>& sets);

} // namespace oor::query

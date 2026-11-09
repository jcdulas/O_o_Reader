#pragma once

#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/table/QueryTable.hpp"

#include <memory>
#include <string>
#include <vector>

namespace oor::query
{

// Which extreme of the value makes a row critical.
enum class EnvelopeBy : std::uint8_t
{
    Max,
    Min,
    AbsMax // the largest magnitude; the value reported keeps its sign
};

[[nodiscard]] std::string_view envelopeByName(EnvelopeBy by) noexcept;
// "max", "min" or "absMax" (also "absmax"); throws QueryError for another word.
[[nodiscard]] EnvelopeBy envelopeByFromName(std::string_view name);

// The envelope of a table over some of its columns (result queries take envelopes over analyses
// as over subcases): the rows are grouped by the key columns, and in each group the row whose `value` is the extreme is
// the one kept, with all of its columns. A column named in `over` is not a key, so the rows that differ only in it fall
// in one group and the row kept says which of them is critical (`analysis` is then the label of the critical analysis,
// `subcase` its critical subcase).
struct EnvelopeSpec
{
    // The column that decides: a number (a result column or an integer); a NaN or null never wins.
    std::string value;
    EnvelopeBy by = EnvelopeBy::Max;
    // The columns the envelope is taken over: not keys. At least one.
    std::vector<std::string> over = {"analysis"};
    // The columns that identify an item. Empty: every column that is not a real number, a list, one of `over`, the
    // value, the provenance (`source`) or the frame, plus the step columns (`time`, `freq`, `mode`, `loadStep`,
    // `eigenvalue`) which are real numbers that identify a step.
    std::vector<std::string> keys;
    // Only the rows of these analyses (labels of the `analysis` column); empty: every row. An unknown label is an
    // error.
    std::vector<std::string> analyses;
};

// The envelope table: the columns of the table, in their order, then
// `envelopeCount` (int64: how many rows the group had). One row per group, groups in the order of their keys. The
// provenance of the input is carried over. Throws QueryError for a column that does not exist, a value that is not a
// number, an `analyses` label that is not in the table.
[[nodiscard]] std::shared_ptr<const QueryTable> envelopeTable(const QueryTable& table, const EnvelopeSpec& spec,
                                                              ExecutionContext& context);

} // namespace oor::query

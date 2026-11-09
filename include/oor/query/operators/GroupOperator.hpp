#pragma once

#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/plan/QueryPlan.hpp"
#include "oor/query/table/QueryColumn.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace oor::query
{

// How the rows fall in groups: the rows in the order of their keys, and where each group starts in that order. Group
// `g` is the rows `order[starts[g]] .. order[starts[g + 1] - 1]`, in their original order (the sort is stable). The
// groups come out in increasing order of the keys, a null or a NaN last; two nulls are the same key.
struct GroupLayout
{
    std::vector<std::uint32_t> order;
    std::vector<std::uint32_t> starts; // groupCount() + 1 entries

    [[nodiscard]] std::size_t groupCount() const noexcept { return starts.empty() ? 0 : starts.size() - 1; }
    // The first row (of the original rows) of group `group`.
    [[nodiscard]] std::uint32_t firstRow(std::size_t group) const noexcept { return order[starts[group]]; }
};

// Groups `rowCount` rows by the key columns. With no key there is one group of all the rows (even when there are none).
[[nodiscard]] GroupLayout groupRows(std::span<const QueryColumn* const> keys, std::size_t rowCount,
                                    ExecutionContext& context);

// The aggregate of `column` in each group, one value per group; `column` is null for the count of rows.
//
//   Count   the rows of the group, or with a column the rows where it is not null and not NaN (Int64)
//   Sum     Int64 for an integer or Bool column (a Bool counts the true), Float64 for a Float64 column; null and NaN
//           are skipped and an empty sum is 0
//   Min Max the type of the column; null when every value is missing; not defined for Bool and text
//   Mean    Float64, the sum over the values present; null when none
//
// Each group is summed in blocks of a fixed number of rows, and the sums of the blocks added in order, so that a value
// is the same for any number of threads and does not depend on the groups around it.
[[nodiscard]] QueryColumn aggregate(AggregateFunction function, const QueryColumn* column, const GroupLayout& layout,
                                    ExecutionContext& context);

// For ArgMax and ArgMin: the row of each group where `column` is largest (or smallest), NO_ROW for a group where every
// value is null or NaN. A tie gives the first such row in the order of the rows (the order of the groups keeps it), so
// the answer does not depend on the number of threads. Defined for the numeric columns only.
[[nodiscard]] std::vector<std::uint32_t> extremeRows(bool largest, const QueryColumn& column, const GroupLayout& layout,
                                                     ExecutionContext& context);

} // namespace oor::query

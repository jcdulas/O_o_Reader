#pragma once

#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/table/QueryColumn.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace oor::query
{

// A row number that stands for "no row": `take` gives a null for it. A join uses it for a key that matches nothing.
inline constexpr std::uint32_t NO_ROW = 0xFFFFFFFFU;

// The rows of `column` that `rows` lists, in that order; a row can be listed twice. NO_ROW gives a null (NaN in a
// Float64 column). The data is copied in chunks over the pool, each chunk to its own part of the result.
[[nodiscard]] QueryColumn take(const QueryColumn& column, std::span<const std::uint32_t> rows,
                               const ExecutionContext& context);

// `length` nulls of the same layout as `like` (its type, fixed width and element type).
[[nodiscard]] QueryColumn nullColumnLike(const QueryColumn& like, std::int64_t length);

// True when the columns have the same type, fixed width and element type: they can be put end to end.
[[nodiscard]] bool sameLayout(const QueryColumn& left, const QueryColumn& right);

// Puts columns end to end. A null pointer in `parts` stands for `lengths[i]` nulls; the others must have the length
// `lengths[i]` and one layout (and not be dictionary encoded). At least one part must be a column.
[[nodiscard]] QueryColumn concatenate(std::span<const QueryColumn* const> parts, std::span<const std::int64_t> lengths);

// A fixed list as a list (the offsets are i * width), so that fixed lists of different widths can be put end to end.
[[nodiscard]] QueryColumn fixedListAsList(const QueryColumn& column);

// A validity bitmap from one flag per row (not zero: the value is there), and the number of rows with no value.
[[nodiscard]] ColumnBuffer validityFromFlags(std::span<const std::uint8_t> valid, std::int64_t& nullCount);

// The values of a numeric column as doubles (integers converted, Bool as 0 and 1), `begin` to `end`, into `out`. A null
// is written as NaN.
void copyAsDouble(const QueryColumn& column, std::size_t begin, std::size_t end, double* out);

} // namespace oor::query

#pragma once

#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/table/QueryColumn.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace oor::query
{

// A column to sort by and the direction.
struct SortColumn
{
    const QueryColumn* column = nullptr;
    bool descending = false;
};

// The order of the rows: a permutation `p` such that row `p[0]` comes first. The sort is stable (rows with equal keys
// keep their order), and a null or a NaN is last in both directions. It is a merge sort over chunks of a fixed size, so
// the permutation is the same for any number of threads. Throws QueryTypeError for a list column.
[[nodiscard]] std::vector<std::uint32_t> sortedOrder(std::span<const SortColumn> keys, std::size_t rowCount,
                                                     ExecutionContext& context);

// Compares two rows of one column, ascending, a null or a NaN after everything (and equal to another one): <0, 0, >0.
// The group operator uses it to tell whether two rows have the same key.
[[nodiscard]] int compareKeys(const QueryColumn& column, std::size_t left, std::size_t right) noexcept;

// Compares two present values of one column: <0, 0, >0.
[[nodiscard]] int compareRows(const QueryColumn& column, std::size_t left, std::size_t right) noexcept;

} // namespace oor::query

#pragma once

#include "oor/query/table/QueryColumn.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oor::query
{

// The values of a numeric column as doubles (a null is NaN).
[[nodiscard]] std::vector<double> asDoubles(const QueryColumn& column);
// The values of an integer column as int32 (a null is 0).
[[nodiscard]] std::vector<std::int32_t> asInt32(const QueryColumn& column);
// One component of a fixed list column of doubles (the x of a position): the values of row i are at i * width + k.
[[nodiscard]] std::vector<double> component(const QueryColumn& fixedList, std::size_t index);
// The texts of a text or dictionary column (empty for a null).
[[nodiscard]] std::vector<std::string> asTexts(const QueryColumn& column);
// The validity of a column as one byte per row (1: a value).
[[nodiscard]] std::vector<std::uint8_t> validityOf(const QueryColumn& column);

// Orders the rows of a table: the permutation that sorts by `keys` (each a value that compares with <), stable.
template <typename Less>
[[nodiscard]] std::vector<std::uint32_t> orderBy(std::size_t count, const Less& less);

} // namespace oor::query

#include <algorithm>
#include <numeric>

namespace oor::query
{

template <typename Less>
std::vector<std::uint32_t> orderBy(std::size_t count, const Less& less)
{
    std::vector<std::uint32_t> order(count);
    std::iota(order.begin(), order.end(), 0U);
    std::stable_sort(order.begin(), order.end(),
                     [&](std::uint32_t left, std::uint32_t right) { return less(left, right); });
    return order;
}

} // namespace oor::query

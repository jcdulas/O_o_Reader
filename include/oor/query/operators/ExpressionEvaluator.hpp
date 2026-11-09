#pragma once

#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/operators/Expression.hpp"
#include "oor/query/table/QueryColumn.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace oor::query
{

// Finds a column of the table the expression runs on by name; null when there is none.
using ColumnLookup = std::function<const QueryColumn*(const std::string&)>;

// Evaluates an expression over whole columns. The rows are cut into chunks of EXPRESSION_CHUNK rows; each chunk is
// evaluated with temporary vectors that stay in cache and writes only its own part of the result, over the pool, so
// the result does not depend on the number of threads.
class ExpressionEvaluator
{
public:
    // The value of the expression for each of the `rowCount` rows: a Bool, Int64, Float64 or Utf8 column (with a
    // validity where a value is null). Throws QueryError for a column that does not exist and QueryTypeError for
    // operands of the wrong types, before touching any data.
    [[nodiscard]] static QueryColumn evaluate(const Expression& expression, const ColumnLookup& lookup,
                                              std::size_t rowCount, const ExecutionContext& context);

    // The rows, in increasing order, where the expression (which must be a condition) is true.
    [[nodiscard]] static std::vector<std::uint32_t> selectRows(const Expression& expression, const ColumnLookup& lookup,
                                                               std::size_t rowCount, const ExecutionContext& context);

    // The type the expression has when the columns have the types `lookup` gives.
    [[nodiscard]] static ValueType resultType(const Expression& expression, const ColumnLookup& lookup);
};

} // namespace oor::query

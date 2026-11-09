#include "oor/query/operators/GroupOperator.hpp"

#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ColumnOps.hpp"
#include "oor/query/operators/SortOperator.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace oor::query
{

namespace
{

// The rows of a group that are summed before the partial sum is added to the total of the group.
constexpr std::size_t AGGREGATE_BLOCK = 8192;
constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

struct Accumulator
{
    std::int64_t count = 0;
    double sum = 0.0;
    std::int64_t integerSum = 0;
    double minimum = std::numeric_limits<double>::infinity();
    double maximum = -std::numeric_limits<double>::infinity();
    std::int64_t integerMinimum = std::numeric_limits<std::int64_t>::max();
    std::int64_t integerMaximum = std::numeric_limits<std::int64_t>::min();

    void merge(const Accumulator& other) noexcept
    {
        count += other.count;
        sum += other.sum;
        integerSum += other.integerSum;
        minimum = std::fmin(minimum, other.minimum);
        maximum = std::fmax(maximum, other.maximum);
        integerMinimum = std::min(integerMinimum, other.integerMinimum);
        integerMaximum = std::max(integerMaximum, other.integerMaximum);
    }
};

template <typename T>
void addIntegers(const QueryColumn& column, const std::uint32_t* rows, std::size_t count, Accumulator& result)
{
    const auto values = column.as<T>();
    for (std::size_t index = 0; index < count; ++index)
    {
        const std::uint32_t row = rows[index];
        if (column.isNull(row))
            continue;
        const auto value = static_cast<std::int64_t>(values[row]);
        ++result.count;
        result.integerSum += value;
        result.integerMinimum = std::min(result.integerMinimum, value);
        result.integerMaximum = std::max(result.integerMaximum, value);
    }
}

// The values of `rows` (a block of one group) in one accumulator.
Accumulator accumulate(const QueryColumn& column, const std::uint32_t* rows, std::size_t count)
{
    Accumulator result;
    switch (column.type())
    {
    case ColumnType::Float64:
    {
        const auto values = column.as<double>();
        for (std::size_t index = 0; index < count; ++index)
        {
            const std::uint32_t row = rows[index];
            const double value = values[row];
            if (std::isnan(value) || column.isNull(row))
                continue;
            ++result.count;
            result.sum += value;
            result.minimum = std::fmin(result.minimum, value);
            result.maximum = std::fmax(result.maximum, value);
        }
        break;
    }
    case ColumnType::Float32:
    {
        const auto values = column.as<float>();
        for (std::size_t index = 0; index < count; ++index)
        {
            const std::uint32_t row = rows[index];
            const double value = static_cast<double>(values[row]);
            if (std::isnan(value) || column.isNull(row))
                continue;
            ++result.count;
            result.sum += value;
            result.minimum = std::fmin(result.minimum, value);
            result.maximum = std::fmax(result.maximum, value);
        }
        break;
    }
    case ColumnType::Int32:
        addIntegers<std::int32_t>(column, rows, count, result);
        break;
    case ColumnType::UInt32:
        addIntegers<std::uint32_t>(column, rows, count, result);
        break;
    case ColumnType::Int64:
        addIntegers<std::int64_t>(column, rows, count, result);
        break;
    case ColumnType::UInt8:
        addIntegers<std::uint8_t>(column, rows, count, result);
        break;
    case ColumnType::Bool:
        for (std::size_t index = 0; index < count; ++index)
        {
            const std::uint32_t row = rows[index];
            if (column.isNull(row))
                continue;
            ++result.count;
            result.integerSum += column.boolAt(row) ? 1 : 0;
        }
        break;
    default: // text: only the count of values is asked for
        for (std::size_t index = 0; index < count; ++index)
        {
            if (!column.isNull(rows[index]))
                ++result.count;
        }
        break;
    }
    return result;
}

Accumulator accumulateGroup(const QueryColumn& column, const GroupLayout& layout, std::size_t group)
{
    Accumulator total;
    const std::size_t end = layout.starts[group + 1];
    for (std::size_t begin = layout.starts[group]; begin < end; begin += AGGREGATE_BLOCK)
    {
        const std::size_t count = std::min(AGGREGATE_BLOCK, end - begin);
        total.merge(accumulate(column, layout.order.data() + begin, count));
    }
    return total;
}

bool isFloat(const QueryColumn& column) noexcept
{
    return isFloating(column.type());
}

} // namespace

GroupLayout groupRows(std::span<const QueryColumn* const> keys, std::size_t rowCount, ExecutionContext& context)
{
    GroupLayout layout;
    if (keys.empty())
    {
        layout.order.resize(rowCount);
        std::iota(layout.order.begin(), layout.order.end(), 0U);
        layout.starts = {0U, static_cast<std::uint32_t>(rowCount)};
        return layout;
    }
    std::vector<SortColumn> sortKeys;
    sortKeys.reserve(keys.size());
    for (const QueryColumn* key : keys)
        sortKeys.push_back(SortColumn{key, false});
    layout.order = sortedOrder(sortKeys, rowCount, context);
    layout.starts.reserve(rowCount / 4 + 2);
    for (std::size_t position = 0; position < rowCount; ++position)
    {
        bool sameAsPrevious = position > 0;
        if (sameAsPrevious)
        {
            for (const QueryColumn* key : keys)
            {
                if (compareKeys(*key, layout.order[position - 1], layout.order[position]) != 0)
                {
                    sameAsPrevious = false;
                    break;
                }
            }
        }
        if (!sameAsPrevious)
            layout.starts.push_back(static_cast<std::uint32_t>(position));
    }
    layout.starts.push_back(static_cast<std::uint32_t>(rowCount));
    return layout;
}

std::vector<std::uint32_t> extremeRows(bool largest, const QueryColumn& column, const GroupLayout& layout,
                                       ExecutionContext& context)
{
    const std::string name = largest ? "argmax" : "argmin";
    if (column.type() == ColumnType::FixedList || column.type() == ColumnType::List)
        throw QueryTypeError("a list column cannot be aggregated");
    if (isText(column.type()) || column.type() == ColumnType::Bool)
        throw QueryTypeError("'" + name + "' is defined for a numeric column only");
    // The values as doubles once (an ID or a count is exact in a double up to 2^53); a null becomes NaN and is skipped.
    std::vector<double> values(static_cast<std::size_t>(column.length()));
    copyAsDouble(column, 0, values.size(), values.data());
    const std::size_t groups = layout.groupCount();
    std::vector<std::uint32_t> rows(groups, NO_ROW);
    context.parallel(groups, 256,
                     [&](base::IndexRange range)
                     {
                         for (std::size_t group = range.begin; group < range.end; ++group)
                         {
                             double best = 0.0;
                             for (std::size_t position = layout.starts[group]; position < layout.starts[group + 1];
                                  ++position)
                             {
                                 const std::uint32_t row = layout.order[position];
                                 const double value = values[row];
                                 if (std::isnan(value))
                                     continue;
                                 if (rows[group] == NO_ROW || (largest ? value > best : value < best))
                                 {
                                     rows[group] = row;
                                     best = value;
                                 }
                             }
                         }
                     });
    return rows;
}

QueryColumn aggregate(AggregateFunction function, const QueryColumn* column, const GroupLayout& layout,
                      ExecutionContext& context)
{
    const std::size_t groups = layout.groupCount();
    if (function == AggregateFunction::ArgMax || function == AggregateFunction::ArgMin)
        throw QueryError("argmax and argmin give the value of another column: see extremeRows");
    if (column == nullptr)
    {
        if (function != AggregateFunction::Count)
            throw QueryError("an aggregate other than count needs a column");
        std::vector<std::int64_t> counts(groups);
        for (std::size_t group = 0; group < groups; ++group)
            counts[group] = static_cast<std::int64_t>(layout.starts[group + 1] - layout.starts[group]);
        return QueryColumn::primitive(std::move(counts));
    }
    if (column->type() == ColumnType::FixedList || column->type() == ColumnType::List)
        throw QueryTypeError("a list column cannot be aggregated");
    if (isText(column->type()) && function != AggregateFunction::Count)
        throw QueryTypeError("'" + std::string(aggregateName(function)) + "' is not defined for a text column");
    if (column->type() == ColumnType::Bool &&
        (function == AggregateFunction::Min || function == AggregateFunction::Max))
        throw QueryTypeError("'" + std::string(aggregateName(function)) + "' is not defined for a bool column");

    std::vector<Accumulator> accumulators(groups);
    if (groups == 1 && layout.order.size() > 2 * AGGREGATE_BLOCK)
    {
        // One big group: the blocks are done in parallel and added in order, the same blocks as the serial loop.
        const std::size_t total = layout.order.size();
        const std::size_t blocks = (total + AGGREGATE_BLOCK - 1) / AGGREGATE_BLOCK;
        std::vector<Accumulator> partial(blocks);
        context.parallel(blocks, 1,
                         [&](base::IndexRange range)
                         {
                             for (std::size_t block = range.begin; block < range.end; ++block)
                             {
                                 const std::size_t begin = block * AGGREGATE_BLOCK;
                                 const std::size_t count = std::min(AGGREGATE_BLOCK, total - begin);
                                 partial[block] = accumulate(*column, layout.order.data() + begin, count);
                             }
                         });
        for (const Accumulator& block : partial)
            accumulators[0].merge(block);
    }
    else
    {
        context.parallel(groups, 256,
                         [&](base::IndexRange range)
                         {
                             for (std::size_t group = range.begin; group < range.end; ++group)
                                 accumulators[group] = accumulateGroup(*column, layout, group);
                         });
    }

    const bool real = isFloat(*column);
    const auto missingGroups = [&](auto&& hasValue)
    {
        std::vector<std::uint8_t> valid(groups);
        for (std::size_t group = 0; group < groups; ++group)
            valid[group] = hasValue(accumulators[group]) ? 1 : 0;
        return valid;
    };
    const auto withValidity = [&](QueryColumn result, const std::vector<std::uint8_t>& valid)
    {
        std::int64_t nullCount = 0;
        ColumnBuffer bits = validityFromFlags(valid, nullCount);
        return nullCount > 0 ? result.withValidity(std::move(bits), nullCount) : result;
    };

    switch (function)
    {
    case AggregateFunction::Count:
    {
        std::vector<std::int64_t> counts(groups);
        for (std::size_t group = 0; group < groups; ++group)
            counts[group] = accumulators[group].count;
        return QueryColumn::primitive(std::move(counts));
    }
    case AggregateFunction::Sum:
    {
        if (real)
        {
            std::vector<double> sums(groups);
            for (std::size_t group = 0; group < groups; ++group)
                sums[group] = accumulators[group].sum;
            return QueryColumn::primitive(std::move(sums));
        }
        std::vector<std::int64_t> sums(groups);
        for (std::size_t group = 0; group < groups; ++group)
            sums[group] = accumulators[group].integerSum;
        return QueryColumn::primitive(std::move(sums));
    }
    case AggregateFunction::Mean:
    {
        std::vector<double> means(groups, NOT_A_NUMBER);
        for (std::size_t group = 0; group < groups; ++group)
        {
            const Accumulator& value = accumulators[group];
            if (value.count > 0)
                means[group] =
                    (real ? value.sum : static_cast<double>(value.integerSum)) / static_cast<double>(value.count);
        }
        return withValidity(QueryColumn::primitive(std::move(means)),
                            missingGroups([](const Accumulator& value) { return value.count > 0; }));
    }
    case AggregateFunction::Min:
    case AggregateFunction::Max:
    {
        const bool minimum = function == AggregateFunction::Min;
        const auto valid = missingGroups([](const Accumulator& value) { return value.count > 0; });
        if (real)
        {
            std::vector<double> values(groups, NOT_A_NUMBER);
            for (std::size_t group = 0; group < groups; ++group)
            {
                if (accumulators[group].count > 0)
                    values[group] = minimum ? accumulators[group].minimum : accumulators[group].maximum;
            }
            return withValidity(QueryColumn::primitive(std::move(values)), valid);
        }
        std::vector<std::int64_t> values(groups, 0);
        for (std::size_t group = 0; group < groups; ++group)
        {
            if (accumulators[group].count > 0)
                values[group] = minimum ? accumulators[group].integerMinimum : accumulators[group].integerMaximum;
        }
        // The result has the type of the column, so that the ID of a property that is the minimum is still an ID.
        QueryColumn result;
        switch (column->type())
        {
        case ColumnType::Int32:
        {
            std::vector<std::int32_t> narrow(values.begin(), values.end());
            result = QueryColumn::primitive(std::move(narrow));
            break;
        }
        case ColumnType::UInt32:
        {
            std::vector<std::uint32_t> narrow(values.begin(), values.end());
            result = QueryColumn::primitive(std::move(narrow));
            break;
        }
        case ColumnType::UInt8:
        {
            std::vector<std::uint8_t> narrow(values.begin(), values.end());
            result = QueryColumn::primitive(std::move(narrow));
            break;
        }
        default:
            result = QueryColumn::primitive(std::move(values));
            break;
        }
        return withValidity(std::move(result), valid);
    }
    case AggregateFunction::ArgMax:
    case AggregateFunction::ArgMin:
        break; // refused above
    }
    throw QueryError("unknown aggregate");
}

} // namespace oor::query

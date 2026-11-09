#include "oor/query/operators/SortOperator.hpp"

#include "oor/query/QueryErrors.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace oor::query
{

namespace
{

// The rows per piece that are sorted on their own before the pieces are merged.
constexpr std::size_t SORT_PIECE = 16384;

template <typename T>
int threeWay(T left, T right) noexcept
{
    return left < right ? -1 : (right < left ? 1 : 0);
}

// Orders two rows by one key: missing values last in both directions.
struct KeyComparer
{
    const QueryColumn* column;
    bool descending;

    [[nodiscard]] int compare(std::size_t left, std::size_t right) const noexcept
    {
        const bool leftMissing = missing(left);
        const bool rightMissing = missing(right);
        if (leftMissing || rightMissing)
            return leftMissing == rightMissing ? 0 : (leftMissing ? 1 : -1);
        const int order = compareRows(*column, left, right);
        return descending ? -order : order;
    }

    [[nodiscard]] bool missing(std::size_t row) const noexcept
    {
        if (column->isNull(row))
            return true;
        if (column->type() == ColumnType::Float32)
            return std::isnan(column->as<float>()[row]);
        return column->type() == ColumnType::Float64 && std::isnan(column->as<double>()[row]);
    }
};

template <typename Less>
void mergeSort(std::vector<std::uint32_t>& order, const Less& less, ExecutionContext& context)
{
    const std::size_t count = order.size();
    if (count < 2)
        return;
    const std::vector<base::IndexRange> pieces = base::splitIntoChunks(count, SORT_PIECE);
    context.parallel(count, SORT_PIECE,
                     [&](base::IndexRange range)
                     {
                         std::stable_sort(order.begin() + static_cast<std::ptrdiff_t>(range.begin),
                                          order.begin() + static_cast<std::ptrdiff_t>(range.end), less);
                     });
    // Merge neighbouring runs two by two, a round at a time: a run is `width` pieces long.
    std::vector<std::uint32_t> buffer(count);
    for (std::size_t width = 1; width < pieces.size(); width *= 2)
    {
        const std::size_t pairs = (pieces.size() + 2 * width - 1) / (2 * width);
        const auto mergePair = [&](std::size_t pair)
        {
            const std::size_t firstPiece = pair * 2 * width;
            const std::size_t middlePiece = std::min(firstPiece + width, pieces.size());
            const std::size_t lastPiece = std::min(firstPiece + 2 * width, pieces.size());
            const std::size_t begin = pieces[firstPiece].begin;
            const std::size_t middle = middlePiece == pieces.size() ? count : pieces[middlePiece].begin;
            const std::size_t end = lastPiece == pieces.size() ? count : pieces[lastPiece].begin;
            std::merge(
                order.begin() + static_cast<std::ptrdiff_t>(begin), order.begin() + static_cast<std::ptrdiff_t>(middle),
                order.begin() + static_cast<std::ptrdiff_t>(middle), order.begin() + static_cast<std::ptrdiff_t>(end),
                buffer.begin() + static_cast<std::ptrdiff_t>(begin), less);
        };
        if (context.pool() == nullptr || context.pool()->workerCount() == 0 || pairs == 1)
        {
            for (std::size_t pair = 0; pair < pairs; ++pair)
            {
                context.checkCancelled();
                mergePair(pair);
            }
        }
        else
        {
            base::parallelFor(*context.pool(), pairs, mergePair, context.token());
        }
        order.swap(buffer);
    }
}

} // namespace

int compareKeys(const QueryColumn& column, std::size_t left, std::size_t right) noexcept
{
    return KeyComparer{&column, false}.compare(left, right);
}

int compareRows(const QueryColumn& column, std::size_t left, std::size_t right) noexcept
{
    switch (column.type())
    {
    case ColumnType::Float32:
        return threeWay(column.as<float>()[left], column.as<float>()[right]);
    case ColumnType::Float64:
        return threeWay(column.as<double>()[left], column.as<double>()[right]);
    case ColumnType::Int32:
        return threeWay(column.as<std::int32_t>()[left], column.as<std::int32_t>()[right]);
    case ColumnType::UInt32:
        return threeWay(column.as<std::uint32_t>()[left], column.as<std::uint32_t>()[right]);
    case ColumnType::Int64:
        return threeWay(column.as<std::int64_t>()[left], column.as<std::int64_t>()[right]);
    case ColumnType::UInt8:
        return threeWay(column.as<std::uint8_t>()[left], column.as<std::uint8_t>()[right]);
    case ColumnType::Bool:
        return threeWay(column.boolAt(left), column.boolAt(right));
    case ColumnType::Utf8:
    case ColumnType::Dictionary:
    {
        const int order = column.textAt(left).compare(column.textAt(right));
        return order < 0 ? -1 : (order > 0 ? 1 : 0);
    }
    default:
        return 0;
    }
}

std::vector<std::uint32_t> sortedOrder(std::span<const SortColumn> keys, std::size_t rowCount,
                                       ExecutionContext& context)
{
    for (const SortColumn& key : keys)
    {
        if (key.column->type() == ColumnType::FixedList || key.column->type() == ColumnType::List)
            throw QueryTypeError("a list column cannot be used to sort");
    }
    std::vector<std::uint32_t> order(rowCount);
    std::iota(order.begin(), order.end(), 0U);
    if (keys.empty() || rowCount < 2)
        return order;
    context.beginStage("sorting", rowCount);

    // One numeric key without nulls is the common case: sort the values themselves, which are read in order.
    if (keys.size() == 1 && !keys[0].column->hasNulls() &&
        (keys[0].column->type() == ColumnType::Int32 || keys[0].column->type() == ColumnType::Float64))
    {
        const QueryColumn& column = *keys[0].column;
        const bool descending = keys[0].descending;
        if (column.type() == ColumnType::Int32)
        {
            const auto values = column.as<std::int32_t>();
            if (descending)
                mergeSort(order, [&](std::uint32_t a, std::uint32_t b) { return values[a] > values[b]; }, context);
            else
                mergeSort(order, [&](std::uint32_t a, std::uint32_t b) { return values[a] < values[b]; }, context);
        }
        else
        {
            const auto values = column.as<double>();
            // NaN last: a NaN is never less than a number, and a number is less than a NaN.
            if (descending)
                mergeSort(
                    order,
                    [&](std::uint32_t a, std::uint32_t b)
                    {
                        const bool leftNan = std::isnan(values[a]);
                        const bool rightNan = std::isnan(values[b]);
                        if (leftNan || rightNan)
                            return !leftNan && rightNan;
                        return values[a] > values[b];
                    },
                    context);
            else
                mergeSort(
                    order,
                    [&](std::uint32_t a, std::uint32_t b)
                    {
                        const bool leftNan = std::isnan(values[a]);
                        const bool rightNan = std::isnan(values[b]);
                        if (leftNan || rightNan)
                            return !leftNan && rightNan;
                        return values[a] < values[b];
                    },
                    context);
        }
        return order;
    }

    std::vector<KeyComparer> comparers;
    comparers.reserve(keys.size());
    for (const SortColumn& key : keys)
        comparers.push_back(KeyComparer{key.column, key.descending});
    mergeSort(
        order,
        [&](std::uint32_t a, std::uint32_t b)
        {
            for (const KeyComparer& comparer : comparers)
            {
                const int order = comparer.compare(a, b);
                if (order != 0)
                    return order < 0;
            }
            return false;
        },
        context);
    return order;
}

} // namespace oor::query

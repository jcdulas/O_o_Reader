#include "oor/query/operators/ColumnOps.hpp"

#include "oor/query/QueryErrors.hpp"

#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();
constexpr float NOT_A_NUMBER_SINGLE = std::numeric_limits<float>::quiet_NaN();

bool bitAt(const std::uint8_t* bits, std::size_t row) noexcept
{
    return ((bits[row >> 3] >> (row & 7)) & 1) != 0;
}

// Copies `width`-byte records: out[i] = in[rows[i]], zeros for NO_ROW (NaN for floating values, so that a view shows
// the missing value as the analysis does). `floatBytes` is the width of the floating values the record is made of (4
// for Float32, 8 for Float64), or 0 for a record that is not floating.
void fillMissing(std::uint8_t* record, std::size_t width, std::size_t floatBytes) noexcept
{
    if (floatBytes == 0 || width % floatBytes != 0)
    {
        std::memset(record, 0, width);
        return;
    }
    for (std::size_t slot = 0; slot < width; slot += floatBytes)
    {
        if (floatBytes == sizeof(double))
            std::memcpy(record + slot, &NOT_A_NUMBER, sizeof(double));
        else
            std::memcpy(record + slot, &NOT_A_NUMBER_SINGLE, sizeof(float));
    }
}

template <std::size_t Width>
void gatherRecords(const std::uint8_t* in, std::span<const std::uint32_t> rows, std::uint8_t* out, std::size_t begin,
                   std::size_t end, std::size_t floatBytes)
{
    for (std::size_t index = begin; index < end; ++index)
    {
        const std::uint32_t row = rows[index];
        if (row == NO_ROW)
            fillMissing(out + index * Width, Width, floatBytes);
        else
            std::memcpy(out + index * Width, in + static_cast<std::size_t>(row) * Width, Width);
    }
}

void gatherRecordsOfWidth(std::size_t width, const std::uint8_t* in, std::span<const std::uint32_t> rows,
                          std::uint8_t* out, std::size_t begin, std::size_t end, std::size_t floatBytes)
{
    switch (width)
    {
    case 1:
        gatherRecords<1>(in, rows, out, begin, end, 0);
        return;
    case 4:
        gatherRecords<4>(in, rows, out, begin, end, floatBytes);
        return;
    case 8:
        gatherRecords<8>(in, rows, out, begin, end, floatBytes);
        return;
    default:
        break;
    }
    for (std::size_t index = begin; index < end; ++index)
    {
        const std::uint32_t row = rows[index];
        if (row != NO_ROW)
            std::memcpy(out + index * width, in + static_cast<std::size_t>(row) * width, width);
        else
            fillMissing(out + index * width, width, floatBytes);
    }
}

// The width of the floating values of a column type: 4, 8, or 0 for a column that is not floating.
std::size_t floatBytesOf(ColumnType type) noexcept
{
    return type == ColumnType::Float32 ? sizeof(float) : (type == ColumnType::Float64 ? sizeof(double) : 0);
}

// The values of a column of fixed-width records (a primitive, the indices of a dictionary, the child of a fixed list).
ColumnBuffer takeRecords(const ColumnBuffer& values, std::size_t recordWidth, std::span<const std::uint32_t> rows,
                         std::size_t floatBytes, const ExecutionContext& context)
{
    std::vector<std::uint8_t> out(rows.size() * recordWidth);
    const auto* in = static_cast<const std::uint8_t*>(values.data());
    std::uint8_t* target = out.data();
    context.parallel(rows.size(), LIGHT_CHUNK, [&](base::IndexRange range)
                     { gatherRecordsOfWidth(recordWidth, in, rows, target, range.begin, range.end, floatBytes); });
    return ColumnBuffer::adopt(std::move(out));
}

ColumnBuffer takeBits(const QueryColumn& column, std::span<const std::uint32_t> rows, const ExecutionContext& context)
{
    std::vector<std::uint8_t> out(bitmapBytes(rows.size()), 0);
    const auto* in = static_cast<const std::uint8_t*>(column.values().data());
    std::uint8_t* target = out.data();
    // The chunk size is a multiple of 8: no two chunks write the same byte.
    context.parallel(rows.size(), LIGHT_CHUNK,
                     [&](base::IndexRange range)
                     {
                         for (std::size_t index = range.begin; index < range.end; ++index)
                         {
                             const std::uint32_t row = rows[index];
                             if (row != NO_ROW && bitAt(in, row))
                                 target[index >> 3] =
                                     static_cast<std::uint8_t>(target[index >> 3] | (1U << (index & 7)));
                         }
                     });
    return ColumnBuffer::adopt(std::move(out));
}

// The validity of a gathered column: a row that is NO_ROW or that is null in the source is null.
QueryColumn withTakenValidity(QueryColumn result, const QueryColumn& source, std::span<const std::uint32_t> rows,
                              const ExecutionContext& context)
{
    std::atomic<std::int64_t> missing{0};
    if (!source.hasNulls())
    {
        context.parallel(rows.size(), LIGHT_CHUNK,
                         [&](base::IndexRange range)
                         {
                             std::int64_t count = 0;
                             for (std::size_t index = range.begin; index < range.end; ++index)
                                 count += rows[index] == NO_ROW ? 1 : 0;
                             missing.fetch_add(count, std::memory_order_relaxed);
                         });
        if (missing.load() == 0)
            return result;
    }
    std::vector<std::uint8_t> bits(bitmapBytes(rows.size()), 0);
    std::uint8_t* target = bits.data();
    const auto* sourceBits = source.hasNulls() ? static_cast<const std::uint8_t*>(source.validity().data()) : nullptr;
    missing.store(0);
    context.parallel(rows.size(), LIGHT_CHUNK,
                     [&](base::IndexRange range)
                     {
                         std::int64_t count = 0;
                         for (std::size_t index = range.begin; index < range.end; ++index)
                         {
                             const std::uint32_t row = rows[index];
                             const bool valid = row != NO_ROW && (sourceBits == nullptr || bitAt(sourceBits, row));
                             if (valid)
                                 target[index >> 3] =
                                     static_cast<std::uint8_t>(target[index >> 3] | (1U << (index & 7)));
                             else
                                 ++count;
                         }
                         missing.fetch_add(count, std::memory_order_relaxed);
                     });
    return result.withValidity(ColumnBuffer::adopt(std::move(bits)), missing.load());
}

QueryColumn takeUnvalidated(const QueryColumn& column, std::span<const std::uint32_t> rows,
                            const ExecutionContext& context);

// The child rows of a list (or text) gather: the new offsets and, for each output row, the range of the source it
// copies.
struct RangePlan
{
    std::vector<std::int64_t> offsets; // rows.size() + 1 entries
};

RangePlan planRanges(const std::int64_t* beginOffsets, std::size_t offsetWidth, std::span<const std::uint32_t> rows,
                     const ExecutionContext& context)
{
    // The length of each output row in offsets[i + 1], then the running total.
    RangePlan plan;
    plan.offsets.assign(rows.size() + 1, 0);
    std::int64_t* lengths = plan.offsets.data();
    (void)offsetWidth;
    context.parallel(rows.size(), LIGHT_CHUNK,
                     [&](base::IndexRange range)
                     {
                         for (std::size_t index = range.begin; index < range.end; ++index)
                         {
                             const std::uint32_t row = rows[index];
                             lengths[index + 1] = row == NO_ROW ? 0 : beginOffsets[row + 1] - beginOffsets[row];
                         }
                     });
    for (std::size_t index = 0; index < rows.size(); ++index)
        lengths[index + 1] += lengths[index];
    return plan;
}

QueryColumn takeList(const QueryColumn& column, std::span<const std::uint32_t> rows, const ExecutionContext& context)
{
    const auto offsets = column.offsets().as<std::int64_t>();
    const RangePlan plan = planRanges(offsets.data(), sizeof(std::int64_t), rows, context);
    const std::int64_t total = plan.offsets.back();
    if (total > static_cast<std::int64_t>(NO_ROW))
        throw QueryError("a list column would hold more than 4 billion values");
    std::vector<std::uint32_t> childRows(static_cast<std::size_t>(total));
    std::uint32_t* target = childRows.data();
    const std::int64_t* newOffsets = plan.offsets.data();
    context.parallel(rows.size(), LIGHT_CHUNK,
                     [&](base::IndexRange range)
                     {
                         for (std::size_t index = range.begin; index < range.end; ++index)
                         {
                             const std::uint32_t row = rows[index];
                             if (row == NO_ROW)
                                 continue;
                             std::uint32_t* out = target + newOffsets[index];
                             const std::int64_t first = offsets[row];
                             const std::int64_t count = newOffsets[index + 1] - newOffsets[index];
                             for (std::int64_t k = 0; k < count; ++k)
                                 out[k] = static_cast<std::uint32_t>(first + k);
                         }
                     });
    QueryColumn child = take(*column.child(), childRows, context);
    return QueryColumn::list(ColumnBuffer::adopt(std::vector<std::int64_t>(plan.offsets)), std::move(child),
                             static_cast<std::int64_t>(rows.size()));
}

QueryColumn takeUtf8(const QueryColumn& column, std::span<const std::uint32_t> rows, const ExecutionContext& context)
{
    const auto offsets = column.offsets().as<std::int32_t>();
    std::vector<std::int64_t> starts(rows.size() + 1, 0);
    std::int64_t* lengths = starts.data();
    context.parallel(rows.size(), LIGHT_CHUNK,
                     [&](base::IndexRange range)
                     {
                         for (std::size_t index = range.begin; index < range.end; ++index)
                         {
                             const std::uint32_t row = rows[index];
                             lengths[index + 1] = row == NO_ROW ? 0 : offsets[row + 1] - offsets[row];
                         }
                     });
    for (std::size_t index = 0; index < rows.size(); ++index)
        lengths[index + 1] += lengths[index];
    if (starts.back() > static_cast<std::int64_t>(INT32_MAX))
        throw QueryError("a text column would hold more than 2 GB of characters");
    std::vector<char> characters(static_cast<std::size_t>(starts.back()));
    char* target = characters.data();
    const auto* in = static_cast<const char*>(column.values().data());
    context.parallel(rows.size(), LIGHT_CHUNK,
                     [&](base::IndexRange range)
                     {
                         for (std::size_t index = range.begin; index < range.end; ++index)
                         {
                             const std::uint32_t row = rows[index];
                             const auto length = static_cast<std::size_t>(starts[index + 1] - starts[index]);
                             // an empty text copies nothing: memcpy of a null buffer is undefined even for 0 bytes
                             // (UndefinedBehaviorSanitizer, a column of empty texts)
                             if (row == NO_ROW || length == 0)
                                 continue;
                             std::memcpy(target + starts[index], in + offsets[row], length);
                         }
                     });
    std::vector<std::int32_t> newOffsets(starts.begin(), starts.end());
    return QueryColumn::utf8FromBuffers(static_cast<std::int64_t>(rows.size()),
                                        ColumnBuffer::adopt(std::move(newOffsets)),
                                        ColumnBuffer::adopt(std::move(characters)));
}

QueryColumn takeUnvalidated(const QueryColumn& column, std::span<const std::uint32_t> rows,
                            const ExecutionContext& context)
{
    const auto length = static_cast<std::int64_t>(rows.size());
    switch (column.type())
    {
    case ColumnType::Bool:
        return QueryColumn::booleanFromBits(length, takeBits(column, rows, context));
    case ColumnType::UInt8:
    case ColumnType::Int32:
    case ColumnType::UInt32:
    case ColumnType::Int64:
    case ColumnType::Float32:
    case ColumnType::Float64:
        return QueryColumn::fromBuffer(
            column.type(), length,
            takeRecords(column.values(), valueWidth(column.type()), rows, floatBytesOf(column.type()), context));
    case ColumnType::Dictionary:
        return QueryColumn::dictionaryOf(takeRecords(column.values(), 4, rows, 0, context), length, *column.child());
    case ColumnType::Utf8:
        return takeUtf8(column, rows, context);
    case ColumnType::FixedList:
    {
        const QueryColumn& child = *column.child();
        const std::size_t record = static_cast<std::size_t>(column.width()) * valueWidth(child.type());
        QueryColumn values =
            QueryColumn::fromBuffer(child.type(), length * column.width(),
                                    takeRecords(child.values(), record, rows, floatBytesOf(child.type()), context));
        return QueryColumn::fixedList(std::move(values), column.width(), length);
    }
    case ColumnType::List:
        return takeList(column, rows, context);
    }
    throw QueryError("take: unknown column type");
}

} // namespace

QueryColumn take(const QueryColumn& column, std::span<const std::uint32_t> rows, const ExecutionContext& context)
{
    // A column of lists whose child is a list of fixed width needs no special care: the child is gathered by `take`.
    QueryColumn result = takeUnvalidated(column, rows, context);
    return withTakenValidity(std::move(result), column, rows, context);
}

bool sameLayout(const QueryColumn& left, const QueryColumn& right)
{
    if (left.type() != right.type() || left.width() != right.width())
        return false;
    if (left.child() != nullptr || right.child() != nullptr)
    {
        if (left.child() == nullptr || right.child() == nullptr)
            return false;
        return sameLayout(*left.child(), *right.child());
    }
    return true;
}

QueryColumn nullColumnLike(const QueryColumn& like, std::int64_t length)
{
    const auto count = static_cast<std::size_t>(length);
    QueryColumn column;
    switch (like.type())
    {
    case ColumnType::Bool:
        column =
            QueryColumn::booleanFromBits(length, ColumnBuffer::adopt(std::vector<std::uint8_t>(bitmapBytes(count), 0)));
        break;
    case ColumnType::UInt8:
    case ColumnType::Int32:
    case ColumnType::UInt32:
    case ColumnType::Int64:
        column = QueryColumn::fromBuffer(
            like.type(), length, ColumnBuffer::adopt(std::vector<std::uint8_t>(count * valueWidth(like.type()), 0)));
        break;
    case ColumnType::Float32:
        column = QueryColumn::primitive(std::vector<float>(count, NOT_A_NUMBER_SINGLE));
        break;
    case ColumnType::Float64:
        column = QueryColumn::primitive(std::vector<double>(count, NOT_A_NUMBER));
        break;
    case ColumnType::Dictionary:
        column =
            QueryColumn::dictionaryOf(ColumnBuffer::adopt(std::vector<std::int32_t>(count, 0)), length, *like.child());
        break;
    case ColumnType::Utf8:
        column = QueryColumn::utf8FromBuffers(length, ColumnBuffer::adopt(std::vector<std::int32_t>(count + 1, 0)),
                                              ColumnBuffer::adopt(std::vector<char>()));
        break;
    case ColumnType::FixedList:
        column = QueryColumn::fixedList(nullColumnLike(*like.child(), length * like.width()), like.width(), length);
        break;
    case ColumnType::List:
        column = QueryColumn::list(ColumnBuffer::adopt(std::vector<std::int64_t>(count + 1, 0)),
                                   nullColumnLike(*like.child(), 0), length);
        break;
    }
    if (length == 0)
        return column;
    return column.withValidity(ColumnBuffer::adopt(std::vector<std::uint8_t>(bitmapBytes(count), 0)), length);
}

QueryColumn fixedListAsList(const QueryColumn& column)
{
    if (column.type() != ColumnType::FixedList)
        return column;
    const auto count = static_cast<std::size_t>(column.length());
    std::vector<std::int64_t> offsets(count + 1);
    for (std::size_t row = 0; row <= count; ++row)
        offsets[row] = static_cast<std::int64_t>(row) * column.width();
    QueryColumn result = QueryColumn::list(ColumnBuffer::adopt(std::move(offsets)), *column.child(), column.length());
    if (column.hasNulls())
        result = result.withValidity(column.validity(), column.nullCount());
    return result;
}

ColumnBuffer validityFromFlags(std::span<const std::uint8_t> valid, std::int64_t& nullCount)
{
    std::vector<std::uint8_t> bits(bitmapBytes(valid.size()), 0);
    nullCount = 0;
    for (std::size_t row = 0; row < valid.size(); ++row)
    {
        if (valid[row] != 0)
            bits[row >> 3] = static_cast<std::uint8_t>(bits[row >> 3] | (1U << (row & 7)));
        else
            ++nullCount;
    }
    return ColumnBuffer::adopt(std::move(bits));
}

QueryColumn concatenate(std::span<const QueryColumn* const> parts, std::span<const std::int64_t> lengths)
{
    const QueryColumn* like = nullptr;
    std::int64_t total = 0;
    bool anyNull = false;
    for (std::size_t index = 0; index < parts.size(); ++index)
    {
        total += lengths[index];
        if (parts[index] == nullptr)
            anyNull = anyNull || lengths[index] > 0;
        else
        {
            if (like == nullptr)
                like = parts[index];
            else if (!sameLayout(*like, *parts[index]))
                throw QueryError("cannot put columns of different types end to end");
            anyNull = anyNull || parts[index]->hasNulls();
        }
    }
    if (like == nullptr)
        throw QueryError("concatenate needs at least one column");
    if (like->type() == ColumnType::Dictionary)
        throw QueryError("cannot put dictionary columns end to end");
    const auto count = static_cast<std::size_t>(total);

    // The validity first: one bit per row, set where the value is there.
    ColumnBuffer validity;
    std::int64_t nullCount = 0;
    if (anyNull)
    {
        std::vector<std::uint8_t> flags;
        flags.reserve(count);
        for (std::size_t index = 0; index < parts.size(); ++index)
        {
            const auto partLength = static_cast<std::size_t>(lengths[index]);
            for (std::size_t row = 0; row < partLength; ++row)
                flags.push_back(parts[index] != nullptr && !parts[index]->isNull(row) ? 1 : 0);
        }
        validity = validityFromFlags(flags, nullCount);
    }

    QueryColumn result;
    switch (like->type())
    {
    case ColumnType::Bool:
    {
        std::vector<std::uint8_t> flags;
        flags.reserve(count);
        for (std::size_t index = 0; index < parts.size(); ++index)
        {
            const auto partLength = static_cast<std::size_t>(lengths[index]);
            for (std::size_t row = 0; row < partLength; ++row)
                flags.push_back(parts[index] != nullptr && parts[index]->boolAt(row) ? 1 : 0);
        }
        result = QueryColumn::boolean(flags);
        break;
    }
    case ColumnType::UInt8:
    case ColumnType::Int32:
    case ColumnType::UInt32:
    case ColumnType::Int64:
    case ColumnType::Float32:
    case ColumnType::Float64:
    {
        const std::size_t width = valueWidth(like->type());
        std::vector<std::uint8_t> bytes(count * width, 0);
        std::size_t position = 0;
        for (std::size_t index = 0; index < parts.size(); ++index)
        {
            const auto partBytes = static_cast<std::size_t>(lengths[index]) * width;
            if (parts[index] != nullptr)
                std::memcpy(bytes.data() + position, parts[index]->values().data(), partBytes);
            else if (isFloating(like->type()))
            {
                for (std::size_t row = 0; row < static_cast<std::size_t>(lengths[index]); ++row)
                    fillMissing(bytes.data() + position + row * width, width, floatBytesOf(like->type()));
            }
            position += partBytes;
        }
        result = QueryColumn::fromBuffer(like->type(), total, ColumnBuffer::adopt(std::move(bytes)));
        break;
    }
    case ColumnType::Utf8:
    {
        std::vector<std::int32_t> offsets;
        offsets.reserve(count + 1);
        offsets.push_back(0);
        std::vector<char> characters;
        for (std::size_t index = 0; index < parts.size(); ++index)
        {
            const auto partLength = static_cast<std::size_t>(lengths[index]);
            if (parts[index] == nullptr)
            {
                offsets.insert(offsets.end(), partLength, static_cast<std::int32_t>(characters.size()));
                continue;
            }
            const auto partOffsets = parts[index]->offsets().as<std::int32_t>();
            const auto* in = static_cast<const char*>(parts[index]->values().data());
            const auto base = static_cast<std::int32_t>(characters.size());
            characters.insert(characters.end(), in, in + partOffsets[partLength]);
            for (std::size_t row = 1; row <= partLength; ++row)
                offsets.push_back(base + partOffsets[row]);
        }
        result = QueryColumn::utf8FromBuffers(total, ColumnBuffer::adopt(std::move(offsets)),
                                              ColumnBuffer::adopt(std::move(characters)));
        break;
    }
    case ColumnType::FixedList:
    {
        std::vector<const QueryColumn*> childParts;
        std::vector<std::int64_t> childLengths;
        for (std::size_t index = 0; index < parts.size(); ++index)
        {
            childParts.push_back(parts[index] != nullptr ? parts[index]->child().get() : nullptr);
            childLengths.push_back(lengths[index] * like->width());
        }
        result = QueryColumn::fixedList(concatenate(childParts, childLengths), like->width(), total);
        break;
    }
    case ColumnType::List:
    {
        std::vector<std::int64_t> offsets;
        offsets.reserve(count + 1);
        offsets.push_back(0);
        std::vector<const QueryColumn*> childParts;
        std::vector<std::int64_t> childLengths;
        std::int64_t base = 0;
        for (std::size_t index = 0; index < parts.size(); ++index)
        {
            const auto partLength = static_cast<std::size_t>(lengths[index]);
            if (parts[index] == nullptr)
            {
                offsets.insert(offsets.end(), partLength, base);
                childParts.push_back(nullptr);
                childLengths.push_back(0);
                continue;
            }
            const auto partOffsets = parts[index]->offsets().as<std::int64_t>();
            for (std::size_t row = 1; row <= partLength; ++row)
                offsets.push_back(base + partOffsets[row] - partOffsets[0]);
            childParts.push_back(parts[index]->child().get());
            childLengths.push_back(partOffsets[partLength] - partOffsets[0]);
            base += partOffsets[partLength] - partOffsets[0];
        }
        result =
            QueryColumn::list(ColumnBuffer::adopt(std::move(offsets)), concatenate(childParts, childLengths), total);
        break;
    }
    case ColumnType::Dictionary:
        break;
    }
    if (anyNull)
        return result.withValidity(std::move(validity), nullCount);
    return result;
}

void copyAsDouble(const QueryColumn& column, std::size_t begin, std::size_t end, double* out)
{
    switch (column.type())
    {
    case ColumnType::Float64:
    {
        const auto values = column.as<double>();
        for (std::size_t row = begin; row < end; ++row)
            out[row - begin] = values[row];
        break;
    }
    case ColumnType::Float32:
    {
        const auto values = column.as<float>();
        for (std::size_t row = begin; row < end; ++row)
            out[row - begin] = static_cast<double>(values[row]);
        break;
    }
    case ColumnType::Int32:
    {
        const auto values = column.as<std::int32_t>();
        for (std::size_t row = begin; row < end; ++row)
            out[row - begin] = static_cast<double>(values[row]);
        break;
    }
    case ColumnType::UInt32:
    {
        const auto values = column.as<std::uint32_t>();
        for (std::size_t row = begin; row < end; ++row)
            out[row - begin] = static_cast<double>(values[row]);
        break;
    }
    case ColumnType::Int64:
    {
        const auto values = column.as<std::int64_t>();
        for (std::size_t row = begin; row < end; ++row)
            out[row - begin] = static_cast<double>(values[row]);
        break;
    }
    case ColumnType::UInt8:
    {
        const auto values = column.as<std::uint8_t>();
        for (std::size_t row = begin; row < end; ++row)
            out[row - begin] = static_cast<double>(values[row]);
        break;
    }
    case ColumnType::Bool:
        for (std::size_t row = begin; row < end; ++row)
            out[row - begin] = column.boolAt(row) ? 1.0 : 0.0;
        break;
    default:
        throw QueryTypeError("a " + std::string(columnTypeName(column.type())) + " column is not a number");
    }
    if (column.hasNulls())
    {
        for (std::size_t row = begin; row < end; ++row)
        {
            if (column.isNull(row))
                out[row - begin] = NOT_A_NUMBER;
        }
    }
}

} // namespace oor::query

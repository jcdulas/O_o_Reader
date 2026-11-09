#include "oor/query/table/PlainColumn.hpp"

#include <limits>
#include <utility>

namespace oor::query
{

namespace
{

// The number of a row of a column of numbers (NaN for a null).
double numberAt(const QueryColumn& column, std::size_t row)
{
    if (column.isNull(row))
        return std::numeric_limits<double>::quiet_NaN();
    switch (column.type())
    {
    case ColumnType::Bool:
        return column.boolAt(row) ? 1.0 : 0.0;
    case ColumnType::UInt8:
        return column.as<std::uint8_t>()[row];
    case ColumnType::Int32:
        return column.as<std::int32_t>()[row];
    case ColumnType::UInt32:
        return column.as<std::uint32_t>()[row];
    case ColumnType::Int64:
        return static_cast<double>(column.as<std::int64_t>()[row]);
    case ColumnType::Float32:
        return column.as<float>()[row];
    case ColumnType::Float64:
        return column.as<double>()[row];
    default:
        return std::numeric_limits<double>::quiet_NaN();
    }
}

bool isWhole(ColumnType type)
{
    return type == ColumnType::Bool || isInteger(type);
}

} // namespace

PlainColumn plainColumn(const QueryTable& table, std::size_t index)
{
    return plainColumn(table.column(index), table.name(index));
}

PlainColumn plainColumn(const QueryColumn& column, std::string name)
{
    PlainColumn plain;
    plain.name = std::move(name);
    const std::size_t rows = column.size();
    if (column.hasNulls())
    {
        plain.valid.resize(rows);
        for (std::size_t row = 0; row < rows; ++row)
            plain.valid[row] = column.isNull(row) ? 0 : 1;
    }

    if (column.type() == ColumnType::List || column.type() == ColumnType::FixedList)
    {
        const QueryColumn& child = *column.child();
        const bool text = isText(child.type());
        plain.kind = text ? PlainKind::TextList : PlainKind::NumberList;
        plain.integer = isWhole(child.type());
        plain.offsets.push_back(0);
        for (std::size_t row = 0; row < rows; ++row)
        {
            const std::int64_t begin = column.listBegin(row);
            const std::int64_t length = column.isNull(row) ? 0 : column.listLength(row);
            for (std::int64_t item = 0; item < length; ++item)
            {
                const auto at = static_cast<std::size_t>(begin + item);
                if (text)
                    plain.texts.emplace_back(child.textAt(at));
                else
                    plain.numbers.push_back(numberAt(child, at));
            }
            plain.offsets.push_back(plain.offsets.back() + length);
        }
        return plain;
    }
    if (isText(column.type()))
    {
        plain.kind = PlainKind::Text;
        plain.texts.reserve(rows);
        for (std::size_t row = 0; row < rows; ++row)
            plain.texts.emplace_back(column.isNull(row) ? std::string_view() : column.textAt(row));
        return plain;
    }
    plain.kind = PlainKind::Number;
    plain.integer = isWhole(column.type());
    plain.numbers.reserve(rows);
    for (std::size_t row = 0; row < rows; ++row)
        plain.numbers.push_back(numberAt(column, row));
    return plain;
}

} // namespace oor::query

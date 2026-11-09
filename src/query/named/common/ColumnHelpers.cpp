#include "oor/query/named/common/ColumnHelpers.hpp"

#include "oor/query/operators/ColumnOps.hpp"

namespace oor::query
{

std::vector<double> asDoubles(const QueryColumn& column)
{
    std::vector<double> values(column.size());
    copyAsDouble(column, 0, column.size(), values.data());
    return values;
}

std::vector<std::int32_t> asInt32(const QueryColumn& column)
{
    std::vector<std::int32_t> values(column.size(), 0);
    for (std::size_t row = 0; row < values.size(); ++row)
    {
        if (column.isNull(row))
            continue;
        switch (column.type())
        {
        case ColumnType::Int32:
            values[row] = column.as<std::int32_t>()[row];
            break;
        case ColumnType::UInt32:
            values[row] = static_cast<std::int32_t>(column.as<std::uint32_t>()[row]);
            break;
        case ColumnType::Int64:
            values[row] = static_cast<std::int32_t>(column.as<std::int64_t>()[row]);
            break;
        default:
            break;
        }
    }
    return values;
}

std::vector<double> component(const QueryColumn& fixedList, std::size_t index)
{
    std::vector<double> values(fixedList.size(), 0.0);
    const auto width = static_cast<std::size_t>(fixedList.width());
    const auto data = fixedList.child()->as<double>();
    for (std::size_t row = 0; row < values.size(); ++row)
        values[row] = data[row * width + index];
    return values;
}

std::vector<std::string> asTexts(const QueryColumn& column)
{
    std::vector<std::string> texts(column.size());
    for (std::size_t row = 0; row < texts.size(); ++row)
    {
        if (!column.isNull(row))
            texts[row] = std::string(column.textAt(row));
    }
    return texts;
}

std::vector<std::uint8_t> validityOf(const QueryColumn& column)
{
    std::vector<std::uint8_t> valid(column.size(), 1);
    for (std::size_t row = 0; row < valid.size(); ++row)
        valid[row] = column.isNull(row) ? 0 : 1;
    return valid;
}

} // namespace oor::query

#include "oor/query/table/CsvExport.hpp"

#include <charconv>
#include <ostream>
#include <string>

namespace oor::query
{

namespace
{

void appendText(std::string& line, std::string_view text)
{
    const bool quote = text.find_first_of(",\"\n\r") != std::string_view::npos ||
                       (!text.empty() && (text.front() == ' ' || text.back() == ' '));
    if (!quote)
    {
        line += text;
        return;
    }
    line += '"';
    for (const char character : text)
    {
        if (character == '"')
            line += '"';
        line += character;
    }
    line += '"';
}

template <typename T>
void appendNumber(std::string& line, T value)
{
    char buffer[40];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    line.append(buffer, result.ptr);
}

void appendDouble(std::string& line, double value)
{
    if (value != value)
    {
        line += "nan";
        return;
    }
    appendNumber(line, value);
}

// A single precision value: the shortest text that reads back as the same float (0.1, not 0.10000000149011612).
void appendSingle(std::string& line, float value)
{
    if (value != value)
    {
        line += "nan";
        return;
    }
    appendNumber(line, value);
}

// One value of a column that is not a list.
void appendScalar(std::string& line, const QueryColumn& column, std::size_t row)
{
    switch (column.type())
    {
    case ColumnType::Bool:
        line += column.boolAt(row) ? "true" : "false";
        break;
    case ColumnType::UInt8:
        appendNumber(line, static_cast<int>(column.as<std::uint8_t>()[row]));
        break;
    case ColumnType::Int32:
        appendNumber(line, column.as<std::int32_t>()[row]);
        break;
    case ColumnType::UInt32:
        appendNumber(line, column.as<std::uint32_t>()[row]);
        break;
    case ColumnType::Int64:
        appendNumber(line, column.as<std::int64_t>()[row]);
        break;
    case ColumnType::Float32:
        appendSingle(line, column.as<float>()[row]);
        break;
    case ColumnType::Float64:
        appendDouble(line, column.as<double>()[row]);
        break;
    case ColumnType::Utf8:
    case ColumnType::Dictionary:
        appendText(line, column.textAt(row));
        break;
    default:
        break;
    }
}

void appendValue(std::string& line, const QueryColumn& column, std::size_t row)
{
    if (column.isNull(row))
        return;
    if (column.type() != ColumnType::FixedList && column.type() != ColumnType::List)
    {
        appendScalar(line, column, row);
        return;
    }
    std::string list = "[";
    const std::int64_t first = column.listBegin(row);
    const std::int64_t count = column.listLength(row);
    for (std::int64_t index = 0; index < count; ++index)
    {
        if (index > 0)
            list += ", ";
        appendScalar(list, *column.child(), static_cast<std::size_t>(first + index));
    }
    list += "]";
    appendText(line, list);
}

} // namespace

void writeCsv(const QueryTable& table, std::ostream& out)
{
    std::string line;
    for (std::size_t column = 0; column < table.columnCount(); ++column)
    {
        if (column > 0)
            line += ',';
        appendText(line, table.name(column));
    }
    line += '\n';
    out << line;
    const auto rows = static_cast<std::size_t>(table.rowCount());
    for (std::size_t row = 0; row < rows; ++row)
    {
        line.clear();
        for (std::size_t column = 0; column < table.columnCount(); ++column)
        {
            if (column > 0)
                line += ',';
            appendValue(line, table.column(column), row);
        }
        line += '\n';
        out << line;
    }
}

} // namespace oor::query

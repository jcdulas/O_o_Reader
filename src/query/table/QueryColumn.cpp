#include "oor/query/table/QueryColumn.hpp"

#include "oor/query/QueryErrors.hpp"

#include <cstring>

namespace oor::query
{

std::string_view columnTypeName(ColumnType type) noexcept
{
    switch (type)
    {
    case ColumnType::Bool:
        return "bool";
    case ColumnType::UInt8:
        return "uint8";
    case ColumnType::Int32:
        return "int32";
    case ColumnType::UInt32:
        return "uint32";
    case ColumnType::Int64:
        return "int64";
    case ColumnType::Float32:
        return "float32";
    case ColumnType::Float64:
        return "float64";
    case ColumnType::Utf8:
        return "utf8";
    case ColumnType::Dictionary:
        return "dictionary";
    case ColumnType::FixedList:
        return "fixedList";
    case ColumnType::List:
        return "list";
    }
    return "?";
}

std::size_t valueWidth(ColumnType type) noexcept
{
    switch (type)
    {
    case ColumnType::UInt8:
        return 1;
    case ColumnType::Int32:
    case ColumnType::UInt32:
    case ColumnType::Float32:
    case ColumnType::Dictionary:
        return 4;
    case ColumnType::Int64:
    case ColumnType::Float64:
        return 8;
    default:
        return 0;
    }
}

bool isNumeric(ColumnType type) noexcept
{
    return type == ColumnType::UInt8 || type == ColumnType::Int32 || type == ColumnType::UInt32 ||
           type == ColumnType::Int64 || type == ColumnType::Float32 || type == ColumnType::Float64;
}

bool isFloating(ColumnType type) noexcept
{
    return type == ColumnType::Float32 || type == ColumnType::Float64;
}

bool isInteger(ColumnType type) noexcept
{
    return type == ColumnType::UInt8 || type == ColumnType::Int32 || type == ColumnType::UInt32 ||
           type == ColumnType::Int64;
}

bool isText(ColumnType type) noexcept
{
    return type == ColumnType::Utf8 || type == ColumnType::Dictionary;
}

std::string_view QueryColumn::textAt(std::size_t row) const noexcept
{
    if (m_type == ColumnType::Dictionary)
    {
        if (isNull(row))
            return {};
        const std::int32_t index = m_values.as<std::int32_t>()[row];
        return m_child->textAt(static_cast<std::size_t>(index));
    }
    const auto offsets = m_offsets.as<std::int32_t>();
    const auto* characters = static_cast<const char*>(m_values.data());
    return std::string_view(characters + offsets[row], static_cast<std::size_t>(offsets[row + 1] - offsets[row]));
}

std::int64_t QueryColumn::listLength(std::size_t row) const noexcept
{
    if (m_type == ColumnType::FixedList)
        return m_width;
    const auto offsets = m_offsets.as<std::int64_t>();
    return offsets[row + 1] - offsets[row];
}

std::int64_t QueryColumn::listBegin(std::size_t row) const noexcept
{
    if (m_type == ColumnType::FixedList)
        return static_cast<std::int64_t>(row) * m_width;
    return m_offsets.as<std::int64_t>()[row];
}

QueryColumn QueryColumn::fromBuffer(ColumnType type, std::int64_t length, ColumnBuffer values)
{
    QueryColumn column;
    column.m_type = type;
    column.m_length = length;
    column.m_values = std::move(values);
    return column;
}

QueryColumn QueryColumn::boolean(std::span<const std::uint8_t> flags)
{
    std::vector<std::uint8_t> bits(bitmapBytes(flags.size()), 0);
    for (std::size_t row = 0; row < flags.size(); ++row)
    {
        if (flags[row] != 0)
            bits[row >> 3] = static_cast<std::uint8_t>(bits[row >> 3] | (1U << (row & 7)));
    }
    return booleanFromBits(static_cast<std::int64_t>(flags.size()), ColumnBuffer::adopt(std::move(bits)));
}

QueryColumn QueryColumn::booleanFromBits(std::int64_t length, ColumnBuffer bits)
{
    return fromBuffer(ColumnType::Bool, length, std::move(bits));
}

QueryColumn QueryColumn::utf8(std::span<const std::string> texts)
{
    std::vector<std::int32_t> offsets;
    offsets.reserve(texts.size() + 1);
    std::vector<char> characters;
    offsets.push_back(0);
    for (const std::string& text : texts)
    {
        characters.insert(characters.end(), text.begin(), text.end());
        if (characters.size() > static_cast<std::size_t>(INT32_MAX))
            throw QueryError("a text column holds more than 2 GB of characters");
        offsets.push_back(static_cast<std::int32_t>(characters.size()));
    }
    return utf8FromBuffers(static_cast<std::int64_t>(texts.size()), ColumnBuffer::adopt(std::move(offsets)),
                           ColumnBuffer::adopt(std::move(characters)));
}

QueryColumn QueryColumn::utf8(std::span<const std::string_view> texts)
{
    std::vector<std::int32_t> offsets;
    offsets.reserve(texts.size() + 1);
    std::vector<char> characters;
    offsets.push_back(0);
    for (const std::string_view text : texts)
    {
        characters.insert(characters.end(), text.begin(), text.end());
        if (characters.size() > static_cast<std::size_t>(INT32_MAX))
            throw QueryError("a text column holds more than 2 GB of characters");
        offsets.push_back(static_cast<std::int32_t>(characters.size()));
    }
    return utf8FromBuffers(static_cast<std::int64_t>(texts.size()), ColumnBuffer::adopt(std::move(offsets)),
                           ColumnBuffer::adopt(std::move(characters)));
}

QueryColumn QueryColumn::utf8FromBuffers(std::int64_t length, ColumnBuffer offsets, ColumnBuffer characters)
{
    QueryColumn column;
    column.m_type = ColumnType::Utf8;
    column.m_length = length;
    column.m_offsets = std::move(offsets);
    column.m_values = std::move(characters);
    return column;
}

QueryColumn QueryColumn::dictionary(std::vector<std::int32_t>&& indices, std::span<const std::string> dictionary)
{
    const auto length = static_cast<std::int64_t>(indices.size());
    return dictionaryOf(ColumnBuffer::adopt(std::move(indices)), length, utf8(dictionary));
}

QueryColumn QueryColumn::dictionaryOf(ColumnBuffer indices, std::int64_t length, QueryColumn dictionary)
{
    QueryColumn column;
    column.m_type = ColumnType::Dictionary;
    column.m_length = length;
    column.m_values = std::move(indices);
    column.m_child = std::make_shared<const QueryColumn>(std::move(dictionary));
    return column;
}

QueryColumn QueryColumn::fixedList(QueryColumn child, std::int32_t width, std::int64_t length)
{
    QueryColumn column;
    column.m_type = ColumnType::FixedList;
    column.m_length = length;
    column.m_width = width;
    column.m_child = std::make_shared<const QueryColumn>(std::move(child));
    return column;
}

QueryColumn QueryColumn::list(ColumnBuffer offsets, QueryColumn child, std::int64_t length)
{
    QueryColumn column;
    column.m_type = ColumnType::List;
    column.m_length = length;
    column.m_offsets = std::move(offsets);
    column.m_child = std::make_shared<const QueryColumn>(std::move(child));
    return column;
}

QueryColumn QueryColumn::withValidity(ColumnBuffer bits, std::int64_t nullCount) const
{
    QueryColumn column = *this;
    column.m_validity = nullCount > 0 ? std::move(bits) : ColumnBuffer();
    column.m_nullCount = nullCount;
    return column;
}

QueryColumn QueryColumn::withoutValidity() const
{
    QueryColumn column = *this;
    column.m_validity = ColumnBuffer();
    column.m_nullCount = 0;
    return column;
}

} // namespace oor::query

#include "oor/query/apply/CellValue.hpp"

#include <charconv>
#include <cmath>

namespace oor::query
{

namespace
{

std::string numberText(double value)
{
    if (value != value)
        return "nan";
    char buffer[40];
    const auto result = std::to_chars(buffer, buffer + sizeof(buffer), value);
    return std::string(buffer, result.ptr);
}

bool sameReal(double left, double right)
{
    return left == right || (left != left && right != right);
}

} // namespace

CellValue CellValue::ofBool(bool value)
{
    CellValue cell;
    cell.m_kind = Kind::Bool;
    cell.m_data = value;
    return cell;
}

CellValue CellValue::ofInteger(std::int64_t value)
{
    CellValue cell;
    cell.m_kind = Kind::Integer;
    cell.m_data = value;
    return cell;
}

CellValue CellValue::ofReal(double value)
{
    CellValue cell;
    cell.m_kind = Kind::Real;
    cell.m_data = value;
    return cell;
}

CellValue CellValue::ofText(std::string value)
{
    CellValue cell;
    cell.m_kind = Kind::Text;
    cell.m_data = std::move(value);
    return cell;
}

CellValue CellValue::ofIntegers(std::vector<std::int64_t> values)
{
    CellValue cell;
    cell.m_kind = Kind::IntegerList;
    cell.m_data = std::move(values);
    return cell;
}

CellValue CellValue::ofReals(std::vector<double> values)
{
    CellValue cell;
    cell.m_kind = Kind::RealList;
    cell.m_data = std::move(values);
    return cell;
}

std::int64_t CellValue::asInteger() const
{
    if (m_kind == Kind::Bool)
        return std::get<bool>(m_data) ? 1 : 0;
    return std::get<std::int64_t>(m_data);
}

double CellValue::asReal() const
{
    switch (m_kind)
    {
    case Kind::Bool:
        return std::get<bool>(m_data) ? 1.0 : 0.0;
    case Kind::Integer:
        return static_cast<double>(std::get<std::int64_t>(m_data));
    default:
        return std::get<double>(m_data);
    }
}

std::size_t CellValue::listSize() const
{
    return m_kind == Kind::IntegerList ? asIntegers().size() : asReals().size();
}

double CellValue::listReal(std::size_t index) const
{
    return m_kind == Kind::IntegerList ? static_cast<double>(asIntegers()[index]) : asReals()[index];
}

bool CellValue::sameAs(const CellValue& other) const
{
    const bool leftNull = isNull() || (m_kind == Kind::Real && std::isnan(asReal()));
    const bool rightNull = other.isNull() || (other.m_kind == Kind::Real && std::isnan(other.asReal()));
    if (leftNull || rightNull)
        return leftNull && rightNull;
    if (isNumber() && other.isNumber())
    {
        if (m_kind != Kind::Real && other.m_kind != Kind::Real)
            return asInteger() == other.asInteger();
        return sameReal(asReal(), other.asReal());
    }
    if (m_kind == Kind::Text || other.m_kind == Kind::Text)
        return m_kind == other.m_kind && asText() == other.asText();
    if (isList() && other.isList())
    {
        if (listSize() != other.listSize())
            return false;
        if (m_kind == Kind::IntegerList && other.m_kind == Kind::IntegerList)
            return asIntegers() == other.asIntegers();
        for (std::size_t index = 0; index < listSize(); ++index)
        {
            if (!sameReal(listReal(index), other.listReal(index)))
                return false;
        }
        return true;
    }
    return false;
}

std::string CellValue::toText() const
{
    switch (m_kind)
    {
    case Kind::Null:
        return "null";
    case Kind::Bool:
        return asBool() ? "true" : "false";
    case Kind::Integer:
        return std::to_string(asInteger());
    case Kind::Real:
        return numberText(asReal());
    case Kind::Text:
        return "'" + asText() + "'";
    case Kind::IntegerList:
    case Kind::RealList:
    {
        std::string text = "[";
        for (std::size_t index = 0; index < listSize(); ++index)
        {
            if (index > 0)
                text += ", ";
            text += m_kind == Kind::IntegerList ? std::to_string(asIntegers()[index]) : numberText(asReals()[index]);
        }
        return text + "]";
    }
    }
    return "?";
}

CellValue cellAt(const QueryColumn& column, std::size_t row)
{
    if (column.isNull(row))
        return {};
    switch (column.type())
    {
    case ColumnType::Bool:
        return CellValue::ofBool(column.boolAt(row));
    case ColumnType::UInt8:
        return CellValue::ofInteger(column.as<std::uint8_t>()[row]);
    case ColumnType::Int32:
        return CellValue::ofInteger(column.as<std::int32_t>()[row]);
    case ColumnType::UInt32:
        return CellValue::ofInteger(column.as<std::uint32_t>()[row]);
    case ColumnType::Int64:
        return CellValue::ofInteger(column.as<std::int64_t>()[row]);
    case ColumnType::Float32:
        return CellValue::ofReal(static_cast<double>(column.as<float>()[row]));
    case ColumnType::Float64:
        return CellValue::ofReal(column.as<double>()[row]);
    case ColumnType::Utf8:
    case ColumnType::Dictionary:
        return CellValue::ofText(std::string(column.textAt(row)));
    case ColumnType::FixedList:
    case ColumnType::List:
    {
        const QueryColumn& child = *column.child();
        const auto first = static_cast<std::size_t>(column.listBegin(row));
        const auto count = static_cast<std::size_t>(column.listLength(row));
        if (child.type() == ColumnType::Float64)
        {
            const auto values = child.as<double>();
            return CellValue::ofReals(std::vector<double>(values.begin() + static_cast<std::ptrdiff_t>(first),
                                                          values.begin() + static_cast<std::ptrdiff_t>(first + count)));
        }
        std::vector<std::int64_t> values(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            switch (child.type())
            {
            case ColumnType::Int32:
                values[index] = child.as<std::int32_t>()[first + index];
                break;
            case ColumnType::UInt32:
                values[index] = child.as<std::uint32_t>()[first + index];
                break;
            case ColumnType::Int64:
                values[index] = child.as<std::int64_t>()[first + index];
                break;
            case ColumnType::UInt8:
                values[index] = child.as<std::uint8_t>()[first + index];
                break;
            default:
                return {}; // a list of lists or of text: not representable
            }
        }
        return CellValue::ofIntegers(std::move(values));
    }
    }
    return {};
}

} // namespace oor::query

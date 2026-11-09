#include "oor/query/apply/ColumnEdits.hpp"

#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ColumnOps.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace oor::query
{

namespace
{

bool fits(ColumnType type, std::int64_t value) noexcept
{
    switch (type)
    {
    case ColumnType::Bool:
        return value == 0 || value == 1;
    case ColumnType::UInt8:
        return value >= 0 && value <= 255;
    case ColumnType::Int32:
        return value >= std::numeric_limits<std::int32_t>::min() && value <= std::numeric_limits<std::int32_t>::max();
    case ColumnType::UInt32:
        return value >= 0 && value <= static_cast<std::int64_t>(std::numeric_limits<std::uint32_t>::max());
    case ColumnType::Int64:
        return true;
    default:
        return false;
    }
}

// A real that is a whole number that an int64 holds.
bool wholeNumber(double value, std::int64_t& result) noexcept
{
    if (!std::isfinite(value) || value != std::floor(value) || std::fabs(value) > 9.0e18)
        return false;
    result = static_cast<std::int64_t>(value);
    return true;
}

std::string typeNameForMessage(ColumnType type)
{
    return std::string(columnTypeName(type));
}

// The integer a number stands for, or an explanation of why it is not one that fits `type`.
void integerFor(ColumnType type, const CellValue& value, std::int64_t& result)
{
    switch (value.kind())
    {
    case CellValue::Kind::Bool:
    case CellValue::Kind::Integer:
        result = value.asInteger();
        break;
    case CellValue::Kind::Real:
        if (!wholeNumber(value.asReal(), result))
            throw QueryTypeError("expected an integer, got " + value.toText());
        break;
    default:
        throw QueryTypeError("expected an integer, got " + value.toText());
    }
    if (!fits(type, result))
        throw QueryTypeError("the value " + std::to_string(result) + " does not fit a column of " +
                             typeNameForMessage(type));
}

// Element `index` of a list as an integer for a list of `type` values.
std::int64_t listInteger(ColumnType type, const CellValue& list, std::size_t index)
{
    std::int64_t result = 0;
    if (list.kind() == CellValue::Kind::IntegerList)
        result = list.asIntegers()[index];
    else if (!wholeNumber(list.asReals()[index], result))
        throw QueryTypeError("expected a list of integers, got " + list.toText());
    if (!fits(type, result))
        throw QueryTypeError("the value " + std::to_string(result) + " does not fit a list of " +
                             typeNameForMessage(type));
    return result;
}

template <typename T>
T converted(double value)
{
    return static_cast<T>(value);
}

template <typename F>
QueryColumn withValueType(ColumnType type, F&& build)
{
    switch (type)
    {
    case ColumnType::Float64:
        return build(double{});
    case ColumnType::Int32:
        return build(std::int32_t{});
    case ColumnType::UInt32:
        return build(std::uint32_t{});
    case ColumnType::Int64:
        return build(std::int64_t{});
    case ColumnType::UInt8:
        return build(std::uint8_t{});
    default:
        throw QueryTypeError("a column of " + typeNameForMessage(type) + " cannot be edited by value");
    }
}

} // namespace

bool ColumnEdits::supports(const QueryColumn& column) noexcept
{
    switch (column.type())
    {
    case ColumnType::Float64:
    case ColumnType::Bool:
    case ColumnType::UInt8:
    case ColumnType::Int32:
    case ColumnType::UInt32:
    case ColumnType::Int64:
    case ColumnType::Utf8:
    case ColumnType::Dictionary:
        return true;
    case ColumnType::FixedList:
    case ColumnType::List:
        return column.child() != nullptr && column.child()->type() != ColumnType::Float32 &&
               (isNumeric(column.child()->type()));
    case ColumnType::Float32: // a result of an analysis: not a card field
        return false;
    }
    return false;
}

ColumnEdits::ColumnEdits(const QueryColumn& base)
{
    if (!supports(base))
        throw QueryTypeError("a column of " + typeNameForMessage(base.type()) + " cannot be edited by value");
    m_columnType = base.type();
    m_valueType = base.type();
    m_state.assign(base.size(), UNCHANGED);
    switch (base.type())
    {
    case ColumnType::Float64:
        m_kind = Kind::Real;
        break;
    case ColumnType::Utf8:
    case ColumnType::Dictionary:
        m_kind = Kind::Text;
        break;
    case ColumnType::FixedList:
    case ColumnType::List:
        m_valueType = base.child()->type();
        m_kind = m_valueType == ColumnType::Float64 ? Kind::RealList : Kind::IntegerList;
        m_width = base.type() == ColumnType::FixedList ? base.width() : 0;
        break;
    default:
        m_kind = Kind::Integer;
        break;
    }
}

std::vector<std::uint32_t> ColumnEdits::changedRows() const
{
    std::vector<std::uint32_t> rows;
    rows.reserve(m_changed);
    for (std::size_t row = 0; row < m_state.size(); ++row)
    {
        if (m_state[row] != UNCHANGED)
            rows.push_back(static_cast<std::uint32_t>(row));
    }
    return rows;
}

void ColumnEdits::touch(std::size_t row, std::uint8_t state) noexcept
{
    const std::uint8_t before = m_state[row];
    if (before == UNCHANGED)
        ++m_changed;
    if (before == SET_NULL)
        --m_nulls;
    if (state == SET_NULL)
        ++m_nulls;
    m_state[row] = state;
}

void ColumnEdits::ensureNumbers()
{
    const std::size_t count = m_state.size() * (m_width > 0 ? static_cast<std::size_t>(m_width) : 1);
    if (m_kind == Kind::Real || (m_kind == Kind::RealList && m_width > 0))
    {
        if (m_reals.size() != count)
            m_reals.resize(count, 0.0);
    }
    else if (m_kind == Kind::Integer || (m_kind == Kind::IntegerList && m_width > 0))
    {
        if (m_integers.size() != count)
            m_integers.resize(count, 0);
    }
}

void ColumnEdits::checkRows(std::span<const std::uint32_t> rows) const
{
    for (const std::uint32_t row : rows)
    {
        if (row >= m_state.size())
            throw QueryError("row " + std::to_string(row) + " is outside the table (" + std::to_string(m_state.size()) +
                             " rows)");
    }
}

void ColumnEdits::check(const CellValue& value) const
{
    if (value.isNull())
        return;
    std::int64_t scratch = 0;
    switch (m_kind)
    {
    case Kind::Real:
        if (value.kind() != CellValue::Kind::Integer && value.kind() != CellValue::Kind::Real)
            throw QueryTypeError("expected a number, got " + value.toText());
        return;
    case Kind::Integer:
        integerFor(m_columnType, value, scratch);
        return;
    case Kind::Text:
        if (value.kind() != CellValue::Kind::Text)
            throw QueryTypeError("expected a text, got " + value.toText());
        return;
    case Kind::RealList:
    case Kind::IntegerList:
        if (!value.isList())
            throw QueryTypeError("expected a list of numbers, got " + value.toText());
        if (m_width > 0 && value.listSize() != static_cast<std::size_t>(m_width))
            throw QueryTypeError("expected a list of " + std::to_string(m_width) + " values, got " + value.toText());
        if (m_kind == Kind::IntegerList)
        {
            for (std::size_t index = 0; index < value.listSize(); ++index)
                (void)listInteger(m_valueType, value, index);
        }
        return;
    }
}

void ColumnEdits::set(std::size_t row, const CellValue& value)
{
    if (row >= m_state.size())
        throw QueryError("row " + std::to_string(row) + " is outside the table (" + std::to_string(m_state.size()) +
                         " rows)");
    check(value);
    if (value.isNull())
    {
        touch(row, SET_NULL);
        return;
    }
    switch (m_kind)
    {
    case Kind::Real:
        ensureNumbers();
        m_reals[row] = value.asReal();
        break;
    case Kind::Integer:
    {
        ensureNumbers();
        std::int64_t number = 0;
        integerFor(m_columnType, value, number);
        m_integers[row] = number;
        break;
    }
    case Kind::Text:
        m_texts[static_cast<std::uint32_t>(row)] = value.asText();
        break;
    case Kind::RealList:
        if (m_width > 0)
        {
            ensureNumbers();
            for (std::size_t index = 0; index < value.listSize(); ++index)
                m_reals[row * static_cast<std::size_t>(m_width) + index] = value.listReal(index);
        }
        else
        {
            std::vector<double> values(value.listSize());
            for (std::size_t index = 0; index < values.size(); ++index)
                values[index] = value.listReal(index);
            m_realLists[static_cast<std::uint32_t>(row)] = std::move(values);
        }
        break;
    case Kind::IntegerList:
        if (m_width > 0)
        {
            ensureNumbers();
            for (std::size_t index = 0; index < value.listSize(); ++index)
                m_integers[row * static_cast<std::size_t>(m_width) + index] = listInteger(m_valueType, value, index);
        }
        else
        {
            std::vector<std::int64_t> values(value.listSize());
            for (std::size_t index = 0; index < values.size(); ++index)
                values[index] = listInteger(m_valueType, value, index);
            m_integerLists[static_cast<std::uint32_t>(row)] = std::move(values);
        }
        break;
    }
    touch(row, SET);
}

void ColumnEdits::setReals(std::span<const std::uint32_t> rows, std::span<const double> values)
{
    if (m_kind != Kind::Real && m_kind != Kind::Integer)
        throw QueryTypeError("expected a value for each cell, not a number");
    if (values.size() != 1 && values.size() != rows.size())
        throw QueryError(std::to_string(values.size()) + " values for " + std::to_string(rows.size()) + " rows");
    checkRows(rows);
    if (m_kind == Kind::Real)
    {
        ensureNumbers();
        for (std::size_t index = 0; index < rows.size(); ++index)
        {
            m_reals[rows[index]] = values.size() == 1 ? values[0] : values[index];
            touch(rows[index], SET);
        }
        return;
    }
    // an integer column: every value must be a whole number that fits, checked before anything is written
    std::vector<std::int64_t> whole(values.size());
    for (std::size_t index = 0; index < values.size(); ++index)
        integerFor(m_columnType, CellValue::ofReal(values[index]), whole[index]);
    setIntegers(rows, whole);
}

void ColumnEdits::setIntegers(std::span<const std::uint32_t> rows, std::span<const std::int64_t> values)
{
    if (m_kind != Kind::Real && m_kind != Kind::Integer)
        throw QueryTypeError("expected a value for each cell, not a number");
    if (values.size() != 1 && values.size() != rows.size())
        throw QueryError(std::to_string(values.size()) + " values for " + std::to_string(rows.size()) + " rows");
    checkRows(rows);
    ensureNumbers();
    if (m_kind == Kind::Real)
    {
        for (std::size_t index = 0; index < rows.size(); ++index)
        {
            m_reals[rows[index]] = static_cast<double>(values.size() == 1 ? values[0] : values[index]);
            touch(rows[index], SET);
        }
        return;
    }
    for (const std::int64_t value : values)
    {
        if (!fits(m_columnType, value))
            throw QueryTypeError("the value " + std::to_string(value) + " does not fit a column of " +
                                 typeNameForMessage(m_columnType));
    }
    for (std::size_t index = 0; index < rows.size(); ++index)
    {
        m_integers[rows[index]] = values.size() == 1 ? values[0] : values[index];
        touch(rows[index], SET);
    }
}

void ColumnEdits::unset(std::size_t row) noexcept
{
    if (row >= m_state.size() || m_state[row] == UNCHANGED)
        return;
    if (m_state[row] == SET_NULL)
        --m_nulls;
    m_state[row] = UNCHANGED;
    --m_changed;
    m_texts.erase(static_cast<std::uint32_t>(row));
    m_realLists.erase(static_cast<std::uint32_t>(row));
    m_integerLists.erase(static_cast<std::uint32_t>(row));
}

void ColumnEdits::clear()
{
    std::fill(m_state.begin(), m_state.end(), UNCHANGED);
    m_changed = 0;
    m_nulls = 0;
    m_reals.clear();
    m_integers.clear();
    m_texts.clear();
    m_realLists.clear();
    m_integerLists.clear();
}

CellValue ColumnEdits::value(std::size_t row, const QueryColumn& base) const
{
    switch (m_state[row])
    {
    case UNCHANGED:
        return cellAt(base, row);
    case SET_NULL:
        return {};
    default:
        break;
    }
    switch (m_kind)
    {
    case Kind::Real:
        return CellValue::ofReal(m_reals[row]);
    case Kind::Integer:
        return m_columnType == ColumnType::Bool ? CellValue::ofBool(m_integers[row] != 0)
                                                : CellValue::ofInteger(m_integers[row]);
    case Kind::Text:
        return CellValue::ofText(m_texts.at(static_cast<std::uint32_t>(row)));
    case Kind::RealList:
    {
        if (m_width > 0)
        {
            const auto first = m_reals.begin() + static_cast<std::ptrdiff_t>(row * static_cast<std::size_t>(m_width));
            return CellValue::ofReals(std::vector<double>(first, first + m_width));
        }
        return CellValue::ofReals(m_realLists.at(static_cast<std::uint32_t>(row)));
    }
    case Kind::IntegerList:
    {
        if (m_width > 0)
        {
            const auto first =
                m_integers.begin() + static_cast<std::ptrdiff_t>(row * static_cast<std::size_t>(m_width));
            return CellValue::ofIntegers(std::vector<std::int64_t>(first, first + m_width));
        }
        return CellValue::ofIntegers(m_integerLists.at(static_cast<std::uint32_t>(row)));
    }
    }
    return {};
}

QueryColumn ColumnEdits::withNulls(QueryColumn column, const QueryColumn& base) const
{
    if (!base.hasNulls() && m_nulls == 0)
        return column;
    std::vector<std::uint8_t> valid(m_state.size());
    for (std::size_t row = 0; row < valid.size(); ++row)
    {
        if (m_state[row] == SET_NULL)
            valid[row] = 0;
        else if (m_state[row] == SET)
            valid[row] = 1;
        else
            valid[row] = base.isNull(row) ? 0 : 1;
    }
    std::int64_t nullCount = 0;
    ColumnBuffer bits = validityFromFlags(valid, nullCount);
    return nullCount > 0 ? column.withValidity(std::move(bits), nullCount) : column;
}

QueryColumn ColumnEdits::materializeScalar(const QueryColumn& base) const
{
    const std::size_t count = m_state.size();
    if (m_kind == Kind::Real)
    {
        const auto source = base.as<double>();
        std::vector<double> values(source.begin(), source.begin() + static_cast<std::ptrdiff_t>(count));
        for (std::size_t row = 0; row < count; ++row)
        {
            if (m_state[row] == SET)
                values[row] = m_reals[row];
            else if (m_state[row] == SET_NULL)
                values[row] = std::numeric_limits<double>::quiet_NaN();
        }
        return withNulls(QueryColumn::primitive(std::move(values)), base);
    }
    if (m_columnType == ColumnType::Bool)
    {
        std::vector<std::uint8_t> flags(count);
        for (std::size_t row = 0; row < count; ++row)
        {
            if (m_state[row] == SET)
                flags[row] = m_integers[row] != 0 ? 1 : 0;
            else if (m_state[row] == SET_NULL)
                flags[row] = 0;
            else
                flags[row] = base.boolAt(row) ? 1 : 0;
        }
        return withNulls(QueryColumn::boolean(flags), base);
    }
    return withNulls(withValueType(m_columnType,
                                   [&](auto tag)
                                   {
                                       using T = decltype(tag);
                                       const auto source = base.as<T>();
                                       std::vector<T> values(source.begin(),
                                                             source.begin() + static_cast<std::ptrdiff_t>(count));
                                       for (std::size_t row = 0; row < count; ++row)
                                       {
                                           if (m_state[row] == SET)
                                               values[row] = static_cast<T>(m_integers[row]);
                                           else if (m_state[row] == SET_NULL)
                                               values[row] = T{};
                                       }
                                       return QueryColumn::primitive(std::move(values));
                                   }),
                     base);
}

QueryColumn ColumnEdits::materializeText(const QueryColumn& base) const
{
    const std::size_t count = m_state.size();
    std::vector<std::string_view> texts(count);
    for (std::size_t row = 0; row < count; ++row)
    {
        if (m_state[row] == SET)
            texts[row] = m_texts.at(static_cast<std::uint32_t>(row));
        else if (m_state[row] == UNCHANGED)
            texts[row] = base.textAt(row);
    }
    return withNulls(QueryColumn::utf8(texts), base);
}

QueryColumn ColumnEdits::materializeList(const QueryColumn& base) const
{
    const std::size_t count = m_state.size();
    const QueryColumn& child = *base.child();
    if (m_width > 0)
    {
        const std::size_t width = static_cast<std::size_t>(m_width);
        QueryColumn values = withValueType(
            m_valueType,
            [&](auto tag)
            {
                using T = decltype(tag);
                const auto source = child.as<T>();
                std::vector<T> out(source.begin(), source.begin() + static_cast<std::ptrdiff_t>(count * width));
                for (std::size_t row = 0; row < count; ++row)
                {
                    if (m_state[row] != SET)
                        continue;
                    for (std::size_t index = 0; index < width; ++index)
                    {
                        out[row * width + index] = m_kind == Kind::RealList
                                                       ? converted<T>(m_reals[row * width + index])
                                                       : static_cast<T>(m_integers[row * width + index]);
                    }
                }
                return QueryColumn::primitive(std::move(out));
            });
        return withNulls(QueryColumn::fixedList(std::move(values), m_width, static_cast<std::int64_t>(count)), base);
    }
    std::vector<std::int64_t> offsets(count + 1, 0);
    QueryColumn values = withValueType(
        m_valueType,
        [&](auto tag)
        {
            using T = decltype(tag);
            const auto source = child.as<T>();
            std::vector<T> out;
            for (std::size_t row = 0; row < count; ++row)
            {
                if (m_state[row] == SET)
                {
                    if (m_kind == Kind::RealList)
                    {
                        for (const double value : m_realLists.at(static_cast<std::uint32_t>(row)))
                            out.push_back(converted<T>(value));
                    }
                    else
                    {
                        for (const std::int64_t value : m_integerLists.at(static_cast<std::uint32_t>(row)))
                            out.push_back(static_cast<T>(value));
                    }
                }
                else if (m_state[row] == UNCHANGED)
                {
                    const auto first = static_cast<std::size_t>(base.listBegin(row));
                    const auto length = static_cast<std::size_t>(base.listLength(row));
                    out.insert(out.end(), source.begin() + static_cast<std::ptrdiff_t>(first),
                               source.begin() + static_cast<std::ptrdiff_t>(first + length));
                }
                offsets[row + 1] = static_cast<std::int64_t>(out.size());
            }
            return QueryColumn::primitive(std::move(out));
        });
    return withNulls(
        QueryColumn::list(ColumnBuffer::adopt(std::move(offsets)), std::move(values), static_cast<std::int64_t>(count)),
        base);
}

QueryColumn ColumnEdits::materialize(const QueryColumn& base) const
{
    if (base.size() != m_state.size())
        throw QueryError("the edits were made for a column of " + std::to_string(m_state.size()) + " rows, not " +
                         std::to_string(base.size()));
    switch (m_kind)
    {
    case Kind::Real:
    case Kind::Integer:
        return materializeScalar(base);
    case Kind::Text:
        return materializeText(base);
    case Kind::RealList:
    case Kind::IntegerList:
        return materializeList(base);
    }
    return base;
}

} // namespace oor::query

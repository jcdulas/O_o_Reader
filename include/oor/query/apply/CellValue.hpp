#pragma once

#include "oor/query/table/QueryColumn.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace oor::query
{

// The value of one cell as the editing code handles it: a null, a bool, an integer, a real, a text, or a list of
// integers or of reals. It is what a record attribute, an entry of the change list and a field written to a card are
// made of; bulk edits of numeric columns go through spans and never build one per cell.
class CellValue
{
public:
    enum class Kind : std::uint8_t
    {
        Null,
        Bool,
        Integer,
        Real,
        Text,
        IntegerList,
        RealList
    };

    CellValue() = default;
    [[nodiscard]] static CellValue ofBool(bool value);
    [[nodiscard]] static CellValue ofInteger(std::int64_t value);
    [[nodiscard]] static CellValue ofReal(double value);
    [[nodiscard]] static CellValue ofText(std::string value);
    [[nodiscard]] static CellValue ofIntegers(std::vector<std::int64_t> values);
    [[nodiscard]] static CellValue ofReals(std::vector<double> values);

    [[nodiscard]] Kind kind() const noexcept { return m_kind; }
    [[nodiscard]] bool isNull() const noexcept { return m_kind == Kind::Null; }
    [[nodiscard]] bool isNumber() const noexcept
    {
        return m_kind == Kind::Bool || m_kind == Kind::Integer || m_kind == Kind::Real;
    }
    [[nodiscard]] bool isList() const noexcept { return m_kind == Kind::IntegerList || m_kind == Kind::RealList; }

    // The value as the kind says; the caller checks the kind first. `asReal` takes a bool or an integer too.
    [[nodiscard]] bool asBool() const { return std::get<bool>(m_data); }
    [[nodiscard]] std::int64_t asInteger() const;
    [[nodiscard]] double asReal() const;
    [[nodiscard]] const std::string& asText() const { return std::get<std::string>(m_data); }
    [[nodiscard]] const std::vector<std::int64_t>& asIntegers() const
    {
        return std::get<std::vector<std::int64_t>>(m_data);
    }
    [[nodiscard]] const std::vector<double>& asReals() const { return std::get<std::vector<double>>(m_data); }
    // The number of values of a list.
    [[nodiscard]] std::size_t listSize() const;
    // Value `index` of a list as a real.
    [[nodiscard]] double listReal(std::size_t index) const;

    // Same value: numbers compare as numbers (3 and 3.0 are the same), NaN is the same as NaN and as a null (a null of
    // a float column is a NaN), lists compare element by element.
    [[nodiscard]] bool sameAs(const CellValue& other) const;

    // A short text for messages: 2.5, 'abc', [1, 2, 3], null.
    [[nodiscard]] std::string toText() const;

private:
    Kind m_kind = Kind::Null;
    std::variant<std::monostate, bool, std::int64_t, double, std::string, std::vector<std::int64_t>,
                 std::vector<double>>
        m_data;
};

// The value of row `row` of a column (a list as a list of its values). A list of lists is not representable: Null.
[[nodiscard]] CellValue cellAt(const QueryColumn& column, std::size_t row);

} // namespace oor::query

#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/core/CardProblem.hpp"
#include "oor/model/format/FieldReader.hpp"
#include "oor/model/format/FieldWriter.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

// The one kind of value the user dummy elements share: the list of fields that follow the identification numbers of a
// CDUMi connection card (grid points, then the additional fields Ai) and of a PDUMi property card (the additional
// fields Aj). Each field may be blank, an integer or a real, and what it means is decided by the user's own element
// routines, which the library does not have. The list is therefore kept as one ordered sequence in the positions of the
// card, a blank included (a grid point left out in the middle of a connection card keeps the positions of the fields
// after it), and every card still reads, writes and checks its own fields by hand.
namespace oor::model
{

enum class DummyFieldKind : std::uint8_t
{
    Blank,
    Integer,
    Real
};

// One field of the list. An integer is held in `number` exactly (it has 32 bits, a double has 53), so that the list
// has one column of kinds and one of numbers.
struct DummyField
{
    DummyFieldKind kind = DummyFieldKind::Blank;
    double number = 0.0;

    [[nodiscard]] static DummyField ofInteger(std::int32_t value) noexcept
    {
        return DummyField{DummyFieldKind::Integer, static_cast<double>(value)};
    }
    [[nodiscard]] static DummyField ofReal(double value) noexcept { return DummyField{DummyFieldKind::Real, value}; }

    [[nodiscard]] bool isBlank() const noexcept { return kind == DummyFieldKind::Blank; }
    [[nodiscard]] bool isInteger() const noexcept { return kind == DummyFieldKind::Integer; }
    [[nodiscard]] bool isReal() const noexcept { return kind == DummyFieldKind::Real; }
    [[nodiscard]] std::int32_t integerValue() const noexcept { return static_cast<std::int32_t>(number); }

    [[nodiscard]] bool operator==(const DummyField&) const noexcept = default;
};

using DummyFieldList = std::vector<DummyField>;

// Reads the fields from `firstField` to the last field of the card that is not blank, as the integer or the real each
// one is. A field that is neither is an error named `name` (the reader goes on). The list never ends with a blank: the
// trailing blank fields of a card carry nothing.
[[nodiscard]] inline DummyFieldList readDummyFields(FieldReader& reader, int firstField, const char* name)
{
    DummyFieldList fields;
    const int lastField = reader.lastUsedField();
    for (int field = firstField; field <= lastField; ++field)
    {
        const std::optional<std::variant<std::int32_t, double>> value = reader.integerOrReal(field, name);
        if (!value)
            fields.push_back(DummyField{});
        else if (std::holds_alternative<std::int32_t>(*value))
            fields.push_back(DummyField::ofInteger(std::get<std::int32_t>(*value)));
        else
            fields.push_back(DummyField::ofReal(std::get<double>(*value)));
    }
    return fields;
}

// Writes the list from `firstField` on, one field each; a blank is left blank.
inline void writeDummyFields(FieldWriter& writer, int firstField, const DummyFieldList& fields)
{
    for (std::size_t index = 0; index < fields.size(); ++index)
    {
        const int field = firstField + static_cast<int>(index);
        if (fields[index].isInteger())
            writer.integer(field, fields[index].integerValue());
        else if (fields[index].isReal())
            writer.real(field, fields[index].number);
    }
}

// The problem with the values of the list, if there is one. The format reference leaves the meaning of the fields to
// the user's routines (the user code checks every field), so the only rule is the
// one that every real has: a number that every format can write and read back.
[[nodiscard]] inline std::optional<CardProblem> checkDummyFields(const DummyFieldList& fields, const char* name)
{
    for (const DummyField& field : fields)
    {
        if (field.isReal() && !std::isfinite(field.number))
            return CardProblem{"card.bad-value", std::string(CardProblem::FIELD) + " must be a finite number", name};
    }
    return std::nullopt;
}

// Removes the blank fields at the end of the list (a card cannot tell them from fields that are not there).
inline void dropTrailingBlankFields(DummyFieldList& fields)
{
    while (!fields.empty() && fields.back().isBlank())
        fields.pop_back();
}

// How many fields at the start of the list are grid points: the run of positive integers, which ends at the first
// blank, real or integer that is not positive. A connection card cannot say where its grid points end and its
// additional fields begin: that is the NG of the ADUMi of the same number, which the reader does not have. So the run
// takes the integer additional fields that follow the last grid point, with no blank or real between, for grid points.
[[nodiscard]] inline std::size_t leadingGridCount(const DummyFieldList& fields) noexcept
{
    std::size_t count = 0;
    while (count < fields.size() && fields[count].isInteger() && fields[count].number > 0.0)
        ++count;
    return count;
}

// The IDs of that run.
[[nodiscard]] inline std::vector<base::EntityId> leadingGrids(const DummyFieldList& fields)
{
    std::vector<base::EntityId> grids;
    const std::size_t count = leadingGridCount(fields);
    grids.reserve(count);
    for (std::size_t index = 0; index < count; ++index)
        grids.push_back(fields[index].integerValue());
    return grids;
}

// The lists of fields of all the cards of a type, in two columns that share their buffers when a model is copied: the
// kind of each field and its number. Every dummy card table keeps its list in one.
class DummyFieldColumns
{
public:
    void push(const DummyFieldList& fields)
    {
        const Split split = splitFields(fields);
        m_kinds.push(split.kinds);
        m_numbers.push(split.numbers);
    }

    [[nodiscard]] DummyFieldList get(std::size_t row) const
    {
        const std::span<const std::uint8_t> kinds = m_kinds.get(row);
        const std::span<const double> numbers = m_numbers.get(row);
        DummyFieldList fields;
        fields.reserve(kinds.size());
        for (std::size_t index = 0; index < kinds.size(); ++index)
            fields.push_back(DummyField{static_cast<DummyFieldKind>(kinds[index]), numbers[index]});
        return fields;
    }

    void set(std::size_t row, const DummyFieldList& fields)
    {
        const Split split = splitFields(fields);
        m_kinds.set(row, split.kinds);
        m_numbers.set(row, split.numbers);
    }

    void keepRows(const std::vector<std::uint32_t>& rows)
    {
        m_kinds.keepRows(rows);
        m_numbers.keepRows(rows);
    }

    [[nodiscard]] VariableLengthColumn<std::uint8_t>& kindColumn() noexcept { return m_kinds; }
    [[nodiscard]] VariableLengthColumn<double>& numberColumn() noexcept { return m_numbers; }

private:
    struct Split
    {
        std::vector<std::uint8_t> kinds;
        std::vector<double> numbers;
    };

    [[nodiscard]] static Split splitFields(const DummyFieldList& fields)
    {
        Split split;
        split.kinds.reserve(fields.size());
        split.numbers.reserve(fields.size());
        for (const DummyField& field : fields)
        {
            split.kinds.push_back(static_cast<std::uint8_t>(field.kind));
            split.numbers.push_back(field.number);
        }
        return split;
    }

    VariableLengthColumn<std::uint8_t> m_kinds;
    VariableLengthColumn<double> m_numbers;
};

} // namespace oor::model

#pragma once

#include "oor/model/cards/parameters/parameter/ParameterCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct ParameterFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ParameterCard& card);
    [[nodiscard]] static std::optional<ParameterCard> read(FieldReader& reader);
    static void write(const ParameterCard& card, FieldWriter& writer);
};

// All the PARAM cards. A parameter has no ID: it is found by name.
class ParameterTable final : public TypedTable<ParameterTable, ParameterCard>
{
public:
    static constexpr std::string_view NAME = "Parameter";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Parameter, base::IdSpace::None};
    using Format = ParameterFormat;

    [[nodiscard]] static base::EntityId cardId(const ParameterCard&) noexcept { return 0; }
    std::uint32_t append(const ParameterCard& card, base::CardKey key);
    [[nodiscard]] ParameterCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ParameterCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {}

    [[nodiscard]] Column<std::string>& nameColumn() noexcept { return m_name; }
    [[nodiscard]] Column<std::uint8_t>& kindColumn() noexcept { return m_kind; }
    [[nodiscard]] Column<std::int32_t>& integerValueColumn() noexcept { return m_integerValue; }
    [[nodiscard]] Column<double>& realValueColumn() noexcept { return m_realValue; }
    [[nodiscard]] Column<double>& imaginaryValueColumn() noexcept { return m_imaginaryValue; }
    [[nodiscard]] Column<std::string>& textValueColumn() noexcept { return m_textValue; }

private:
    Column<std::string> m_name;
    Column<std::uint8_t> m_kind;
    Column<std::int32_t> m_integerValue;
    Column<double> m_realValue;
    Column<double> m_imaginaryValue;
    Column<std::string> m_textValue;
};

} // namespace oor::model

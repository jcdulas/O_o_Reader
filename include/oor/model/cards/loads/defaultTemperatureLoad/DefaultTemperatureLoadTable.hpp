#pragma once

#include "oor/model/cards/loads/defaultTemperatureLoad/DefaultTemperatureLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DefaultTemperatureLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by readAll and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DefaultTemperatureLoadCard& card);
    // One default temperature per pair of fields (up to four pairs).
    [[nodiscard]] static std::vector<DefaultTemperatureLoadCard> readAll(FieldReader& reader);
    static void write(const DefaultTemperatureLoadCard& card, FieldWriter& writer);
};

// All the TEMPD default temperatures, one row each. The ID is a temperature set ID, shared with the TEMP cards of the
// set and with the other load cards (IdSpace::Group, category Load): TEMPERATURE(LOAD) = SID of Case Control selects
// it.
class DefaultTemperatureLoadTable final : public TypedTable<DefaultTemperatureLoadTable, DefaultTemperatureLoadCard>
{
public:
    static constexpr std::string_view NAME = "DefaultTemperatureLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = DefaultTemperatureLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const DefaultTemperatureLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const DefaultTemperatureLoadCard& card, base::CardKey key);
    [[nodiscard]] DefaultTemperatureLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const DefaultTemperatureLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& temperatureColumn() noexcept { return m_temperature; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_temperature;
};

} // namespace oor::model

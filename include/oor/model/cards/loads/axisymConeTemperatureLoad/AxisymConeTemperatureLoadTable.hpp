#pragma once

#include "oor/model/cards/loads/axisymConeTemperatureLoad/AxisymConeTemperatureLoadCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymConeTemperatureLoadFormat
{
    // The first thing wrong with the values of a card, if anything (used by readAll and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymConeTemperatureLoadCard& card);
    // One temperature, or two when the second group of four fields is there.
    [[nodiscard]] static std::vector<AxisymConeTemperatureLoadCard> readAll(FieldReader& reader);
    static void write(const AxisymConeTemperatureLoadCard& card, FieldWriter& writer);
};

// All the TEMPAX temperatures, one row each. The ID is a temperature set ID (IdSpace::Group, category Load).
class AxisymConeTemperatureLoadTable final
    : public TypedTable<AxisymConeTemperatureLoadTable, AxisymConeTemperatureLoadCard>
{
public:
    static constexpr std::string_view NAME = "AxisymConeTemperatureLoad";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = AxisymConeTemperatureLoadFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymConeTemperatureLoadCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymConeTemperatureLoadCard& card, base::CardKey key);
    [[nodiscard]] AxisymConeTemperatureLoadCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymConeTemperatureLoadCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& ringIdColumn() noexcept { return m_ringId; }
    [[nodiscard]] Column<double>& phiColumn() noexcept { return m_phi; }
    [[nodiscard]] Column<double>& temperatureColumn() noexcept { return m_temperature; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_ringId;
    Column<double> m_phi;
    Column<double> m_temperature;
};

} // namespace oor::model

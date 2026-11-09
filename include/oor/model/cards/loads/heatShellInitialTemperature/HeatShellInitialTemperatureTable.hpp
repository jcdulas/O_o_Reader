#pragma once

#include "oor/model/cards/loads/heatShellInitialTemperature/HeatShellInitialTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HeatShellInitialTemperatureFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatShellInitialTemperatureCard& card);
    [[nodiscard]] static std::optional<HeatShellInitialTemperatureCard> read(FieldReader& reader);
    static void write(const HeatShellInitialTemperatureCard& card, FieldWriter& writer);
};

// All the TEMPN1 cards. The ID is a temperature set ID shared by many cards (IdSpace::Group, category Load), so that
// `TEMP(INIT) = n`, `IC = n` and a TLOAD1 EXCITEID find it. The points are three lists in CSR form (the grid points,
// the components and the temperatures); each grid point is a Node reference.
class HeatShellInitialTemperatureTable final
    : public TypedTable<HeatShellInitialTemperatureTable, HeatShellInitialTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "HeatShellInitialTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Load, base::IdSpace::Group};
    using Format = HeatShellInitialTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const HeatShellInitialTemperatureCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatShellInitialTemperatureCard& card, base::CardKey key);
    [[nodiscard]] HeatShellInitialTemperatureCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatShellInitialTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] VariableLengthColumn<base::EntityId>& nodeColumn() noexcept { return m_node; }
    [[nodiscard]] VariableLengthColumn<std::int32_t>& componentsColumn() noexcept { return m_components; }
    [[nodiscard]] VariableLengthColumn<double>& temperatureColumn() noexcept { return m_temperature; }

private:
    Column<base::EntityId> m_id;
    VariableLengthColumn<base::EntityId> m_node;
    VariableLengthColumn<std::int32_t> m_components;
    VariableLengthColumn<double> m_temperature;
};

} // namespace oor::model

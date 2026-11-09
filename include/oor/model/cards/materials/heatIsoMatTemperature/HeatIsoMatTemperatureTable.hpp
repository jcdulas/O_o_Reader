#pragma once

#include "oor/model/cards/materials/heatIsoMatTemperature/HeatIsoMatTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HeatIsoMatTemperatureFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatIsoMatTemperatureCard& card);
    [[nodiscard]] static std::optional<HeatIsoMatTemperatureCard> read(FieldReader& reader);
    static void write(const HeatIsoMatTemperatureCard& card, FieldWriter& writer);
};

// All the MATT4 cards. The ID space is the card type's own: the ID is the one of the MAT4 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it), and the tables are references to
// TABLEMi cards (ReferenceTarget::MaterialTable).
class HeatIsoMatTemperatureTable final : public TypedTable<HeatIsoMatTemperatureTable, HeatIsoMatTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "HeatIsoMatTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ThermalMaterial, base::IdSpace::PerCardType};
    using Format = HeatIsoMatTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const HeatIsoMatTemperatureCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatIsoMatTemperatureCard& card, base::CardKey key);
    [[nodiscard]] HeatIsoMatTemperatureCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatIsoMatTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& conductivityTableIdColumn() noexcept { return m_conductivityTableId; }
    [[nodiscard]] Column<base::EntityId>& specificHeatTableIdColumn() noexcept { return m_specificHeatTableId; }
    [[nodiscard]] Column<base::EntityId>& convectionCoefficientTableIdColumn() noexcept
    {
        return m_convectionCoefficientTableId;
    }
    [[nodiscard]] Column<base::EntityId>& viscosityTableIdColumn() noexcept { return m_viscosityTableId; }
    [[nodiscard]] Column<base::EntityId>& heatGenerationFactorTableIdColumn() noexcept
    {
        return m_heatGenerationFactorTableId;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_conductivityTableId;
    Column<base::EntityId> m_specificHeatTableId;
    Column<base::EntityId> m_convectionCoefficientTableId;
    Column<base::EntityId> m_viscosityTableId;
    Column<base::EntityId> m_heatGenerationFactorTableId;
};

} // namespace oor::model

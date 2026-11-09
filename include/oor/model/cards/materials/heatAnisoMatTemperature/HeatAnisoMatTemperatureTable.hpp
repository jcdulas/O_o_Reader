#pragma once

#include "oor/model/cards/materials/heatAnisoMatTemperature/HeatAnisoMatTemperatureCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HeatAnisoMatTemperatureFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatAnisoMatTemperatureCard& card);
    [[nodiscard]] static std::optional<HeatAnisoMatTemperatureCard> read(FieldReader& reader);
    static void write(const HeatAnisoMatTemperatureCard& card, FieldWriter& writer);
};

// All the MATT5 cards. The ID space is the card type's own: the ID is the one of the MAT5 that the card extends, so it
// is also a reference to a material (renumbering the materials follows into it), and the tables are references to
// TABLEMi cards (ReferenceTarget::MaterialTable).
class HeatAnisoMatTemperatureTable final : public TypedTable<HeatAnisoMatTemperatureTable, HeatAnisoMatTemperatureCard>
{
public:
    static constexpr std::string_view NAME = "HeatAnisoMatTemperature";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ThermalMaterial, base::IdSpace::PerCardType};
    using Format = HeatAnisoMatTemperatureFormat;

    [[nodiscard]] static base::EntityId cardId(const HeatAnisoMatTemperatureCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatAnisoMatTemperatureCard& card, base::CardKey key);
    [[nodiscard]] HeatAnisoMatTemperatureCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatAnisoMatTemperatureCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& conductivityXxTableIdColumn() noexcept { return m_conductivityXxTableId; }
    [[nodiscard]] Column<base::EntityId>& conductivityXyTableIdColumn() noexcept { return m_conductivityXyTableId; }
    [[nodiscard]] Column<base::EntityId>& conductivityXzTableIdColumn() noexcept { return m_conductivityXzTableId; }
    [[nodiscard]] Column<base::EntityId>& conductivityYyTableIdColumn() noexcept { return m_conductivityYyTableId; }
    [[nodiscard]] Column<base::EntityId>& conductivityYzTableIdColumn() noexcept { return m_conductivityYzTableId; }
    [[nodiscard]] Column<base::EntityId>& conductivityZzTableIdColumn() noexcept { return m_conductivityZzTableId; }
    [[nodiscard]] Column<base::EntityId>& specificHeatTableIdColumn() noexcept { return m_specificHeatTableId; }
    [[nodiscard]] Column<base::EntityId>& heatGenerationFactorTableIdColumn() noexcept
    {
        return m_heatGenerationFactorTableId;
    }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_conductivityXxTableId;
    Column<base::EntityId> m_conductivityXyTableId;
    Column<base::EntityId> m_conductivityXzTableId;
    Column<base::EntityId> m_conductivityYyTableId;
    Column<base::EntityId> m_conductivityYzTableId;
    Column<base::EntityId> m_conductivityZzTableId;
    Column<base::EntityId> m_specificHeatTableId;
    Column<base::EntityId> m_heatGenerationFactorTableId;
};

} // namespace oor::model

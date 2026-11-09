#pragma once

#include "oor/model/cards/thermal/heatAnisoMat/HeatAnisoMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HeatAnisoMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatAnisoMatCard& card);
    [[nodiscard]] static std::optional<HeatAnisoMatCard> read(FieldReader& reader);
    static void write(const HeatAnisoMatCard& card, FieldWriter& writer);
};

// All the MAT5 cards. The ID is a heat transfer material ID, in the space that MAT4 shares (IdSpace::ThermalMaterial).
class HeatAnisoMatTable final : public TypedTable<HeatAnisoMatTable, HeatAnisoMatCard>
{
public:
    static constexpr std::string_view NAME = "HeatAnisoMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ThermalMaterial, base::IdSpace::ThermalMaterial};
    using Format = HeatAnisoMatFormat;

    HeatAnisoMatTable();

    [[nodiscard]] static base::EntityId cardId(const HeatAnisoMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatAnisoMatCard& card, base::CardKey key);
    [[nodiscard]] HeatAnisoMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatAnisoMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a MAT5 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& conductivityXxColumn() noexcept { return m_conductivityXx; }
    [[nodiscard]] Column<double>& conductivityXyColumn() noexcept { return m_conductivityXy; }
    [[nodiscard]] Column<double>& conductivityXzColumn() noexcept { return m_conductivityXz; }
    [[nodiscard]] Column<double>& conductivityYyColumn() noexcept { return m_conductivityYy; }
    [[nodiscard]] Column<double>& conductivityYzColumn() noexcept { return m_conductivityYz; }
    [[nodiscard]] Column<double>& conductivityZzColumn() noexcept { return m_conductivityZz; }
    [[nodiscard]] Column<double>& specificHeatColumn() noexcept { return m_specificHeat; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& heatGenerationFactorColumn() noexcept { return m_heatGenerationFactor; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_conductivityXx;
    Column<double> m_conductivityXy;
    Column<double> m_conductivityXz;
    Column<double> m_conductivityYy;
    Column<double> m_conductivityYz;
    Column<double> m_conductivityZz;
    Column<double> m_specificHeat;
    Column<double> m_density;
    Column<double> m_heatGenerationFactor;
};

} // namespace oor::model

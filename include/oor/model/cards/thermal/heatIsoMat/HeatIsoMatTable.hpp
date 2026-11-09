#pragma once

#include "oor/model/cards/thermal/heatIsoMat/HeatIsoMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HeatIsoMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatIsoMatCard& card);
    [[nodiscard]] static std::optional<HeatIsoMatCard> read(FieldReader& reader);
    static void write(const HeatIsoMatCard& card, FieldWriter& writer);
};

// All the MAT4 cards. The ID is a heat transfer material ID, in the space that MAT5 shares (IdSpace::ThermalMaterial):
// it may be the ID of a MAT1, the two spaces are apart. The absent values are NaN.
class HeatIsoMatTable final : public TypedTable<HeatIsoMatTable, HeatIsoMatCard>
{
public:
    static constexpr std::string_view NAME = "HeatIsoMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ThermalMaterial, base::IdSpace::ThermalMaterial};
    using Format = HeatIsoMatFormat;

    HeatIsoMatTable();

    [[nodiscard]] static base::EntityId cardId(const HeatIsoMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatIsoMatCard& card, base::CardKey key);
    [[nodiscard]] HeatIsoMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HeatIsoMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a MAT4 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& conductivityColumn() noexcept { return m_conductivity; }
    [[nodiscard]] Column<double>& specificHeatColumn() noexcept { return m_specificHeat; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& convectionCoefficientColumn() noexcept { return m_convectionCoefficient; }
    [[nodiscard]] Column<double>& viscosityColumn() noexcept { return m_viscosity; }
    [[nodiscard]] Column<double>& heatGenerationFactorColumn() noexcept { return m_heatGenerationFactor; }
    [[nodiscard]] Column<double>& referenceEnthalpyColumn() noexcept { return m_referenceEnthalpy; }
    [[nodiscard]] Column<double>& phaseChangeTemperatureColumn() noexcept { return m_phaseChangeTemperature; }
    [[nodiscard]] Column<double>& phaseChangeRangeColumn() noexcept { return m_phaseChangeRange; }
    [[nodiscard]] Column<double>& latentHeatColumn() noexcept { return m_latentHeat; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_conductivity;
    Column<double> m_specificHeat;
    Column<double> m_density;
    Column<double> m_convectionCoefficient;
    Column<double> m_viscosity;
    Column<double> m_heatGenerationFactor;
    Column<double> m_referenceEnthalpy;
    Column<double> m_phaseChangeTemperature;
    Column<double> m_phaseChangeRange;
    Column<double> m_latentHeat;
};

} // namespace oor::model

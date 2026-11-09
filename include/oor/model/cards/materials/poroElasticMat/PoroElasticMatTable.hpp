#pragma once

#include "oor/model/cards/materials/poroElasticMat/PoroElasticMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

struct PoroElasticMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PoroElasticMatCard& card);
    [[nodiscard]] static std::optional<PoroElasticMatCard> read(FieldReader& reader);
    static void write(const PoroElasticMatCard& card, FieldWriter& writer);
};

// All the MATPE1 cards. The ID is a material ID, in the space every material type shares (IdSpace::Material). MAT1 and
// MAT10 refer to materials (ReferenceTarget::Material).
class PoroElasticMatTable final : public TypedTable<PoroElasticMatTable, PoroElasticMatCard>
{
public:
    static constexpr std::string_view NAME = "PoroElasticMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = PoroElasticMatFormat;

    [[nodiscard]] static base::EntityId cardId(const PoroElasticMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const PoroElasticMatCard& card, base::CardKey key);
    [[nodiscard]] PoroElasticMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const PoroElasticMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& solidMatIdColumn() noexcept { return m_solidMatId; }
    [[nodiscard]] Column<base::EntityId>& fluidMatIdColumn() noexcept { return m_fluidMatId; }
    [[nodiscard]] Column<double>& biotFactorColumn() noexcept { return m_biotFactor; }
    [[nodiscard]] Column<std::string>& simplifiedModelColumn() noexcept { return m_simplifiedModel; }
    [[nodiscard]] Column<double>& solidDensityColumn() noexcept { return m_solidDensity; }
    [[nodiscard]] Column<double>& viscosityColumn() noexcept { return m_viscosity; }
    [[nodiscard]] Column<double>& specificHeatRatioColumn() noexcept { return m_specificHeatRatio; }
    [[nodiscard]] Column<double>& prandtlNumberColumn() noexcept { return m_prandtlNumber; }
    [[nodiscard]] Column<double>& porosityColumn() noexcept { return m_porosity; }
    [[nodiscard]] Column<double>& tortuosityColumn() noexcept { return m_tortuosity; }
    [[nodiscard]] Column<double>& flowResistivityColumn() noexcept { return m_flowResistivity; }
    [[nodiscard]] Column<double>& viscousLengthColumn() noexcept { return m_viscousLength; }
    [[nodiscard]] Column<double>& thermalLengthColumn() noexcept { return m_thermalLength; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_solidMatId;
    Column<base::EntityId> m_fluidMatId;
    Column<double> m_biotFactor{1.0};
    Column<std::string> m_simplifiedModel;
    Column<double> m_solidDensity;
    Column<double> m_viscosity;
    Column<double> m_specificHeatRatio{1.402};
    Column<double> m_prandtlNumber{0.71};
    Column<double> m_porosity;
    Column<double> m_tortuosity{1.0};
    Column<double> m_flowResistivity;
    Column<double> m_viscousLength;
    Column<double> m_thermalLength;
};

} // namespace oor::model

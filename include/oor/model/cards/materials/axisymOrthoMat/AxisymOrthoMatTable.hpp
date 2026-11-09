#pragma once

#include "oor/model/cards/materials/axisymOrthoMat/AxisymOrthoMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AxisymOrthoMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AxisymOrthoMatCard& card);
    [[nodiscard]] static std::optional<AxisymOrthoMatCard> read(FieldReader& reader);
    static void write(const AxisymOrthoMatCard& card, FieldWriter& writer);
};

// All the MAT3 cards. The ID is a material ID, in the space every material type shares (IdSpace::Material).
class AxisymOrthoMatTable final : public TypedTable<AxisymOrthoMatTable, AxisymOrthoMatCard>
{
public:
    static constexpr std::string_view NAME = "AxisymOrthoMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = AxisymOrthoMatFormat;

    [[nodiscard]] static base::EntityId cardId(const AxisymOrthoMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const AxisymOrthoMatCard& card, base::CardKey key);
    [[nodiscard]] AxisymOrthoMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const AxisymOrthoMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a MAT3 refers to no card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& youngsModulusXColumn() noexcept { return m_youngsModulusX; }
    [[nodiscard]] Column<double>& youngsModulusThetaColumn() noexcept { return m_youngsModulusTheta; }
    [[nodiscard]] Column<double>& youngsModulusZColumn() noexcept { return m_youngsModulusZ; }
    [[nodiscard]] Column<double>& poissonRatioXthColumn() noexcept { return m_poissonRatioXth; }
    [[nodiscard]] Column<double>& poissonRatioThzColumn() noexcept { return m_poissonRatioThz; }
    [[nodiscard]] Column<double>& poissonRatioZxColumn() noexcept { return m_poissonRatioZx; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& shearModulusZxColumn() noexcept { return m_shearModulusZx; }
    [[nodiscard]] Column<double>& thermalExpansionXColumn() noexcept { return m_thermalExpansionX; }
    [[nodiscard]] Column<double>& thermalExpansionThetaColumn() noexcept { return m_thermalExpansionTheta; }
    [[nodiscard]] Column<double>& thermalExpansionZColumn() noexcept { return m_thermalExpansionZ; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }
    [[nodiscard]] Column<double>& structuralDampingColumn() noexcept { return m_structuralDamping; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_youngsModulusX;
    Column<double> m_youngsModulusTheta;
    Column<double> m_youngsModulusZ;
    Column<double> m_poissonRatioXth;
    Column<double> m_poissonRatioThz;
    Column<double> m_poissonRatioZx;
    Column<double> m_density;
    Column<double> m_shearModulusZx;
    Column<double> m_thermalExpansionX;
    Column<double> m_thermalExpansionTheta;
    Column<double> m_thermalExpansionZ;
    Column<double> m_referenceTemperature;
    Column<double> m_structuralDamping;
};

} // namespace oor::model

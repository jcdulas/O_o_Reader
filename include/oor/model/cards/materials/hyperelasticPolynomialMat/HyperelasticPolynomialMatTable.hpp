#pragma once

#include "oor/model/cards/materials/hyperelasticPolynomialMat/HyperelasticPolynomialMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct HyperelasticPolynomialMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HyperelasticPolynomialMatCard& card);
    [[nodiscard]] static std::optional<HyperelasticPolynomialMatCard> read(FieldReader& reader);
    static void write(const HyperelasticPolynomialMatCard& card, FieldWriter& writer);

    // The D1 of a card that leaves it blank (format reference: 10^3 (A10 + A01)).
    [[nodiscard]] static double defaultCoefficientD1(double coefficientA10, double coefficientA01) noexcept
    {
        return 1000.0 * (coefficientA10 + coefficientA01);
    }
};

// All the MATHP cards. The ID is a material ID, in the space every material type shares (IdSpace::Material). The
// experiment tables are references to TABLES1 cards (ReferenceTarget::StressTable).
class HyperelasticPolynomialMatTable final
    : public TypedTable<HyperelasticPolynomialMatTable, HyperelasticPolynomialMatCard>
{
public:
    static constexpr std::string_view NAME = "HyperelasticPolynomialMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = HyperelasticPolynomialMatFormat;

    HyperelasticPolynomialMatTable() : m_distortionalOrder(1), m_volumetricOrder(1) {}

    [[nodiscard]] static base::EntityId cardId(const HyperelasticPolynomialMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const HyperelasticPolynomialMatCard& card, base::CardKey key);
    [[nodiscard]] HyperelasticPolynomialMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HyperelasticPolynomialMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<double>& coefficientA10Column() noexcept { return m_coefficientA10; }
    [[nodiscard]] Column<double>& coefficientA01Column() noexcept { return m_coefficientA01; }
    [[nodiscard]] Column<double>& coefficientD1Column() noexcept { return m_coefficientD1; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& thermalExpansionColumn() noexcept { return m_thermalExpansion; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }
    [[nodiscard]] Column<double>& structuralDampingColumn() noexcept { return m_structuralDamping; }
    [[nodiscard]] Column<std::int32_t>& distortionalOrderColumn() noexcept { return m_distortionalOrder; }
    [[nodiscard]] Column<std::int32_t>& volumetricOrderColumn() noexcept { return m_volumetricOrder; }
    [[nodiscard]] Column<std::array<double, 18>>& coefficientsAColumn() noexcept { return m_coefficientsA; }
    [[nodiscard]] Column<std::array<double, 4>>& coefficientsDColumn() noexcept { return m_coefficientsD; }
    [[nodiscard]] Column<base::EntityId>& tensionTableIdColumn() noexcept { return m_tensionTableId; }
    [[nodiscard]] Column<base::EntityId>& equibiaxialTableIdColumn() noexcept { return m_equibiaxialTableId; }
    [[nodiscard]] Column<base::EntityId>& simpleShearTableIdColumn() noexcept { return m_simpleShearTableId; }
    [[nodiscard]] Column<base::EntityId>& pureShearTableIdColumn() noexcept { return m_pureShearTableId; }
    [[nodiscard]] Column<base::EntityId>& volumetricTableIdColumn() noexcept { return m_volumetricTableId; }

private:
    Column<base::EntityId> m_id;
    Column<double> m_coefficientA10;
    Column<double> m_coefficientA01;
    Column<double> m_coefficientD1;
    Column<double> m_density;
    Column<double> m_thermalExpansion;
    Column<double> m_referenceTemperature;
    Column<double> m_structuralDamping;
    Column<std::int32_t> m_distortionalOrder;
    Column<std::int32_t> m_volumetricOrder;
    Column<std::array<double, 18>> m_coefficientsA;
    Column<std::array<double, 4>> m_coefficientsD;
    Column<base::EntityId> m_tensionTableId;
    Column<base::EntityId> m_equibiaxialTableId;
    Column<base::EntityId> m_simpleShearTableId;
    Column<base::EntityId> m_pureShearTableId;
    Column<base::EntityId> m_volumetricTableId;
};

} // namespace oor::model

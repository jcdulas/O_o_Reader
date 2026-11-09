#pragma once

#include "oor/model/cards/materials/hyperelasticMat/HyperelasticMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct HyperelasticMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HyperelasticMatCard& card);
    [[nodiscard]] static std::optional<HyperelasticMatCard> read(FieldReader& reader);
    static void write(const HyperelasticMatCard& card, FieldWriter& writer);

    // The word of a model (MOONEY, OGDEN, ..., GHEM6) and the model of a word in any case (nothing for another word).
    [[nodiscard]] static const char* modelWord(HyperelasticMatModel model) noexcept;
    [[nodiscard]] static std::optional<HyperelasticMatModel> parseModel(std::string_view text) noexcept;
};

// All the MATHE cards. The ID is a material ID, in the space every material type shares (IdSpace::Material). The five
// experiment tables are references to TABLES1 cards (ReferenceTarget::StressTable). K is NaN for a card that has none.
class HyperelasticMatTable final : public TypedTable<HyperelasticMatTable, HyperelasticMatCard>
{
public:
    static constexpr std::string_view NAME = "HyperelasticMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::Material};
    using Format = HyperelasticMatFormat;

    HyperelasticMatTable();

    [[nodiscard]] static base::EntityId cardId(const HyperelasticMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const HyperelasticMatCard& card, base::CardKey key);
    [[nodiscard]] HyperelasticMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const HyperelasticMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& modelColumn() noexcept { return m_model; }
    [[nodiscard]] Column<double>& bulkModulusColumn() noexcept { return m_bulkModulus; }
    [[nodiscard]] Column<double>& densityColumn() noexcept { return m_density; }
    [[nodiscard]] Column<double>& thermalExpansionColumn() noexcept { return m_thermalExpansion; }
    [[nodiscard]] Column<double>& referenceTemperatureColumn() noexcept { return m_referenceTemperature; }
    [[nodiscard]] Column<double>& structuralDampingColumn() noexcept { return m_structuralDamping; }
    [[nodiscard]] Column<std::array<double, 20>>& coefficientsCColumn() noexcept { return m_coefficientsC; }
    [[nodiscard]] Column<base::EntityId>& tensionTableIdColumn() noexcept { return m_tensionTableId; }
    [[nodiscard]] Column<base::EntityId>& equibiaxialTableIdColumn() noexcept { return m_equibiaxialTableId; }
    [[nodiscard]] Column<base::EntityId>& simpleShearTableIdColumn() noexcept { return m_simpleShearTableId; }
    [[nodiscard]] Column<base::EntityId>& pureShearTableIdColumn() noexcept { return m_pureShearTableId; }
    [[nodiscard]] Column<base::EntityId>& volumetricTableIdColumn() noexcept { return m_volumetricTableId; }
    [[nodiscard]] Column<std::int32_t>& distortionalOrderColumn() noexcept { return m_distortionalOrder; }
    [[nodiscard]] Column<std::int32_t>& volumetricOrderColumn() noexcept { return m_volumetricOrder; }
    [[nodiscard]] Column<std::int32_t>& termCountColumn() noexcept { return m_termCount; }
    [[nodiscard]] Column<std::array<double, 5>>& ogdenModuliColumn() noexcept { return m_ogdenModuli; }
    [[nodiscard]] Column<std::array<double, 5>>& alphaColumn() noexcept { return m_alpha; }
    [[nodiscard]] Column<std::array<double, 5>>& betaColumn() noexcept { return m_beta; }
    [[nodiscard]] Column<double>& rubberModulusColumn() noexcept { return m_rubberModulus; }
    [[nodiscard]] Column<double>& exponentOrModulusColumn() noexcept { return m_exponentOrModulus; }
    [[nodiscard]] Column<double>& invariantLimitColumn() noexcept { return m_invariantLimit; }
    [[nodiscard]] Column<std::array<double, 5>>& coefficientsDColumn() noexcept { return m_coefficientsD; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_model;
    Column<double> m_bulkModulus;
    Column<double> m_density;
    Column<double> m_thermalExpansion;
    Column<double> m_referenceTemperature;
    Column<double> m_structuralDamping;
    Column<std::array<double, 20>> m_coefficientsC;
    Column<base::EntityId> m_tensionTableId;
    Column<base::EntityId> m_equibiaxialTableId;
    Column<base::EntityId> m_simpleShearTableId;
    Column<base::EntityId> m_pureShearTableId;
    Column<base::EntityId> m_volumetricTableId;
    Column<std::int32_t> m_distortionalOrder;
    Column<std::int32_t> m_volumetricOrder;
    Column<std::int32_t> m_termCount;
    Column<std::array<double, 5>> m_ogdenModuli;
    Column<std::array<double, 5>> m_alpha;
    Column<std::array<double, 5>> m_beta;
    Column<double> m_rubberModulus;
    Column<double> m_exponentOrModulus;
    Column<double> m_invariantLimit;
    Column<std::array<double, 5>> m_coefficientsD;
};

} // namespace oor::model

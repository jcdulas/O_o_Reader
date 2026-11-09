#pragma once

#include "oor/model/cards/materials/elastoPlasticMat/ElastoPlasticMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct ElastoPlasticMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ElastoPlasticMatCard& card);
    [[nodiscard]] static std::optional<ElastoPlasticMatCard> read(FieldReader& reader);
    static void write(const ElastoPlasticMatCard& card, FieldWriter& writer);

    // The words of the options and the option of a word in any case (nothing for another word).
    [[nodiscard]] static const char* formWord(ElastoPlasticMatForm form) noexcept;
    [[nodiscard]] static std::optional<ElastoPlasticMatForm> parseForm(std::string_view text) noexcept;
    [[nodiscard]] static const char* yieldWord(ElastoPlasticMatYield yield) noexcept;
    [[nodiscard]] static std::optional<ElastoPlasticMatYield> parseYield(std::string_view text) noexcept;
    [[nodiscard]] static const char* hardeningWord(ElastoPlasticMatHardening hardening) noexcept;
    [[nodiscard]] static std::optional<ElastoPlasticMatHardening> parseHardening(std::string_view text) noexcept;
    [[nodiscard]] static const char* rateWord(ElastoPlasticMatRateOption option) noexcept;
    [[nodiscard]] static std::optional<ElastoPlasticMatRateOption> parseRate(std::string_view text) noexcept;
    [[nodiscard]] static const char* pressureWord(ElastoPlasticMatPressureOption option) noexcept;
    [[nodiscard]] static std::optional<ElastoPlasticMatPressureOption> parsePressure(std::string_view text) noexcept;
};

// All the MATEP cards. The ID space is the card type's own: the ID is the one of the material that the card extends, so
// it is also a reference to a material (renumbering the materials follows into it). FID is a reference to a TABLES1 or
// TABLEST, or to a TABL3Di (the format allows a stress table or a
// multi-dimensional one; ReferenceTarget::StressOrMultiDimensionalTable), RTID to a TABLES1
// (ReferenceTarget::StressTable). `groups` says which groups a card has (bit 1 REFFECT, 2 ANISO, 4 PRESS, 8 CHABOCHE,
// 16 JHCOOK, 32 IMPCREEP); the columns of the other groups hold their defaults.
class ElastoPlasticMatTable final : public TypedTable<ElastoPlasticMatTable, ElastoPlasticMatCard>
{
public:
    static constexpr std::string_view NAME = "ElastoPlasticMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = ElastoPlasticMatFormat;

    ElastoPlasticMatTable();

    [[nodiscard]] static base::EntityId cardId(const ElastoPlasticMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const ElastoPlasticMatCard& card, base::CardKey key);
    [[nodiscard]] ElastoPlasticMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ElastoPlasticMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& formColumn() noexcept { return m_form; }
    [[nodiscard]] Column<double>& yieldStressColumn() noexcept { return m_yieldStress; }
    [[nodiscard]] Column<base::EntityId>& stressStrainTableIdColumn() noexcept { return m_stressStrainTableId; }
    [[nodiscard]] Column<std::uint8_t>& yieldCriterionColumn() noexcept { return m_yieldCriterion; }
    [[nodiscard]] Column<std::uint8_t>& workHardeningColumn() noexcept { return m_workHardening; }
    [[nodiscard]] Column<double>& hardeningSlopeColumn() noexcept { return m_hardeningSlope; }
    [[nodiscard]] Column<std::uint8_t>& groupsColumn() noexcept { return m_groups; }
    [[nodiscard]] Column<std::uint8_t>& rateOptionColumn() noexcept { return m_rateOption; }
    [[nodiscard]] Column<base::EntityId>& strainRateTableIdColumn() noexcept { return m_strainRateTableId; }
    [[nodiscard]] Column<double>& cowperCoefficientColumn() noexcept { return m_cowperCoefficient; }
    [[nodiscard]] Column<double>& cowperExponentColumn() noexcept { return m_cowperExponent; }
    [[nodiscard]] Column<std::array<double, 6>>& anisotropyColumn() noexcept { return m_anisotropy; }
    [[nodiscard]] Column<std::uint8_t>& pressureOptionColumn() noexcept { return m_pressureOption; }
    [[nodiscard]] Column<double>& mohrCoulombAlphaColumn() noexcept { return m_mohrCoulombAlpha; }
    [[nodiscard]] Column<double>& mohrCoulombBetaColumn() noexcept { return m_mohrCoulombBeta; }
    [[nodiscard]] Column<std::array<double, 10>>& chabocheHardeningColumn() noexcept { return m_chabocheHardening; }
    [[nodiscard]] Column<std::array<double, 8>>& johnsonCookHardeningColumn() noexcept
    {
        return m_johnsonCookHardening;
    }
    [[nodiscard]] Column<double>& creepEquivalentStressColumn() noexcept { return m_creepEquivalentStress; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_form;
    Column<double> m_yieldStress;
    Column<base::EntityId> m_stressStrainTableId;
    Column<std::uint8_t> m_yieldCriterion;
    Column<std::uint8_t> m_workHardening;
    Column<double> m_hardeningSlope;
    Column<std::uint8_t> m_groups;
    Column<std::uint8_t> m_rateOption;
    Column<base::EntityId> m_strainRateTableId;
    Column<double> m_cowperCoefficient;
    Column<double> m_cowperExponent;
    Column<std::array<double, 6>> m_anisotropy;
    Column<std::uint8_t> m_pressureOption;
    Column<double> m_mohrCoulombAlpha;
    Column<double> m_mohrCoulombBeta;
    Column<std::array<double, 10>> m_chabocheHardening;
    Column<std::array<double, 8>> m_johnsonCookHardening;
    Column<double> m_creepEquivalentStress;
};

} // namespace oor::model

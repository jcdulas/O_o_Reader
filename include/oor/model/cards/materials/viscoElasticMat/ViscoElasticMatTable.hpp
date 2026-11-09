#pragma once

#include "oor/model/cards/materials/viscoElasticMat/ViscoElasticMatCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"
#include "oor/model/storage/VariableLengthColumn.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace oor::model
{

struct ViscoElasticMatFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ViscoElasticMatCard& card);
    [[nodiscard]] static std::optional<ViscoElasticMatCard> read(FieldReader& reader);
    static void write(const ViscoElasticMatCard& card, FieldWriter& writer);

    // The word of a model (ISO, ISO1, ..., ORTHO) and the model of a word in any case (LINEAR is ISO1; nothing for
    // another word).
    [[nodiscard]] static const char* modelWord(ViscoElasticMatModel model) noexcept;
    [[nodiscard]] static std::optional<ViscoElasticMatModel> parseModel(std::string_view text) noexcept;
    // True for the models that have the fixed layout of at most five terms (ISO, MOONEY, OGDEN, FOAM).
    [[nodiscard]] static bool isFixedLayout(ViscoElasticMatModel model) noexcept;
};

// All the MATVE cards. The ID space is the card type's own: the ID is the one of the material that the card extends, so
// it is also a reference to a material (renumbering the materials follows into it). The terms of every card are one
// list in CSR form with four values to a term (G or WD, TD, K or WV, TV), the terms of ORTHO another with ten.
class ViscoElasticMatTable final : public TypedTable<ViscoElasticMatTable, ViscoElasticMatCard>
{
public:
    static constexpr std::string_view NAME = "ViscoElasticMat";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Material, base::IdSpace::PerCardType};
    using Format = ViscoElasticMatFormat;

    [[nodiscard]] static base::EntityId cardId(const ViscoElasticMatCard& card) noexcept { return card.id; }
    std::uint32_t append(const ViscoElasticMatCard& card, base::CardKey key);
    [[nodiscard]] ViscoElasticMatCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const ViscoElasticMatCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::uint8_t>& modelColumn() noexcept { return m_model; }
    [[nodiscard]] Column<double>& solidThermalExpansionColumn() noexcept { return m_solidThermalExpansion; }
    [[nodiscard]] Column<double>& liquidThermalExpansionColumn() noexcept { return m_liquidThermalExpansion; }
    [[nodiscard]] VariableLengthColumn<double>& termsColumn() noexcept { return m_terms; }
    [[nodiscard]] VariableLengthColumn<double>& orthoTermsColumn() noexcept { return m_orthoTerms; }

private:
    Column<base::EntityId> m_id;
    Column<std::uint8_t> m_model;
    Column<double> m_solidThermalExpansion;
    Column<double> m_liquidThermalExpansion;
    VariableLengthColumn<double> m_terms;      // 4 values to a term
    VariableLengthColumn<double> m_orthoTerms; // 10 values to a term
};

} // namespace oor::model

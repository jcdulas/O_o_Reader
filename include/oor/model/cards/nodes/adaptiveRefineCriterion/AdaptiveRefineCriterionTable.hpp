#pragma once

#include "oor/model/cards/nodes/adaptiveRefineCriterion/AdaptiveRefineCriterionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AdaptiveRefineCriterionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AdaptiveRefineCriterionCard& card);
    [[nodiscard]] static std::optional<AdaptiveRefineCriterionCard> read(FieldReader& reader);
    static void write(const AdaptiveRefineCriterionCard& card, FieldWriter& writer);
};

// All the HADACRI cards, in the category AdaptivityCriterion: the CRITID is unique per card type
// (IdSpace::PerCardType) and a HADAPTL names it. A HADACRI names no other card. The cards are kept whole next to a
// column of their CRITIDs.
class AdaptiveRefineCriterionTable final : public TypedTable<AdaptiveRefineCriterionTable, AdaptiveRefineCriterionCard>
{
public:
    static constexpr std::string_view NAME = "AdaptiveRefineCriterion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AdaptivityCriterion, base::IdSpace::PerCardType};
    using Format = AdaptiveRefineCriterionFormat;

    [[nodiscard]] static base::EntityId cardId(const AdaptiveRefineCriterionCard& card) noexcept { return card.id; }
    std::uint32_t append(const AdaptiveRefineCriterionCard& card, base::CardKey key);
    [[nodiscard]] AdaptiveRefineCriterionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AdaptiveRefineCriterionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a HADACRI refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AdaptiveRefineCriterionCard> m_cards;
};

} // namespace oor::model

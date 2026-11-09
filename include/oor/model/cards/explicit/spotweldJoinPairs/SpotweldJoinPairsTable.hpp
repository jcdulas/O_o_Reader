#pragma once

#include "oor/model/cards/explicit/spotweldJoinPairs/SpotweldJoinPairsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SpotweldJoinPairsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SpotweldJoinPairsCard& card);
    [[nodiscard]] static std::optional<SpotweldJoinPairsCard> read(FieldReader& reader);
    static void write(const SpotweldJoinPairsCard& card, FieldWriter& writer);
};

// All the BJOIN cards. The BID is unique among them (IdSpace::PerCardType) and no other card names it, so the category
// is Other. The SID is a BCGRID, a card of the contact body category: the reference is visited with that target
// (ReferenceTarget::ContactBody), so that renumbering the BCGRID cards follows into the join. The cards are kept whole
// (CardRows) next to their ID.
class SpotweldJoinPairsTable final : public TypedTable<SpotweldJoinPairsTable, SpotweldJoinPairsCard>
{
public:
    static constexpr std::string_view NAME = "SpotweldJoinPairs";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = SpotweldJoinPairsFormat;

    [[nodiscard]] static base::EntityId cardId(const SpotweldJoinPairsCard& card) noexcept { return card.id; }
    std::uint32_t append(const SpotweldJoinPairsCard& card, base::CardKey key);
    [[nodiscard]] SpotweldJoinPairsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SpotweldJoinPairsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<SpotweldJoinPairsCard> m_cards;
};

} // namespace oor::model

#pragma once

#include "oor/model/cards/nodes/adaptiveRefineControl/AdaptiveRefineControlCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AdaptiveRefineControlFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AdaptiveRefineControlCard& card);
    [[nodiscard]] static std::optional<AdaptiveRefineControlCard> read(FieldReader& reader);
    static void write(const AdaptiveRefineControlCard& card, FieldWriter& writer);
};

// All the HADAPTL cards, in the category AdaptivitySet. The ID is shared by the cards of a set (IdSpace::Group: several
// cards may have one ID), so `model.hadaptl[1]` is the list of the cards of the set 1.
// CRITID is a reference to a HADACRI; WHEREID is a reference to a property when WHEREMET is PROP, to a superelement
// when it is SUPER and nothing when it is ALL (the format reference says the field is ignored then). The cards are kept
// whole next to a column of their IDs.
class AdaptiveRefineControlTable final : public TypedTable<AdaptiveRefineControlTable, AdaptiveRefineControlCard>
{
public:
    static constexpr std::string_view NAME = "AdaptiveRefineControl";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AdaptivitySet, base::IdSpace::Group};
    using Format = AdaptiveRefineControlFormat;

    [[nodiscard]] static base::EntityId cardId(const AdaptiveRefineControlCard& card) noexcept { return card.id; }
    std::uint32_t append(const AdaptiveRefineControlCard& card, base::CardKey key);
    [[nodiscard]] AdaptiveRefineControlCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AdaptiveRefineControlCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AdaptiveRefineControlCard> m_cards;
};

} // namespace oor::model

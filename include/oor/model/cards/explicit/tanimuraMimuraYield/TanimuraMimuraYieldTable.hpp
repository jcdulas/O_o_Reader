#pragma once

#include "oor/model/cards/explicit/tanimuraMimuraYield/TanimuraMimuraYieldCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TanimuraMimuraYieldFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TanimuraMimuraYieldCard& card);
    [[nodiscard]] static std::optional<TanimuraMimuraYieldCard> read(FieldReader& reader);
    static void write(const TanimuraMimuraYieldCard& card, FieldWriter& writer);
};

// All the YLDTM cards. The ID is the number of a yield model, unique among the cards of its category
// (IdSpace::PerCardType: the other YLDxxx cards share the category YieldModel, and a check reports an ID two of them
// have). The cards are kept whole (CardRows) next to their ID: they are few, and the query engine reads the ID.
class TanimuraMimuraYieldTable final : public TypedTable<TanimuraMimuraYieldTable, TanimuraMimuraYieldCard>
{
public:
    static constexpr std::string_view NAME = "TanimuraMimuraYield";
    static inline const CardTypeInfo INFO{NAME, CardCategory::YieldModel, base::IdSpace::PerCardType};
    using Format = TanimuraMimuraYieldFormat;

    [[nodiscard]] static base::EntityId cardId(const TanimuraMimuraYieldCard& card) noexcept { return card.id; }
    std::uint32_t append(const TanimuraMimuraYieldCard& card, base::CardKey key);
    [[nodiscard]] TanimuraMimuraYieldCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TanimuraMimuraYieldCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a YLDTM refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TanimuraMimuraYieldCard> m_cards;
};

} // namespace oor::model

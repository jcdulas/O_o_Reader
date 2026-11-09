#pragma once

#include "oor/model/cards/explicit/vonMisesYield/VonMisesYieldCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct VonMisesYieldFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const VonMisesYieldCard& card);
    [[nodiscard]] static std::optional<VonMisesYieldCard> read(FieldReader& reader);
    static void write(const VonMisesYieldCard& card, FieldWriter& writer);
};

// All the YLDVM cards. The ID is the number of a yield model, unique among the cards of its category
// (IdSpace::PerCardType: the other YLDxxx cards share the category YieldModel, and a check reports an ID two of them
// have). TABLE and TABY are TABLED1 cards (ReferenceTarget::DynamicTable). The cards are kept whole (CardRows) next to
// their ID: they are few, and the query engine reads the ID.
class VonMisesYieldTable final : public TypedTable<VonMisesYieldTable, VonMisesYieldCard>
{
public:
    static constexpr std::string_view NAME = "VonMisesYield";
    static inline const CardTypeInfo INFO{NAME, CardCategory::YieldModel, base::IdSpace::PerCardType};
    using Format = VonMisesYieldFormat;

    [[nodiscard]] static base::EntityId cardId(const VonMisesYieldCard& card) noexcept { return card.id; }
    std::uint32_t append(const VonMisesYieldCard& card, base::CardKey key);
    [[nodiscard]] VonMisesYieldCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const VonMisesYieldCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<VonMisesYieldCard> m_cards;
};

} // namespace oor::model

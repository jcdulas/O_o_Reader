#pragma once

#include "oor/model/cards/dynamics/hybridDamping/HybridDampingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HybridDampingFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HybridDampingCard& card);
    [[nodiscard]] static std::optional<HybridDampingCard> read(FieldReader& reader);
    static void write(const HybridDampingCard& card, FieldWriter& writer);
};

// All the HYBDAMP cards, in the category HybridDamping: the ID is unique per card type (IdSpace::PerCardType) and is
// what the HYBRID of a DAMPING and the HYBDAMPi of a ROTHYBD name. METHOD is the eigenvalue method of the modes and
// SDAMP the TABDMP1 of the modal damping. The cards are kept whole next to a column of their IDs.
class HybridDampingTable final : public TypedTable<HybridDampingTable, HybridDampingCard>
{
public:
    static constexpr std::string_view NAME = "HybridDamping";
    static inline const CardTypeInfo INFO{NAME, CardCategory::HybridDamping, base::IdSpace::PerCardType};
    using Format = HybridDampingFormat;

    [[nodiscard]] static base::EntityId cardId(const HybridDampingCard& card) noexcept { return card.id; }
    std::uint32_t append(const HybridDampingCard& card, base::CardKey key);
    [[nodiscard]] HybridDampingCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const HybridDampingCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<HybridDampingCard> m_cards;
};

} // namespace oor::model

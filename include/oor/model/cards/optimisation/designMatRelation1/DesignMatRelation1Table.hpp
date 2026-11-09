#pragma once

#include "oor/model/cards/optimisation/designMatRelation1/DesignMatRelation1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignMatRelation1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignMatRelation1Card& card);
    [[nodiscard]] static std::optional<DesignMatRelation1Card> read(FieldReader& reader);
    static void write(const DesignMatRelation1Card& card, FieldWriter& writer);
};

// All the DVMREL1 cards. The ID is unique among the DVMREL1 cards (IdSpace::PerCardType); the category MaterialRelation
// is shared with the DVMREL2 cards, so that a renumbering and the DRESP2 lists treat the two kinds as one set of
// numbers. The MID is a material of the type TYPE names, and the DVIDs are design variables.
class DesignMatRelation1Table final : public TypedTable<DesignMatRelation1Table, DesignMatRelation1Card>
{
public:
    static constexpr std::string_view NAME = "DesignMatRelation1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::MaterialRelation, base::IdSpace::PerCardType};
    using Format = DesignMatRelation1Format;

    [[nodiscard]] static base::EntityId cardId(const DesignMatRelation1Card& card) noexcept { return card.id; }
    std::uint32_t append(const DesignMatRelation1Card& card, base::CardKey key);
    [[nodiscard]] DesignMatRelation1Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignMatRelation1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DesignMatRelation1Card> m_cards;
};

} // namespace oor::model

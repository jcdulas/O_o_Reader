#pragma once

#include "oor/model/cards/optimisation/designElemRelation1/DesignElemRelation1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignElemRelation1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignElemRelation1Card& card);
    [[nodiscard]] static std::optional<DesignElemRelation1Card> read(FieldReader& reader);
    static void write(const DesignElemRelation1Card& card, FieldWriter& writer);
};

// All the DVCREL1 cards. The ID is unique among the DVCREL1 cards (IdSpace::PerCardType); the category
// ConnectivityRelation is shared with the DVCREL2 cards, so that a renumbering and the DRESP2 lists treat the two kinds
// as one set of numbers. The EID is an element of the type TYPE names, and the DVIDs are design variables.
class DesignElemRelation1Table final : public TypedTable<DesignElemRelation1Table, DesignElemRelation1Card>
{
public:
    static constexpr std::string_view NAME = "DesignElemRelation1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ConnectivityRelation, base::IdSpace::PerCardType};
    using Format = DesignElemRelation1Format;

    [[nodiscard]] static base::EntityId cardId(const DesignElemRelation1Card& card) noexcept { return card.id; }
    std::uint32_t append(const DesignElemRelation1Card& card, base::CardKey key);
    [[nodiscard]] DesignElemRelation1Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignElemRelation1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DesignElemRelation1Card> m_cards;
};

} // namespace oor::model

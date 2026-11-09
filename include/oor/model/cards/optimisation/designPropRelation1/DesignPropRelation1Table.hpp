#pragma once

#include "oor/model/cards/optimisation/designPropRelation1/DesignPropRelation1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignPropRelation1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignPropRelation1Card& card);
    [[nodiscard]] static std::optional<DesignPropRelation1Card> read(FieldReader& reader);
    static void write(const DesignPropRelation1Card& card, FieldWriter& writer);
};

// All the DVPREL1 cards. The ID is unique among the DVPREL1 cards (IdSpace::PerCardType); the category PropertyRelation
// is shared with the DVPREL2 cards, so that a renumbering and the DPIPi of a DRESP2 treat the two kinds as one set of
// numbers. The PID is a property of the type TYPE names, and the DVIDs are design variables.
class DesignPropRelation1Table final : public TypedTable<DesignPropRelation1Table, DesignPropRelation1Card>
{
public:
    static constexpr std::string_view NAME = "DesignPropRelation1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::PropertyRelation, base::IdSpace::PerCardType};
    using Format = DesignPropRelation1Format;

    [[nodiscard]] static base::EntityId cardId(const DesignPropRelation1Card& card) noexcept { return card.id; }
    std::uint32_t append(const DesignPropRelation1Card& card, base::CardKey key);
    [[nodiscard]] DesignPropRelation1Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignPropRelation1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<DesignPropRelation1Card> m_cards;
};

} // namespace oor::model

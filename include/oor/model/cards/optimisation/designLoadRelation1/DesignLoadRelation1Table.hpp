#pragma once

#include "oor/model/cards/optimisation/designLoadRelation1/DesignLoadRelation1Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct DesignLoadRelation1Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const DesignLoadRelation1Card& card);
    [[nodiscard]] static std::optional<DesignLoadRelation1Card> read(FieldReader& reader);
    static void write(const DesignLoadRelation1Card& card, FieldWriter& writer);
};

// All the DVLREL1 cards. The ID is unique among them (IdSpace::PerCardType, category Other: no card names a DVLREL1).
// The SID is a set of loads (LoadCombinationSet) and the DVIDs are design variables (DesignVariable). What ATT1 and
// ATT2 name depends on the TYPE: of a FORCE or a MOMENT a grid point (Node) and a coordinate system (Coordinate), of a
// LOAD (with LNAME other than S) a set of loads (LoadCombinationSet). The cards are kept whole next to the columns the
// query engine reads: the ID and the set of loads.
class DesignLoadRelation1Table final : public TypedTable<DesignLoadRelation1Table, DesignLoadRelation1Card>
{
public:
    static constexpr std::string_view NAME = "DesignLoadRelation1";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = DesignLoadRelation1Format;

    [[nodiscard]] static base::EntityId cardId(const DesignLoadRelation1Card& card) noexcept { return card.id; }
    std::uint32_t append(const DesignLoadRelation1Card& card, base::CardKey key);
    [[nodiscard]] DesignLoadRelation1Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const DesignLoadRelation1Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& setIdColumn() noexcept { return m_setId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_setId;
    CardRows<DesignLoadRelation1Card> m_cards;
};

} // namespace oor::model

#pragma once

#include "oor/model/cards/rigid_elements/rigidBodySearched/RigidBodySearchedCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RigidBodySearchedFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RigidBodySearchedCard& card);
    [[nodiscard]] static std::optional<RigidBodySearchedCard> read(FieldReader& reader);
    static void write(const RigidBodySearchedCard& card, FieldWriter& writer);
};

// All the RBE2GS cards. A rigid element is an element: the ID is in the space shared by all the elements. The cards are
// kept whole (CardRows) next to the columns the query engine reads, the EID and the grid or point GS. GS names a node
// (ReferenceTarget::Node, a GRID or a POINT; blank when the search location is a coordinate triple). The grids of the
// exclusion lists are nodes too: a single ID is a Node, the two ends of a THRU range are NodeInRange (the format
// reference says that not every grid of a range needs to exist).
class RigidBodySearchedTable final : public TypedTable<RigidBodySearchedTable, RigidBodySearchedCard>
{
public:
    static constexpr std::string_view NAME = "RigidBodySearched";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = RigidBodySearchedFormat;

    [[nodiscard]] static base::EntityId cardId(const RigidBodySearchedCard& card) noexcept { return card.id; }
    std::uint32_t append(const RigidBodySearchedCard& card, base::CardKey key);
    [[nodiscard]] RigidBodySearchedCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RigidBodySearchedCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    // The search grid or point of a card without copying the card (the geometry reads it for every element).
    [[nodiscard]] base::EntityId gsAt(std::uint32_t row) const { return m_spotNode.get(row); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& spotNodeColumn() noexcept { return m_spotNode; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_spotNode;
    CardRows<RigidBodySearchedCard> m_cards;
};

} // namespace oor::model

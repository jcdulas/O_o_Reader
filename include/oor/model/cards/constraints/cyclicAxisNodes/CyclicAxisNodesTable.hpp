#pragma once

#include "oor/model/cards/constraints/cyclicAxisNodes/CyclicAxisNodesCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CyclicAxisNodesFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CyclicAxisNodesCard& card);
    [[nodiscard]] static std::optional<CyclicAxisNodesCard> read(FieldReader& reader);
    static void write(const CyclicAxisNodesCard& card, FieldWriter& writer);
};

// All the CYAX cards (category Other). A card has no ID of its own (IdSpace::None). The grid points are references: a
// single point is a ReferenceTarget::Node, the two ends of a THRU range are ReferenceTarget::NodeInRange (a point of
// the range may not exist; the ends are renumbered with the nodes). The cards are kept whole next to a column of the
// number of their items, which is what the query engine reads.
class CyclicAxisNodesTable final : public TypedTable<CyclicAxisNodesTable, CyclicAxisNodesCard>
{
public:
    static constexpr std::string_view NAME = "CyclicAxisNodes";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = CyclicAxisNodesFormat;

    [[nodiscard]] static base::EntityId cardId(const CyclicAxisNodesCard&) noexcept { return 0; }
    std::uint32_t append(const CyclicAxisNodesCard& card, base::CardKey key);
    [[nodiscard]] CyclicAxisNodesCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CyclicAxisNodesCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::int32_t>& itemCountColumn() noexcept { return m_itemCount; }

private:
    Column<std::int32_t> m_itemCount;
    CardRows<CyclicAxisNodesCard> m_cards;
};

} // namespace oor::model

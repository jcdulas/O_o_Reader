#pragma once

#include "oor/model/cards/constraints/cyclicBoundaryJoin/CyclicBoundaryJoinCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct CyclicBoundaryJoinFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const CyclicBoundaryJoinCard& card);
    [[nodiscard]] static std::optional<CyclicBoundaryJoinCard> read(FieldReader& reader);
    static void write(const CyclicBoundaryJoinCard& card, FieldWriter& writer);
};

// All the CYJOIN cards (category Other). A card has no ID of its own (IdSpace::None): the SIDE is 1 or 2, not an ID.
// The points are references to the nodes (a single point is a ReferenceTarget::Node, which a scalar point is too; the
// two ends of a THRU range are ReferenceTarget::NodeInRange). The cards are kept whole next to a column of their sides.
class CyclicBoundaryJoinTable final : public TypedTable<CyclicBoundaryJoinTable, CyclicBoundaryJoinCard>
{
public:
    static constexpr std::string_view NAME = "CyclicBoundaryJoin";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = CyclicBoundaryJoinFormat;

    [[nodiscard]] static base::EntityId cardId(const CyclicBoundaryJoinCard&) noexcept { return 0; }
    std::uint32_t append(const CyclicBoundaryJoinCard& card, base::CardKey key);
    [[nodiscard]] CyclicBoundaryJoinCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const CyclicBoundaryJoinCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& sideCodeColumn() noexcept { return m_sideCode; }

private:
    Column<base::EntityId> m_sideCode;
    CardRows<CyclicBoundaryJoinCard> m_cards;
};

} // namespace oor::model

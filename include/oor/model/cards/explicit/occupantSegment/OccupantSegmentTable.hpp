#pragma once

#include "oor/model/cards/explicit/occupantSegment/OccupantSegmentCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct OccupantSegmentFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const OccupantSegmentCard& card);
    [[nodiscard]] static std::optional<OccupantSegmentCard> read(FieldReader& reader);
    static void write(const OccupantSegmentCard& card, FieldWriter& writer);
};

// All the ATBSEG cards. The ID is unique among them (IdSpace::PerCardType, category Other: no other card names it). The
// grid points G0 to G3 are references to GRID cards; the other numbers are given to cards that SOL 700 generates and
// are not references. The cards are kept whole next to their ID.
class OccupantSegmentTable final : public TypedTable<OccupantSegmentTable, OccupantSegmentCard>
{
public:
    static constexpr std::string_view NAME = "OccupantSegment";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = OccupantSegmentFormat;

    [[nodiscard]] static base::EntityId cardId(const OccupantSegmentCard& card) noexcept { return card.id; }
    std::uint32_t append(const OccupantSegmentCard& card, base::CardKey key);
    [[nodiscard]] OccupantSegmentCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const OccupantSegmentCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<OccupantSegmentCard> m_cards;
};

} // namespace oor::model

#pragma once

#include "oor/model/cards/thermal/acousticSlotBoundary/AcousticSlotBoundaryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AcousticSlotBoundaryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AcousticSlotBoundaryCard& card);
    [[nodiscard]] static std::optional<AcousticSlotBoundaryCard> read(FieldReader& reader);
    static void write(const AcousticSlotBoundaryCard& card, FieldWriter& writer);
};

// All the SLBDY cards, category Other, no ID (IdSpace::None: a deck has one for each boundary and none is named). They
// are kept whole next to a column of their number of points. The points are GRIDS cards, points of the node ID space.
class AcousticSlotBoundaryTable final : public TypedTable<AcousticSlotBoundaryTable, AcousticSlotBoundaryCard>
{
public:
    static constexpr std::string_view NAME = "AcousticSlotBoundary";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = AcousticSlotBoundaryFormat;

    [[nodiscard]] static base::EntityId cardId(const AcousticSlotBoundaryCard&) noexcept { return 0; }
    std::uint32_t append(const AcousticSlotBoundaryCard& card, base::CardKey key);
    [[nodiscard]] AcousticSlotBoundaryCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AcousticSlotBoundaryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::int32_t>& pointCountColumn() noexcept { return m_pointCount; }

private:
    Column<std::int32_t> m_pointCount;
    CardRows<AcousticSlotBoundaryCard> m_cards;
};

} // namespace oor::model

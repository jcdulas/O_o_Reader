#pragma once

#include "oor/model/cards/superelements/superelementExternalBoundary/SuperelementExternalBoundaryCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementExternalBoundaryFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementExternalBoundaryCard& card);
    [[nodiscard]] static std::optional<SuperelementExternalBoundaryCard> read(FieldReader& reader);
    static void write(const SuperelementExternalBoundaryCard& card, FieldWriter& writer);
};

// All the EXTRN cards, category Superelement, no ID of their own (IdSpace::None). They are kept whole next to a column
// of the number of connection points each has.
class SuperelementExternalBoundaryTable final
    : public TypedTable<SuperelementExternalBoundaryTable, SuperelementExternalBoundaryCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementExternalBoundary";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Superelement, base::IdSpace::None};
    using Format = SuperelementExternalBoundaryFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementExternalBoundaryCard&) noexcept { return 0; }
    std::uint32_t append(const SuperelementExternalBoundaryCard& card, base::CardKey key);
    [[nodiscard]] SuperelementExternalBoundaryCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementExternalBoundaryCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    // The points are grid or scalar points of the part the card is in; the card defines them (they resolve through
    // `connectionPoints`, not through a GRID card).
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    // The points of every live card, as runs in ascending order that do not overlap (the lookup of the reference
    // checker).
    [[nodiscard]] std::vector<IdRun> connectionPoints() const;

    [[nodiscard]] Column<std::int32_t>& countColumn() noexcept { return m_count; }

private:
    Column<std::int32_t> m_count;
    CardRows<SuperelementExternalBoundaryCard> m_cards;
};

} // namespace oor::model

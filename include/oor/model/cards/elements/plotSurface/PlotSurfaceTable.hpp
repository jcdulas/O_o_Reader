#pragma once

#include "oor/model/cards/elements/plotSurface/PlotSurfaceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct PlotSurfaceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PlotSurfaceCard& card);
    [[nodiscard]] static std::optional<PlotSurfaceCard> read(FieldReader& reader);
    static void write(const PlotSurfaceCard& card, FieldWriter& writer);
};

// All the PLTSURF elements. Like the PLOTEL, a PLTSURF is only drawn: decks reuse the ID of a structural element for
// such an element, so the cards have an ID space of their own (unique among themselves), a structural element is never
// replaced by one, and the ElementTopology does not see them. The grid points are one column of eight IDs per element
// (0 after the last point); each is a node.
class PlotSurfaceTable final : public TypedTable<PlotSurfaceTable, PlotSurfaceCard>
{
public:
    static constexpr std::string_view NAME = "PlotSurface";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::PerCardType};
    using Format = PlotSurfaceFormat;

    [[nodiscard]] static base::EntityId cardId(const PlotSurfaceCard& card) noexcept { return card.id; }
    std::uint32_t append(const PlotSurfaceCard& card, base::CardKey key);
    [[nodiscard]] PlotSurfaceCard card(std::uint32_t row) const;
    void store(std::uint32_t row, const PlotSurfaceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<std::array<base::EntityId, PlotSurfaceCard::MAX_GRIDS>>& gridsColumn() noexcept
    {
        return m_grids;
    }

private:
    Column<base::EntityId> m_id;
    Column<std::array<base::EntityId, PlotSurfaceCard::MAX_GRIDS>> m_grids;
};

} // namespace oor::model

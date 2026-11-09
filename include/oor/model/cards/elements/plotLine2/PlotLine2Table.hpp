#pragma once

#include "oor/model/cards/elements/plotLine2/PlotLine2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PlotLine2Format
{
    // The first thing wrong with the values of a card, if anything (used by readAll and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PlotLine2Card& card);
    // One element, or two when the second group of three fields (5 to 7) is there.
    [[nodiscard]] static std::vector<PlotLine2Card> readAll(FieldReader& reader);
    static void write(const PlotLine2Card& card, FieldWriter& writer);
};

// All the PLOTEL elements. A PLOTEL is only drawn on plots, never used in a solution phase: the format reference asks
// for an ID that is unique among the elements, but decks that the solver ran reuse the ID of a structural element for a
// PLOTEL (the public elements/geom.inc has a PLOTEL 13 and a CBAR 13), so the PLOTELs have an ID space of their own and
// a structural element is never replaced by one.
class PlotLine2Table final : public TypedTable<PlotLine2Table, PlotLine2Card>
{
public:
    static constexpr std::string_view NAME = "PlotLine2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::PerCardType};
    using Format = PlotLine2Format;

    [[nodiscard]] static base::EntityId cardId(const PlotLine2Card& card) noexcept { return card.id; }
    std::uint32_t append(const PlotLine2Card& card, base::CardKey key);
    [[nodiscard]] PlotLine2Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const PlotLine2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& node1Column() noexcept { return m_node1; }
    [[nodiscard]] Column<base::EntityId>& node2Column() noexcept { return m_node2; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_node1;
    Column<base::EntityId> m_node2;
};

} // namespace oor::model

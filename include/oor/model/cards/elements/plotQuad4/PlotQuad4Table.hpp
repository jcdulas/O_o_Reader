#pragma once

#include "oor/model/cards/elements/plotQuad4/PlotQuad4Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct PlotQuad4Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const PlotQuad4Card& card);
    [[nodiscard]] static std::optional<PlotQuad4Card> read(FieldReader& reader);
    static void write(const PlotQuad4Card& card, FieldWriter& writer);
};

// All the PLOTEL4 elements. Like the PLOTEL, a PLOTEL4 is only drawn on plots and never used in a solution phase, and
// decks that the solver ran reuse the ID of a structural element for such a plot element: the PLOTEL4 cards have an ID
// space of their own (unique among themselves), and a structural element is never replaced by one. The ElementTopology
// does not see them as elements for the same reason.
class PlotQuad4Table final : public TypedTable<PlotQuad4Table, PlotQuad4Card>
{
public:
    static constexpr std::string_view NAME = "PlotQuad4";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::PerCardType};
    using Format = PlotQuad4Format;

    [[nodiscard]] static base::EntityId cardId(const PlotQuad4Card& card) noexcept { return card.id; }
    std::uint32_t append(const PlotQuad4Card& card, base::CardKey key);
    [[nodiscard]] PlotQuad4Card card(std::uint32_t row) const;
    void store(std::uint32_t row, const PlotQuad4Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id) { m_id.set(row, id); }
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& node1Column() noexcept { return m_node1; }
    [[nodiscard]] Column<base::EntityId>& node2Column() noexcept { return m_node2; }
    [[nodiscard]] Column<base::EntityId>& node3Column() noexcept { return m_node3; }
    [[nodiscard]] Column<base::EntityId>& node4Column() noexcept { return m_node4; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_node1;
    Column<base::EntityId> m_node2;
    Column<base::EntityId> m_node3;
    Column<base::EntityId> m_node4;
};

} // namespace oor::model

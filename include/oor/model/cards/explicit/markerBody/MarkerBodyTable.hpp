#pragma once

#include "oor/model/cards/explicit/markerBody/MarkerBodyCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct MarkerBodyFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MarkerBodyCard& card);
    [[nodiscard]] static std::optional<MarkerBodyCard> read(FieldReader& reader);
    static void write(const MarkerBodyCard& card, FieldWriter& writer);
};

// All the CMARKB2 cards. A marker element is an element (IdSpace::Element, CardCategory::Element): its ID is unique
// among all the elements. Its PID names a PMARKER and its grid points are nodes.
class MarkerBodyTable final : public TypedTable<MarkerBodyTable, MarkerBodyCard>
{
public:
    static constexpr std::string_view NAME = "MarkerBody";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = MarkerBodyFormat;

    [[nodiscard]] static base::EntityId cardId(const MarkerBodyCard& card) noexcept { return card.id; }
    std::uint32_t append(const MarkerBodyCard& card, base::CardKey key);
    [[nodiscard]] MarkerBodyCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const MarkerBodyCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    // What the geometry of the element needs, without copying the whole card.
    [[nodiscard]] base::EntityId propId(std::uint32_t row) const { return m_cards.get(row).propId; }
    [[nodiscard]] std::array<base::EntityId, 2> nodes(std::uint32_t row) const { return m_cards.get(row).nodes(); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<MarkerBodyCard> m_cards;
};

} // namespace oor::model

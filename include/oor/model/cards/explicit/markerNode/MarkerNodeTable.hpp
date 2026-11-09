#pragma once

#include "oor/model/cards/explicit/markerNode/MarkerNodeCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

struct MarkerNodeFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MarkerNodeCard& card);
    [[nodiscard]] static std::optional<MarkerNodeCard> read(FieldReader& reader);
    static void write(const MarkerNodeCard& card, FieldWriter& writer);
};

// All the CMARKN1 cards. A marker element is an element (IdSpace::Element, CardCategory::Element): its ID is unique
// among all the elements. Its PID names a PMARKER and its grid point is a node.
class MarkerNodeTable final : public TypedTable<MarkerNodeTable, MarkerNodeCard>
{
public:
    static constexpr std::string_view NAME = "MarkerNode";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Element, base::IdSpace::Element};
    using Format = MarkerNodeFormat;

    [[nodiscard]] static base::EntityId cardId(const MarkerNodeCard& card) noexcept { return card.id; }
    std::uint32_t append(const MarkerNodeCard& card, base::CardKey key);
    [[nodiscard]] MarkerNodeCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const MarkerNodeCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    // What the geometry of the element needs, without copying the whole card.
    [[nodiscard]] base::EntityId propId(std::uint32_t row) const { return m_cards.get(row).propId; }
    [[nodiscard]] std::array<base::EntityId, 1> nodes(std::uint32_t row) const { return m_cards.get(row).nodes(); }

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<MarkerNodeCard> m_cards;
};

} // namespace oor::model

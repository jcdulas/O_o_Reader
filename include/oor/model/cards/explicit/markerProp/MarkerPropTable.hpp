#pragma once

#include "oor/model/cards/explicit/markerProp/MarkerPropCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct MarkerPropFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const MarkerPropCard& card);
    [[nodiscard]] static std::optional<MarkerPropCard> read(FieldReader& reader);
    static void write(const MarkerPropCard& card, FieldWriter& writer);
};

// All the PMARKER cards: properties (IdSpace::Property), so the PID of a marker element finds them. A PMARKER names no
// other card.
class MarkerPropTable final : public TypedTable<MarkerPropTable, MarkerPropCard>
{
public:
    static constexpr std::string_view NAME = "MarkerProp";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Property, base::IdSpace::Property};
    using Format = MarkerPropFormat;

    [[nodiscard]] static base::EntityId cardId(const MarkerPropCard& card) noexcept { return card.id; }
    std::uint32_t append(const MarkerPropCard& card, base::CardKey key);
    [[nodiscard]] MarkerPropCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const MarkerPropCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a PMARKER refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<MarkerPropCard> m_cards;
};

} // namespace oor::model

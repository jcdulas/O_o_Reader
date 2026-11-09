#pragma once

#include "oor/model/cards/aero/aeroStripProp4/AeroStripProp4Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroStripProp4Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroStripProp4Card& card);
    [[nodiscard]] static std::optional<AeroStripProp4Card> read(FieldReader& reader);
    static void write(const AeroStripProp4Card& card, FieldWriter& writer);
};

// All the PAERO4 cards. The PID is in the space of the aerodynamic properties (IdSpace::AeroProperty, category
// AeroProperty), which PAERO1 to PAERO5 share: a CAERO4 names it (ReferenceTarget::AeroProperty). LCLA and LCIRC are
// AEFACT cards (ReferenceTarget::AeroFactor). The card is kept whole: the strips are a list of triples that nobody
// reads as columns.
class AeroStripProp4Table final : public TypedTable<AeroStripProp4Table, AeroStripProp4Card>
{
public:
    static constexpr std::string_view NAME = "AeroStripProp4";
    static inline const CardTypeInfo INFO{NAME, CardCategory::AeroProperty, base::IdSpace::AeroProperty};
    using Format = AeroStripProp4Format;

    [[nodiscard]] static base::EntityId cardId(const AeroStripProp4Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroStripProp4Card& card, base::CardKey key);
    [[nodiscard]] AeroStripProp4Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroStripProp4Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroStripProp4Card> m_cards;
};

} // namespace oor::model

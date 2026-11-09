#pragma once

#include "oor/model/cards/aero/aeroQuad4/AeroQuad4Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroQuad4Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroQuad4Card& card);
    [[nodiscard]] static std::optional<AeroQuad4Card> read(FieldReader& reader);
    static void write(const AeroQuad4Card& card, FieldWriter& writer);
};

// All the AEQUAD4 cards (category Aero: no card names an element of the aerodynamic mesh). The EID is unique among the
// AEQUAD4 cards (IdSpace::PerCardType); the model shares the element numbers of the mesh between this card and the
// AETRIA3, which a check of the model verifies. The four points are AEGRID cards (ReferenceTarget::AeroNode). The card
// is kept whole next to its ID.
class AeroQuad4Table final : public TypedTable<AeroQuad4Table, AeroQuad4Card>
{
public:
    static constexpr std::string_view NAME = "AeroQuad4";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Aero, base::IdSpace::PerCardType};
    using Format = AeroQuad4Format;

    [[nodiscard]] static base::EntityId cardId(const AeroQuad4Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroQuad4Card& card, base::CardKey key);
    [[nodiscard]] AeroQuad4Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroQuad4Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroQuad4Card> m_cards;
};

} // namespace oor::model

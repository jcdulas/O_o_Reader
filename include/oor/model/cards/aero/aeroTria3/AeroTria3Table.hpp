#pragma once

#include "oor/model/cards/aero/aeroTria3/AeroTria3Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroTria3Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroTria3Card& card);
    [[nodiscard]] static std::optional<AeroTria3Card> read(FieldReader& reader);
    static void write(const AeroTria3Card& card, FieldWriter& writer);
};

// All the AETRIA3 cards (category Aero: no card names an element of the aerodynamic mesh). The EID is unique among the
// AETRIA3 cards (IdSpace::PerCardType); the model shares the element numbers of the mesh between this card and the
// AEQUAD4, which a check of the model verifies. The three points are AEGRID cards (ReferenceTarget::AeroNode). The card
// is kept whole next to its ID.
class AeroTria3Table final : public TypedTable<AeroTria3Table, AeroTria3Card>
{
public:
    static constexpr std::string_view NAME = "AeroTria3";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Aero, base::IdSpace::PerCardType};
    using Format = AeroTria3Format;

    [[nodiscard]] static base::EntityId cardId(const AeroTria3Card& card) noexcept { return card.id; }
    std::uint32_t append(const AeroTria3Card& card, base::CardKey key);
    [[nodiscard]] AeroTria3Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroTria3Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroTria3Card> m_cards;
};

} // namespace oor::model

#pragma once

#include "oor/model/cards/nonlinear_contact/brakeSqueal/BrakeSquealCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct BrakeSquealFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const BrakeSquealCard& card);
    [[nodiscard]] static std::optional<BrakeSquealCard> read(FieldReader& reader);
    static void write(const BrakeSquealCard& card, FieldWriter& writer);
};

// All the BSQUEAL cards. The ID is unique among them (IdSpace::PerCardType) and is in the category BrakeSqueal, as the
// ID of a BRKSYS is: the Case Control BSQUEAL selects either, and a check reports a number that both have. The card
// names no other card. The cards are kept whole next to their ID.
class BrakeSquealTable final : public TypedTable<BrakeSquealTable, BrakeSquealCard>
{
public:
    static constexpr std::string_view NAME = "BrakeSqueal";
    static inline const CardTypeInfo INFO{NAME, CardCategory::BrakeSqueal, base::IdSpace::PerCardType};
    using Format = BrakeSquealFormat;

    [[nodiscard]] static base::EntityId cardId(const BrakeSquealCard& card) noexcept { return card.id; }
    std::uint32_t append(const BrakeSquealCard& card, base::CardKey key);
    [[nodiscard]] BrakeSquealCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const BrakeSquealCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no field names another card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<BrakeSquealCard> m_cards;
};

} // namespace oor::model

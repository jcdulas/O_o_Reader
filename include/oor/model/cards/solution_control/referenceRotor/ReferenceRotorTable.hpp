#pragma once

#include "oor/model/cards/solution_control/referenceRotor/ReferenceRotorCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ReferenceRotorFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ReferenceRotorCard& card);
    [[nodiscard]] static std::optional<ReferenceRotorCard> read(FieldReader& reader);
    static void write(const ReferenceRotorCard& card, FieldWriter& writer);
};

// All the REFROT cards. The ID is the RGYRO set the card belongs to (IdSpace::Group, category Other like RGYRO: the
// REFROT cards and the RGYRO cards of one RID are all invoked by a subcase that has RGYRO=RID). REFROTID and the
// non-reference rotors (the ends of a THRU range included) are Rotor references.
class ReferenceRotorTable final : public TypedTable<ReferenceRotorTable, ReferenceRotorCard>
{
public:
    static constexpr std::string_view NAME = "ReferenceRotor";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = ReferenceRotorFormat;

    [[nodiscard]] static base::EntityId cardId(const ReferenceRotorCard& card) noexcept { return card.id; }
    std::uint32_t append(const ReferenceRotorCard& card, base::CardKey key);
    [[nodiscard]] ReferenceRotorCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ReferenceRotorCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& referenceRotorIdColumn() noexcept { return m_referenceRotorId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_referenceRotorId;
    CardRows<ReferenceRotorCard> m_cards;
};

} // namespace oor::model

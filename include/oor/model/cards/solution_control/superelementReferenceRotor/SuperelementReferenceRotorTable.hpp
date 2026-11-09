#pragma once

#include "oor/model/cards/solution_control/superelementReferenceRotor/SuperelementReferenceRotorCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct SuperelementReferenceRotorFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const SuperelementReferenceRotorCard& card);
    [[nodiscard]] static std::optional<SuperelementReferenceRotorCard> read(FieldReader& reader);
    static void write(const SuperelementReferenceRotorCard& card, FieldWriter& writer);
};

// All the REFRTSE cards. The ID is the RGYRO set the card belongs to (IdSpace::Group, category Other like RGYRO and
// REFROT). REFROTID and each NFROTIDi are Rotor references when their superelement is 0.
class SuperelementReferenceRotorTable final
    : public TypedTable<SuperelementReferenceRotorTable, SuperelementReferenceRotorCard>
{
public:
    static constexpr std::string_view NAME = "SuperelementReferenceRotor";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::Group};
    using Format = SuperelementReferenceRotorFormat;

    [[nodiscard]] static base::EntityId cardId(const SuperelementReferenceRotorCard& card) noexcept { return card.id; }
    std::uint32_t append(const SuperelementReferenceRotorCard& card, base::CardKey key);
    [[nodiscard]] SuperelementReferenceRotorCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const SuperelementReferenceRotorCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }
    [[nodiscard]] Column<base::EntityId>& referenceRotorIdColumn() noexcept { return m_referenceRotorId; }
    [[nodiscard]] Column<std::int32_t>& referenceSuperelementIdColumn() noexcept { return m_referenceSuperelementId; }

private:
    Column<base::EntityId> m_id;
    Column<base::EntityId> m_referenceRotorId;
    Column<std::int32_t> m_referenceSuperelementId;
    CardRows<SuperelementReferenceRotorCard> m_cards;
};

} // namespace oor::model

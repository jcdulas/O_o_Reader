#pragma once

#include "oor/model/cards/dynamics/initialVelocityField/InitialVelocityFieldCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct InitialVelocityFieldFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const InitialVelocityFieldCard& card);
    [[nodiscard]] static std::optional<InitialVelocityFieldCard> read(FieldReader& reader);
    static void write(const InitialVelocityFieldCard& card, FieldWriter& writer);
};

// All the TIC3 cards. The SID is a set ID shared by the cards of the set (IdSpace::Group, category InitialCondition:
// the IC command selects the set, as for the TIC). G is a grid point and so are the IDs of the list, whose ranges name
// their two ends with NodeInRange. The cards are kept whole (CardRows) next to their ID, because the list of runs has a
// length of its own.
class InitialVelocityFieldTable final : public TypedTable<InitialVelocityFieldTable, InitialVelocityFieldCard>
{
public:
    static constexpr std::string_view NAME = "InitialVelocityField";
    static inline const CardTypeInfo INFO{NAME, CardCategory::InitialCondition, base::IdSpace::Group};
    using Format = InitialVelocityFieldFormat;

    [[nodiscard]] static base::EntityId cardId(const InitialVelocityFieldCard& card) noexcept { return card.id; }
    std::uint32_t append(const InitialVelocityFieldCard& card, base::CardKey key);
    [[nodiscard]] InitialVelocityFieldCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const InitialVelocityFieldCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<InitialVelocityFieldCard> m_cards;
};

} // namespace oor::model

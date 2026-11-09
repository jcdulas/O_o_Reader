#pragma once

#include "oor/model/cards/dynamics/initialValuesUserRoutine/InitialValuesUserRoutineCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct InitialValuesUserRoutineFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const InitialValuesUserRoutineCard& card);
    [[nodiscard]] static std::optional<InitialValuesUserRoutineCard> read(FieldReader& reader);
    static void write(const InitialValuesUserRoutineCard& card, FieldWriter& writer);
};

// All the TICEUDS cards. The SID is a set ID shared by the cards of the set (IdSpace::Group, category
// InitialCondition: the IC command selects the set, as for the TIC). SETID names a SET1. The cards are kept whole
// (CardRows) next to their ID, as every card of the explicit solver.
class InitialValuesUserRoutineTable final
    : public TypedTable<InitialValuesUserRoutineTable, InitialValuesUserRoutineCard>
{
public:
    static constexpr std::string_view NAME = "InitialValuesUserRoutine";
    static inline const CardTypeInfo INFO{NAME, CardCategory::InitialCondition, base::IdSpace::Group};
    using Format = InitialValuesUserRoutineFormat;

    [[nodiscard]] static base::EntityId cardId(const InitialValuesUserRoutineCard& card) noexcept { return card.id; }
    std::uint32_t append(const InitialValuesUserRoutineCard& card, base::CardKey key);
    [[nodiscard]] InitialValuesUserRoutineCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const InitialValuesUserRoutineCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<InitialValuesUserRoutineCard> m_cards;
};

} // namespace oor::model

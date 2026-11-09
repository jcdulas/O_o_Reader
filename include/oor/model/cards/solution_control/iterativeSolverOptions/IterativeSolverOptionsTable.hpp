#pragma once

#include "oor/model/cards/solution_control/iterativeSolverOptions/IterativeSolverOptionsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct IterativeSolverOptionsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const IterativeSolverOptionsCard& card);
    [[nodiscard]] static std::optional<IterativeSolverOptionsCard> read(FieldReader& reader);
    static void write(const IterativeSolverOptionsCard& card, FieldWriter& writer);
};

// All the ITER cards, in the category IterativeSolver: the Case Control SMETHOD names one by its SID, which is unique
// (IdSpace::PerCardType). The card names no other card. It is kept whole next to a column of its IDs.
class IterativeSolverOptionsTable final : public TypedTable<IterativeSolverOptionsTable, IterativeSolverOptionsCard>
{
public:
    static constexpr std::string_view NAME = "IterativeSolverOptions";
    static inline const CardTypeInfo INFO{NAME, CardCategory::IterativeSolver, base::IdSpace::PerCardType};
    using Format = IterativeSolverOptionsFormat;

    [[nodiscard]] static base::EntityId cardId(const IterativeSolverOptionsCard& card) noexcept { return card.id; }
    std::uint32_t append(const IterativeSolverOptionsCard& card, base::CardKey key);
    [[nodiscard]] IterativeSolverOptionsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const IterativeSolverOptionsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no field names another card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<IterativeSolverOptionsCard> m_cards;
};

} // namespace oor::model

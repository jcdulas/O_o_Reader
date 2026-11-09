#pragma once

#include "oor/model/cards/solution_control/nonlinearHarmonicSolverParams/NonlinearHarmonicSolverParamsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonlinearHarmonicSolverParamsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearHarmonicSolverParamsCard& card);
    [[nodiscard]] static std::optional<NonlinearHarmonicSolverParamsCard> read(FieldReader& reader);
    static void write(const NonlinearHarmonicSolverParamsCard& card, FieldWriter& writer);
};

// All the NHRMPRM cards (no ID: IdSpace::None, category Other). The card names no other card. It is kept whole: its
// pairs of name and value are a list that nobody reads as columns.
class NonlinearHarmonicSolverParamsTable final
    : public TypedTable<NonlinearHarmonicSolverParamsTable, NonlinearHarmonicSolverParamsCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearHarmonicSolverParams";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = NonlinearHarmonicSolverParamsFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearHarmonicSolverParamsCard&) noexcept { return 0; }
    std::uint32_t append(const NonlinearHarmonicSolverParamsCard& card, base::CardKey key);
    [[nodiscard]] NonlinearHarmonicSolverParamsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const NonlinearHarmonicSolverParamsCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // no field names another card

private:
    CardRows<NonlinearHarmonicSolverParamsCard> m_cards;
};

} // namespace oor::model

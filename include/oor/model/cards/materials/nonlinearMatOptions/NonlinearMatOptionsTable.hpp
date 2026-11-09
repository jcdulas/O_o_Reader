#pragma once

#include "oor/model/cards/materials/nonlinearMatOptions/NonlinearMatOptionsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct NonlinearMatOptionsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const NonlinearMatOptionsCard& card);
    [[nodiscard]] static std::optional<NonlinearMatOptionsCard> read(FieldReader& reader);
    static void write(const NonlinearMatOptionsCard& card, FieldWriter& writer);
};

// All the NLMOPTS cards (the options of a job: a deck has one in practice). The card has no ID and refers to no card.
// The cards are kept whole (CardRows).
class NonlinearMatOptionsTable final : public TypedTable<NonlinearMatOptionsTable, NonlinearMatOptionsCard>
{
public:
    static constexpr std::string_view NAME = "NonlinearMatOptions";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = NonlinearMatOptionsFormat;

    [[nodiscard]] static base::EntityId cardId(const NonlinearMatOptionsCard&) noexcept { return 0; }
    std::uint32_t append(const NonlinearMatOptionsCard& card, base::CardKey key);
    [[nodiscard]] NonlinearMatOptionsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const NonlinearMatOptionsCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the options refer to no card

private:
    CardRows<NonlinearMatOptionsCard> m_cards;
};

} // namespace oor::model

#pragma once

#include "oor/model/cards/solution_control/initialStress/InitialStressCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct InitialStressFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const InitialStressCard& card);
    [[nodiscard]] static std::optional<InitialStressCard> read(FieldReader& reader);
    static void write(const InitialStressCard& card, FieldWriter& writer);
};

// All the ISTRESS cards. A card has no ID (IdSpace::None, category Other) and any number of them may be in a deck; the
// two ends of its range of elements are `ElementInRange` references. The CID of the COORD line is -1 or 0 and names no
// coordinate system card. The cards are kept whole.
class InitialStressTable final : public TypedTable<InitialStressTable, InitialStressCard>
{
public:
    static constexpr std::string_view NAME = "InitialStress";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = InitialStressFormat;

    [[nodiscard]] static base::EntityId cardId(const InitialStressCard&) noexcept { return 0; }
    std::uint32_t append(const InitialStressCard& card, base::CardKey key);
    [[nodiscard]] InitialStressCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const InitialStressCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

private:
    CardRows<InitialStressCard> m_cards;
};

} // namespace oor::model

#pragma once

#include "oor/model/cards/aero/aeroDivergence/AeroDivergenceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct AeroDivergenceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const AeroDivergenceCard& card);
    [[nodiscard]] static std::optional<AeroDivergenceCard> read(FieldReader& reader);
    static void write(const AeroDivergenceCard& card, FieldWriter& writer);
};

// All the DIVERG cards, in the category DivergenceSet. The SID is unique among the DIVERG cards (IdSpace::PerCardType)
// and is what the Case Control DIVERG names (ReferenceTarget::DivergenceSet). The card is kept whole next to a column
// of its ID.
class AeroDivergenceTable final : public TypedTable<AeroDivergenceTable, AeroDivergenceCard>
{
public:
    static constexpr std::string_view NAME = "AeroDivergence";
    static inline const CardTypeInfo INFO{NAME, CardCategory::DivergenceSet, base::IdSpace::PerCardType};
    using Format = AeroDivergenceFormat;

    [[nodiscard]] static base::EntityId cardId(const AeroDivergenceCard& card) noexcept { return card.id; }
    std::uint32_t append(const AeroDivergenceCard& card, base::CardKey key);
    [[nodiscard]] AeroDivergenceCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const AeroDivergenceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the Mach numbers are reals

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<AeroDivergenceCard> m_cards;
};

} // namespace oor::model

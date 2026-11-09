#pragma once

#include "oor/model/cards/solution_control/timeToSpectrum/TimeToSpectrumCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TimeToSpectrumFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TimeToSpectrumCard& card);
    [[nodiscard]] static std::optional<TimeToSpectrumCard> read(FieldReader& reader);
    static void write(const TimeToSpectrumCard& card, FieldWriter& writer);
};

// All the TIM2PSD cards (IdSpace::PerCardType, category Other: the Case Control FATIGUE command selects one). A TIM2PSD
// names no other card (its event numbers are the TIDs of FTGLOAD cards, its subcases are Case Control subcases). The
// cards are kept whole (CardRows) next to their ID: they have lists of any length, and the query engine reads the ID.
class TimeToSpectrumTable final : public TypedTable<TimeToSpectrumTable, TimeToSpectrumCard>
{
public:
    static constexpr std::string_view NAME = "TimeToSpectrum";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = TimeToSpectrumFormat;

    [[nodiscard]] static base::EntityId cardId(const TimeToSpectrumCard& card) noexcept { return card.id; }
    std::uint32_t append(const TimeToSpectrumCard& card, base::CardKey key);
    [[nodiscard]] TimeToSpectrumCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TimeToSpectrumCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // a TIM2PSD refers to no other card

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TimeToSpectrumCard> m_cards;
};

} // namespace oor::model

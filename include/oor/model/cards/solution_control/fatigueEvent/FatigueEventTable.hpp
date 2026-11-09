#pragma once

#include "oor/model/cards/solution_control/fatigueEvent/FatigueEventCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FatigueEventFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FatigueEventCard& card);
    [[nodiscard]] static std::optional<FatigueEventCard> read(FieldReader& reader);
    static void write(const FatigueEventCard& card, FieldWriter& writer);
};

// All the FTGEVNT cards, in the category FatigueEvent that an FTGSEQ names (IdSpace::PerCardType: the number is shared
// with the FTGSEQ cards and a check reports a clash). Each FLOADi is a FatigueLoad reference. The cards are kept whole
// (CardRows) next to their ID: the list of loads has any length, and the query engine reads the ID.
class FatigueEventTable final : public TypedTable<FatigueEventTable, FatigueEventCard>
{
public:
    static constexpr std::string_view NAME = "FatigueEvent";
    static inline const CardTypeInfo INFO{NAME, CardCategory::FatigueEvent, base::IdSpace::PerCardType};
    using Format = FatigueEventFormat;

    [[nodiscard]] static base::EntityId cardId(const FatigueEventCard& card) noexcept { return card.id; }
    std::uint32_t append(const FatigueEventCard& card, base::CardKey key);
    [[nodiscard]] FatigueEventCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FatigueEventCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FatigueEventCard> m_cards;
};

} // namespace oor::model

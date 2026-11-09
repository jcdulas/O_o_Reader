#pragma once

#include "oor/model/cards/solution_control/fatigueSequence/FatigueSequenceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FatigueSequenceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FatigueSequenceCard& card);
    [[nodiscard]] static std::optional<FatigueSequenceCard> read(FieldReader& reader);
    static void write(const FatigueSequenceCard& card, FieldWriter& writer);
};

// All the FTGSEQ cards, in the category FatigueEvent (IdSpace::PerCardType: the number is shared with the FTGEVNT cards
// and a check reports a clash). Each FIDi is a FatigueEvent reference, to an FTGEVNT or to another FTGSEQ. The cards
// are kept whole (CardRows) next to their ID: the list of pairs has any length, and the query engine reads the ID.
class FatigueSequenceTable final : public TypedTable<FatigueSequenceTable, FatigueSequenceCard>
{
public:
    static constexpr std::string_view NAME = "FatigueSequence";
    static inline const CardTypeInfo INFO{NAME, CardCategory::FatigueEvent, base::IdSpace::PerCardType};
    using Format = FatigueSequenceFormat;

    [[nodiscard]] static base::EntityId cardId(const FatigueSequenceCard& card) noexcept { return card.id; }
    std::uint32_t append(const FatigueSequenceCard& card, base::CardKey key);
    [[nodiscard]] FatigueSequenceCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FatigueSequenceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FatigueSequenceCard> m_cards;
};

} // namespace oor::model

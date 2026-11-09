#pragma once

#include "oor/model/cards/nodes/moduleMoveSequence/ModuleMoveSequenceCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleMoveSequenceFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleMoveSequenceCard& card);
    [[nodiscard]] static std::optional<ModuleMoveSequenceCard> read(FieldReader& reader);
    static void write(const ModuleMoveSequenceCard& card, FieldWriter& writer);
};

// All the MDMOVE cards, in the category Other: the MODID is unique per card type (IdSpace::PerCardType). Each MVID is a
// reference to a move (ReferenceTarget::ModuleMove: an MDTRAN, MDROT1, MDROT2, MDMIR1 or MDMIR2), so a renumbering of
// the moves follows into the list. The cards are kept whole next to a column of their MODIDs.
class ModuleMoveSequenceTable final : public TypedTable<ModuleMoveSequenceTable, ModuleMoveSequenceCard>
{
public:
    static constexpr std::string_view NAME = "ModuleMoveSequence";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleMoveSequenceFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleMoveSequenceCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleMoveSequenceCard& card, base::CardKey key);
    [[nodiscard]] ModuleMoveSequenceCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleMoveSequenceCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleMoveSequenceCard> m_cards;
};

} // namespace oor::model

#pragma once

#include "oor/model/cards/solution_control/tireSelection/TireSelectionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct TireSelectionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const TireSelectionCard& card);
    [[nodiscard]] static std::optional<TireSelectionCard> read(FieldReader& reader);
    static void write(const TireSelectionCard& card, FieldWriter& writer);
};

// All the TIRE cards. The ID is unique among them (IdSpace::PerCardType, category Other): the Case Control TIRE command
// selects it, and the command is not resolved. The names of the tires are those of DTI entries, which no card types, so
// nothing is looked up. The cards are kept whole next to their ID.
class TireSelectionTable final : public TypedTable<TireSelectionTable, TireSelectionCard>
{
public:
    static constexpr std::string_view NAME = "TireSelection";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = TireSelectionFormat;

    [[nodiscard]] static base::EntityId cardId(const TireSelectionCard& card) noexcept { return card.id; }
    std::uint32_t append(const TireSelectionCard& card, base::CardKey key);
    [[nodiscard]] TireSelectionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const TireSelectionCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // the names are those of DTI entries

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<TireSelectionCard> m_cards;
};

} // namespace oor::model

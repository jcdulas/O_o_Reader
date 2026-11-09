#pragma once

#include "oor/model/cards/nodes/moduleBulkOptions/ModuleBulkOptionsCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleBulkOptionsFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleBulkOptionsCard& card);
    [[nodiscard]] static std::optional<ModuleBulkOptionsCard> read(FieldReader& reader);
    static void write(const ModuleBulkOptionsCard& card, FieldWriter& writer);
};

// All the MDBULK cards. The module is the ID, unique among them (IdSpace::PerCardType, category Other); the word ALL is
// not a number and is indexed as 0, which no module number has, so a deck has one MDBULK,ALL at most. A module is not a
// card of the model, so the card names no other card. The cards are kept whole next to their ID.
class ModuleBulkOptionsTable final : public TypedTable<ModuleBulkOptionsTable, ModuleBulkOptionsCard>
{
public:
    static constexpr std::string_view NAME = "ModuleBulkOptions";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleBulkOptionsFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleBulkOptionsCard& card) noexcept
    {
        return card.module.idOf();
    }
    std::uint32_t append(const ModuleBulkOptionsCard& card, base::CardKey key);
    [[nodiscard]] ModuleBulkOptionsCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleBulkOptionsCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // modules are not cards: nothing to look up

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleBulkOptionsCard> m_cards;
};

} // namespace oor::model

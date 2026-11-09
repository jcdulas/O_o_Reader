#pragma once

#include "oor/model/cards/nodes/moduleEntryExclusion/ModuleEntryExclusionCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleEntryExclusionFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleEntryExclusionCard& card);
    [[nodiscard]] static std::optional<ModuleEntryExclusionCard> read(FieldReader& reader);
    static void write(const ModuleEntryExclusionCard& card, FieldWriter& writer);
};

// All the EXCLUDE cards, category Other. A card has no ID (IdSpace::None) and names no card of the model: the names are
// entry names. The cards are kept whole, one value per row.
class ModuleEntryExclusionTable final : public TypedTable<ModuleEntryExclusionTable, ModuleEntryExclusionCard>
{
public:
    static constexpr std::string_view NAME = "ModuleEntryExclusion";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ModuleEntryExclusionFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleEntryExclusionCard&) noexcept { return 0; }
    std::uint32_t append(const ModuleEntryExclusionCard& card, base::CardKey key);
    [[nodiscard]] ModuleEntryExclusionCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleEntryExclusionCard& card) { m_cards.set(row, card); }
    void keepRows(const std::vector<std::uint32_t>& rows) { m_cards.keepRows(rows); }

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // entry names, not IDs: nothing to look up

private:
    CardRows<ModuleEntryExclusionCard> m_cards;
};

} // namespace oor::model

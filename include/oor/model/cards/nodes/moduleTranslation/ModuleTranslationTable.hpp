#pragma once

#include "oor/model/cards/nodes/moduleTranslation/ModuleTranslationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleTranslationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleTranslationCard& card);
    [[nodiscard]] static std::optional<ModuleTranslationCard> read(FieldReader& reader);
    static void write(const ModuleTranslationCard& card, FieldWriter& writer);
};

// All the MDTRAN cards, in the category ModuleMove with the other four moves (MDROT1, MDROT2, MDMIR1, MDMIR2): the MVID
// is unique per card type (IdSpace::PerCardType) and an MDMOVE names it. IDS and IDE are references to the points
// (GRID or POINT) of the main Bulk Data. The cards are kept whole next to a column of their MVIDs.
class ModuleTranslationTable final : public TypedTable<ModuleTranslationTable, ModuleTranslationCard>
{
public:
    static constexpr std::string_view NAME = "ModuleTranslation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ModuleMove, base::IdSpace::PerCardType};
    using Format = ModuleTranslationFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleTranslationCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleTranslationCard& card, base::CardKey key);
    [[nodiscard]] ModuleTranslationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleTranslationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleTranslationCard> m_cards;
};

} // namespace oor::model

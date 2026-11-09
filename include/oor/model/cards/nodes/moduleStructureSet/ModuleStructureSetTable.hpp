#pragma once

#include "oor/model/cards/nodes/moduleStructureSet/ModuleStructureSetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleStructureSetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleStructureSetCard& card);
    [[nodiscard]] static std::optional<ModuleStructureSetCard> read(FieldReader& reader);
    static void write(const ModuleStructureSetCard& card, FieldWriter& writer);
};

// All the MDSSET cards, category Other. A card has no ID of its own (IdSpace::None): several cards may list the same
// module. They are kept whole next to a column of their MODIDs. The IDs are structural points or elements of a module
// and name no card of this model: nothing is visited.
class ModuleStructureSetTable final : public TypedTable<ModuleStructureSetTable, ModuleStructureSetCard>
{
public:
    static constexpr std::string_view NAME = "ModuleStructureSet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ModuleStructureSetFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleStructureSetCard&) noexcept { return 0; }
    std::uint32_t append(const ModuleStructureSetCard& card, base::CardKey key);
    [[nodiscard]] ModuleStructureSetCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleStructureSetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // plain IDs of a module: nothing to look up

    [[nodiscard]] Column<base::EntityId>& moduleIdColumn() noexcept { return m_moduleId; }

private:
    Column<base::EntityId> m_moduleId;
    CardRows<ModuleStructureSetCard> m_cards;
};

} // namespace oor::model

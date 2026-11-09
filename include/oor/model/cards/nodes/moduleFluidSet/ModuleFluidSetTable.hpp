#pragma once

#include "oor/model/cards/nodes/moduleFluidSet/ModuleFluidSetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleFluidSetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleFluidSetCard& card);
    [[nodiscard]] static std::optional<ModuleFluidSetCard> read(FieldReader& reader);
    static void write(const ModuleFluidSetCard& card, FieldWriter& writer);
};

// All the MDFSET cards, category Other. A card has no ID of its own (IdSpace::None): several cards may list the same
// module. They are kept whole next to a column of their MODIDs. The IDs are fluid points or elements of a module and
// name no card of this model: nothing is visited.
class ModuleFluidSetTable final : public TypedTable<ModuleFluidSetTable, ModuleFluidSetCard>
{
public:
    static constexpr std::string_view NAME = "ModuleFluidSet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = ModuleFluidSetFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleFluidSetCard&) noexcept { return 0; }
    std::uint32_t append(const ModuleFluidSetCard& card, base::CardKey key);
    [[nodiscard]] ModuleFluidSetCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleFluidSetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t, ReferenceVisitor&) override {} // plain IDs of a module: nothing to look up

    [[nodiscard]] Column<base::EntityId>& moduleIdColumn() noexcept { return m_moduleId; }

private:
    Column<base::EntityId> m_moduleId;
    CardRows<ModuleFluidSetCard> m_cards;
};

} // namespace oor::model

#pragma once

#include "oor/model/cards/nodes/moduleLocation/ModuleLocationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleLocationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleLocationCard& card);
    [[nodiscard]] static std::optional<ModuleLocationCard> read(FieldReader& reader);
    static void write(const ModuleLocationCard& card, FieldWriter& writer);
};

// All the MDLOC cards, in the category Other: the MODID is unique per card type (IdSpace::PerCardType). PB1, PB2 and
// PB3 are references to points (GRID or POINT) of the main Bulk Data; the PAi are points of the module (a BEGIN MODULE
// section, a part of the deck) and the MODID is a module number: none of them is looked up. The cards are kept whole
// next to a column of their MODIDs.
class ModuleLocationTable final : public TypedTable<ModuleLocationTable, ModuleLocationCard>
{
public:
    static constexpr std::string_view NAME = "ModuleLocation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleLocationFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleLocationCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleLocationCard& card, base::CardKey key);
    [[nodiscard]] ModuleLocationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleLocationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleLocationCard> m_cards;
};

} // namespace oor::model

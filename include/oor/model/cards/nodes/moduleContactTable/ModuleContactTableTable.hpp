#pragma once

#include "oor/model/cards/nodes/moduleContactTable/ModuleContactTableCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleContactTableFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleContactTableCard& card);
    [[nodiscard]] static std::optional<ModuleContactTableCard> read(FieldReader& reader);
    static void write(const ModuleContactTableCard& card, FieldWriter& writer);
};

// All the MDBCTB1 cards. The BCID is a contact table number (IdSpace::PerCardType, category ContactTable): the format
// reference says it is unique among the BCTABL1 and MDBCTB1 cards, which is what the contact table check of the
// validation looks for across the types of the category. The IDs of the pairs are references to contact pairs (BCONECT
// or MDBCNCT) in their module: the main Bulk Data, or the part of another module. The cards are kept whole next to
// their ID.
class ModuleContactTableTable final : public TypedTable<ModuleContactTableTable, ModuleContactTableCard>
{
public:
    static constexpr std::string_view NAME = "ModuleContactTable";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ContactTable, base::IdSpace::PerCardType};
    using Format = ModuleContactTableFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleContactTableCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleContactTableCard& card, base::CardKey key);
    [[nodiscard]] ModuleContactTableCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleContactTableCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleContactTableCard> m_cards;
};

} // namespace oor::model

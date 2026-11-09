#pragma once

#include "oor/model/cards/nodes/moduleRigidRod/ModuleRigidRodCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleRigidRodFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleRigidRodCard& card);
    [[nodiscard]] static std::optional<ModuleRigidRodCard> read(FieldReader& reader);
    static void write(const ModuleRigidRodCard& card, FieldWriter& writer);
};

// All the MDRROD cards. The EID is an element number, which the rules of the RROD want unique among all the elements;
// the table only keeps it unique among the MDRROD cards (IdSpace::PerCardType, category Other) and a validation check
// looks for a number that another kind of element shares. GA and GB are references to grid points in their module: the
// main Bulk Data, or the part of another module. The cards are kept whole next to their ID.
class ModuleRigidRodTable final : public TypedTable<ModuleRigidRodTable, ModuleRigidRodCard>
{
public:
    static constexpr std::string_view NAME = "ModuleRigidRod";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleRigidRodFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleRigidRodCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleRigidRodCard& card, base::CardKey key);
    [[nodiscard]] ModuleRigidRodCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleRigidRodCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleRigidRodCard> m_cards;
};

} // namespace oor::model

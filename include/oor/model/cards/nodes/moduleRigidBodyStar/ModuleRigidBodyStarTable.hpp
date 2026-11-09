#pragma once

#include "oor/model/cards/nodes/moduleRigidBodyStar/ModuleRigidBodyStarCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleRigidBodyStarFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleRigidBodyStarCard& card);
    [[nodiscard]] static std::optional<ModuleRigidBodyStarCard> read(FieldReader& reader);
    static void write(const ModuleRigidBodyStarCard& card, FieldWriter& writer);
};

// All the MDRBE2 cards. The EID is an element number, which the rules of the RBE2 want unique among all the elements;
// the table only keeps it unique among the MDRBE2 cards (IdSpace::PerCardType, category Other) and a validation check
// looks for a number that another kind of element shares. The independent point and the dependent points are references
// to grid points in their module: the main Bulk Data, or the part of another module. The cards are kept
// whole next to their ID.
class ModuleRigidBodyStarTable final : public TypedTable<ModuleRigidBodyStarTable, ModuleRigidBodyStarCard>
{
public:
    static constexpr std::string_view NAME = "ModuleRigidBodyStar";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleRigidBodyStarFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleRigidBodyStarCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleRigidBodyStarCard& card, base::CardKey key);
    [[nodiscard]] ModuleRigidBodyStarCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleRigidBodyStarCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleRigidBodyStarCard> m_cards;
};

} // namespace oor::model

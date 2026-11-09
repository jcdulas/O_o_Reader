#pragma once

#include "oor/model/cards/nodes/moduleRigidInterpolation/ModuleRigidInterpolationCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleRigidInterpolationFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleRigidInterpolationCard& card);
    [[nodiscard]] static std::optional<ModuleRigidInterpolationCard> read(FieldReader& reader);
    static void write(const ModuleRigidInterpolationCard& card, FieldWriter& writer);
};

// All the MDRBE3 cards. The EID is an element number, which the format reference wants unique with respect to all
// elements; the table only keeps it unique among the MDRBE3 cards (IdSpace::PerCardType, category Other) and a
// validation check looks for a number that another kind of element shares. The reference point, the points of the
// groups and the points after UM are references to grid points in their module: the main Bulk Data, or the part of
// another module. The cards are kept whole next to their ID.
class ModuleRigidInterpolationTable final
    : public TypedTable<ModuleRigidInterpolationTable, ModuleRigidInterpolationCard>
{
public:
    static constexpr std::string_view NAME = "ModuleRigidInterpolation";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleRigidInterpolationFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleRigidInterpolationCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleRigidInterpolationCard& card, base::CardKey key);
    [[nodiscard]] ModuleRigidInterpolationCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleRigidInterpolationCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleRigidInterpolationCard> m_cards;
};

} // namespace oor::model

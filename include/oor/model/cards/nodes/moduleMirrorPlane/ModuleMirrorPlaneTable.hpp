#pragma once

#include "oor/model/cards/nodes/moduleMirrorPlane/ModuleMirrorPlaneCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleMirrorPlaneFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleMirrorPlaneCard& card);
    [[nodiscard]] static std::optional<ModuleMirrorPlaneCard> read(FieldReader& reader);
    static void write(const ModuleMirrorPlaneCard& card, FieldWriter& writer);
};

// All the MDMPLN cards, in the category Other: the MODID is unique per card type (IdSpace::PerCardType). P1, P2 and P3
// are references to points (GRID or POINT) of the main Bulk Data; the MODID is a module number, not a reference. The
// cards are kept whole next to a column of their MODIDs.
class ModuleMirrorPlaneTable final : public TypedTable<ModuleMirrorPlaneTable, ModuleMirrorPlaneCard>
{
public:
    static constexpr std::string_view NAME = "ModuleMirrorPlane";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleMirrorPlaneFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleMirrorPlaneCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleMirrorPlaneCard& card, base::CardKey key);
    [[nodiscard]] ModuleMirrorPlaneCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleMirrorPlaneCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleMirrorPlaneCard> m_cards;
};

} // namespace oor::model

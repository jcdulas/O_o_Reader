#pragma once

#include "oor/model/cards/nodes/moduleRotation2/ModuleRotation2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleRotation2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleRotation2Card& card);
    [[nodiscard]] static std::optional<ModuleRotation2Card> read(FieldReader& reader);
    static void write(const ModuleRotation2Card& card, FieldWriter& writer);
};

// All the MDROT2 cards, in the category ModuleMove with the other four moves: the MVID is unique per card type
// (IdSpace::PerCardType) and an MDMOVE names it. CID is a reference to a coordinate system and RID to a point (GRID or
// POINT) of the main Bulk Data. The cards are kept whole next to a column of their MVIDs.
class ModuleRotation2Table final : public TypedTable<ModuleRotation2Table, ModuleRotation2Card>
{
public:
    static constexpr std::string_view NAME = "ModuleRotation2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ModuleMove, base::IdSpace::PerCardType};
    using Format = ModuleRotation2Format;

    [[nodiscard]] static base::EntityId cardId(const ModuleRotation2Card& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleRotation2Card& card, base::CardKey key);
    [[nodiscard]] ModuleRotation2Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleRotation2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleRotation2Card> m_cards;
};

} // namespace oor::model

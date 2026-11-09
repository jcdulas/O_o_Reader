#pragma once

#include "oor/model/cards/nodes/moduleMirror2/ModuleMirror2Card.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleMirror2Format
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleMirror2Card& card);
    [[nodiscard]] static std::optional<ModuleMirror2Card> read(FieldReader& reader);
    static void write(const ModuleMirror2Card& card, FieldWriter& writer);
};

// All the MDMIR2 cards, in the category ModuleMove with the other four moves: the MVID is unique per card type
// (IdSpace::PerCardType) and an MDMOVE names it. CID is a reference to a coordinate system and RID to a point (GRID or
// POINT) of the main Bulk Data. The cards are kept whole next to a column of their MVIDs.
class ModuleMirror2Table final : public TypedTable<ModuleMirror2Table, ModuleMirror2Card>
{
public:
    static constexpr std::string_view NAME = "ModuleMirror2";
    static inline const CardTypeInfo INFO{NAME, CardCategory::ModuleMove, base::IdSpace::PerCardType};
    using Format = ModuleMirror2Format;

    [[nodiscard]] static base::EntityId cardId(const ModuleMirror2Card& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleMirror2Card& card, base::CardKey key);
    [[nodiscard]] ModuleMirror2Card card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleMirror2Card& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleMirror2Card> m_cards;
};

} // namespace oor::model

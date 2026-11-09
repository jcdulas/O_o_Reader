#pragma once

#include "oor/model/cards/nodes/moduleBushing/ModuleBushingCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct ModuleBushingFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const ModuleBushingCard& card);
    [[nodiscard]] static std::optional<ModuleBushingCard> read(FieldReader& reader);
    static void write(const ModuleBushingCard& card, FieldWriter& writer);
};

// All the MDBUSH cards. The EID is unique among them (IdSpace::PerCardType, category Other: the element is of the
// multi-module family, and the format reference leaves it to the deck to keep it apart from the element IDs of the
// model). The property is looked up as a property of the main Bulk Data; GA, GB, GO, CID and OCID are looked up in this
// model only when their module (MODA, MODB, MODC) is 0. The cards are kept whole next to their ID.
class ModuleBushingTable final : public TypedTable<ModuleBushingTable, ModuleBushingCard>
{
public:
    static constexpr std::string_view NAME = "ModuleBushing";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = ModuleBushingFormat;

    [[nodiscard]] static base::EntityId cardId(const ModuleBushingCard& card) noexcept { return card.id; }
    std::uint32_t append(const ModuleBushingCard& card, base::CardKey key);
    [[nodiscard]] ModuleBushingCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const ModuleBushingCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<ModuleBushingCard> m_cards;
};

} // namespace oor::model

#pragma once

#include "oor/model/cards/explicit/fluidStructureSwitch/FluidStructureSwitchCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct FluidStructureSwitchFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const FluidStructureSwitchCard& card);
    [[nodiscard]] static std::optional<FluidStructureSwitchCard> read(FieldReader& reader);
    static void write(const FluidStructureSwitchCard& card, FieldWriter& writer);
};

// All the DYFSISW cards. The SWID is unique among them (IdSpace::PerCardType) and no other card names it, so the
// category is Other. The TID is a TABLED1. The cards are kept whole (CardRows) next to their ID.
class FluidStructureSwitchTable final : public TypedTable<FluidStructureSwitchTable, FluidStructureSwitchCard>
{
public:
    static constexpr std::string_view NAME = "FluidStructureSwitch";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = FluidStructureSwitchFormat;

    [[nodiscard]] static base::EntityId cardId(const FluidStructureSwitchCard& card) noexcept { return card.id; }
    std::uint32_t append(const FluidStructureSwitchCard& card, base::CardKey key);
    [[nodiscard]] FluidStructureSwitchCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const FluidStructureSwitchCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<FluidStructureSwitchCard> m_cards;
};

} // namespace oor::model

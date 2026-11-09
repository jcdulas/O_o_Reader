#pragma once

#include "oor/model/cards/sets/radiatedPowerPanelSet/RadiatedPowerPanelSetCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct RadiatedPowerPanelSetFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const RadiatedPowerPanelSetCard& card);
    [[nodiscard]] static std::optional<RadiatedPowerPanelSetCard> read(FieldReader& reader);
    static void write(const RadiatedPowerPanelSetCard& card, FieldWriter& writer);
};

// All the ERPPNL cards (category Other). A card has no ID of its own (IdSpace::None), so there may be many. The SETIDi
// are references to SET1 and SET3 cards (ReferenceTarget::Set, which a renumbering of the sets follows); the NAMEi are
// labels, not references. The cards are kept whole next to a column of the number of their panels, which is what the
// query engine reads.
class RadiatedPowerPanelSetTable final : public TypedTable<RadiatedPowerPanelSetTable, RadiatedPowerPanelSetCard>
{
public:
    static constexpr std::string_view NAME = "RadiatedPowerPanelSet";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::None};
    using Format = RadiatedPowerPanelSetFormat;

    [[nodiscard]] static base::EntityId cardId(const RadiatedPowerPanelSetCard&) noexcept { return 0; }
    std::uint32_t append(const RadiatedPowerPanelSetCard& card, base::CardKey key);
    [[nodiscard]] RadiatedPowerPanelSetCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const RadiatedPowerPanelSetCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t) const { return 0; }
    void setIdAtRow(std::uint32_t, base::EntityId) {}
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<std::int32_t>& panelCountColumn() noexcept { return m_panelCount; }

private:
    Column<std::int32_t> m_panelCount;
    CardRows<RadiatedPowerPanelSetCard> m_cards;
};

} // namespace oor::model

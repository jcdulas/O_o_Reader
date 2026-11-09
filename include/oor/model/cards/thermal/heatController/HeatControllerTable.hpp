#pragma once

#include "oor/model/cards/thermal/heatController/HeatControllerCard.hpp"
#include "oor/model/core/TypedTable.hpp"
#include "oor/model/storage/CardRows.hpp"
#include "oor/model/storage/Column.hpp"

#include <optional>
#include <vector>

namespace oor::model
{

struct HeatControllerFormat
{
    // The first thing wrong with the values of a card, if anything (used by read and by editing).
    [[nodiscard]] static std::optional<CardProblem> check(const HeatControllerCard& card);
    [[nodiscard]] static std::optional<HeatControllerCard> read(FieldReader& reader);
    static void write(const HeatControllerCard& card, FieldWriter& writer);
};

// All the CONTRLT cards (category Other): the ID is unique per card type (IdSpace::PerCardType). The CNTRLND of the
// heat cards that use a controller is a plain number (a grid point may have the same one), so nothing visits the ID.
// The SENSOR is a grid point or a scalar point (ReferenceTarget::ScalarOrGridPoint). The cards are kept whole next to a
// column of their IDs.
class HeatControllerTable final : public TypedTable<HeatControllerTable, HeatControllerCard>
{
public:
    static constexpr std::string_view NAME = "HeatController";
    static inline const CardTypeInfo INFO{NAME, CardCategory::Other, base::IdSpace::PerCardType};
    using Format = HeatControllerFormat;

    [[nodiscard]] static base::EntityId cardId(const HeatControllerCard& card) noexcept { return card.id; }
    std::uint32_t append(const HeatControllerCard& card, base::CardKey key);
    [[nodiscard]] HeatControllerCard card(std::uint32_t row) const { return m_cards.get(row); }
    void store(std::uint32_t row, const HeatControllerCard& card);
    void keepRows(const std::vector<std::uint32_t>& rows);

    [[nodiscard]] base::EntityId idAtRow(std::uint32_t row) const { return m_id.get(row); }
    void setIdAtRow(std::uint32_t row, base::EntityId id);
    void visitReferences(std::uint32_t row, ReferenceVisitor& visitor) override;

    [[nodiscard]] Column<base::EntityId>& idColumn() noexcept { return m_id; }

private:
    Column<base::EntityId> m_id;
    CardRows<HeatControllerCard> m_cards;
};

} // namespace oor::model

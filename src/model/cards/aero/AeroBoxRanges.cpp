#include "oor/model/cards/aero/AeroBoxRanges.hpp"

#include "oor/model/cards/aero/aeroBody2/AeroBody2Table.hpp"
#include "oor/model/cards/aero/aeroFactorList/AeroFactorListTable.hpp"
#include "oor/model/cards/aero/aeroLiftingPanel1/AeroLiftingPanel1Table.hpp"
#include "oor/model/cards/aero/aeroMachBoxPanel3/AeroMachBoxPanel3Table.hpp"
#include "oor/model/cards/aero/aeroPistonPanel5/AeroPistonPanel5Table.hpp"
#include "oor/model/cards/aero/aeroStripPanel4/AeroStripPanel4Table.hpp"

#include <algorithm>

namespace oor::model
{

namespace
{

// The number of values of the AEFACT `list` (0 when it does not exist).
std::size_t valueCount(const Model& model, base::EntityId list)
{
    if (list <= 0)
        return 0;
    const auto* table = model.findTable<AeroFactorListTable>();
    if (table == nullptr)
        return 0;
    const std::optional<CardLocation> found = model.locateInType(typeIndexOf<AeroFactorListTable>(), list);
    return found ? table->valuesAt(found->row).size() : 0;
}

// A card owns at least its EID, whatever its lists say.
base::EntityId ownedIds(std::int64_t count, base::EntityId panelId)
{
    return static_cast<base::EntityId>(std::clamp<std::int64_t>(count, 1, base::MAX_ENTITY_ID - panelId));
}

// The ranges of the cards of one type: `count` gives the number of IDs a card takes.
template <typename TableT, typename CountFn>
void addRanges(const Model& model, std::vector<AeroBoxRange>& ranges, const CountFn& count)
{
    const auto* table = model.findTable<TableT>();
    if (table == nullptr)
        return;
    const std::uint16_t type = typeIndexOf<TableT>();
    for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
    {
        if (!table->rows().isAlive(row))
            continue;
        const typename TableT::CardType card = table->card(row);
        const std::int64_t owned = count(card);
        ranges.push_back(AeroBoxRange{card.id, ownedIds(owned, card.id), CardLocation{type, row}, owned >= 1});
    }
}

} // namespace

base::EntityId divisionCount(const Model& model, std::int32_t count, base::EntityId list)
{
    if (count > 0)
        return count;
    const std::size_t points = valueCount(model, list);
    return points >= 2 ? static_cast<base::EntityId>(points - 1) : 0;
}

AeroBoxRanges::AeroBoxRanges(const Model& model)
{
    // CAERO1: the boxes of the lattice, span by span.
    addRanges<AeroLiftingPanel1Table>(
        model, m_ranges,
        [&](const AeroLiftingPanel1Card& card)
        {
            return static_cast<std::int64_t>(divisionCount(model, card.spanBoxCount, card.spanDivisionListId)) *
                   divisionCount(model, card.chordBoxCount, card.chordDivisionListId);
        });
    // CAERO2: the slender body elements, then the interference elements.
    addRanges<AeroBody2Table>(
        model, m_ranges,
        [&](const AeroBody2Card& card)
        {
            return static_cast<std::int64_t>(divisionCount(model, card.slenderElemCount, card.slenderListId)) +
                   divisionCount(model, card.interferenceElemCount, card.interferenceListId);
        });
    // CAERO3: the (x, y) pairs of LISTW, then LISTC1 and LISTC2: two numbers a pair.
    addRanges<AeroMachBoxPanel3Table>(
        model, m_ranges,
        [&](const AeroMachBoxPanel3Card& card)
        {
            return static_cast<std::int64_t>((valueCount(model, card.wingPointListId) +
                                              valueCount(model, card.controlPointListId1) +
                                              valueCount(model, card.controlPointListId2)) /
                                             2);
        });
    // CAERO4 and CAERO5: one strip a division of the span.
    addRanges<AeroStripPanel4Table>(model, m_ranges, [&](const AeroStripPanel4Card& card)
                                    { return divisionCount(model, card.spanBoxCount, card.spanDivisionListId); });
    addRanges<AeroPistonPanel5Table>(model, m_ranges, [&](const AeroPistonPanel5Card& card)
                                     { return divisionCount(model, card.spanBoxCount, card.spanDivisionListId); });
    std::ranges::sort(m_ranges, {}, &AeroBoxRange::panelId);
}

const AeroBoxRange* AeroBoxRanges::owner(base::EntityId box) const noexcept
{
    // The last range that starts at or below the box; the ones before it start lower, so only a card that overlaps it
    // could also hold the box, and then the later card is the answer.
    const auto next = std::ranges::upper_bound(m_ranges, box, {}, &AeroBoxRange::panelId);
    if (next == m_ranges.begin())
        return nullptr;
    const AeroBoxRange& candidate = *std::prev(next);
    return box <= candidate.lastBox() ? &candidate : nullptr;
}

} // namespace oor::model

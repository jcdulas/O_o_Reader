#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace oor::model
{

// One SLBDY card: the list of the GRIDS slot points on the boundary of a slot cavity with the axisymmetric fluid
// cavity. The card needs an AXSLOT card in the deck, which is a rule between cards.
//
// Card layout:
//   SLBDY  RHO  M  ID1  ID2  ID3  ID4  ID5  ID6
//          ID7  ...
// The IDs are on the first line from field 3 and go on on the continuation lines from field 9 (eight to a line).
//
// RHO is the density of the fluid at the boundary (above zero) and M the number of slots (zero or more: no matrices are
// made at the boundary for zero slots); a blank one is given by the RHOD or MD field of the AXSLOT card,
// which only the model can tell, so a blank field is kept blank (an empty optional). The effective RHO must not be
// zero: a rule between cards. The IDs are GRIDS points (node references) listed in the order of the boundary,
// travelling along it in either direction, so their order is kept; at least two are needed. Blank fields
// between them are padding.
struct AcousticSlotBoundaryCard
{
    static constexpr std::size_t MIN_POINTS = 2;

    std::optional<double> density;
    std::optional<std::int32_t> slotCount;
    std::vector<base::EntityId> points;

    [[nodiscard]] bool operator==(const AcousticSlotBoundaryCard&) const noexcept = default;
};

} // namespace oor::model

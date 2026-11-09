#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// A grid of a ROTORG card, or a run of grids "first THRU last BY by".
struct RotorLineNodesRun
{
    base::EntityId first = 0;
    base::EntityId last = 0;      // equal to first for a single grid
    base::EntityId increment = 1; // the increment of a THRU run

    [[nodiscard]] bool operator==(const RotorLineNodesRun&) const noexcept = default;
};

// One ROTORG card: the grids that make the line model of a rotor.
//
// Card layout:  ROTORG  ROTORID  GRID1  GRID2  GRID3  -etc.-
//        or:   ROTORG  ROTORID  GRID1  THRU  GRID2  BY  INC
// The grids are kept as written: single grids and runs, in order, from field 2 on over as many continuation lines as
// needed (a run is never split between two lines). The ROTORID is shared by the ROTORG cards of one rotor
// and by the other rotor cards (ROTOR, ROTORAX and ROTORSE: the category Rotor, which the RSPINR, RSPINT, REFROT,
// REFRTSE, ROTBENT, ROTHYBD and RGYRO name). The ROTORIDs of the ROTORG, ROTORAX and ROTORSE cards must be different
// (the check `compat.rotor-id-repeated`).
struct RotorLineNodesCard
{
    base::EntityId id = 0;
    std::vector<RotorLineNodesRun> runs;
};

} // namespace oor::model

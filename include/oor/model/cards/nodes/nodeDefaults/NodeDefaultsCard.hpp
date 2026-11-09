#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One GRDSET card: the default CP, CD, PS and SEID of every GRID whose field is blank.
//
// Card layout (fields 1, 3, 4 and 5 are blank):
//   GRDSET        CP                  CD   PS   SEID
//
// The defaults are applied when a deck is read, to the blank fields of the GRIDs, wherever the GRDSET is in the deck
// (NodeTable::finishRead), and again by Model::applyDefaults (Model.apply_defaults in Python), which is how a GRDSET
// added to a model afterwards gives its defaults to the GRIDs that still note blank fields. A GRID added from Python
// without a CP, CD, PS or SEID notes it as blank. Only one GRDSET may appear.
struct NodeDefaultsCard
{
    base::EntityId positionFrameId = 0;     // coordinate system of the grid locations (0 = basic)
    base::EntityId displacementFrameId = 0; // coordinate system of the displacements (-1 is allowed, 0 = basic)
    std::int32_t permanentConstraints = 0;  // permanent single-point constraints, as digits 1..6 (0 = none)
    base::EntityId superelementId = 0;
};

} // namespace oor::model

#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/explicit/BoundaryBox.hpp"
#include "oor/model/cards/explicit/ExplicitKeywords.hpp"

#include <optional>

namespace oor::model
{

// One BARRIER card (SOL 700): faces of an Euler mesh that material cannot cross. It overrules the FLOW and FLOWT
// definitions of the same faces. No other entry of the format reference has this name.
//
// Card layout: BARRIER  BID  BCID  MESH  DIR  SKFRIC                   fields 1 to 5 (fields 6 to 8 are blank)
//             +        XMIN  XMAX  YMIN  YMAX  ZMIN  ZMAX             fields 9 to 14
// BID numbers the barrier. The faces are chosen in one of three ways, which exclude each other in this
// order: BCID, the number of a set of BCSEG cards that lists the element faces (it also gives the barrier inside
// the mesh); else DIR, one of the six faces of the mesh box; else the box XMIN to ZMAX on the boundary of the mesh. A
// choice that is outranked is kept as given and ignored by the solver. MESH is the Euler mesh the barrier belongs to;
// it is used only when several meshes exist and BCID is blank, and a blank MESH means all of them. SKFRIC is the skin
// friction coefficient (not negative): the solver uses it only with a viscous fluid, and a blank SKFRIC means no slip
// at the wall, which is not the same thing as 0.0 (free slip), so a blank is kept as such. The box has the defaults and
// the shape of BoundaryBox.
struct EulerBarrierCard
{
    base::EntityId id = 0;            // unique among the barriers
    base::EntityId contactBodyId = 0; // a set of BCSEG cards, 0 when blank
    base::EntityId meshId = 0;        // an Euler MESH, 0 when blank
    std::optional<BoundaryFace> meshFace;
    std::optional<double> skinFriction;
    BoundaryBox box;
};

} // namespace oor::model

#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// One AESURFS card: the structural grid points that belong to a control surface, whose mass gives the moment of inertia
// of the surface about its hinge line (so that the hinge moments include the inertial forces).
//
// Card layout: AESURFS  ID  LABEL  (blank)  LIST1  (blank)  LIST2
// Fields 3 and 5 are blank in the format reference and the card refuses a value there. LABEL must be the label of an
// AESURF (a check between cards, which compares the labels without regard to the case); the
// ID is ignored by the solver, so several cards may have the same one. LIST1 is the SET1 that lists the grid points of
// the first component of the surface, LIST2 the SET1 of the second component. The format reference gives both, but a
// surface that has one component has no second list, so LIST2 is optional here (0 when blank). The label is kept as
// written.
struct AeroControlSurfaceNodesCard
{
    base::EntityId id = 0;
    std::string label; // up to eight characters
    base::EntityId nodeSetId1 = 0;
    base::EntityId nodeSetId2 = 0; // 0 when blank

    [[nodiscard]] bool operator==(const AeroControlSurfaceNodesCard&) const noexcept = default;
};

} // namespace oor::model

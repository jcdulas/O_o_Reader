#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// One AESTAT card: a rigid body motion that is a variable of a static aeroelastic trim (the angle of attack, a rate, an
// acceleration of the aircraft).
//
// Card layout: AESTAT  ID  LABEL
// LABEL names the variable: ANGLEA, SIDES, ROLL, PITCH, YAW, URDD1 to URDD6 are the labels the program knows; any other
// label is allowed when the user supplies the forces. A TRIM card fixes a variable by its LABEL, an
// AELINK ties it to others and a UXVEC gives it a magnitude, so the label (not the ID) is what the other cards match,
// without regard to the case, against the labels of every AESTAT, AESURF and AEPARM. The ID is ignored by the solver:
// two cards may have the same one, and the card is filed under it only because
// the format reference makes it a field. The label is kept as written.
struct AeroRigidTrimVariableCard
{
    base::EntityId id = 0;
    std::string label; // up to eight characters

    [[nodiscard]] bool operator==(const AeroRigidTrimVariableCard&) const noexcept = default;
};

} // namespace oor::model

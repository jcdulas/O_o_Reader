#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One PSHELL card: the property of a shell element (membrane, bending, transverse shear, coupling).
//
// Card layout:
//   PSHELL  PID  MID1  T  MID2  12I/T**3  MID3  TS/T  NSM
//           Z1   Z2    MID4
//
// A blank material is stored as 0 (none); MID2 may also be -1. Z1 and Z2 stay empty when blank: they
// are then -T/2 and +T/2 of the thickness of each element, which may be its own
// (the Ti of a CQUAD4), so no number of the property stands for them.
struct ShellPropCard
{
    base::EntityId id = 0;
    base::EntityId membraneMatId = 0; // membrane
    double thickness = 0.0;
    base::EntityId bendingMatId = 0;       // bending
    double bendingStiffnessRatio = 1.0;    // 12I/T**3
    base::EntityId shearMatId = 0;         // transverse shear
    double shearThicknessRatio = 0.833333; // TS/T
    double nonStructMass = 0.0;
    std::optional<double> fiberDistance1; // the fibre distances for stress recovery; empty when blank
    std::optional<double> fiberDistance2;
    base::EntityId couplingMatId = 0; // membrane-bending coupling
};

} // namespace oor::model

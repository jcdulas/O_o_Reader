#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

namespace oor::model
{

// One PCONEAX card: the property of the conical shell elements (CCONEAX).
//
// Card layout:
//   PCONEAX  ID    MID1  T1    MID2  I     MID3  T2    NSM
//            Z1    Z2    PHI1  PHI2  PHI3  PHI4  PHI5  PHI6
//            PHI7  PHI8  PHI9  PHI10 PHI11 PHI12 PHI13 PHI14
//
// MID1, MID2 and MID3 are the materials for membrane, bending and transverse shear behaviour; a blank one is 0 and so
// is its blank T1, I or T2 (the format reference: a material and its thickness are both given or both zero or blank).
// Z1 and Z2, the fiber distances for stress recovery, have no default: they are NaN when blank. PHI1 to PHI14 (at most
// 14, in degrees) are the azimuthal coordinates of the stress recovery.
struct ConicalShellPropCard
{
    static constexpr double NO_VALUE = std::numeric_limits<double>::quiet_NaN();
    static constexpr std::size_t MAX_PHI = 14;

    base::EntityId id = 0;
    base::EntityId matId1 = 0;
    double thickness1 = 0.0; // membrane thickness
    base::EntityId matId2 = 0;
    double inertia = 0.0; // moment of inertia per unit width
    base::EntityId matId3 = 0;
    double thickness2 = 0.0; // transverse shear thickness
    double nonStructMass = 0.0;
    double fiberDistance1 = NO_VALUE;
    double fiberDistance2 = NO_VALUE;
    std::vector<double> phiAngle;
};

} // namespace oor::model

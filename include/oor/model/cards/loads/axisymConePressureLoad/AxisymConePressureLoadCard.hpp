#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PRESAX card: the static pressure on a conical shell element, the one between two rings over an azimuthal sector.
// The card needs an AXIC card in the deck, which is a rule between cards, and a LOAD Case Control command that selects
// its set.
//
// Card layout:
//   PRESAX  SID  P  RID1  RID2  PHI1  PHI2
//
// SID is a load set number, shared with the other loads (IdSpace::Group). P is the pressure and RID1 and RID2 the
// numbers of the two RINGAX cards (node references). PHI1 and PHI2 are the azimuthal angles in degrees, PHI2 above
// PHI1; for a loading over 360 degrees they are 0.0 and 360.0.
//
// The format gives no default for the angles. The card reads a blank PHI1 as 0.0 and a blank PHI2 as 360.0, which are
// the values for the whole circumference; the values are stored and always written, so that the card the writer gives
// is the one the format describes.
struct AxisymConePressureLoadCard
{
    static constexpr double DEFAULT_PHI_ANGLE1 = 0.0;
    static constexpr double DEFAULT_PHI_ANGLE2 = 360.0;

    base::EntityId id = 0;
    double pressure = 0.0;
    base::EntityId ringId1 = 0;
    base::EntityId ringId2 = 0;
    double phiAngle1 = DEFAULT_PHI_ANGLE1;
    double phiAngle2 = DEFAULT_PHI_ANGLE2;
};

} // namespace oor::model

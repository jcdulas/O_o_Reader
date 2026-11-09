#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// One PGAP card: the property of a gap element (CGAP).
//
// Card layout:
//   PGAP  PID  U0  F0  KA  KB  KT  MU1  MU2
//         TMAX MAR TRMIN
//
// KB, KT and MU2 have defaults that depend on other fields (1.0E-14 KA, MU1 KA and MU1), so they are stored resolved
// and always written. A blank KA is 1.0E8 (the format reference names no default for it).
struct GapPropCard
{
    static constexpr double DEFAULT_CLOSED_STIFFNESS = 1.0e8;
    static constexpr double DEFAULT_MAX_ADJUSTMENT_RATIO = 100.0;
    static constexpr double DEFAULT_MIN_PENETRATION_RATIO = 0.001;

    base::EntityId id = 0;
    double initialOpening = 0.0;                       // initial gap opening
    double preload = 0.0;                              // preload
    double closedStiffness = DEFAULT_CLOSED_STIFFNESS; // axial stiffness of the closed gap
    double openStiffness = 0.0;                        // axial stiffness of the open gap
    double transverseStiffness = 0.0;                  // transverse stiffness of the closed gap
    double frictionCoeff1 = 0.0; // coefficient of static friction (adaptive gap) or of friction in the y direction
    double frictionCoeff2 = 0.0; // coefficient of kinetic friction (adaptive gap) or of friction in the z direction
    double maxPenetration = 0.0; // maximum allowable penetration; a positive value activates the penalty adjustment
    // maximum allowable adjustment ratio of the penalty values
    double maxAdjustmentRatio = DEFAULT_MAX_ADJUSTMENT_RATIO;
    // fraction of TMAX that is the lower bound of the allowable penetration
    double minPenetrationRatio = DEFAULT_MIN_PENETRATION_RATIO;
};

} // namespace oor::model

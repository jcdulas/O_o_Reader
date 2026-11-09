#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/bushPropFreq/BushPropFreqCard.hpp"

#include <array>

namespace oor::model
{

// One PFASTT card: the frequency dependent and nonlinear properties of the PFAST with the same ID.
//
// Card layout: after the PID, groups that each start on a line with a flag in field 2 of the line, in any order, each
// at most once, then the optional fuse lines (the same fields as the PBUSHT ones, see BushPropFreqCard):
//   PFASTT  PID
//           "K"    TKID1   TKID2   TKID3   TKID4   TKID5   TKID6    stiffness against frequency
//           "GE"   TGEID                                           structural damping against frequency
//           "KN"   TKNID1  TKNID2  TKNID3  TKNID4  TKNID5  TKNID6   nonlinear force against deflection
//                  FDC     FUSE    DIR     OPTION  LOWER   UPPER
//                  FSR     LRGR
//
// The ID is the one of the PFAST it extends (the card has an ID space of its own: one PFASTT per PFAST). The tables are
// TABLEDi entries, 0 when blank. A group with only zeros is the same as a group that is left out.
struct FastenerPropFreqCard
{
    static constexpr double DEFAULT_FUSE_STIFFNESS_RETENTION = 1.0e-5;

    base::EntityId id = 0;
    std::array<base::EntityId, 6> stiffnessTableIds = {};      // TKID1 .. TKID6
    base::EntityId structuralDampingTableId = 0;               // TGEID
    std::array<base::EntityId, 6> nonlinearForceTableIds = {}; // TKNID1 .. TKNID6
    ForceDeflectionRule forceDeflectionRule = ForceDeflectionRule::None;
    int fuseMode =
        0; // 0: always active, 1: deactivated at failure (kept for post processing), 2: deactivated and removed
    int direction = 0; // the fuse directions: digits 1 to 6, 0 for any
    FuseOption fuseOption = FuseOption::RelativeDisplacement;
    double lowerBound = 0.0;                                          // lower failure bound
    double upperBound = 0.0;                                          // upper failure bound
    double fuseStiffnessRetention = DEFAULT_FUSE_STIFFNESS_RETENTION; // fuse stiffness retention factor
    int largeRotationEndA = 0;                                        // large rotation at end A
};

} // namespace oor::model

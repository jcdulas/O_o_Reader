#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>

namespace oor::model
{

// The force deflection curve rule FDC of a PBUSHT or PFASTT: a dependence between displacement components.
enum class ForceDeflectionRule : std::uint8_t
{
    None, // blank or NR: the six directions are independent
    Trxy, // radial dependence between the x and y displacements
    Trxz,
    Tryz,
    Ts // spherical dependence between x, y and z
};

// The OPTION field: the failure mode that the fuse uses.
enum class FuseOption : std::uint8_t
{
    RelativeDisplacement, // RELDIS (the default)
    UltimateLoad,         // ULTLD
    ActualUltimateLoad    // RULTLD
};

// One PBUSHT card: the frequency dependent or nonlinear properties of the PBUSH with the same ID.
//
// Card layout: after the PID, groups that each start on a line with a flag in field 2 of the line, in any order, each
// at most once, then the optional fuse lines:
//   PBUSHT  PID
//           "K"    TKID1   TKID2   TKID3   TKID4   TKID5   TKID6    stiffness against frequency
//           "B"    TBID1   TBID2   TBID3   TBID4   TBID5   TBID6    damping force per velocity against frequency
//           "GE"   TGEID1  TGEID2  TGEID3  TGEID4  TGEID5  TGEID6   structural damping against frequency
//           "KN"   TKNID1  TKNID2  TKNID3  TKNID4  TKNID5  TKNID6   nonlinear force against deflection
//                  FDC     FUSE    DIR     OPTION  LOWER   UPPER
//                  FSRS    LRGR
//
// The ID is the one of the PBUSH it extends (the card has an ID space of its own: one PBUSHT per PBUSH). The tables are
// TABLEDi entries, 0 when blank. when only TGEID1 is given, TGEID2 to TGEID6 blank, the one damping table
// is the one of all six directions; the card stores that resolved (all six), and a TGEID2 to TGEID6 given (a 0 too)
// makes the six independent. A group with only zeros is the same as a group that is left out. The fuse lines follow the
// last group line (their first field is in field 3 of the line, the flag field is blank).
struct BushPropFreqCard
{
    static constexpr double DEFAULT_FUSE_STIFFNESS_RETENTION = 1.0e-5;

    base::EntityId id = 0;
    std::array<base::EntityId, 6> stiffnessTableIds = {};         // TKID1 .. TKID6
    std::array<base::EntityId, 6> dampingTableIds = {};           // TBID1 .. TBID6
    std::array<base::EntityId, 6> structuralDampingTableIds = {}; // TGEID1 .. TGEID6
    std::array<base::EntityId, 6> nonlinearForceTableIds = {};    // TKNID1 .. TKNID6
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

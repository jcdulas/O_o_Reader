#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace oor::model
{

// One FORCUDS card (SOL 700): enforced velocities at grid points that a user subroutine computes. The FMS statement
// CONNECT SERVICE gives the group a name and the library of routines it holds. No other entry of the format reference
// has this name.
//
// Card layout: FORCUDS  LID  GROUP  UNAME                                    fields 1 to 3
//             +        G1  G2  THRU  G3  ...                                 fields 9 on, any number
// LID is the number of a set of loads: a TLOAD1 of type 2 (velocity) names it as its EXCITEID and leaves its TID blank.
// The card also gives each FORCUDS its own GROUP (two cards cannot share a group name, which a check of the
// whole model could verify). GROUP is required and UNAME is EXTVEL when blank (the routine for velocities on grid
// points, the only one the format lists); both are names of at most eight characters, kept in capitals. The grid points
// are IDs, a run "a THRU b" and "a THRU b BY n" (ContactRuns: a run is kept as a run, written on one line as the format
// asks, and goes upwards). A grid point that does not exist inside a THRU run is skipped, and the points
// must be given (the format marks them required). The fields between UNAME and the list are blank.
struct UserEnforcedVelocityCard
{
    static constexpr std::string_view DEFAULT_ROUTINE_NAME = "ExternalVelocity";

    base::EntityId setId = 0; // the set of loads, shared by the cards of the set
    std::string groupName;
    std::string userRoutineName{DEFAULT_ROUTINE_NAME};
    std::vector<ContactRun> grids;
};

} // namespace oor::model

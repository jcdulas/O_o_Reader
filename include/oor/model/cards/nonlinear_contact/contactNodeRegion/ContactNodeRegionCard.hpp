#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactRuns.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One BCGRID card: a contact region made of a list of grid points. The format reference has two entries with this name:
//
//  - SOLs 101, 103 to 112, 200 and 400: a contact region of the node-to-segment method, secondary only. Layout
//    BCGRID  BID  BPID  DIM  GID1  GID2  GID3  THRU  GID4  BY  GID5 ...  A BCONECT names it by its BID.
//  - SOL 700: a "cloud" of grid points for the secondary body of a BCTABLE, or for RCONN, BJOIN or WALL. Layout
//    BCGRID  CID  GID1  GID2 ...  (THRU and BY as above), and the grid points start in field 2.
//
// The two layouts have no keyword to tell them apart: field 3 is DIM (blank, 2D or 3D) in the first and a grid point in
// the second, so the card is read as the SOL 700 form when field 3 is a number. A SOL 700 card of a single grid point
// has a blank field 3 and reads as the first form.
struct ContactNodeRegionCard
{
    base::EntityId id = 0;           // BID (SOLs 101 and 400) or CID (SOL 700)
    base::EntityId bodyParamsId = 0; // the BCBDPRP of the body parameters; 0 when blank; 0 in the SOL 700 form
    std::string dimension = "ThreeDimensional"; // 2D or 3D; "3D" in the SOL 700 form
    bool sol700 = false;                        // the SOL 700 layout
    std::vector<ContactRun> grids;              // the grid points, single or in ranges
};

} // namespace oor::model

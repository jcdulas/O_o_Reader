#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace oor::model
{

// One NLOUTUD card: the user defined output of the explicit nonlinear analysis of SOL 700 (the user subroutine is
// attached with CONNECT SERVICE in the File Management Section).
//
// Card layout: NLOUTUD  NID  GROUP  UNAME  DTOUT  STEPS  OUTNAME
//                      EL1/G1  EL2/G2  EL3/G3  -etc.-
// The IDs on the continuation lines are element IDs with UNAME = EEXOUT and grid point IDs with UNAME = GEXOUT. At most
// one of DTOUT and STEPS is given.
struct NonlinearUserOutputCard
{
    base::EntityId id = 0;                         // NID: the unique output number
    std::string groupName;                         // GROUP: the name of the group of the CONNECT SERVICE statement
    std::string outputKind = "ElemExplicitOutput"; // UNAME: EEXOUT (elements) or GEXOUT (Lagrangian grid points)
    std::optional<double> timeInterval;            // DTOUT: the output time interval; blank when STEPS is used
    std::optional<std::int32_t> stepInterval;      // STEPS: the output step interval; blank when DTOUT is used
    std::string outputName;                // OUTNAME: the name of the output request (the output file names have it)
    std::vector<base::EntityId> entityIds; // the elements or grid points of the output
};

} // namespace oor::model

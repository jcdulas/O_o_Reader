#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oor::model
{

// One BCONUDS card: the user-defined contact service of SOL 400: it triggers a user subroutine for a contact body, with
// the integer, real and character data the subroutine reads (the GROUP of the FMS CONNECT SERVICE statement names the
// library). The subroutines the format reference lists are for BCBODY: MOTION (velocity of rigid surfaces), UFRIC
// (friction coefficients), SEPFOR and SEPSTR (separation force and stress) and UBSQUEAL (brake squeal).
//
// Card layout: BCONUDS  BID  BTYPE  GROUP  UNAME
//             (continuation)  INT   IDATA1  IDATA2 ...
//             (continuation)  REAL  RDATA1  RDATA2 ...
//             (continuation)  CHAR  CDATA1  CDATA2 ...
// each list on lines of its own, in any order, each at most once.
struct ContactUserServiceCard
{
    base::EntityId bodyId = 0;            // the BID of the BCBODY or BCBODY1
    std::string bodyKind = "ContactBody"; // the neutral name of the body card: ContactBody or ContactBody1
    std::string groupName;                // the group name of the CONNECT SERVICE statement (at most 8 characters)
    std::string userRoutineName;          // the user subroutine (at most 8 characters)
    std::vector<std::int32_t> integers;   // INT
    std::vector<double> reals;            // REAL
    std::vector<std::string> characters;  // CHAR
};

} // namespace oor::model

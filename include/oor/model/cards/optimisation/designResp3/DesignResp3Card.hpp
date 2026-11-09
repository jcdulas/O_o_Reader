#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/optimisation/DesignGroups.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One DRESP3 card: a response computed by a routine of the user, from design variables, constants and other responses.
//
// Card layout: DRESP3  ID  LABEL  GROUP  TYPE  REGION                                      fields 1 to 5
//                     DESVAR  DVID1 ...                                                 the lines after, from field 9
//                     DTABLE ...  DRESP1 ...  DNODE ...  DVPREL1 ...  ...  USRDATA  text  (`ResponseArguments.hpp`)
// GROUP names the group of external responses (the FMS CONNECT entry that gives the routine) and TYPE the
// response in that group. The arguments are those of a DRESP2 and the text of USRDATA (the constants passed
// to the routine; it may not name another DRESP3).
struct DesignResp3Card
{
    base::EntityId id = 0;
    std::string entryLabel;
    std::string groupName;           // upper case
    std::string type;                // upper case
    base::EntityId regionId = 0;     // 0: blank
    std::vector<DesignGroup> groups; // in the order of DESIGN_RESP3_GROUPS
};

} // namespace oor::model

#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// One WETLOAD card (SOL 400): the load set that stands for an external fluid-structure service, with the wetted surface
// the service loads.
//
// Card layout: WETLOAD  WLID  WSID  SERV_ID                                  fields 1 to 3
// WLID is the EXCITEID of a TLOAD1 (the TLOAD1 has to exist and be selected for the generator of the solver
// to build the load, though its values do not matter to the external program). WSID names a WETSURF, and SERV_ID the
// service, at most eight characters and kept as written: it is the name of the CONNECT SERVICE statement and of the
// FSICTRL of the service. Several WETLOAD cards may have one SERV_ID: the surfaces are merged and the WLID of the first
// replaces the others, so several cards may also share one WLID.
//
// The card is filed under the WLID as a set (IdSpace::Group, category Other: it is not a load that a LOAD or a DLOAD
// selects) and defines excitation IDs (CardTable::definesExcitationIds), as the FLOW, FORCUDS and ATBACC cards do, so
// the EXCITEID of a TLOAD1 that names it resolves.
struct WettedSurfaceLoadCard
{
    base::EntityId id = 0;
    base::EntityId wettedSurfaceId = 0;
    std::string serviceName;
};

} // namespace oor::model

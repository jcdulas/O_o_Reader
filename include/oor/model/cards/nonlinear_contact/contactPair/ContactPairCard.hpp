#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One BCONECT card: a contact pair (SOLs 101, 103 to 112, 200, 400 and 700): the bodies that touch (secondary, the
// "touching" bodies) and the bodies they touch (primary), with the parameter sets of the pair.
//
// Card layout:  BCONECT  ID  BCGPID  BCPPID  IDSCND  IDPRIM
//              (continuation)  SECNDRY  IDSEC1  IDSEC2 ...  IDSEC7 / IDSEC8  IDSEC9 ...
//              (continuation)  PRIMARY  IDPRIM1  IDPRIM2 ... IDPRIM7 / IDPRIM8  IDPRIM9 ...
// IDSCND and IDPRIM are the short form of one secondary and one primary body; the SECNDRY and PRIMARY lists are for
// several, and are ignored when the short field is there.
struct ContactPairCard
{
    base::EntityId id = 0;               // referenced by a BCTABL1 or by the BCONTACT command; 0 is the load case 0
    base::EntityId geomParamsId = 0;     // the BCONPRG of the geometric parameters; 0 when blank (defaults)
    base::EntityId physicalParamsId = 0; // the BCONPRP of the physical parameters; 0 when blank (defaults)
    base::EntityId secondaryBodyId = 0;  // the touching body (BCBODY1, BCSURF or BCGRID); 0 when blank
    base::EntityId primaryBodyId = 0;    // the touched body (BCBODY1 or BCSURF); 0 when blank
    std::vector<base::EntityId> secondaryBodies; // the SECNDRY list; a 0 stands for "all the elements" (SOL 700)
    std::vector<base::EntityId> primaryBodies;   // the PRIMARY list
};

} // namespace oor::model

#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nodes/ModuleIds.hpp"

#include <vector>

namespace oor::model
{

// One MDBCNCT card: a contact pair whose touching and touched bodies lie in different modules (SOLs 101, 103 to 112 and
// 400): general contact, glued, step glued or permanently glued. The format reference files it among the node entries;
// it is the multi-module counterpart of the BCONECT (the first fields, the short form and the SECNDRY and PRIMARY lists
// are the same), except that every body is a module and the ID of its BCBODY1 in that module.
//
// Card layout: MDBCNCT  ID  BCGPID  BCPPID  MODS  IDSCND  MODP  IDPRIM                 fields 1 to 7
//             (line)   SECNDRY  MODS1  IDSEC1  MODS2  IDSEC2  MODS3  IDSEC3  (blank)
//             (line)   (blank)  MODS4  IDSEC4  ...
//             (line)   PRIMARY  MODP1  IDPRIM1  MODP2  IDPRIM2  MODP3  IDPRIM3
// A list line has its keyword in the first field of the line (a blank on the lines that go on), then three pairs of
// module and ID, then a blank field. The two a common deck are read as drawn: the first leaves BCPPID and IDPRIM blank;
// the second has no short form and both lists. ID is referenced by an MDBCTB1 (or by BCONTACT when there is none) and
// may be 0. BCGPID and BCPPID are BCONPRG and BCONPRP numbers, blank for the defaults. IDSCND is the short form of one
// touching body and IDPRIM of one touched body; when IDSCND (IDPRIM) is blank the SECNDRY (PRIMARY) list is required,
// and the list is ignored when the short form is there: the card keeps both as the deck wrote them. A
// blank module is module 0, the main Bulk Data. The bodies are BCBODY1 cards of the modules; only those of the main
// Bulk Data can be found in the model.
struct ModuleContactPairCard
{
    base::EntityId id = 0;                // referenced by an MDBCTB1 or by BCONTACT; 0 is allowed
    base::EntityId geometricParamsId = 0; // the BCONPRG of the geometric parameters, 0 when blank
    base::EntityId physicalParamsId = 0;  // the BCONPRP of the physical parameters, 0 when blank
    base::EntityId secondaryModule = 0;   // the module of IDSCND, 0 for the main Bulk Data
    base::EntityId secondaryBodyId = 0;   // the touching body, 0 when blank
    base::EntityId primaryModule = 0;     // the module of IDPRIM, 0 for the main Bulk Data
    base::EntityId primaryBodyId = 0;     // the touched body, 0 when blank
    std::vector<ModuleId> secondary;      // the SECNDRY list: (MODSi, IDSECi)
    std::vector<ModuleId> primary;        // the PRIMARY list: (MODPi, IDPRIMi)
};

} // namespace oor::model

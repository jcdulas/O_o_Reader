#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nodes/ModuleIds.hpp"

#include <vector>

namespace oor::model
{

// One MDBCTB1 card: a contact table for bodies that lie in different modules (SOLs 101 and 400): the list of contact
// pairs that the Case Control BCONTACT command activates. The format reference files it among the node entries; it is
// the multi-module counterpart of the BCTABL1, and the two share the BCONTACT numbers.
//
// Card layout: MDBCTB1  BCID  MOD1  ID1  MOD2  ID2  MOD3  ID3  MOD4 / ID4  MOD5  ID5 ...
// fields 1, 2, 3 ... : after BCID the fields alternate module and ID, one pair after the other without regard to the
// lines (the pair MOD4, ID4 of the drawing is split over the two lines). BCID may be 0: the table of load case 0,
// so zero is a number here, not a blank. MODi is the module that defines the pair of IDi (blank or 0: the
// main Bulk Data) and IDi names a contact pair in it: the format calls it a BCONECT, and the format reference text says
// the table works through the MDBCNCT option; both belong to the category of the contact pairs. The THRU option is not
// supported and blank fields between the pairs are not allowed. Both a common deck are read as module
// and ID pairs; the first one has an odd number of values after BCID (its last module has no ID), which the card
// refuses.
struct ModuleContactTableCard
{
    base::EntityId id = 0;       // the number the BCONTACT command names; 0 is allowed
    std::vector<ModuleId> pairs; // (MODi, IDi), at least one
};

} // namespace oor::model

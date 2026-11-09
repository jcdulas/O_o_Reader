#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"

#include <vector>

namespace oor::model
{

// One BCONPRG card: the geometric contact parameters of touching bodies (SOLs 101 and 400, and the glued contact of the
// other solutions; SOL 700 has its own list of names, METHOD, ADAPT, SIDE, ... which the card reads too): the search
// order, glue options, interference fit, initial gap and the numerical parameters of the segment-to-segment method, as
// (name, value) pairs. The format lists the names. A BCONECT names the card by its
// BCGPID.
//
// Card layout: BCONPRG  BCGPID  (blank)  PARAM1  VAL1  PARAM2  VAL2  PARAM3  VAL3  / PARAM4  VAL4  PARAM5  VAL5 ...
// (the pairs go on in the continuation lines).
struct ContactPairGeomParamsCard
{
    base::EntityId id = 0;                // BCGPID
    std::vector<ContactParameter> params; // in the order of the card
};

} // namespace oor::model

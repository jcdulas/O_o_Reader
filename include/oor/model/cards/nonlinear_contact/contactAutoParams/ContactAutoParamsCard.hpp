#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"

#include <vector>

namespace oor::model
{

// One BCAUTOP card: the parameters of the automatic contact generation of SOL 400 (Case Control BCONTACT=AUTO), as
// (name, value) pairs. The primary parameters (CTYPE, DISTANCE, ESET, SEEDESET, SEEDGSET, PROPSET, RIGIDSET) say how
// the contact bodies and pairs are found, the secondary ones (BEAMCNT, BEAMCAP, DIMENS, EDGECNT, FTYPE, FRIC, IGNTHK,
// INISF, SELFCONT) refine it, and any parameter of BCPARA, BCONPRP, BCONPRG and BCBDPRP may be given too.
//
// Card layout: BCAUTOP  PID  (blank)  Param1  Value1  Param2  Value2  Param3  Value3  / Param4  Value4 ...
// (the pairs go on in the continuation lines). PID is reserved for future use (0).
struct ContactAutoParamsCard
{
    base::EntityId id = 0;
    std::vector<ContactParameter> params; // in the order of the card
};

} // namespace oor::model

#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"

#include <vector>

namespace oor::model
{

// One BCONPRP card: the physical contact parameters of touching bodies (SOLs 101 and 400: friction, breaking glue,
// thermal contact; SOL 700 has its own names, FK, EXP, FACT, TSTART, TEND, IADJ, DAMPING, which the card reads too), as
// (name, value) pairs. The format lists the names. A BCONECT names the card by its
// BCPPID.
//
// Card layout: BCONPRP  PID  (blank)  PARAM1  VAL1  PARAM2  VAL2  PARAM3  VAL3  / PARAM4  VAL4  PARAM5  VAL5 ...
// (the pairs go on in the continuation lines).
struct ContactPairPhysicalParamsCard
{
    base::EntityId id = 0;                // PID
    std::vector<ContactParameter> params; // in the order of the card
};

} // namespace oor::model

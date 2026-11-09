#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"

#include <vector>

namespace oor::model
{

// One BCBDPRP card: the parameters of a contact body (SOLs 101 and 400): friction, spline smoothing of the surface,
// check of the contact conditions, heat transfer coefficients, as (name, value) pairs. The format
// lists the names. A BCBODY1, BCSURF or BCGRID names the card by its BPID.
//
// Card layout: BCBDPRP  PID  (blank)  PARAM1  VAL1  PARAM2  VAL2  PARAM3  VAL3  / PARAM4  VAL4  PARAM5  VAL5 ...
// (the pairs go on in the continuation lines).
struct ContactBodyParamsCard
{
    base::EntityId id = 0;                // PID
    std::vector<ContactParameter> params; // in the order of the card
};

} // namespace oor::model

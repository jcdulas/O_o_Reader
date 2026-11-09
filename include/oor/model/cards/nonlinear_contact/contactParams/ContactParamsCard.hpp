#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"

#include <vector>

namespace oor::model
{

// One BCPARA card: the contact parameters of SOLs 101 and 400 (error tolerance, bias, separation, friction type,
// segment-to-segment method, ...), as (name, value) pairs. The format lists the names and what
// each holds.
//
// Card layout: BCPARA  ID  Param1  Value1  Param2  Value2  Param3  Value3  / Param4  Value4  Param5  Value5 ...
// (the pairs go on in the continuation lines). ID is the subcase the parameters belong to; blank or 0 means all of
// them, and the only value the solutions support is 0.
struct ContactParamsCard
{
    base::EntityId id = 0;
    std::vector<ContactParameter> params; // in the order of the card
};

} // namespace oor::model

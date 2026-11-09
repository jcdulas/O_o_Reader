#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"

#include <vector>

namespace oor::model
{

// One BCTPARM card: the control parameters of the contact algorithm of the contact set that has the same number (the
// older contact of SOLs 101, 103, 111, 112 and 401 of other implementations of the format), as (name, value) pairs. The
// format reference does not describe this card; it is read as below
// (`BCTPARM 108 PENTYP 1 INIPENE 0`).
//
// Layout: BCTPARM  CSID  Param1  Value1  Param2  Value2  Param3  Value3  (blank)    fields 1 to 7
//         (line 2)  Param4  Value4  Param5  Value5 ...                              the pairs go on from field 9
// CSID is the number of a BCTSET: the parameters apply to that contact set (and the card is looked up as a contact
// table). The names the card knows are PENN, PENT and CTOL (reals) and SHLTHK (an integer), and every other name is
// read as an integer or a real (PENTYP, INIPENE, FRICMOD, ...): every value is required. The name list is open, so the
// card accepts any name of up to eight letters and digits, and keeps the pairs in the order they were written.
struct ContactControlParamsCard
{
    base::EntityId contactSetId = 0;
    std::vector<ContactParameter> params; // in the order of the card
};

} // namespace oor::model

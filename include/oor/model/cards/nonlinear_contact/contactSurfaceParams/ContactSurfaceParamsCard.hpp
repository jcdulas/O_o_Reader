#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/nonlinear_contact/ContactParameters.hpp"

#include <vector>

namespace oor::model
{

// One BCTPARA card: the parameters of the surface-to-surface contact of the contact set that has the same number (the
// older contact of other implementations of the format: type of the contact, sides,
// normals, stiffness, tied contact, offsets, birth and death times), as (name, value) pairs. The format reference does
// not describe this card; it is read as below (`BCTPARA 100 TIED 1`).
//
// Layout: BCTPARA  CSID  Param1  Value1  Param2  Value2  Param3  Value3  (blank)    fields 1 to 7
//         (line 2)  Param4  Value4  Param5  Value5 ...                              the pairs go on from field 9
// CSID is the number of a BCTSET: the parameters apply to that contact set (and the card is looked up as a contact
// table). The names the card checks, with the values it accepts and, for a blank value, the default it gives (a blank
// value is kept blank here and writes blank): TYPE 0, 1 or 2 (0), NSIDE 1 or 2 (1), TBIRTH and TDEATH reals (0.0),
// INIPENE 0 to 3 (0), PDEPTH real (0.0), SEGNORM -1, 0 or 1 (0), OFFTYPE 0 to 2 (0), OFFSET real (0.0), TZPENE real
// (0.0), CSTIFF 0 or 1 (0), TIED 0 or 1 (0), TIEDTOL real (0.0) and EXTFAC from 1.0E-6 to 0.1 (0.001). Every other name
// (FRICMOD, FPARA1 to FPARA5, EPSN, EPST, CFACTOR1, PENETOL, NCMOD, TCMOD, RFORCE, LFORCE, RTPCHECK, RTPMAX, XTYPE,
// ...) is read as an integer or a real that may not be blank, so the card accepts any name of up to eight letters and
// digits and keeps the pairs in the order they were written.
struct ContactSurfaceParamsCard
{
    base::EntityId contactSetId = 0;
    std::vector<ContactParameter> params; // in the order of the card
};

} // namespace oor::model

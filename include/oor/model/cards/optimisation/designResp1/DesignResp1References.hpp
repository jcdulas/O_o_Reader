#pragma once

#include "oor/model/cards/optimisation/designResp1/DesignResp1Card.hpp"
#include "oor/model/references/ReferenceVisitor.hpp"

namespace oor::model
{

// Visits the numbers of a DRESP1 that name cards, and stores what the visitor returns. What a number names depends on
// the response type: the ATTi of DISP, SPCFORCE, the frequency, time and random responses are grid points;
// those of STRESS, STRAIN, FORCE, the composite and the frequency element responses are elements when PTYPE is ELEM and
// properties of the type PTYPE names otherwise; GPFORCE names elements; FRMASS and WMPID name properties by PTYPE; the
// ATT1 of FLUTTER is a set of modes and that of ERP a SET3; ATTA of GPFORCP is a grid point and that of WMPID a
// material; the PTYPE of a PSD response and the ATTB of an RMS response number a RANDPS. A number next to the word THRU
// is the end of a range, which may name nothing. The responses of types that the library does not type (the aeroelastic
// ones) and the card types it does not type name nothing that could be looked up.
void visitDesignResp1References(DesignResp1Card& card, ReferenceVisitor& visitor);

} // namespace oor::model

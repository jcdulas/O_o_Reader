#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// The control surface of one strip of a PAERO4: the three ratios to the chord of the strip.
struct AeroStripProp4Strip
{
    double hingeOffsetRatio = 0.0;  // DOCi: hinge distance behind the quarter chord / strip chord
    double controlChordRatio = 0.0; // CAOCi: control surface chord / strip chord (0.0: none)
    double gapRatio = 0.0;          // GAPOCi: control surface gap / strip chord (0.0: no slot flow)

    [[nodiscard]] bool operator==(const AeroStripProp4Strip&) const noexcept = default;
};

// One PAERO4 card: the properties of the strips of a CAERO4 panel (strip theory): the lift curve slope and lag function
// selections, and the control surface of every strip.
//
// Card layout: PAERO4  PID    CLA    LCLA    CIRC   LCIRC  DOC1   CAOC1  GAPOC1
//                     DOC2   CAOC2  GAPOC2  DOC3   CAOC3  GAPOC3 -etc.-
// The strips are triples of fields (DOCi, CAOCi, GAPOCi) that run on from field 6 without a gap, so the first triple
// ends the first line and the next ones fill the continuation lines eight fields to a line (field 9 is DOC2, field 14
// GAPOC3, field 15 DOC4, and a triple may be split between two lines). The format gives the pattern but does not say
// how a triple that does not fit the end of a line is laid out; this card splits it between
// the two lines. One triple per strip of the CAERO4 that names the property, in the order of the strips (the count is a
// rule between the two cards, not of this one). Blank fields inside the list are not allowed and at least
// one strip is required (three fields per strip).
//
// CLA selects the lift curve slope: 0 (default) is the theoretical 2*pi, -1 corrects data given for one Mach number
// with the Prandtl-Glauert rule, +1 takes the data for every Mach number of the MKAEROi cards as they are. Both data
// forms are in the AEFACT LCLA, which is required (> 0) when CLA is not 0. CIRC selects the lag function: 0 (default)
// is Theodorsen's C(k), 1 to 3 an approximation by that many exponential terms, whose coefficients are in the AEFACT
// LCIRC, required (> 0) when CIRC is not 0; CIRC must be 0 when CLA is not 0.
//
// What a blank selector means for its list is not enforced in the other direction: an LCLA (LCIRC) given while CLA
// (CIRC) is 0 names a list that is not used, as CAERO1 keeps an LSPAN next to an NSPAN. The format wants LCLA to be 0
// when CLA is 0, but a card that refused it could not be edited one field at a time in Python (CLA and LCLA would each
// need the other one first). A reference check can report an unused list if it wants to.
//
// CAOCi = 0.0 means the strip has no control surface and GAPOCi = 0.0 no slot flow; a gap between 0 and 0.01 is raised
// to 0.01 by the aerodynamic solver, the card keeps the value as it was given. The three ratios are not
// negative.
struct AeroStripProp4Card
{
    base::EntityId id = 0;
    std::int32_t liftSlopeOption = 0;     // -1, 0 or 1
    base::EntityId liftSlopeListId = 0;   // an AEFACT (0 when blank)
    std::int32_t circulationOption = 0;   // 0, 1, 2 or 3
    base::EntityId circulationListId = 0; // an AEFACT (0 when blank)
    std::vector<AeroStripProp4Strip> strips;
};

} // namespace oor::model

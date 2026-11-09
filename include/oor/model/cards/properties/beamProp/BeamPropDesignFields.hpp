#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a PBEAM that a DVPREL1 or DVPREL2 drives. A field of a station is named with the station: A, A(A) (the
// same: end A), A(B) (the station at X/XB = 1.0) and A(1), A(2), ... (the stations after end A in the order the card
// gives them, the one at X/XB = 1.0 included: the solver lists a PBEAM of two stations as I1(A), I1(1) and I1(B), with
// I1(1) the value of end B); the fields of a station are A, I1, I2, I12, J, NSM and C1 to F2. The fields of the whole
// beam are K1, K2, S1, S2, NSI(A), NSI(B), CW(A), CW(B), M1(A), M2(A), M1(B), M2(B), N1(A), N2(A), N1(B) and N2(B). By
// positive FID only the fields of end A: A 4, I1 5, I2 6, I12 7, J 8, NSM 9 and C1 to F2 12 to 19 (the positions after
// end A depend on the stress lines the card has: name them). By negative FID the word of the element property table:
// five words, then sixteen for each station (in order of X/XB), whose area is the third, so the area of end A is FID -8
// (the public SOL 200 decks drive the PBEAM with it).
[[nodiscard]] const DesignableCard& beamPropDesignFields();

} // namespace oor::model

#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a PBAR that a DVPREL1 or DVPREL2 drives: A, I1, I2, J, NSM, C1 to F2, K1, K2 and I12. By positive FID
// the position on the card (A 4, I1 5, I2 6, J 7, NSM 8, C1 12 to F2 19, K1 22, K2 23, I12 24); by negative FID the
// word of the element property table (A -3, the common decks, I1 -4, I2 -5, J -6, NSM -7, C1 -9 to F2 -16, K1
// -17, K2 -18, I12 -19).
[[nodiscard]] const DesignableCard& barPropDesignFields();

} // namespace oor::model

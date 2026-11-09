#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a PELAS property that a DVPREL1 or DVPREL2 drives: K, GE and S, named K1, GE1 and S1 for the first
// property of a card and K2, GE2 and S2 for the second (a PELAS card with two properties is two rows here, so the
// property named by its PID takes either spelling), or by position 3, 4 and 5 (7, 8 and 9 for the second property).
[[nodiscard]] const DesignableCard& springPropDesignFields();

} // namespace oor::model

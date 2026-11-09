#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a CTRIA3 that a DVCREL1 or DVCREL2 drives, by name: ZOFFS and the corner thicknesses T1, T2 and T3. A
// corner thickness the card leaves blank is -1 here (the property thickness is used); a relation gives it a value.
[[nodiscard]] const DesignableCard& shellTria3DesignFields();

} // namespace oor::model

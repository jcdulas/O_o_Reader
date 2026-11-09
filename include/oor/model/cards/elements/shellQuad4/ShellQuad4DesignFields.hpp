#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a CQUAD4 that a DVCREL1 or DVCREL2 drives, by name: ZOFFS and the corner thicknesses T1, T2, T3 and T4.
// A corner thickness the card leaves blank is -1 here (the property thickness is used); a relation gives it a value.
[[nodiscard]] const DesignableCard& shellQuad4DesignFields();

} // namespace oor::model

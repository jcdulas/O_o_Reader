#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a PTUBE that a DVPREL1 or DVPREL2 drives: OD (field 4), T (5), NSM (6) and OD2 (7).
[[nodiscard]] const DesignableCard& tubePropDesignFields();

} // namespace oor::model

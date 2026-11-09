#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a PSHELL that a DVPREL1 or DVPREL2 drives: T, TS/T, NSM, Z1 and Z2 by name; by position T (4), 12I/T**3
// (6), TS/T (8), NSM (9), Z1 (12) and Z2 (13). The name of 12I/T**3 begins with an integer, so the format reference
// lets it be given by position only.
[[nodiscard]] const DesignableCard& shellPropDesignFields();

} // namespace oor::model

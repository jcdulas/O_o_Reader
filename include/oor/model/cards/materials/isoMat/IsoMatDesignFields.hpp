#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a MAT1 that a DVMREL1 or DVMREL2 drives, by the name the format reference gives them (a DVMREL names a
// field by MPNAME and never by position): E, G, NU, RHO, A, TREF, GE, ST, SC and SS. A DTABLE2 entry reads them by the
// same names.
[[nodiscard]] const DesignableCard& isoMatDesignFields();

} // namespace oor::model

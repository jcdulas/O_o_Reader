#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a CELAS2 that a DVCREL1 or DVCREL2 drives, by name: K, GE and S.
[[nodiscard]] const DesignableCard& springDofInlineDesignFields();

} // namespace oor::model

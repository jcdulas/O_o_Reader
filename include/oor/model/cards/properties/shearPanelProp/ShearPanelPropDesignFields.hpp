#pragma once

#include "oor/model/design/DesignableCard.hpp"

namespace oor::model
{

// The fields of a PSHEAR that a DVPREL1 or DVPREL2 drives: T (field 4), NSM (5), F1 (6) and F2 (7).
[[nodiscard]] const DesignableCard& shearPanelPropDesignFields();

} // namespace oor::model

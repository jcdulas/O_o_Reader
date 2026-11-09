#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>

namespace oor::model
{

// One DVPSURF card: the setting of a control surface in a trim case as a design variable times a coefficient.
//
// Card layout: DVPSURF  ID  AELABEL  TRIMID  DVID  COEF
//   control surface setting = COEF * (design variable DVID)
// AELABEL is the LABEL of the AESURF that is designed: a name, which the card keeps as written and which the checks and
// the design match without regard to the case, not an ID, so nothing visits it (the AESURF must also appear on the TRIM
// that TRIMID names, a rule between cards). TRIMID is the TRIM (or TRIM2) of the single subcase the setting
// is for and DVID a DESVAR that no other designed property uses. COEF has no default. The ID is
// unique among the DVPSURF cards (IdSpace::PerCardType).
struct DesignControlSurfaceCard
{
    base::EntityId id = 0;
    std::string controlSurfaceLabel; // 1 to 8 characters, as written
    base::EntityId trimId = 0;
    base::EntityId designVarId = 0;
    double coefficient = 0.0;

    [[nodiscard]] bool operator==(const DesignControlSurfaceCard&) const noexcept = default;
};

} // namespace oor::model

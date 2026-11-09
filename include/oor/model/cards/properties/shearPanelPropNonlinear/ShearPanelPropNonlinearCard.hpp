#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/PropertyExtensionWords.hpp"

#include <array>

namespace oor::model
{

// The words the C4 line of a PSHEARN allows: a shear panel has the membrane behaviour and one
// integration scheme.
inline constexpr std::array<ElementBehavior, 1> SHEAR_PANEL_PROP_NONLINEAR_BEHAVIORS{ElementBehavior::Shear};
inline constexpr std::array<IntegrationScheme, 1> SHEAR_PANEL_PROP_NONLINEAR_INTEGRATIONS{IntegrationScheme::Linear};

// The keyword lines of the card, in the order of `ShearPanelPropNonlinearCard::groups`: only C4 (an element with four
// corner grids).
inline constexpr std::array<BehaviorRules, 1> SHEAR_PANEL_PROP_NONLINEAR_GROUPS{
    BehaviorRules{.keyword = "FourNodeElems",
                  .behaviorName = "behavior4",
                  .integrationName = "integration4",
                  .behaviors = SHEAR_PANEL_PROP_NONLINEAR_BEHAVIORS,
                  .integrations = SHEAR_PANEL_PROP_NONLINEAR_INTEGRATIONS}};

// One PSHEARN card (SOL 400): the nonlinear extension of the PSHEAR with the same PID. Elements that name the PSHEAR
// get the properties of this card too; the PSHEARN is honoured in SOL 400 only, with large membrane rotation and no
// stringer effectiveness.
//
// Card layout:
//   PSHEARN PID    MID     (blank)  (blank)  ANAL
//           "C4"   BEH4    INT4                   fields 9 to 11
//
// The ID is the one of the PSHEAR it extends, so it is in the space of its own card type (one PSHEARN for each PSHEAR)
// and is a reference to a property. A blank MID, or 0, keeps the material of the PSHEAR; a MID above 0 replaces it.
// ANAL is IS (the default), the only analysis a shear panel has: IH and ISH are refused.
//
// The C4 line may be missing: its words then take their defaults, so `groups` always holds them resolved and the line
// is written only when a word is not the default.
struct ShearPanelPropNonlinearCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0; // 0: the material of the PSHEAR
    PropertyAnalysis analysisType = PropertyAnalysis::Structural;
    std::array<BehaviorGroup, 1> groups = defaultGroups(SHEAR_PANEL_PROP_NONLINEAR_GROUPS);
};

} // namespace oor::model

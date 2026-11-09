#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/PropertyExtensionWords.hpp"

#include <array>

namespace oor::model
{

// The words the C2 line of a PRODN1 allows: a rod has one behaviour and one integration scheme,
// for the structure and for the heat.
inline constexpr std::array<ElementBehavior, 1> ROD_PROP_NONLINEAR_BEHAVIORS{ElementBehavior::Rod};
inline constexpr std::array<IntegrationScheme, 1> ROD_PROP_NONLINEAR_INTEGRATIONS{IntegrationScheme::Linear};

// The keyword lines of the card, in the order of `RodPropNonlinearCard::groups`: only C2 (an element with two end
// grids).
inline constexpr std::array<BehaviorRules, 1> ROD_PROP_NONLINEAR_GROUPS{
    BehaviorRules{.keyword = "TwoNodeElems",
                  .behaviorName = "behavior2",
                  .integrationName = "integration2",
                  .heatBehaviorName = "heatBehavior2",
                  .heatIntegrationName = "heatIntegration2",
                  .behaviors = ROD_PROP_NONLINEAR_BEHAVIORS,
                  .integrations = ROD_PROP_NONLINEAR_INTEGRATIONS,
                  .heatBehaviors = ROD_PROP_NONLINEAR_BEHAVIORS,
                  .heatIntegrations = ROD_PROP_NONLINEAR_INTEGRATIONS}};

// One PRODN1 card (SOL 400): the nonlinear extension of the PROD with the same PID. Elements that name the PROD get the
// properties of this card too; the PRODN1 is honoured in SOL 400 only, and the J, C and NSM of the PROD are ignored.
//
// Card layout:
//   PRODN1  PID    MID     (blank)  (blank)  ANAL
//           "C2"   BEH2    INT2     BEH2H    INT2H          fields 9 to 13
//
// The ID is the one of the PROD it extends, so it is in the space of its own card type (one PRODN1 for each PROD) and
// is a reference to a property. A blank MID, or 0, keeps the material of the PROD; a MID above 0 replaces it (it may
// name a MAT1 or, for the heat analysis, a MAT4 or MAT5). ANAL says which elements the card is for: IS (structural), IH
// (heat) or ISH, the default.
//
// The C2 line may be missing: its words then take their defaults, so `groups` always holds them resolved and the line
// is written only when a word is not the default.
struct RodPropNonlinearCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0; // 0: the material of the PROD
    PropertyAnalysis analysisType = PropertyAnalysis::StructuralAndHeat;
    std::array<BehaviorGroup, 1> groups = defaultGroups(ROD_PROP_NONLINEAR_GROUPS);
};

} // namespace oor::model

#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/PropertyExtensionWords.hpp"

#include <array>

namespace oor::model
{

// The words the C2 line of a PBARN1 allows: the one behaviour of a bar and the three integration
// schemes, the first of which, linear/cubic, is the default.
inline constexpr std::array<ElementBehavior, 1> BAR_PROP_NONLINEAR_BEHAVIORS{ElementBehavior::Bar};
inline constexpr std::array<IntegrationScheme, 3> BAR_PROP_NONLINEAR_INTEGRATIONS{
    IntegrationScheme::LinearCubic, IntegrationScheme::LinearCubicClosed, IntegrationScheme::LinearShear};

// The keyword lines of the card, in the order of `BarPropNonlinearCard::groups`: only C2 (an element with two end
// grids).
inline constexpr std::array<BehaviorRules, 1> BAR_PROP_NONLINEAR_GROUPS{
    BehaviorRules{.keyword = "TwoNodeElems",
                  .behaviorName = "behavior2",
                  .integrationName = "integration2",
                  .behaviors = BAR_PROP_NONLINEAR_BEHAVIORS,
                  .integrations = BAR_PROP_NONLINEAR_INTEGRATIONS}};

// One PBARN1 card (SOL 400): the nonlinear extension of the PBAR or PBARL with the same PID. Elements that name the
// PBAR or PBARL get the properties of this card too; the PBARN1 is honoured in SOL 400 only.
//
// Card layout:
//   PBARN1  PID    MID     (blank)  SECT
//           "C2"   BEH2    INT2                   fields 9 to 11
//
// The ID is the one of the PBAR or PBARL it extends, so it is in the space of its own card type (one PBARN1 for each
// PBAR or PBARL) and is a reference to a property. A blank MID, or 0, keeps the material of the PBAR or PBARL; a MID
// above 0 replaces it (it may name a MAT1 and the nonlinear laws that go with it by the same ID).
//
// The C2 line may be missing: its words then take their defaults, so `groups` always holds them resolved and the line
// is written only when a word is not the default.
struct BarPropNonlinearCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0; // 0: the material of the PBAR or PBARL
    SectionIntegration sectionIntegration = SectionIntegration::Smeared;
    std::array<BehaviorGroup, 1> groups = defaultGroups(BAR_PROP_NONLINEAR_GROUPS);
};

} // namespace oor::model

#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/PropertyExtensionWords.hpp"

#include <array>

namespace oor::model
{

// The words the C2 line of a PBEMN1 allows: the one behaviour of a beam and the four integration
// schemes (closed and open thin-walled sections have their own), the first of which, linear/cubic, is the default.
inline constexpr std::array<ElementBehavior, 1> BEAM_PROP_NONLINEAR_BEHAVIORS{ElementBehavior::Beam};
inline constexpr std::array<IntegrationScheme, 4> BEAM_PROP_NONLINEAR_INTEGRATIONS{
    IntegrationScheme::LinearCubic, IntegrationScheme::LinearCubicClosed, IntegrationScheme::LinearCubicOpen,
    IntegrationScheme::LinearShear};

// The keyword lines of the card, in the order of `BeamPropNonlinearCard::groups`: only C2 (an element with two end
// grids).
inline constexpr std::array<BehaviorRules, 1> BEAM_PROP_NONLINEAR_GROUPS{
    BehaviorRules{.keyword = "TwoNodeElems",
                  .behaviorName = "behavior2",
                  .integrationName = "integration2",
                  .behaviors = BEAM_PROP_NONLINEAR_BEHAVIORS,
                  .integrations = BEAM_PROP_NONLINEAR_INTEGRATIONS}};

// One PBEMN1 card (SOL 400): the nonlinear extension of the PBEAM or PBEAML with the same PID. Elements that name the
// PBEAM or PBEAML get the properties of this card too; the PBEMN1 is honoured in SOL 400 only, and the taper of the
// CBEAM is ignored (only the section at end A is used).
//
// Card layout:
//   PBEMN1  PID    MID     (blank)  SECT
//           "C2"   BEH2    INT2                   fields 9 to 11
//
// The ID is the one of the PBEAM or PBEAML it extends, so it is in the space of its own card type (one PBEMN1 for each
// PBEAM or PBEAML) and is a reference to a property. A blank MID, or 0, keeps the material of the PBEAM or PBEAML; a
// MID above 0 replaces it.
//
// The C2 line may be missing: its words then take their defaults, so `groups` always holds them resolved and the line
// is written only when a word is not the default.
struct BeamPropNonlinearCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0; // 0: the material of the PBEAM or PBEAML
    SectionIntegration sectionIntegration = SectionIntegration::Smeared;
    std::array<BehaviorGroup, 1> groups = defaultGroups(BEAM_PROP_NONLINEAR_GROUPS);
};

} // namespace oor::model

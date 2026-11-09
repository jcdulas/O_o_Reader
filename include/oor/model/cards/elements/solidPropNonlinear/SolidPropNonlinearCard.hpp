#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/PropertyExtensionWords.hpp"

#include <array>

namespace oor::model
{

// The words the keyword lines of a PSLDN1 allow. A line is for the elements with
// the corner grids (and midside grids) its keyword says; the integration code goes with the element type and the number
// of grids (L: the 8-node hexahedron, the 4-node tetrahedron, the 6-node pentahedron and the 5-node pyramid; Q: the
// 20-node hexahedron, the 10-node tetrahedron, the 15-node pentahedron and the 13-node pyramid). The first word of each
// list is the one the format reference underlines, the default of a line that is not in the card.
inline constexpr std::array<ElementBehavior, 1> SOLID_PROP_NONLINEAR_SOLID{ElementBehavior::Solid};
inline constexpr std::array<ElementBehavior, 2> SOLID_PROP_NONLINEAR_C4_BEHAVIORS{ElementBehavior::Solid,
                                                                                  ElementBehavior::IncompressibleSolid};
inline constexpr std::array<ElementBehavior, 2> SOLID_PROP_NONLINEAR_C8_BEHAVIORS{ElementBehavior::Solid,
                                                                                  ElementBehavior::SolidComposite};
inline constexpr std::array<IntegrationScheme, 1> SOLID_PROP_NONLINEAR_LINEAR{IntegrationScheme::Linear};
inline constexpr std::array<IntegrationScheme, 1> SOLID_PROP_NONLINEAR_QUADRATIC{IntegrationScheme::Quadratic};
inline constexpr std::array<IntegrationScheme, 2> SOLID_PROP_NONLINEAR_C5_INTEGRATIONS{
    IntegrationScheme::Linear, IntegrationScheme::LinearReducedHourglass};
inline constexpr std::array<IntegrationScheme, 3> SOLID_PROP_NONLINEAR_C8_INTEGRATIONS{
    IntegrationScheme::Linear, IntegrationScheme::LinearReducedHourglass, IntegrationScheme::AssumedStrain};
// The table lists LRIH with the 10-node tetrahedron (it is printed as written).
inline constexpr std::array<IntegrationScheme, 2> SOLID_PROP_NONLINEAR_C10_INTEGRATIONS{
    IntegrationScheme::Quadratic, IntegrationScheme::LinearReducedHourglass};
inline constexpr std::array<IntegrationScheme, 2> SOLID_PROP_NONLINEAR_C20_INTEGRATIONS{
    IntegrationScheme::Quadratic, IntegrationScheme::QuadraticReduced};

// The keyword lines of the card, in the order of `SolidPropNonlinearCard::groups`.
inline constexpr std::size_t SOLID_PROP_NONLINEAR_C4 = 0;
inline constexpr std::size_t SOLID_PROP_NONLINEAR_C5 = 1;
inline constexpr std::size_t SOLID_PROP_NONLINEAR_C6 = 2;
inline constexpr std::size_t SOLID_PROP_NONLINEAR_C8 = 3;
inline constexpr std::size_t SOLID_PROP_NONLINEAR_C10 = 4;
inline constexpr std::size_t SOLID_PROP_NONLINEAR_C13 = 5;
inline constexpr std::size_t SOLID_PROP_NONLINEAR_C15 = 6;
inline constexpr std::size_t SOLID_PROP_NONLINEAR_C20 = 7;

inline constexpr std::array<BehaviorRules, 8> SOLID_PROP_NONLINEAR_GROUPS{
    BehaviorRules{.keyword = "FourNodeElems",
                  .behaviorName = "behavior4",
                  .integrationName = "integration4",
                  .heatBehaviorName = "heatBehavior4",
                  .heatIntegrationName = "heatIntegration4",
                  .behaviors = SOLID_PROP_NONLINEAR_C4_BEHAVIORS,
                  .integrations = SOLID_PROP_NONLINEAR_LINEAR,
                  .heatBehaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .heatIntegrations = SOLID_PROP_NONLINEAR_LINEAR},
    BehaviorRules{.keyword = "FiveNodeElems",
                  .behaviorName = "behavior5",
                  .integrationName = "integration5",
                  .heatBehaviorName = "heatBehavior5",
                  .heatIntegrationName = "heatIntegration5",
                  .behaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .integrations = SOLID_PROP_NONLINEAR_C5_INTEGRATIONS,
                  .heatBehaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .heatIntegrations = SOLID_PROP_NONLINEAR_LINEAR},
    BehaviorRules{.keyword = "SixNodeElems",
                  .behaviorName = "behavior6",
                  .integrationName = "integration6",
                  .heatBehaviorName = "heatBehavior6",
                  .heatIntegrationName = "heatIntegration6",
                  .behaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .integrations = SOLID_PROP_NONLINEAR_LINEAR,
                  .heatBehaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .heatIntegrations = SOLID_PROP_NONLINEAR_LINEAR},
    BehaviorRules{.keyword = "EightNodeElems",
                  .behaviorName = "behavior8",
                  .integrationName = "integration8",
                  .heatBehaviorName = "heatBehavior8",
                  .heatIntegrationName = "heatIntegration8",
                  .behaviors = SOLID_PROP_NONLINEAR_C8_BEHAVIORS,
                  .integrations = SOLID_PROP_NONLINEAR_C8_INTEGRATIONS,
                  .heatBehaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .heatIntegrations = SOLID_PROP_NONLINEAR_LINEAR},
    BehaviorRules{.keyword = "TenNodeElems",
                  .behaviorName = "behavior10",
                  .integrationName = "integration10",
                  .heatBehaviorName = "heatBehavior10",
                  .heatIntegrationName = "heatIntegration10",
                  .behaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .integrations = SOLID_PROP_NONLINEAR_C10_INTEGRATIONS,
                  .heatBehaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .heatIntegrations = SOLID_PROP_NONLINEAR_QUADRATIC},
    BehaviorRules{.keyword = "ThirteenNodeElems",
                  .behaviorName = "behavior13",
                  .integrationName = "integration13",
                  .heatBehaviorName = "heatBehavior13",
                  .heatIntegrationName = "heatIntegration13",
                  .behaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .integrations = SOLID_PROP_NONLINEAR_QUADRATIC,
                  .heatBehaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .heatIntegrations = SOLID_PROP_NONLINEAR_QUADRATIC},
    BehaviorRules{.keyword = "FifteenNodeElems",
                  .behaviorName = "behavior15",
                  .integrationName = "integration15",
                  .heatBehaviorName = "heatBehavior15",
                  .heatIntegrationName = "heatIntegration15",
                  .behaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .integrations = SOLID_PROP_NONLINEAR_QUADRATIC,
                  .heatBehaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .heatIntegrations = SOLID_PROP_NONLINEAR_QUADRATIC},
    BehaviorRules{.keyword = "TwentyNodeElems",
                  .behaviorName = "behavior20",
                  .integrationName = "integration20",
                  .heatBehaviorName = "heatBehavior20",
                  .heatIntegrationName = "heatIntegration20",
                  .behaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .integrations = SOLID_PROP_NONLINEAR_C20_INTEGRATIONS,
                  .heatBehaviors = SOLID_PROP_NONLINEAR_SOLID,
                  .heatIntegrations = SOLID_PROP_NONLINEAR_QUADRATIC}};

// One PSLDN1 card (SOL 400): the nonlinear extension of the PSOLID with the same PID, for the solid elements (CHEXA,
// CPENTA, CPYRAM and CTETRA) that name the PSOLID. The PSLDN1 is honoured in SOL 400 only.
//
// Card layout:
//   PSLDN1  PID    MID     DIRECT   (blank)  ANAL
//           "C4"   BEH4    INT4     BEH4H    INT4H          one line for each keyword,
//           "C5"   BEH5    INT5     BEH5H    INT5H          fields 9 to 13 of its line
//           ...    (C6, C8, C10, C13, C15, C20)
//
// The ID is the one of the PSOLID it extends, so it is in the space of its own card type (one PSLDN1 for each PSOLID)
// and is a reference to a property. A blank MID, or 0, keeps the material of the PSOLID; a MID above 0 replaces it (a
// MAT1, MAT9, MATORT, MATHE or MATG, and a MAT4 or MAT5 for the heat analysis). DIRECT (1, 2 or 3, default 1) is the
// direction of the layers of a BEH8 of SLCOMP; the solid shell (SLCOMP with the integration ASTN) takes DIRECT 1 only.
// ANAL is IS, IH or ISH, the default.
//
// The keyword lines may come in any order or not at all; a line that is not in the card gets the
// defaults of its keyword, so `groups` always holds the eight lines resolved and a line is written only when a word of
// it is not the default. The words of the C8 line go together: SLCOMP takes L (the 3D gasket) or ASTN (the solid
// shell), SOLID takes L or LRIH.
struct SolidPropNonlinearCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0; // 0: the material of the PSOLID
    int direction = 1;
    PropertyAnalysis analysisType = PropertyAnalysis::StructuralAndHeat;
    std::array<BehaviorGroup, 8> groups = defaultGroups(SOLID_PROP_NONLINEAR_GROUPS);
};

} // namespace oor::model

#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/PropertyExtensionWords.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

// The words the keyword lines of a PCOMPLS allow: a layered solid is one behaviour, SLCOMP, for
// the structure and for the heat; the integration code goes with the number of grids (L and ASTN for the 8-node
// hexahedron, Q for the 20-node one). The first word of each list is the one the format reference underlines, the
// default of a line that is not in the card.
inline constexpr std::array<ElementBehavior, 1> COMP_SOLID_PROP_NONLINEAR_BEHAVIORS{ElementBehavior::SolidComposite};
inline constexpr std::array<IntegrationScheme, 2> COMP_SOLID_PROP_NONLINEAR_C8_INTEGRATIONS{
    IntegrationScheme::Linear, IntegrationScheme::AssumedStrain};
inline constexpr std::array<IntegrationScheme, 1> COMP_SOLID_PROP_NONLINEAR_LINEAR{IntegrationScheme::Linear};
inline constexpr std::array<IntegrationScheme, 1> COMP_SOLID_PROP_NONLINEAR_QUADRATIC{IntegrationScheme::Quadratic};

// The keyword lines of the card, in the order of `CompSolidPropNonlinearCard::groups`: C8 (eight corner grids) and C20
// (eight corner grids and twelve midside grids).
inline constexpr std::size_t COMP_SOLID_PROP_NONLINEAR_C8 = 0;
inline constexpr std::size_t COMP_SOLID_PROP_NONLINEAR_C20 = 1;

inline constexpr std::array<BehaviorRules, 2> COMP_SOLID_PROP_NONLINEAR_GROUPS{
    BehaviorRules{.keyword = "EightNodeElems",
                  .behaviorName = "behavior8",
                  .integrationName = "integration8",
                  .heatBehaviorName = "heatBehavior8",
                  .heatIntegrationName = "heatIntegration8",
                  .behaviors = COMP_SOLID_PROP_NONLINEAR_BEHAVIORS,
                  .integrations = COMP_SOLID_PROP_NONLINEAR_C8_INTEGRATIONS,
                  .heatBehaviors = COMP_SOLID_PROP_NONLINEAR_BEHAVIORS,
                  .heatIntegrations = COMP_SOLID_PROP_NONLINEAR_LINEAR},
    BehaviorRules{.keyword = "TwentyNodeElems",
                  .behaviorName = "behavior20",
                  .integrationName = "integration20",
                  .heatBehaviorName = "heatBehavior20",
                  .heatIntegrationName = "heatIntegration20",
                  .behaviors = COMP_SOLID_PROP_NONLINEAR_BEHAVIORS,
                  .integrations = COMP_SOLID_PROP_NONLINEAR_QUADRATIC,
                  .heatBehaviors = COMP_SOLID_PROP_NONLINEAR_BEHAVIORS,
                  .heatIntegrations = COMP_SOLID_PROP_NONLINEAR_QUADRATIC}};

// One ply of a PCOMPLS: the global ply ID, the material, the thickness (a fraction of the element thickness when DIRECT
// is positive, the actual thickness when it is negative) and the orientation angle.
struct CompSolidPropNonlinearPly
{
    base::EntityId globalId = 0; // IDi, unique in the card
    base::EntityId matId = 0;
    double thickness = 0.0;
    double thetaInDegrees = 0.0;

    [[nodiscard]] bool operator==(const CompSolidPropNonlinearPly&) const noexcept = default;
};

// One PCOMPLS card (SOL 400, the linear solutions SOL 101 to SOL 112, and SOL 200 for the analysis only): the property
// of a layered solid composite, which a CHEXA names. Its plies have global ply IDs, shared with the PCOMPG and PLCOMP
// plies.
//
// Card layout (the lines after the first may come as shown or, for the two keyword lines, in any order):
//   PCOMPLS  PID    DIRECT  CORDM   SB       ANAL
//            "C8"   BEH8    INT8    BEH8H    INT8H
//            "C20"  BEH20   INT20   BEH20H   INT20H
//            ID1    MID1    T1      THETA1                one line for each ply, bottom ply first
//            ID2    MID2    T2      THETA2
//
// DIRECT is 1, 2 or 3 (default 1) for the direction of the layers, negative when the thicknesses are actual lengths and
// not fractions; the solid shell (INT8 of ASTN) takes DIRECT 1 or -1 only. CORDM is the material coordinate system: 0
// for the basic system (the default), -1 for the element system, or the ID of a CORD2x. SB, the allowable interlaminar
// shear stress of the bond, has no default: a blank one is `std::nullopt`. ANAL is IS, IH or ISH, the default.
//
// A keyword line that is not in the card gets the defaults of its keyword, so `groups` always holds both lines resolved
// and a line is written only when a word of it is not the default. A card has at least one ply, with distinct IDs. The
// format reference asks that fractions add up to 1.0 and that a PCOMPLS has an ID no other property has (PSOLID,
// PLCOMP, PCOMPG...): rules between values and between cards, left to the checks of the model. This is a primary
// property: the card has the ID of the property space.
struct CompSolidPropNonlinearCard
{
    base::EntityId id = 0;
    int direction = 1;
    base::EntityId matFrameId = 0; // 0: basic, -1: the element system
    std::optional<double> bondAllowable;
    PropertyAnalysis analysisType = PropertyAnalysis::StructuralAndHeat;
    std::array<BehaviorGroup, 2> groups = defaultGroups(COMP_SOLID_PROP_NONLINEAR_GROUPS);
    std::vector<CompSolidPropNonlinearPly> plies;

    [[nodiscard]] double totalThickness() const noexcept
    {
        double sum = 0.0;
        for (const CompSolidPropNonlinearPly& ply : plies)
            sum += ply.thickness;
        return sum;
    }
};

} // namespace oor::model

#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/PropertyExtensionWords.hpp"

#include <array>
#include <optional>
#include <vector>

namespace oor::model
{

// The words the keyword lines of a PLCOMP allow: a plane composite is plane strain (COMPS, the
// default) or axisymmetric (AXCOMP), for the structure and for the heat; the integration code goes with the number of
// grids (L for the 4-node quadrilateral, Q for the 8-node one). The first word of each list is the one the format
// reference underlines, the default of a line that is not in the card.
inline constexpr std::array<ElementBehavior, 2> PLANE_COMP_PROP_BEHAVIORS{ElementBehavior::PlaneStrainComposite,
                                                                          ElementBehavior::AxisymmetricComposite};
inline constexpr std::array<IntegrationScheme, 1> PLANE_COMP_PROP_LINEAR{IntegrationScheme::Linear};
inline constexpr std::array<IntegrationScheme, 1> PLANE_COMP_PROP_QUADRATIC{IntegrationScheme::Quadratic};

// The keyword lines of the card, in the order of `PlaneCompPropCard::groups`: C4 (four corner grids) and C8 (four
// corner grids and four midside grids).
inline constexpr std::size_t PLANE_COMP_PROP_C4 = 0;
inline constexpr std::size_t PLANE_COMP_PROP_C8 = 1;

inline constexpr std::array<BehaviorRules, 2> PLANE_COMP_PROP_GROUPS{
    BehaviorRules{.keyword = "FourNodeElems",
                  .behaviorName = "behavior4",
                  .integrationName = "integration4",
                  .heatBehaviorName = "heatBehavior4",
                  .heatIntegrationName = "heatIntegration4",
                  .behaviors = PLANE_COMP_PROP_BEHAVIORS,
                  .integrations = PLANE_COMP_PROP_LINEAR,
                  .heatBehaviors = PLANE_COMP_PROP_BEHAVIORS,
                  .heatIntegrations = PLANE_COMP_PROP_LINEAR},
    BehaviorRules{.keyword = "EightNodeElems",
                  .behaviorName = "behavior8",
                  .integrationName = "integration8",
                  .heatBehaviorName = "heatBehavior8",
                  .heatIntegrationName = "heatIntegration8",
                  .behaviors = PLANE_COMP_PROP_BEHAVIORS,
                  .integrations = PLANE_COMP_PROP_QUADRATIC,
                  .heatBehaviors = PLANE_COMP_PROP_BEHAVIORS,
                  .heatIntegrations = PLANE_COMP_PROP_QUADRATIC}};

// One ply of a PLCOMP: the global ply ID, the material, the thickness (a fraction of the element thickness when DIRECT
// is positive, the actual thickness when it is negative) and the orientation angle.
struct PlaneCompPropPly
{
    base::EntityId globalId = 0; // IDi, unique in the card
    base::EntityId matId = 0;
    double thickness = 0.0;
    double thetaInDegrees = 0.0;

    [[nodiscard]] bool operator==(const PlaneCompPropPly&) const noexcept = default;
};

// One PLCOMP card (SOL 400): the property of a plane strain or axisymmetric composite, which a CQUAD, CQUAD4, CQUAD8 or
// CQUADX names. Its plies have global ply IDs, shared with the PCOMPG and PCOMPLS plies.
//
// Card layout (the lines after the first may come as shown or, for the two keyword lines, in any order):
//   PLCOMP   PID    DIRECT  THICKOP SB       ANAL
//            "C4"   BEH4    INT4    BEH4H    INT4H
//            "C8"   BEH8    INT8    BEH8H    INT8H
//            ID1    MID1    T1      THETA1                one line for each ply, bottom ply first
//            ID2    MID2    T2      THETA2
//
// DIRECT is 1 or 2 (default 1) for the direction of the layers, negative when the thicknesses are actual lengths and
// not fractions. THICKOP is the out-of-plane thickness (default 1.0). SB, the allowable interlaminar shear stress of
// the bond, has no default: a blank one is `std::nullopt`. ANAL is IS, IH or ISH, the default.
//
// A keyword line that is not in the card gets the defaults of its keyword, so `groups` always holds both lines resolved
// and a line is written only when a word of it is not the default. A card has at least one ply, with distinct IDs. The
// format reference asks that fractions add up to 1.0 and that a PLCOMP has an ID no other property has: rules between
// values and between cards, left to the checks of the model. This is a primary property: the card has the ID of the
// property space.
struct PlaneCompPropCard
{
    base::EntityId id = 0;
    int direction = 1;
    double thicknessOption = 1.0;
    std::optional<double> bondAllowable;
    PropertyAnalysis analysisType = PropertyAnalysis::StructuralAndHeat;
    std::array<BehaviorGroup, 2> groups = defaultGroups(PLANE_COMP_PROP_GROUPS);
    std::vector<PlaneCompPropPly> plies;

    [[nodiscard]] double totalThickness() const noexcept
    {
        double sum = 0.0;
        for (const PlaneCompPropPly& ply : plies)
            sum += ply.thickness;
        return sum;
    }
};

} // namespace oor::model

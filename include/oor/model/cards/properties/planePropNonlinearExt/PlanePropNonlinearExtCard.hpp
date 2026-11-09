#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/properties/ShellExtensionWords.hpp"

#include <array>

namespace oor::model
{

// The rows of the tables of the format reference for each keyword line, from the elements the
// keyword names: C3 the triangle with three grids, C4 the quadrilateral with four, C6 the triangle with six, C8 the
// quadrilateral with eight. A row pairs a behaviour with an integration scheme; the first row of each list is the
// default of the line (plane strain with L on the lower order elements and Q on the quadratic ones, the underlined rows
// of the format reference, which are also the defaults of its BEHi and INTi fields). The structural table has plane
// stress (PSTRS), plane strain (PLSTRN), the plane strain and axisymmetric composites (COMPS, AXCOMP: L on the
// four-node quadrilateral), the axisymmetric solid (AXSOLID, with the twist schemes LT and QT) and the incompressible
// plane strain and axisymmetric elements (IPS, IAX: L on the three-node triangle); the heat table has plane strain and
// axisymmetric solid with L or Q. The format reference has no plane stress row for the three-node triangle.
inline constexpr std::array<ShellWords, 4> PLANE_PROP_NONLINEAR_EXT_C3_STRUCTURAL{
    ShellWords{ShellBehavior::PlaneStrain, ShellIntegration::Linear},
    ShellWords{ShellBehavior::AxisymmetricSolid, ShellIntegration::Linear},
    ShellWords{ShellBehavior::IncompressiblePlaneStrain, ShellIntegration::Linear},
    ShellWords{ShellBehavior::IncompressibleAxisymmetric, ShellIntegration::Linear}};
inline constexpr std::array<ShellWords, 9> PLANE_PROP_NONLINEAR_EXT_C4_STRUCTURAL{
    ShellWords{ShellBehavior::PlaneStrain, ShellIntegration::Linear},
    ShellWords{ShellBehavior::PlaneStrain, ShellIntegration::LinearReducedHourglass},
    ShellWords{ShellBehavior::PlaneStress, ShellIntegration::Linear},
    ShellWords{ShellBehavior::PlaneStress, ShellIntegration::LinearReducedHourglass},
    ShellWords{ShellBehavior::PlaneStrainComposite, ShellIntegration::Linear},
    ShellWords{ShellBehavior::AxisymmetricSolid, ShellIntegration::Linear},
    ShellWords{ShellBehavior::AxisymmetricSolid, ShellIntegration::LinearTwist},
    ShellWords{ShellBehavior::AxisymmetricSolid, ShellIntegration::LinearReducedHourglass},
    ShellWords{ShellBehavior::AxisymmetricComposite, ShellIntegration::Linear}};
inline constexpr std::array<ShellWords, 3> PLANE_PROP_NONLINEAR_EXT_C6_STRUCTURAL{
    ShellWords{ShellBehavior::PlaneStrain, ShellIntegration::Quadratic},
    ShellWords{ShellBehavior::PlaneStress, ShellIntegration::Quadratic},
    ShellWords{ShellBehavior::AxisymmetricSolid, ShellIntegration::Quadratic}};
inline constexpr std::array<ShellWords, 7> PLANE_PROP_NONLINEAR_EXT_C8_STRUCTURAL{
    ShellWords{ShellBehavior::PlaneStrain, ShellIntegration::Quadratic},
    ShellWords{ShellBehavior::PlaneStrain, ShellIntegration::QuadraticReduced},
    ShellWords{ShellBehavior::PlaneStress, ShellIntegration::Quadratic},
    ShellWords{ShellBehavior::PlaneStress, ShellIntegration::QuadraticReduced},
    ShellWords{ShellBehavior::AxisymmetricSolid, ShellIntegration::Quadratic},
    ShellWords{ShellBehavior::AxisymmetricSolid, ShellIntegration::QuadraticReduced},
    ShellWords{ShellBehavior::AxisymmetricSolid, ShellIntegration::QuadraticTwist}};

inline constexpr std::array<ShellWords, 2> PLANE_PROP_NONLINEAR_EXT_C3_HEAT{
    ShellWords{ShellBehavior::PlaneStrain, ShellIntegration::Linear},
    ShellWords{ShellBehavior::AxisymmetricSolid, ShellIntegration::Linear}};
inline constexpr std::array<ShellWords, 2> PLANE_PROP_NONLINEAR_EXT_C4_HEAT{
    ShellWords{ShellBehavior::PlaneStrain, ShellIntegration::Linear},
    ShellWords{ShellBehavior::AxisymmetricSolid, ShellIntegration::Linear}};
inline constexpr std::array<ShellWords, 2> PLANE_PROP_NONLINEAR_EXT_C6_HEAT{
    ShellWords{ShellBehavior::PlaneStrain, ShellIntegration::Quadratic},
    ShellWords{ShellBehavior::AxisymmetricSolid, ShellIntegration::Quadratic}};
inline constexpr std::array<ShellWords, 2> PLANE_PROP_NONLINEAR_EXT_C8_HEAT{
    ShellWords{ShellBehavior::PlaneStrain, ShellIntegration::Quadratic},
    ShellWords{ShellBehavior::AxisymmetricSolid, ShellIntegration::Quadratic}};

// The keyword lines of the card, in the order of `PlanePropNonlinearExtCard::lines`.
inline constexpr std::size_t PLANE_PROP_NONLINEAR_EXT_C3 = 0;
inline constexpr std::size_t PLANE_PROP_NONLINEAR_EXT_C4 = 1;
inline constexpr std::size_t PLANE_PROP_NONLINEAR_EXT_C6 = 2;
inline constexpr std::size_t PLANE_PROP_NONLINEAR_EXT_C8 = 3;

inline constexpr std::array<ShellLineRules, 4> PLANE_PROP_NONLINEAR_EXT_LINES{
    ShellLineRules{.keyword = "ThreeNodeElems",
                   .behaviorName = "behavior3",
                   .integrationName = "integration3",
                   .heatBehaviorName = "heatBehavior3",
                   .heatIntegrationName = "heatIntegration3",
                   .structural = PLANE_PROP_NONLINEAR_EXT_C3_STRUCTURAL,
                   .heat = PLANE_PROP_NONLINEAR_EXT_C3_HEAT},
    ShellLineRules{.keyword = "FourNodeElems",
                   .behaviorName = "behavior4",
                   .integrationName = "integration4",
                   .heatBehaviorName = "heatBehavior4",
                   .heatIntegrationName = "heatIntegration4",
                   .structural = PLANE_PROP_NONLINEAR_EXT_C4_STRUCTURAL,
                   .heat = PLANE_PROP_NONLINEAR_EXT_C4_HEAT},
    ShellLineRules{.keyword = "SixNodeElems",
                   .behaviorName = "behavior6",
                   .integrationName = "integration6",
                   .heatBehaviorName = "heatBehavior6",
                   .heatIntegrationName = "heatIntegration6",
                   .structural = PLANE_PROP_NONLINEAR_EXT_C6_STRUCTURAL,
                   .heat = PLANE_PROP_NONLINEAR_EXT_C6_HEAT},
    ShellLineRules{.keyword = "EightNodeElems",
                   .behaviorName = "behavior8",
                   .integrationName = "integration8",
                   .heatBehaviorName = "heatBehavior8",
                   .heatIntegrationName = "heatIntegration8",
                   .structural = PLANE_PROP_NONLINEAR_EXT_C8_STRUCTURAL,
                   .heat = PLANE_PROP_NONLINEAR_EXT_C8_HEAT}};

// One PSHLN2 card (SOL 400): the nonlinear extension of the PLPLANE with the same PID, for the plane stress, plane
// strain and axisymmetric elements (CQUAD4, CQUAD, CQUAD8, CTRIA3, CTRIA6, CQUADX, CTRIAX) that name it. It also gives
// the implicit heat transfer elements their formulation (ANAL = IH or ISH). It is honoured in SOL 400 only, and the
// PLPLANE it extends still needs its MATHP.
//
// Card layout:
//   PSHLN2  PID    MID     DIRECT   T        ANAL
//           "C3"   BEH3    INT3     BEH3H    INT3H          one line for each keyword,
//           "C4"   BEH4    INT4     BEH4H    INT4H          fields 9 to 13 of its line
//           "C6"   BEH6    INT6     BEH6H    INT6H
//           "C8"   BEH8    INT8     BEH8H    INT8H
//
// The ID is the one of the PLPLANE it extends, so it is in the space of its own card type (one PSHLN2 for each PLPLANE)
// and is a reference to a property. MID is required (it replaces the MID of the PLPLANE) and names a
// structural material, or a MAT4 or MAT5 for the heat analysis (the Material target falls back to those). DIRECT is the
// direction of the layers of a COMPS or AXCOMP element, 1 or 2 (default 1); T is a thickness (default 1.0; the format
// states no limit). ANAL is IS, IH or ISH, the default.
//
// The keyword lines may come in any order or not at all; a line that is not in the card gets the
// defaults of its keyword, so `lines` always holds the four lines resolved and a line is written only when a word of it
// is not the default. A blank behaviour is the default of the line; a blank integration is the first scheme the table
// lists for the behaviour (see resolveShellLine).
struct PlanePropNonlinearExtCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0;
    int direction = 1;
    double thickness = 1.0;
    PropertyAnalysis analysisType = PropertyAnalysis::StructuralAndHeat;
    std::array<ShellLine, 4> lines = defaultLines(PLANE_PROP_NONLINEAR_EXT_LINES);
};

} // namespace oor::model

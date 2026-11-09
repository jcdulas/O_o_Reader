#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <optional>

namespace oor::model
{

// One MATFAB card: a bi-directional woven fabric material for shell elements (SOL 700), a fabric with an optional
// coating.
//
// Card layout (the fields are numbered across the lines, eight to a line):
//   MATFAB  MID    RHO    ECOAT  NUCOAT GCOAT  DAMPCOAT COMPCOAT PERC            fields 1 to 8
//           E1L    E1Q    (blank) THETA1 XWARP  YWARP  ZWARP                     9 to 15   the warp ends
//           E2L    E2Q    (blank) THETA2 XWEFT  YWEFT  ZWEFT                     17 to 23  the weft picks
//           SCOF   G12    DAMPFIB COMPFIB LOCKANG1 LOCKANG2                      25 to 30
// RHO, E1L and E2L are greater than zero. ECOAT, NUCOAT and GCOAT (the coating, absent when blank) are needed two out
// of three when PERC, the thickness percentage of the coating (0.0 to 100.0, 0.0 for no coating), is greater than zero.
// DAMPCOAT and DAMPFIB (0.1) are the damping of the coating and of the fibres, COMPCOAT and COMPFIB (1.0, from 0.0
// to 1.0) scale their compressive stresses (0.0 gives a tension only model). E1Q and E2Q (0.0) are the quadratic
// coefficients of the moduli. The orientation of the fibres is given by the angles THETA1 and THETA2 (absent when
// blank) or, when they are not there, by the vectors (XWARP, YWARP, ZWARP) (1.0, 0.0, 0.0) and (XWEFT, YWEFT, ZWEFT)
// (0.0, 1.0, 0.0) in the basic system. SCOF is the shear coefficient of friction (0.0), G12 the shear modulus (absent
// when blank: computed from the two linear moduli) and LOCKANG1 and LOCKANG2 (10.0 and 15.0, not negative) the locking
// angles. The material ID is in the space every material type shares.
struct FabricMatCard
{
    base::EntityId id = 0;
    double density = 0.0;
    std::optional<double> coatingYoungsModulus;
    std::optional<double> coatingPoissonRatio;
    std::optional<double> coatingShearModulus;
    double coatingDamping = 0.1;
    double coatingCompressionScale = 1.0;
    double coatingThicknessPercent = 0.0;
    double warpModulusLinear = 0.0;
    double warpModulusQuadratic = 0.0;
    std::optional<double> warpAngle;
    std::array<double, 3> warpDirection = {1.0, 0.0, 0.0};
    double weftModulusLinear = 0.0;
    double weftModulusQuadratic = 0.0;
    std::optional<double> weftAngle;
    std::array<double, 3> weftDirection = {0.0, 1.0, 0.0};
    double shearFriction = 0.0;
    std::optional<double> shearModulus12;
    double fiberDamping = 0.1;
    double fiberCompressionScale = 1.0;
    double lockAngle1 = 10.0;
    double lockAngle2 = 15.0;
};

} // namespace oor::model

#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One cross-section of a PBEAM3: end A, end B or the middle node C.
struct BeamProp3Station
{
    double area = 0.0;
    double bendingInertiaZ = 0.0; // area moments of inertia about the local z and y axes, and the product of inertia
    double bendingInertiaY = 0.0;
    double productInertiaYZ = 0.0;
    double torsionConstant = 0.0;            // torsional stiffness parameter
    double nonStructMass = 0.0;              // nonstructural mass per unit length
    std::array<double, 8> stressPoints = {}; // the local (y, z) of the stress points: CY CZ DY DZ EY EZ FY FZ
    std::array<double, 2> neutralAxis = {};  // NY NZ: the neutral axis
    std::array<double, 2> nsmCenter = {};    // MY MZ: the center of gravity of the nonstructural mass
    std::array<double, 3> nsmInertia = {};   // NSIY NSIZ NSIYZ: the nonstructural mass moments of inertia per length
    double warpingCoefficient = 0.0;
    // The warping function at the stress points and its gradients, point after point:
    // WC WYC WZC  WD WYD WZD  WE WYE WZE  WF WYF WZF
    std::array<double, 12> warping = {};
    // SO of ends B and C: YES (the stress points of the card) when true, YESA (the points of end A, the default) when
    // false. End A has no SO field and holds false.
    bool stressOutput = false;
};

// One PBEAM3 card: the property of a three-node beam element (CBEAM3).
//
// Card layout (fixed positions: a line is eight fields, the numbers are the format reference fields):
//    1 PID    2 MID    3 A(A)   4 IZ(A)  5 IY(A)  6 IYZ(A) 7 J(A)   8 NSM(A)
//    9 to 16  CY(A) CZ(A) DY(A) DZ(A) EY(A) EZ(A) FY(A) FZ(A)
//   17 SO(B)  19 A(B) .. 24 NSM(B)        25 to 32 CY(B) .. FZ(B)
//   33 SO(C)  35 A(C) .. 40 NSM(C)        41 to 48 CY(C) .. FZ(C)
//   49 KY  50 KZ  51 to 56 NY(A) NZ(A) NY(B) NZ(B) NY(C) NZ(C)  57 to 62 MY(A) MZ(A) MY(B) MZ(B) MY(C) MZ(C)
//   63 to 71 NSIY(A) NSIZ(A) NSIYZ(A) NSIY(B) .. NSIYZ(C)  72 to 74 CW(A) CW(B) CW(C)  75 STRESS
//   81 to 116 the warping function and its gradients at C, D, E and F of A, then B, then C
//
// Blank values are stored resolved (as the field descriptions say): IYZ(A) is 0.0, J(A) is IZ(A) + IY(A), and
// every value of B and C that is blank takes the value of end A, except the stress points (0.0).
struct BeamProp3Card
{
    base::EntityId id = 0;
    base::EntityId matId = 0;
    std::array<BeamProp3Station, 3> stations; // end A, end B, node C
    double shearFactorY = 1.0;                // shear effectiveness factors in the local y and z directions
    double shearFactorZ = 1.0;
    bool gaussStress = false; // STRESS = GAUSS (output at the Gauss points) or GRID (at A, B and C, the default)
    // when every warping field of the card is blank, the stresses and strains of the warping are not
    // recovered. A zero given is not a blank, so the card keeps whether any warping field was given (the warping
    // values of every station are then written, and they are all zero when it is false).
    bool warpingGiven = false;
};

} // namespace oor::model

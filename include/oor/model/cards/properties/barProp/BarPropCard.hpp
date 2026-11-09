#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One PBAR card: the property of a simple beam element (CBAR).
//
// Card layout:
//   PBAR  PID  MID  A    I1   I2   J    NSM
//         C1   C2   D1   D2   E1   E2   F1   F2
//         K1   K2   I12
//
// The stress recovery points C, D, E and F are (y, z) pairs in the element system. K1 and K2, the area factors for
// shear, are blank for "no shear flexibility"; a blank is NO_SHEAR_FACTOR here, as the default is not
// a number.
struct BarPropCard
{
    static constexpr double NO_SHEAR_FACTOR = -1.0;

    base::EntityId id = 0;
    base::EntityId matId = 0;
    double area = 0.0;            // area
    double bendingInertia1 = 0.0; // moments of inertia about the two bending axes
    double bendingInertia2 = 0.0;
    double torsionConstant = 0.0; // torsional constant
    double nonStructMass = 0.0;   // nonstructural mass per unit length
    std::array<double, 2> stressPointC = {};
    std::array<double, 2> stressPointD = {};
    std::array<double, 2> stressPointE = {};
    std::array<double, 2> stressPointF = {};
    double shearFactor1 = NO_SHEAR_FACTOR;
    double shearFactor2 = NO_SHEAR_FACTOR;
    double productInertia12 = 0.0; // product of inertia
};

} // namespace oor::model

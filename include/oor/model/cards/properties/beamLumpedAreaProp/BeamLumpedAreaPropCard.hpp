#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <vector>

namespace oor::model
{

// One lumped area of the cross-section of a PBCOMP.
struct BeamLumpedAreaPropArea
{
    double y = 0.0; // the (y, z) coordinates of the area in the element coordinate system
    double z = 0.0;
    double fraction = 0.0;    // the fraction of the total area (0.0 gives a stress output location only)
    base::EntityId matId = 0; // the material of the area
};

// One PBCOMP card: the alternate form of a PBEAM for a uniform beam, with the lumped areas of its cross-section.
//
// Card layout:
//   PBCOMP  PID  MID  A   I1  I2  I12  J       NSM
//           K1   K2   M1  M2  N1  N2   SYMOPT
//           Y1   Z1   C1  MID1
//           Y2   Z2   C2  MID2
//           -etc.-
//
// A blank MIDi is MID (stored resolved). I1, I2, I12 and J are 0.0 when blank (a common deck leaves them blank, and
// they are ignored when lumped areas are given). A line of lumped area has four fields, the four after them are blank.
// There are at most 20 lumped areas.
struct BeamLumpedAreaPropCard
{
    static constexpr std::size_t MAX_AREAS = 20;

    base::EntityId id = 0;
    base::EntityId matId = 0;
    double area = 0.0;            // area
    double bendingInertia1 = 0.0; // area moments of inertia in planes 1 and 2
    double bendingInertia2 = 0.0;
    double productInertia12 = 0.0; // area product of inertia
    double torsionConstant = 0.0;  // torsional stiffness parameter
    double nonStructMass = 0.0;    // nonstructural mass per unit length
    double shearFactor1 = 1.0;     // shear stiffness factors in planes 1 and 2
    double shearFactor2 = 1.0;
    double nsmCenterY = 0.0; // the (y, z) coordinates of the center of gravity of the nonstructural mass
    double nsmCenterZ = 0.0;
    double neutralAxisY = 0.0; // the (y, z) coordinates of the neutral axis
    double neutralAxisZ = 0.0;
    int symmetricOption = 0; // the symmetry option of the lumped areas, 0 to 5
    std::vector<BeamLumpedAreaPropArea> areas;
};

} // namespace oor::model

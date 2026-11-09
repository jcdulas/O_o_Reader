#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <limits>

namespace oor::model
{

// The two layouts of a PBEND.
enum class CurvedBeamPropForm : std::uint8_t
{
    Section, // the cross-section is given by its area, moments of inertia and torsional stiffness
    Pipe     // the alternate format for elbows and curved pipes: mean radius and wall thickness
};

// One PBEND card: the property of a curved beam, curved pipe or elbow (CBEND).
//
// Card layout, general section (the form is Section when field 3 is a real):
//   PBEND  PID  MID  A   I1  I2  J   RB     THETAB
//          C1   C2   D1  D2  E1  E2  F1     F2
//          K1   K2   NSM RC  ZC  DELTAN
// Card layout, elbow or curved pipe (the form is Pipe when field 3 is an integer):
//   PBEND  PID  MID  FSI RM  T   P   RB     THETAB
//                    NSM RC  ZC
//
// Only the members of the form of the card are used; the others stay at their values below (zero, NaN for a blank
// field) and `check` refuses anything else. RB, THETAB, P and DELTAN can be blank: RB and THETAB are then given by the
// CBEND (they are NaN here), a blank DELTAN is the neutral axis offset the format reference formula gives,
// a blank P is no internal pressure. K1 and K2 are 0.0 when blank.
struct CurvedBeamPropCard
{
    static constexpr double NOT_GIVEN = std::numeric_limits<double>::quiet_NaN();

    base::EntityId id = 0;
    base::EntityId matId = 0;
    CurvedBeamPropForm form = CurvedBeamPropForm::Section;

    // Section form.
    double area = 0.0;            // area
    double bendingInertia1 = 0.0; // area moments of inertia in planes 1 and 2
    double bendingInertia2 = 0.0;
    double torsionConstant = 0.0; // torsional stiffness
    // (r, z) of the stress recovery points C, D, E, F: C1 C2 D1 D2 E1 E2 F1 F2
    std::array<double, 8> stressPoints = {};
    double shearFactor1 = 0.0; // shear stiffness factors in planes 1 and 2
    double shearFactor2 = 0.0;
    double neutralAxisOffset = NOT_GIVEN; // radial offset of the neutral axis from the geometric centroid

    // Pipe form.
    int flexibilityOption = 0;           // flexibility and stress intensification factors, 1 to 3
    double meanRadius = 0.0;             // mean cross-sectional radius
    double thickness = 0.0;              // wall thickness
    double internalPressure = NOT_GIVEN; // internal pressure

    // Both forms.
    double bendRadius = NOT_GIVEN;     // bend radius of the line of centroids
    double arcAngle = NOT_GIVEN;       // arc angle of the element, in degrees
    double nonStructMass = 0.0;        // nonstructural mass per unit length
    double centroidRadialOffset = 0.0; // radial offset of the geometric centroid from GA and GB
    double centerZ = 0.0; // offset of the geometric centroid perpendicular to the plane of GA, GB and the vector v
};

} // namespace oor::model

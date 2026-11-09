#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace oor::model
{

// The SO field of a station: where stress is recovered.
enum class BeamStressOutput : std::uint8_t
{
    No,  // "NO": no stresses or forces
    Yes, // "YES": at the points C, D, E, F given for the station
    YesA // "YESA": at the points of end A
};

// One cross-section of a PBEAM: end A, an intermediate station or end B.
struct BeamPropStation
{
    BeamStressOutput stressOutput = BeamStressOutput::Yes;
    // X/XB: distance from end A over the length of the element, 0.0 at end A and 1.0 at end B
    double stationRatio = 0.0;
    double area = 0.0;
    double bendingInertia1 = 0.0; // moments and product of inertia, torsional constant, nonstructural mass per length
    double bendingInertia2 = 0.0;
    double productInertia12 = 0.0;
    double torsionConstant = 0.0;
    double nonStructMass = 0.0;
    // The stress points as (y, z) pairs; used only where `stressOutput` is Yes (zero elsewhere, the card has no such
    // fields).
    std::array<double, 2> stressPointC = {};
    std::array<double, 2> stressPointD = {};
    std::array<double, 2> stressPointE = {};
    std::array<double, 2> stressPointF = {};
};

// One PBEAM card: the property of a beam element (CBEAM) that may taper.
//
// Card layout (fields are numbered across the continuation lines, 8 to a line):
//   PBEAM  PID    MID    A(A)   I1(A)  I2(A)  I12(A) J(A)   NSM(A)
//          C1(A)  C2(A)  D1(A)  D2(A)  E1(A)  E2(A)  F1(A)  F2(A)          only if stress is recovered at A
// then, for each further station (up to ten, one of them at X/XB = 1.0, in any order):
//          SO     X/XB   A      I1     I2     I12    J      NSM
//          C1     C2     D1     D2     E1     E2     F1     F2             only if SO = YES
// and the two optional last lines:
//          K1     K2     S1     S2     NSI(A) NSI(B) CW(A)  CW(B)
//          M1(A)  M2(A)  M1(B)  M2(B)  N1(A)  N2(A)  N1(B)  N2(B)
//
// `stations` holds every station in the order of the card, end A first (stationRatio 0.0). Blank values are stored
// resolved: at the station X/XB = 1.0 a blank section property is the value at end A, at an intermediate station it is
// the linear interpolation between the two ends; NSI(B), CW(B), M(B) and N(B) are those of end A when
// blank. A card with only end A is a uniform beam. `stressOutput` of end A is Yes when the line of stress points is
// there (blank or not) and No when it is left out, which can only be told apart when stations follow.
struct BeamPropCard
{
    static constexpr std::size_t MAX_STATIONS = 11; // end A and up to ten more

    base::EntityId id = 0;
    base::EntityId matId = 0;
    std::vector<BeamPropStation> stations{BeamPropStation{}};

    double shearFactor1 = 1.0; // shear stiffness factors in planes 1 and 2
    double shearFactor2 = 1.0;
    double shearRelief1 = 0.0; // shear relief coefficients due to taper
    double shearRelief2 = 0.0;
    double nsmInertiaA = 0.0; // nonstructural mass moment of inertia per unit length at ends A and B
    double nsmInertiaB = 0.0;
    double warpingCoefficientA = 0.0; // warping coefficients at ends A and B
    double warpingCoefficientB = 0.0;
    double nsmCenterYA = 0.0; // (y, z) of the center of gravity of the nonstructural mass at ends A and B
    double nsmCenterZA = 0.0;
    double nsmCenterYB = 0.0;
    double nsmCenterZB = 0.0;
    double neutralAxisYA = 0.0; // (y, z) of the neutral axis at ends A and B
    double neutralAxisZA = 0.0;
    double neutralAxisYB = 0.0;
    double neutralAxisZB = 0.0;
};

} // namespace oor::model

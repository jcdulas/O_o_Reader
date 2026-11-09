#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <array>

namespace oor::model
{

// One PBUSH card: the property of a generalized spring-and-damper (CBUSH).
//
// Card layout: after the PID, groups that each start on a line with a flag and carry the values behind it, in any
// order, each group at most once:
//   PBUSH  PID
//          "K"    K1     K2     K3     K4     K5     K6        stiffness in the six directions
//          "B"    B1     B2     B3     B4     B5     B6        damping (force per velocity)
//          "GE"   GE1    GE2    GE3    GE4    GE5    GE6       structural damping
//          "RCV"  SA     ST     EA     ET                      stress and strain recovery coefficients
//          "M"    M                                            lumped mass
//          "T"    ALPHA  TREF   COINL                          thermal expansion, reference temperature, length
//
// The card has the groups it needs and the others are not there: this value has all of them, each with the default
// of its blank values (0.0, and 1.0 for the recovery coefficients). when only GE1 is given and GE2 to
// GE6 are all blank, GE1 applies to all six directions; GE is stored resolved, so `ge` holds the six values.
struct BushPropCard
{
    base::EntityId id = 0;
    std::array<double, 6> stiffness = {};
    std::array<double, 6> damping = {};
    std::array<double, 6> structuralDamping = {};
    double stressFactorTranslational = 1.0; // stress recovery coefficients, translations and rotations
    double stressFactorRotational = 1.0;
    double strainFactorTranslational = 1.0; // strain recovery coefficients, translations and rotations
    double strainFactorRotational = 1.0;
    double mass = 0.0; // M
    double alphaCoefficient = 0.0;
    double referenceTemperature = 0.0;
    double coincidentLength = 0.0; // length of a CBUSH whose grid points coincide
};

} // namespace oor::model

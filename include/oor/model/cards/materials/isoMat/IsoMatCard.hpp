#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>

namespace oor::model
{

// One MAT1 card: an isotropic material.
//
// Card layout:
//   MAT1  MID  E   G   NU  RHO  A  TREF  GE
//         ST   SC  SS  MCSID
//
// E, G and NU are stored resolved: when one of them is blank on the card it is computed from the other two,
// so a card never depends on a default that could change later. Which ones were blank is kept (`blankFields`): a
// design relation that changes E, G or NU re-derives the blank one as the solver does, and the card is written back
// with that field blank as long as it still holds the value the blank gives.
struct IsoMatCard
{
    static constexpr std::uint8_t BLANK_YOUNGS_MODULUS = 1;
    static constexpr std::uint8_t BLANK_SHEAR_MODULUS = 2;
    static constexpr std::uint8_t BLANK_POISSON_RATIO = 4;

    base::EntityId id = 0;
    double youngsModulus = 0.0; // Young's modulus
    double shearModulus = 0.0;  // shear modulus
    double poissonRatio = 0.0;  // Poisson's ratio
    double density = 0.0;
    double thermalExpansion = 0.0; // thermal expansion coefficient
    double referenceTemperature = 0.0;
    double structuralDamping = 0.0; // structural damping coefficient
    double tensionAllowable = 0.0;  // stress limits (tension, compression, shear)
    double compressionAllowable = 0.0;
    double shearAllowable = 0.0;
    base::EntityId stressFrameId = 0; // material coordinate system for the temperature-dependent properties
    std::uint8_t blankFields = 0;     // the BLANK_* bits of E, G and NU that the card left blank
};

} // namespace oor::model

#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <optional>

namespace oor::model
{

// One MAT10 card: the material of a fluid element in a coupled fluid-structure analysis.
//
// Card layout:
//   MAT10  MID   BULK   RHO   C   GE   ALPHA
//
// BULK, RHO and C are related by C^2 = BULK / RHO and two of them are enough: the card is stored with
// the three, the one a blank card leaves out computed from the two it gives (resolved, and always written). When a card
// gives all three and they disagree, the program uses BULK and RHO: the card keeps C as it was given. GE is 0.0 when
// blank, ALPHA (the normalised admittance of a porous material) is absent when blank. With an ALPHA, BULK and RHO may
// be negative.
struct FluidMatCard
{
    base::EntityId id = 0;
    double bulkModulus = 0.0;       // bulk modulus
    double density = 0.0;           // mass density
    double soundSpeed = 0.0;        // speed of sound
    double structuralDamping = 0.0; // fluid element damping coefficient
    std::optional<double> normalizedAdmittance;
};

} // namespace oor::model

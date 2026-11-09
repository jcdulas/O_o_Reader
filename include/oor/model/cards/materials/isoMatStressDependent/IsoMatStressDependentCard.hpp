#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>

namespace oor::model
{

// The kind of nonlinearity of a MATS1.
enum class IsoMatStressDependentType : std::uint8_t
{
    NonlinearElastic, // NLELAST
    Plastic           // PLASTIC
};

// One MATS1 card: the stress dependent properties of a MAT1 (nonlinear elasticity or plasticity).
//
// Card layout:
//   MATS1  MID   TID   TYPE   H   YF   HR   LIMIT1   LIMIT2
// TID is the TABLES1 (or, for NLELAST, TABLEST) with the stress-strain curve, 0 when blank. H is the work hardening
// slope: it is given or the table is, not both, and a blank H is absent. YF (the yield function: 1 von
// Mises, 2 Tresca, 3 Mohr-Coulomb, 4 Drucker-Prager) and HR (the hardening rule: 1 isotropic, 2 kinematic, 3 combined)
// are 1 when blank. LIMIT1 is the initial yield point (required for a plastic material) and LIMIT2 the internal
// friction angle in degrees, which the Mohr-Coulomb and Drucker-Prager criteria need. A nonlinear elastic material does
// not use H, YF, HR, LIMIT1 and LIMIT2; whatever a card gives there is kept as written.
struct IsoMatStressDependentCard
{
    base::EntityId id = 0; // the MAT1 that the card extends
    base::EntityId tableId = 0;
    IsoMatStressDependentType kind = IsoMatStressDependentType::NonlinearElastic;
    std::optional<double> hardeningSlope;
    int yieldFunction = 1;
    int hardeningRule = 1;
    std::optional<double> initialYieldStress;
    std::optional<double> frictionAngle;
};

} // namespace oor::model

#pragma once

#include "oor/base/ids/EntityId.hpp"

namespace oor::model
{

// The default of FVTOL, the volume cutoff tolerance.
inline constexpr double CONSTANT_SPALLATION_DEFAULT_CUTOFF_TOLERANCE = 1.0E-4;

// One PMINC card (SOL 700): the constant spallation model, in which the minimum pressure is constant. An element whose
// pressure falls below the minimum pressure spalls: its pressure and its yield stress are set to zero and a void is
// created. A MATDEUL names the model in its PID field.
//
// Card layout: PMINC  PID  VALUE  FVTOL  FVTOL2            fields 1 to 4
// PID is the number of the spall model: the first field of the card, not a property (the category is SpallModel and
// the ID is unique among the PMINC cards). VALUE is the minimum pressure, not positive (0.0). A void fraction smaller
// than FVTOL (greater than zero, 1.E-4; Eulerian elements only) is set to zero, to keep the void fractions from being
// too small. FVTOL2 (not negative, 0.0) is the largest void fraction an element may have and still carry a tensile
// stress; with zero any void fraction means failure. The format reference advises a FVTOL2 larger than FVTOL, a rule of
// good practice that the card does not check. VALUE is meant to be negative, yet its default is zero:
// the card accepts zero, the default.
struct ConstantSpallationCard
{
    base::EntityId id = 0;    // unique among the spall models
    double minPressure = 0.0; // minimum pressure, not positive
    double volumeCutoffTolerance =
        CONSTANT_SPALLATION_DEFAULT_CUTOFF_TOLERANCE; // volume cutoff tolerance, greater than zero
    double maxVoidFraction = 0.0;                     // largest void fraction under tension, not negative
};

} // namespace oor::model

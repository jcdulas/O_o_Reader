#pragma once

#include <string>

namespace oor::model
{

// One FLSYM card: the symmetry of the structural boundary of an axisymmetric fluid model.
//
// Card layout: FLSYM  M  S1  S2
// M is the (even) number of symmetric sections of the structure around the circumference; S1 and S2 are the constraints
// of the structure at the first and second planes of symmetry: S (symmetric) or A (antisymmetric).
struct AxisymFluidSymmetryCard
{
    int sectionCount = 2;
    std::string planeSymmetry1 = "Symmetric";
    std::string planeSymmetry2 = "Symmetric";
};

} // namespace oor::model

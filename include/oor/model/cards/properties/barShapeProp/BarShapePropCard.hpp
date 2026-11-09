#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One PBARL card: a simple beam property given by the dimensions of its cross-section.
//
// Card layout:
//   PBARL  PID   MID   GROUP  TYPE
//          DIM1  DIM2  DIM3   DIM4  DIM5  DIM6  DIM7  DIM8
//          DIM9  -etc.-       NSM
//
// Only the standard section library (sections::STANDARD_SECTION_LIBRARY, the default) is known: `type` is one of its
// sections and `dims` has the number of dimensions that section takes (BarShapePropFormat::dimensionCount). NSM comes
// right after the last dimension, so its field depends on the type. A DBOX section with DIM5 to DIM10 blank gets the
// defaults (DIM5 to DIM8 equal to DIM4, DIM9 and DIM10 equal to DIM6), stored resolved.
struct BarShapePropCard
{
    base::EntityId id = 0;
    base::EntityId matId = 0;
    std::string kind;           // "ROD", "TUBE", "I", "BOX", ...
    std::vector<double> dims;   // DIM1 .. DIMn
    double nonStructMass = 0.0; // nonstructural mass per unit length
};

} // namespace oor::model

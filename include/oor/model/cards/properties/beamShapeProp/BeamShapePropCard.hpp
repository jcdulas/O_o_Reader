#pragma once

#include "oor/base/ids/EntityId.hpp"

#include <string>
#include <vector>

namespace oor::model
{

// One cross-section of a PBEAML: end A, an intermediate station or end B.
struct BeamShapePropStation
{
    // X/XB: distance from end A over the length of the element (0.0 at end A, 1.0 at end B)
    double stationRatio = 0.0;
    bool stressOutput = true;   // SO: YES (true) or NO; end A has no SO field and is always true
    std::vector<double> dims;   // DIM1 .. DIMn of the section at this station (n depends on the type)
    double nonStructMass = 0.0; // nonstructural mass per unit length at this station
};

// One PBEAML card: a beam property given by the dimensions of its cross-section, possibly different at several
// stations along the element.
//
// Card layout (the fields run on across the continuation lines, eight to a line; n is the number of dimensions of the
// section):
//   PBEAML  PID       MID      GROUP    TYPE
//           DIM1(A)   DIM2(A)  -etc.-   DIMn(A)  NSM(A)
//           SO(1)     X(1)/XB  DIM1(1)  -etc.-   DIMn(1)  NSM(1)
//           SO(2)     X(2)/XB  DIM1(2)  ...
//           SO(B)     1.0      DIM1(B)  -etc.-   DIMn(B)  NSM(B)
//
// Only the standard section library (sections::STANDARD_SECTION_LIBRARY, the default) is known: `type` is one of its
// sections (sections::standardSections) and every station has the number of dimensions of that section. `stations`
// holds every station in the order of the card, end A first (stationRatio 0.0). Blank values are stored resolved: at
// the station X/XB = 1.0 a blank dimension or NSM is the value at end A, at an intermediate station it is the linear
// interpolation between the two ends. A card with only end A is a uniform beam. A DBOX section with DIM5 to DIM10 blank
// gets the defaults of a PBARL DBOX (DIM5 to DIM8 equal to DIM4, DIM9 and DIM10 to DIM6).
struct BeamShapePropCard
{
    // The total number of dimensions at all stations must be less than 200.
    static constexpr std::size_t MAX_TOTAL_DIMENSIONS = 199;

    base::EntityId id = 0;
    base::EntityId matId = 0;
    std::string kind; // "ROD", "TUBE", "I", "BOX", ...
    std::vector<BeamShapePropStation> stations{BeamShapePropStation{}};
};

} // namespace oor::model

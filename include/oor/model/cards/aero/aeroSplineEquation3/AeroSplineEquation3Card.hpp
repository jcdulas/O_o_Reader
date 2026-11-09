#pragma once

#include "oor/base/ids/EntityId.hpp"
#include "oor/model/cards/aero/AeroSupport.hpp"

#include <cstdint>
#include <vector>

namespace oor::model
{

// One independent degree of freedom of a SPLINE3 and its coefficient: the term A * u(G, C) of the constraint equation.
struct AeroSplineEquation3Point
{
    base::EntityId node = 0;    // G: a grid point or a scalar point of the g-set
    std::int32_t component = 0; // C: the component of the displacement system, 1 to 6 for a grid point, 0 for a
                                // scalar point
    double coefficient = 0.0;   // A

    [[nodiscard]] bool operator==(const AeroSplineEquation3Point&) const noexcept = default;
};

// One SPLINE3 card: a constraint equation that sets one component of motion of one aerodynamic box from the
// displacements of structural degrees of freedom, the way a control surface is tied to its actuator.
//
// Card layout: SPLINE3  EID  CAERO  BOXID  COMP  G1  C1  A1  USAGE
//                      G2   C2     A2           G3  C3  A3
//                      G4   C4     A4           -etc.-
// The dependent value is the component COMP of the box BOXID of the aerodynamic element CAERO
// (ReferenceTarget::AeroElement and ReferenceTarget::AeroBox), and it equals the sum of A * u(G, C) over the
// independent degrees of freedom. COMP is 1 to 6; which components each kind of CAERO accepts is a rule between cards
// (CAERO1: 3 and 5, CAERO2: 2, 3, 5 and 6, CAERO3: 3, CAERO4 and CAERO5: 3, 5 and 6, a mesh of AEGRID points: 1, 2, 3,
// 5 and 6).
//
// The independent points are triples (G, C, A) that start at field 5 and repeat every four fields: fields 5 to 7, 9 to
// 11, 13 to 15, ... The fourth field of each group is not used, except in the first one, where it is USAGE (field 8,
// FORCE, DISP or BOTH, BOTH by default). A group is complete or blank (a blank one is the padding of a line and is
// skipped); at least the first one is required. Each G is a grid point or scalar point
// (ReferenceTarget::ScalarOrGridPoint) of the g-set, C is 1 to 6 for a grid point and 0 for a scalar point (the card
// cannot tell which G is which: that is a rule between cards), and A is any real. The points are kept in the order of
// the card.
struct AeroSplineEquation3Card
{
    base::EntityId id = 0;
    base::EntityId aeroPanelId = 0;
    base::EntityId boxId = 0;
    std::int32_t component = 0;
    SplineUsage usage = SplineUsage::Both;
    std::vector<AeroSplineEquation3Point> points;
};

} // namespace oor::model

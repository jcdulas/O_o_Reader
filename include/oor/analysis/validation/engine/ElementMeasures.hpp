#pragma once

#include "oor/model/core/Model.hpp"

#include <vector>

namespace oor::analysis
{

// What the geometry says of one element, for the checks that look for degenerate elements. A value that does not apply
// to the element (the area of a solid) or cannot be computed (a missing node) is NaN.
struct ElementMeasure
{
    model::CardLocation location;
    double length = 0.0; // rods, tubes, bars and beams: between the end points
    double area = 0.0;   // shells
    double volume = 0.0; // solids (absolute)
    // A bar or beam whose orientation vector is zero or parallel to its axis, so that its element system is undefined.
    bool orientationUndefined = false;
};

// The measures of every rod, tube, bar, beam, shell and solid of a model, in the order of the registry and of the rows,
// and the size of the model (the diagonal of its bounding box), which is what "zero" is relative to. Made once per
// validation, by the first check that needs it.
struct ElementMeasures
{
    std::vector<ElementMeasure> elements;
    double scale = 0.0;

    // An element that cannot be measured (a node that does not exist) is left out: the reference check reports it.
    [[nodiscard]] static ElementMeasures build(const model::Model& model);
};

} // namespace oor::analysis

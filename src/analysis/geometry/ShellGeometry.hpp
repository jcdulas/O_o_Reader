#pragma once

#include "ElementProvider.hpp"

#include "oor/analysis/frames/FrameMath.hpp"

#include <span>

// The geometry of the shell families: one function for the triangles and one for the quadrilaterals. Each card type of
// the two families gathers what its card holds into a ShellInput and calls its function.

namespace oor::analysis::detail
{

struct ShellInput
{
    std::span<const base::Vec3> corners; // 3 (triangle) or 4 (quadrilateral) corner points; mid-side nodes are not used
    base::EntityId pid = 0;
    // The corner thicknesses T1..Tn of the card; a negative one is blank. Null for a card that has none.
    const double* cornerThickness = nullptr;
    int thicknessFlag = 0;    // TFLAG: 0 the Ti are thicknesses, 1 they are fractions of the property's
    bool hasThickness = true; // false for the elements that have no thickness (the axisymmetric and nonlinear ones)
    // A thickness the element has whatever its property says (the plane strain elements are one unit deep); NaN for
    // none.
    double fixedThickness = NOT_DEFINED;
    // The material direction: THETA in degrees from the line G1-G2, or the x axis of the coordinate system MCID.
    double thetaInDegrees = 0.0;
    base::EntityId mcid = -1; // negative: no system, THETA applies
    bool hasFrame = true;      // false for the elements that have no element coordinate system
    // The element x axis along G1-G2 (in the plane of the normal) instead of the bisector of the diagonals, and no
    // material axes: the shear panel (CSHEAR, format reference Figure 9-65).
    bool shearPanel = false;
    // An axisymmetric element is a ring: its volume is the area times the length of the circle its centre describes
    // around the axis (the z axis of the basic system, grid points in the x-z plane).
    bool axisymmetric = false;
};

void evaluateTriangle(const Evaluation& evaluation, const ShellInput& input, ElementValues& values);
void evaluateQuadrilateral(const Evaluation& evaluation, const ShellInput& input, ElementValues& values);

// The thickness of a shell element: the property's, or the mean of the corner thicknesses of the element when it gives
// some (a blank corner takes the property's thickness; with TFLAG 1 the corner values are fractions of it). A laminate
// has its total thickness whatever the corners say. NaN when there is no property.
[[nodiscard]] double effectiveShellThickness(const GeometryContext& context, const ShellInput& input);

} // namespace oor::analysis::detail

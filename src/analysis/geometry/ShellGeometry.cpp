#include "ShellGeometry.hpp"

#include <cmath>
#include <numbers>

namespace oor::analysis::detail
{

namespace
{

// The material coordinate system of a shell, from its normal `z` and the direction of the line G1-G2.
//
// THETA: the material x axis is the line G1-G2 (taken in the plane of the element) turned by THETA about the normal, as
// the format reference draws it for the triangles and says for the quadrilaterals (the angle starts at the G1-G2 side).
// MCID: the x axis of the coordinate system projected onto the element; for a cylindrical or spherical system, its R
// axis at the centre of the element (format reference: the vector goes onto the shell surface through the centre).
base::Mat3 materialFrameOf(const Evaluation& evaluation, const ShellInput& input, const base::Vec3& center,
                           const base::Vec3& z, const base::Vec3& g12)
{
    if (!isFinite(z))
        return nanFrame();
    if (input.mcid >= 0)
    {
        const base::Vec3 axis = evaluation.context.systemAxesAt(input.mcid, center).column(0);
        const base::Vec3 projected = axis - dot(axis, z) * z;
        if (!(base::norm(projected) > 1.0e-9))
            return nanFrame(); // the axis is perpendicular to the element: no direction to project
        return frameFromXAndZ(unitOrNan(projected), z);
    }
    const base::Vec3 along = unitOrNan(g12 - dot(g12, z) * z);
    if (!isFinite(along))
        return nanFrame();
    const double angle = input.thetaInDegrees * (std::numbers::pi / 180.0);
    const base::Vec3 x = std::cos(angle) * along + std::sin(angle) * cross(z, along);
    return frameFromXAndZ(x, z);
}

// What the triangle and the quadrilateral have in common once the area vector, the centre and the element x axis are
// known.
void finishShell(const Evaluation& evaluation, const ShellInput& input, const base::Vec3& areaVector,
                 const base::Vec3& center, const base::Vec3& elementX, const base::Vec3& g12, ElementValues& values)
{
    const double area = 0.5 * base::norm(areaVector);
    const base::Vec3 normal = unitOrNan(areaVector);
    if (evaluation.wants(Quantity::Center))
        values.center = center;
    if (evaluation.wants(Quantity::Normal))
        values.normal = normal;
    if (evaluation.wants(Quantity::Area))
        values.area = area;
    if (evaluation.wants(Quantity::Thickness) || evaluation.wants(Quantity::Volume))
    {
        const double thickness = effectiveShellThickness(evaluation.context, input);
        if (evaluation.wants(Quantity::Thickness))
            values.thickness = thickness;
        if (evaluation.wants(Quantity::Volume))
        {
            if (input.axisymmetric)
                values.volume = 2.0 * std::numbers::pi * std::fabs(center.x) * area;
            else if (input.hasThickness)
                values.volume = area * thickness;
        }
    }
    if (input.hasFrame && evaluation.wants(Quantity::Frame))
        values.frame = frameFromXAndZ(elementX, normal);
    if (input.hasFrame && !input.shearPanel && evaluation.wants(Quantity::MaterialFrame))
        values.materialFrame = materialFrameOf(evaluation, input, center, normal, g12);
}

} // namespace

double effectiveShellThickness(const GeometryContext& context, const ShellInput& input)
{
    if (!input.hasThickness)
        return NOT_DEFINED;
    if (input.fixedThickness == input.fixedThickness)
        return input.fixedThickness;
    const double property = context.shellThickness(input.pid);
    if (input.cornerThickness == nullptr || context.isLaminate(input.pid))
        return property;
    double sum = 0.0;
    for (std::size_t corner = 0; corner < input.corners.size(); ++corner)
    {
        const double given = input.cornerThickness[corner];
        if (given < 0.0)
            sum += property; // blank: the thickness of the property
        else
            sum += input.thicknessFlag == 1 ? given * property : given;
    }
    return sum / static_cast<double>(input.corners.size());
}

void evaluateTriangle(const Evaluation& evaluation, const ShellInput& input, ElementValues& values)
{
    const base::Vec3& first = input.corners[0];
    const base::Vec3& second = input.corners[1];
    const base::Vec3& third = input.corners[2];
    if (!isFinite(first) || !isFinite(second) || !isFinite(third))
        return;
    const base::Vec3 side = second - first;
    const base::Vec3 areaVector = cross(side, third - first);
    finishShell(evaluation, input, areaVector, (first + second + third) / 3.0, unitOrNan(side), side, values);
}

void evaluateQuadrilateral(const Evaluation& evaluation, const ShellInput& input, ElementValues& values)
{
    const base::Vec3& first = input.corners[0];
    const base::Vec3& second = input.corners[1];
    const base::Vec3& third = input.corners[2];
    const base::Vec3& fourth = input.corners[3];
    if (!isFinite(first) || !isFinite(second) || !isFinite(third) || !isFinite(fourth))
        return;
    // The area vector is half of the cross product of the diagonals (the planar quadrilateral's area, and for a warped
    // one the area of its projection on the mean plane); the element x axis bisects the angle between the diagonals
    // (format reference, the figure of the CQUAD4 element coordinate system).
    const base::Vec3 diagonal13 = third - first;
    const base::Vec3 diagonal24 = fourth - second;
    const base::Vec3 areaVector = cross(diagonal13, diagonal24);
    base::Vec3 elementX = unitOrNan(unitOrNan(diagonal13) - unitOrNan(diagonal24));
    if (input.shearPanel)
    {
        // x from G1 to G2, in the plane normal to the cross product of the diagonals
        const base::Vec3 normal = unitOrNan(areaVector);
        const base::Vec3 side = second - first;
        elementX = unitOrNan(side - dot(side, normal) * normal);
    }
    finishShell(evaluation, input, areaVector, (first + second + third + fourth) / 4.0, elementX, second - first,
                values);
}

} // namespace oor::analysis::detail

#include "ElementProvider.hpp"
#include "oor/analysis/geometry/SurfaceSides.hpp"
#include "oor/model/cards/elements/bar2/Bar2Table.hpp"
#include "oor/model/cards/elements/beam2/Beam2Table.hpp"
#include "oor/model/cards/elements/solidHexa/SolidHexaTable.hpp"
#include "oor/model/cards/elements/solidPenta/SolidPentaTable.hpp"
#include "oor/model/cards/elements/solidPyramid/SolidPyramidTable.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/shellQuad8/ShellQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4Drilling/ShellQuad4DrillingTable.hpp"
#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "oor/model/cards/elements/rod2InlineSection/Rod2InlineSectionTable.hpp"
#include "oor/model/cards/elements/solidTetra/SolidTetraTable.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/cards/elements/shellTria6/ShellTria6Table.hpp"
#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingTable.hpp"
#include "oor/model/cards/elements/tube2/Tube2Table.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceNodes/HeatSurfaceNodesTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProperty/HeatSurfacePropertyTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceSide/HeatSurfaceSideTable.hpp"
#include "oor/model/cards/thermal/heatSurfaceProp/HeatSurfacePropTable.hpp"

#include <array>
#include <cmath>
#include <numbers>
#include <optional>
#include <vector>

// The heat transfer surface elements: CHBDYG (a surface of grid points), CHBDYE (a side of a conduction element) and
// CHBDYP (a point, a line or a tube of grid points with a PHBDY).
//
// What each gives (with the data of its PHBDY):
//   * an area surface (AREA3, AREA4, AREA6, AREA8, a face of a solid, the top and the bottom of a shell): its area,
//   centre
//     (the average of the corners) and unit normal; the mid-side points are not used, as for the shells, and the area
//     of a quadrilateral is half the cross product of its diagonals. The normal of a CHBDYG follows the right-hand rule
//     on its points; that of a face of a solid points out of the element (the front face of the format reference); that
//     of the top of a shell is the shell's own and that of its bottom the opposite one;
//   * a surface of revolution (CHBDYG REV, REV1): the lateral area of the cone the segment G1-G2 describes about the z
//     axis (REV, in the x-z plane) or the y axis (REV1, in the x-y plane), pi (r1 + r2) times the slant length; the
//     normal is (ey x T) for REV (figure 9-28) and, the format reference giving none for REV1, the same rule with the
//     axis turned from z to y: (T x ez), T being the unit vector from G1 to G2 (outward when the segment runs along the
//     axis);
//   * a line (a CHBDYP LINE, ELCYL, FTUBE or TUBE, an edge of a shell, the linear side of a line element): its length
//   and
//     centre. The area of a LINE or an ELCYL is the area factor AF of the PHBDY times the length; that
//     of a FTUBE or an outer TUBE is the lateral area of the truncated cone of the diameters D1 and D2, pi (D1 + D2) /
//     2 times the slant length (the inner side of a TUBE is not told apart: the area is the outer one); the area of an
//     edge of a shell is its length times the thickness of the shell. The normal of a LINE or an ELCYL lies in the
//     plane of the line and its orientation vector V (G0 - G1, or E1 E2 E3 in the system CE), perpendicular to the
//     line: T x (V x T) / |T x (V x T)| (figure 9-32); that of a FTUBE or a TUBE is not defined; that of an edge of a
//     shell lies in the plane of the shell, away from it;
//   * a point (a CHBDYP POINT, the end of a line element): the area factor AF of the PHBDY, or the cross-section of the
//     line element, and the normal (G1 to G0 or the vector E1 E2 E3; the axis of the element, pointing away from it,
//     for the end of a line element).
// A side of an element this library has no side numbering for (the axisymmetric elements, and the elements the format
// reference gives none for) gives nothing (NaN).

namespace oor::analysis::detail
{

namespace
{

using base::Vec3;

bool allFinite(std::span<const Vec3> points)
{
    for (const Vec3& point : points)
    {
        if (!isFinite(point))
            return false;
    }
    return true;
}

Vec3 averageOf(std::span<const Vec3> points)
{
    Vec3 sum{0.0, 0.0, 0.0};
    for (const Vec3& point : points)
        sum += point;
    return sum / static_cast<double>(points.size());
}

// Twice the area, along the normal, of a triangle or a quadrilateral (the diagonals' cross product).
Vec3 areaVectorOf(std::span<const Vec3> corners)
{
    if (corners.size() == 3)
        return base::cross(corners[1] - corners[0], corners[2] - corners[0]);
    return base::cross(corners[2] - corners[0], corners[3] - corners[1]);
}

// Gives what an area surface gives, from its corners. `insidePoint` is a point inside the element the surface bounds
// (the normal is turned away from it), NaN for none; `reverse` turns the normal around (the bottom of a shell).
void setAreaSurface(const Evaluation& evaluation, std::span<const Vec3> corners, const Vec3& insidePoint, bool reverse,
                    ElementValues& values)
{
    if (!allFinite(corners))
        return;
    const Vec3 center = averageOf(corners);
    Vec3 areaVector = areaVectorOf(corners);
    if (isFinite(insidePoint) && base::dot(areaVector, insidePoint - center) > 0.0)
        areaVector = -areaVector;
    if (reverse)
        areaVector = -areaVector;
    if (evaluation.wants(Quantity::Center))
        values.center = center;
    if (evaluation.wants(Quantity::Area))
        values.area = 0.5 * base::norm(areaVector);
    if (evaluation.wants(Quantity::Normal))
        values.normal = unitOrNan(areaVector);
}

// The cone the segment `first`-`second` describes about an axis in the direction `axis`; the distance of each end to
// the axis is the x coordinate (the grid points are in the x-z plane for REV, the x-y plane for REV1).
void setRevolution(const Evaluation& evaluation, const Vec3& first, const Vec3& second, const Vec3& axis,
                   ElementValues& values)
{
    if (!isFinite(first) || !isFinite(second))
        return;
    const Vec3 tangent = second - first;
    const double slant = base::norm(tangent);
    if (evaluation.wants(Quantity::Center))
        values.center = 0.5 * (first + second);
    if (evaluation.wants(Quantity::Length))
        values.length = slant;
    if (evaluation.wants(Quantity::Area))
        values.area = std::numbers::pi * (std::fabs(first.x) + std::fabs(second.x)) * slant;
    if (evaluation.wants(Quantity::Normal))
        values.normal = unitOrNan(base::cross(axis, tangent));
}

// ----- CHBDYG -----------------------------------------------------------------------------------------------------

struct HeatSurfaceNodesShape
{
    static void nodes(const model::HeatSurfaceNodesTable& table, std::uint32_t row,
                      std::vector<base::EntityId>& ids)
    {
        const model::HeatSurfaceNodesCard card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }
    static void evaluate(const model::HeatSurfaceNodesTable& table, const Evaluation& evaluation,
                         ElementValues& values)
    {
        const model::HeatSurfaceNodesCard card = table.card(evaluation.row);
        const std::span<const Vec3> points = evaluation.points;
        const Vec3 none = undefinedVector();
        if (card.kind == "AreaTria3" || card.kind == "AreaTria6")
            setAreaSurface(evaluation, points.first(3), none, false, values);
        else if (card.kind == "AreaQuad4" || card.kind == "AreaQuad8")
            setAreaSurface(evaluation, points.first(4), none, false, values);
        else if (card.kind == "ReversedNormal")
            setRevolution(evaluation, points[0], points[1], Vec3{0.0, 1.0, 0.0}, values);
        else if (card.kind == "ReversedNormal1")
            setRevolution(evaluation, points[0], points[1], Vec3{0.0, 0.0, -1.0}, values);
    }
};

// ----- CHBDYP -----------------------------------------------------------------------------------------------------

// The orientation vector of a CHBDYP, from G1: the line to G0 when there is one, else E1 E2 E3 in the system CE.
Vec3 orientationOf(const Evaluation& evaluation, const model::HeatSurfacePropertyCard& card)
{
    const Vec3& first = evaluation.points[0];
    if (!isFinite(first))
        return undefinedVector();
    if (card.orientationNode != 0)
        return isFinite(evaluation.points[2]) ? evaluation.points[2] - first : undefinedVector();
    if (!card.direction1 && !card.direction2 && !card.direction3)
        return undefinedVector();
    const Vec3 components{card.direction1.value_or(0.0), card.direction2.value_or(0.0), card.direction3.value_or(0.0)};
    return evaluation.context.systemAxesAt(card.directionFrameId, first) * components;
}

struct HeatSurfacePropertyShape
{
    static void nodes(const model::HeatSurfacePropertyTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::HeatSurfacePropertyCard card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
        ids.push_back(card.orientationNode);
    }
    static void evaluate(const model::HeatSurfacePropertyTable& table, const Evaluation& evaluation,
                         ElementValues& values)
    {
        const model::HeatSurfacePropertyCard card = table.card(evaluation.row);
        const Vec3& first = evaluation.points[0];
        const Vec3& second = evaluation.points[1];
        // The PHBDY of the surface (a property of its own: its IDs are not those of the structural properties).
        std::optional<model::HeatSurfacePropCard> property;
        const model::Model& model = evaluation.context.model();
        if (const auto location = model.locateInType(model::typeIndexOf<model::HeatSurfacePropTable>(), card.propId))
        {
            const model::CardTable* phbdys = model.findTable(location->table);
            if (phbdys != nullptr)
                property = static_cast<const model::HeatSurfacePropTable&>(*phbdys).card(location->row);
        }
        if (card.kind == "Point")
        {
            if (!isFinite(first))
                return;
            if (evaluation.wants(Quantity::Center))
                values.center = first;
            if (evaluation.wants(Quantity::Area) && property && property->area)
                values.area = *property->area;
            if (evaluation.wants(Quantity::Normal))
                values.normal = unitOrNan(orientationOf(evaluation, card));
            return;
        }
        if (!isFinite(first) || !isFinite(second))
            return;
        const Vec3 tangent = second - first;
        const double length = base::norm(tangent);
        if (evaluation.wants(Quantity::Center))
            values.center = 0.5 * (first + second);
        if (evaluation.wants(Quantity::Length))
            values.length = length;
        if (evaluation.wants(Quantity::Area) && property)
        {
            if (card.kind == "Line" || card.kind == "EllipticCylinder")
            {
                if (property->area)
                    values.area = *property->area * length;
            }
            else if (property->diameter1 && property->diameter2)
            {
                // FTUBE and TUBE: the cone of the diameters D1 at G1 and D2 at G2.
                const double mean = 0.5 * (*property->diameter1 + *property->diameter2);
                const double radialStep = 0.5 * (*property->diameter1 - *property->diameter2);
                values.area = std::numbers::pi * mean * std::sqrt(length * length + radialStep * radialStep);
            }
        }
        if (evaluation.wants(Quantity::Normal) && (card.kind == "Line" || card.kind == "EllipticCylinder"))
        {
            const Vec3 axis = unitOrNan(tangent);
            const Vec3 orientation = orientationOf(evaluation, card);
            if (isFinite(axis) && isFinite(orientation))
                values.normal = unitOrNan(base::cross(axis, base::cross(orientation, axis)));
        }
    }
};

// ----- CHBDYE -----------------------------------------------------------------------------------------------------

enum class SideKind
{
    None,
    Area,
    Line,
    Point
};

// A side of a conduction element: what it is, and which corners of the element (0-based, in the order of its node list)
// make it up.
struct Side
{
    SideKind kind = SideKind::None;
    std::array<std::uint8_t, 4> corners = {};
    std::size_t count = 0;
};

enum class ElementKind
{
    None,
    Solid,
    Shell,
    Line
};

// What the side needs to know of the element it belongs to.
struct Underlying
{
    ElementKind kind = ElementKind::None;
    std::vector<base::EntityId> corners; // the corner node IDs, in the order of the element
    base::EntityId pid = 0;              // a shell's or a line element's property
    double ownArea = NOT_DEFINED;        // a CONROD's cross-section
};

using SideTable = std::array<std::array<std::uint8_t, 4>, 6>;

// The sides of the solid elements, as the corners of each.
constexpr SideTable HEXA_SIDES = {{{3, 2, 1, 0}, {0, 1, 5, 4}, {1, 2, 6, 5}, {2, 3, 7, 6}, {3, 0, 4, 7}, {4, 5, 6, 7}}};
constexpr SideTable PENTA_SIDES = {{{2, 1, 0, 0}, {0, 1, 4, 3}, {1, 2, 5, 4}, {2, 0, 3, 5}, {3, 4, 5, 0}, {}}};
constexpr SideTable TETRA_SIDES = {{{2, 1, 0, 0}, {0, 1, 3, 0}, {1, 2, 3, 0}, {2, 0, 3, 0}, {}, {}}};
constexpr SideTable PYRAMID_SIDES = {{{2, 1, 4, 0}, {0, 3, 4, 0}, {1, 0, 4, 0}, {2, 4, 3, 0}, {3, 0, 1, 2}, {}}};

// `triangleMask` has bit n set when side n + 1 has three corners.
Side solidSide(int number, const SideTable& table, std::size_t sideCount, std::size_t triangleMask)
{
    Side side;
    if (number < 1 || static_cast<std::size_t>(number) > sideCount)
        return side;
    side.kind = SideKind::Area;
    side.corners = table[static_cast<std::size_t>(number - 1)];
    side.count = (triangleMask & (std::size_t{1} << (number - 1))) != 0 ? 3 : 4;
    return side;
}

Side shellSide(int number, std::size_t cornerCount)
{
    Side side;
    if (number == 1 || number == 6)
    {
        side.kind = SideKind::Area;
        side.count = cornerCount;
        for (std::size_t index = 0; index < cornerCount; ++index)
            side.corners[index] = static_cast<std::uint8_t>(index);
    }
    else if (number >= 2 && static_cast<std::size_t>(number - 2) < cornerCount)
    {
        side.kind = SideKind::Line;
        side.count = 2;
        side.corners[0] = static_cast<std::uint8_t>(number - 2);
        side.corners[1] = static_cast<std::uint8_t>((number - 1) % static_cast<int>(cornerCount));
    }
    return side;
}

Side lineSide(int number)
{
    Side side;
    if (number == 1)
    {
        side.kind = SideKind::Line;
        side.count = 2;
        side.corners = {0, 1, 0, 0};
    }
    else if (number == 2 || number == 3)
    {
        side.kind = SideKind::Point;
        side.count = 1;
        side.corners = {static_cast<std::uint8_t>(number - 2), 0, 0, 0};
    }
    return side;
}

template <typename Card>
void takeCorners(const Card& card, std::size_t count, Underlying& underlying)
{
    underlying.corners.assign(card.nodes.begin(), card.nodes.begin() + static_cast<std::ptrdiff_t>(count));
}

template <typename TableT>
auto cardAt(const model::Model& model, const model::CardLocation& location)
{
    return static_cast<const TableT&>(*model.findTable(location.table)).card(location.row);
}

// The side `number` of the conduction element in `element`; a Side of kind None when the element has no side numbering
// here.
Side sideOfElement(const model::Model& model, const model::CardLocation& element, int number, Underlying& underlying)
{
    const std::uint16_t type = element.table;
    if (type == model::typeIndexOf<model::SolidHexaTable>())
    {
        underlying.kind = ElementKind::Solid;
        takeCorners(cardAt<model::SolidHexaTable>(model, element), 8, underlying);
        return solidSide(number, HEXA_SIDES, 6, 0);
    }
    if (type == model::typeIndexOf<model::SolidPentaTable>())
    {
        underlying.kind = ElementKind::Solid;
        takeCorners(cardAt<model::SolidPentaTable>(model, element), 6, underlying);
        return solidSide(number, PENTA_SIDES, 5, 0b10001);
    }
    if (type == model::typeIndexOf<model::SolidTetraTable>())
    {
        underlying.kind = ElementKind::Solid;
        takeCorners(cardAt<model::SolidTetraTable>(model, element), 4, underlying);
        return solidSide(number, TETRA_SIDES, 4, 0b1111);
    }
    if (type == model::typeIndexOf<model::SolidPyramidTable>())
    {
        underlying.kind = ElementKind::Solid;
        takeCorners(cardAt<model::SolidPyramidTable>(model, element), 5, underlying);
        return solidSide(number, PYRAMID_SIDES, 5, 0b01111);
    }
    // The shells: side 1 is the top (an area), 2 to the number of corners plus 1 are the edges G1-G2, G2-G3, ...
    // (lines), 6 is the bottom.
    if (type == model::typeIndexOf<model::ShellQuad4Table>())
    {
        const auto card = cardAt<model::ShellQuad4Table>(model, element);
        underlying.kind = ElementKind::Shell;
        underlying.pid = card.propId;
        takeCorners(card, 4, underlying);
        return shellSide(number, 4);
    }
    if (type == model::typeIndexOf<model::ShellQuad4DrillingTable>())
    {
        const auto card = cardAt<model::ShellQuad4DrillingTable>(model, element);
        underlying.kind = ElementKind::Shell;
        underlying.pid = card.propId;
        takeCorners(card, 4, underlying);
        return shellSide(number, 4);
    }
    if (type == model::typeIndexOf<model::ShellQuad8Table>())
    {
        const auto card = cardAt<model::ShellQuad8Table>(model, element);
        underlying.kind = ElementKind::Shell;
        underlying.pid = card.propId;
        takeCorners(card, 4, underlying);
        return shellSide(number, 4);
    }
    if (type == model::typeIndexOf<model::ShellTria3Table>())
    {
        const auto card = cardAt<model::ShellTria3Table>(model, element);
        underlying.kind = ElementKind::Shell;
        underlying.pid = card.propId;
        takeCorners(card, 3, underlying);
        return shellSide(number, 3);
    }
    if (type == model::typeIndexOf<model::ShellTria3DrillingTable>())
    {
        const auto card = cardAt<model::ShellTria3DrillingTable>(model, element);
        underlying.kind = ElementKind::Shell;
        underlying.pid = card.propId;
        takeCorners(card, 3, underlying);
        return shellSide(number, 3);
    }
    if (type == model::typeIndexOf<model::ShellTria6Table>())
    {
        const auto card = cardAt<model::ShellTria6Table>(model, element);
        underlying.kind = ElementKind::Shell;
        underlying.pid = card.propId;
        takeCorners(card, 3, underlying);
        return shellSide(number, 3);
    }
    // The line elements: side 1 is the linear side, 2 and 3 the point sides at the first and the second grid point.
    if (type == model::typeIndexOf<model::Rod2Table>())
    {
        const auto card = cardAt<model::Rod2Table>(model, element);
        underlying.kind = ElementKind::Line;
        underlying.pid = card.propId;
        takeCorners(card, 2, underlying);
        return lineSide(number);
    }
    if (type == model::typeIndexOf<model::Rod2InlineSectionTable>())
    {
        const auto card = cardAt<model::Rod2InlineSectionTable>(model, element);
        underlying.kind = ElementKind::Line;
        underlying.ownArea = card.area;
        takeCorners(card, 2, underlying);
        return lineSide(number);
    }
    if (type == model::typeIndexOf<model::Tube2Table>())
    {
        const auto card = cardAt<model::Tube2Table>(model, element);
        underlying.kind = ElementKind::Line;
        underlying.pid = card.propId;
        takeCorners(card, 2, underlying);
        return lineSide(number);
    }
    if (type == model::typeIndexOf<model::Bar2Table>())
    {
        const auto card = cardAt<model::Bar2Table>(model, element);
        underlying.kind = ElementKind::Line;
        underlying.pid = card.propId;
        underlying.corners = {card.nodeA, card.nodeB};
        return lineSide(number);
    }
    if (type == model::typeIndexOf<model::Beam2Table>())
    {
        const auto card = cardAt<model::Beam2Table>(model, element);
        underlying.kind = ElementKind::Line;
        underlying.pid = card.propId;
        underlying.corners = {card.nodeA, card.nodeB};
        return lineSide(number);
    }
    return Side{};
}

// The basic position of the grid point with this ID; NaN when it is not one.
Vec3 positionOf(const Evaluation& evaluation, base::EntityId id)
{
    const model::Model& model = evaluation.context.model();
    if (const auto location = model.locate(base::IdSpace::Node, id))
    {
        if (location->table == model::typeIndexOf<model::NodeTable>())
            return evaluation.context.cache().position(location->row);
    }
    return undefinedVector();
}

std::vector<Vec3> positionsOf(const Evaluation& evaluation, const std::vector<base::EntityId>& ids)
{
    std::vector<Vec3> points;
    points.reserve(ids.size());
    for (const base::EntityId id : ids)
        points.push_back(positionOf(evaluation, id));
    return points;
}

struct HeatSurfaceSideShape
{
    // The points of a side are the ones of the element it is a side of, which this table cannot look up: evaluate()
    // does.
    static void nodes(const model::HeatSurfaceSideTable&, std::uint32_t, std::vector<base::EntityId>&) {}

    static void evaluate(const model::HeatSurfaceSideTable& table, const Evaluation& evaluation,
                         ElementValues& values)
    {
        const model::HeatSurfaceSideCard card = table.card(evaluation.row);
        const model::Model& model = evaluation.context.model();
        const auto element = model.locate(base::IdSpace::Element, card.conductionElemId);
        if (!element)
            return;
        Underlying underlying;
        const Side side = sideOfElement(model, *element, card.sideCode, underlying);
        if (side.kind == SideKind::None)
            return;
        const std::vector<Vec3> corners = positionsOf(evaluation, underlying.corners);
        std::array<Vec3, 4> sidePoints;
        for (std::size_t index = 0; index < side.count; ++index)
        {
            if (side.corners[index] >= corners.size())
                return;
            sidePoints[index] = corners[side.corners[index]];
        }
        const std::span<const Vec3> points(sidePoints.data(), side.count);
        if (!allFinite(points))
            return;

        if (side.kind == SideKind::Area)
        {
            const bool solid = underlying.kind == ElementKind::Solid;
            const Vec3 inside = solid && allFinite(corners) ? averageOf(corners) : undefinedVector();
            setAreaSurface(evaluation, points, inside, underlying.kind == ElementKind::Shell && card.sideCode == 6,
                           values);
            return;
        }
        if (side.kind == SideKind::Line)
        {
            const Vec3 tangent = points[1] - points[0];
            if (evaluation.wants(Quantity::Center))
                values.center = 0.5 * (points[0] + points[1]);
            if (evaluation.wants(Quantity::Length))
                values.length = base::norm(tangent);
            if (underlying.kind == ElementKind::Shell)
            {
                // The edge of a shell: as long as the edge and as thick as the shell; its normal lies in the plane of
                // the shell, away from it.
                if (evaluation.wants(Quantity::Area))
                    values.area = base::norm(tangent) * evaluation.context.shellThickness(underlying.pid);
                if (evaluation.wants(Quantity::Normal) && allFinite(corners))
                {
                    const Vec3 shellNormal = unitOrNan(areaVectorOf(corners));
                    values.normal = unitOrNan(base::cross(tangent, shellNormal));
                }
            }
            return;
        }
        // A point side of a line element: its end, with the cross-section of the element and the axis of the element
        // pointing away from it.
        if (evaluation.wants(Quantity::Center))
            values.center = points[0];
        if (evaluation.wants(Quantity::Normal) && corners.size() == 2 && isFinite(corners[0]) && isFinite(corners[1]))
            values.normal = unitOrNan(card.sideCode == 2 ? corners[0] - corners[1] : corners[1] - corners[0]);
        if (evaluation.wants(Quantity::Area))
        {
            values.area = underlying.ownArea == underlying.ownArea ? underlying.ownArea
                                                                   : evaluation.context.sectionArea(underlying.pid);
        }
    }
};

} // namespace

void registerSurfaceProviders(ProviderRegistry& registry)
{
    registry.add<model::HeatSurfaceNodesTable, HeatSurfaceNodesShape>();
    registry.add<model::HeatSurfacePropertyTable, HeatSurfacePropertyShape>();
    registry.add<model::HeatSurfaceSideTable, HeatSurfaceSideShape>();
}

} // namespace oor::analysis::detail

namespace oor::analysis
{

SurfaceSide surfaceSideOf(const model::Model& model, model::CardLocation element, int side)
{
    detail::Underlying underlying;
    const detail::Side found = detail::sideOfElement(model, element, side, underlying);
    if (found.kind != detail::SideKind::None)
        return SurfaceSide::Valid;
    return underlying.kind == detail::ElementKind::None ? SurfaceSide::NotNumbered : SurfaceSide::NoSuchSide;
}

SurfaceDescription describeSurface(const model::Model& model, model::CardLocation surface)
{
    SurfaceDescription description;
    const std::uint16_t type = surface.table;
    if (type == model::typeIndexOf<model::HeatSurfaceNodesTable>())
    {
        const auto card = detail::cardAt<model::HeatSurfaceNodesTable>(model, surface);
        if (card.kind == "ReversedNormal" || card.kind == "ReversedNormal1")
        {
            description.kind = SurfaceKind::Line;
            description.corners = 2;
        }
        else
        {
            description.kind = SurfaceKind::Area;
            description.corners = card.kind == "AreaTria3" || card.kind == "AreaTria6" ? 3 : 4;
        }
        for (const base::EntityId node : card.nodes)
        {
            if (node != 0)
                description.nodes.push_back(node);
        }
    }
    else if (type == model::typeIndexOf<model::HeatSurfacePropertyTable>())
    {
        const auto card = detail::cardAt<model::HeatSurfacePropertyTable>(model, surface);
        const bool point = card.kind == "Point";
        description.kind = point ? SurfaceKind::Point : SurfaceKind::Line;
        description.corners = point ? 1 : 2;
        description.nodes.push_back(card.node1);
        if (!point)
            description.nodes.push_back(card.node2);
    }
    else if (type == model::typeIndexOf<model::HeatSurfaceSideTable>())
    {
        const auto card = detail::cardAt<model::HeatSurfaceSideTable>(model, surface);
        const auto element = model.locate(base::IdSpace::Element, card.conductionElemId);
        if (!element)
            return description;
        detail::Underlying underlying;
        const detail::Side side = detail::sideOfElement(model, *element, card.sideCode, underlying);
        if (side.kind == detail::SideKind::None)
            return description;
        description.kind = side.kind == detail::SideKind::Area   ? SurfaceKind::Area
                           : side.kind == detail::SideKind::Line ? SurfaceKind::Line
                                                                 : SurfaceKind::Point;
        description.corners = side.count;
        for (std::size_t index = 0; index < side.count; ++index)
        {
            if (side.corners[index] < underlying.corners.size())
                description.nodes.push_back(underlying.corners[side.corners[index]]);
        }
    }
    return description;
}

} // namespace oor::analysis

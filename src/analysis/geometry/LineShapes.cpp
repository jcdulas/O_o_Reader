#include "ElementProvider.hpp"
#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/model/cards/elements/BarOffsetKinds.hpp"
#include "oor/model/cards/elements/bar2/Bar2Table.hpp"
#include "oor/model/cards/elements/beam2/Beam2Table.hpp"
#include "oor/model/cards/elements/beam3/Beam3Table.hpp"
#include "oor/model/cards/elements/bushing/BushingTable.hpp"
#include "oor/model/cards/elements/bushing1D/Bushing1DTable.hpp"
#include "oor/model/cards/elements/curvedBeam2/CurvedBeam2Table.hpp"
#include "oor/model/cards/elements/damperDofProperty/DamperDofPropertyTable.hpp"
#include "oor/model/cards/elements/damperDofInline/DamperDofInlineTable.hpp"
#include "oor/model/cards/elements/fastener/FastenerTable.hpp"
#include "oor/model/cards/elements/gap2/Gap2Table.hpp"
#include "oor/model/cards/elements/massDofProperty/MassDofPropertyTable.hpp"
#include "oor/model/cards/elements/massDofInline/MassDofInlineTable.hpp"
#include "oor/model/cards/elements/plotLine2/PlotLine2Table.hpp"
#include "oor/model/cards/elements/rod2/Rod2Table.hpp"
#include "oor/model/cards/elements/rod2InlineSection/Rod2InlineSectionTable.hpp"
#include "oor/model/cards/elements/springDofProperty/SpringDofPropertyTable.hpp"
#include "oor/model/cards/elements/springDofInline/SpringDofInlineTable.hpp"
#include "oor/model/cards/elements/tube2/Tube2Table.hpp"
#include "oor/model/cards/elements/viscousDamper2/ViscousDamper2Table.hpp"
#include "oor/model/cards/elements/weld/WeldTable.hpp"
#include "oor/model/cards/explicit/markerBody/MarkerBodyTable.hpp"

#include <string>

// The line elements: rods, tubes, bars, beams, bends, bushes, gaps, springs, dampers, viscous dampers, scalar-and-grid
// mass elements, plot elements, fasteners and welds.
//
// For every one the centre is the middle of the segment between its two end points (the point itself when only one end
// is a grid point) and the length is the distance between the ends; an end that is a scalar point or is blank gives
// nothing, so a spring between scalar points has no geometry at all (NaN). Bars and beams have end points shifted by
// their offset vectors (the format reference draws the element between the offset ends), an element coordinate system,
// and the area of their property.

namespace oor::analysis::detail
{

namespace
{

using base::Vec3;

// The segment between two points, either of which may be missing (a scalar point, or a blank field).
void evaluateSegment(const Evaluation& evaluation, const Vec3& first, const Vec3& second, ElementValues& values)
{
    const bool hasFirst = isFinite(first);
    const bool hasSecond = isFinite(second);
    if (evaluation.wants(Quantity::Center))
    {
        if (hasFirst && hasSecond)
            values.center = 0.5 * (first + second);
        else if (hasFirst)
            values.center = first;
        else if (hasSecond)
            values.center = second;
    }
    if (hasFirst && hasSecond && evaluation.wants(Quantity::Length))
        values.length = base::norm(second - first);
}

// A rod, a tube or a bar with a section: length, centre, cross-section area and volume.
void evaluateRod(const Evaluation& evaluation, const Vec3& first, const Vec3& second, double area,
                 ElementValues& values)
{
    if (!isFinite(first) || !isFinite(second))
        return;
    const double length = base::norm(second - first);
    if (evaluation.wants(Quantity::Center))
        values.center = 0.5 * (first + second);
    if (evaluation.wants(Quantity::Length))
        values.length = length;
    if (evaluation.wants(Quantity::Area))
        values.area = area;
    if (evaluation.wants(Quantity::Volume))
        values.volume = area * length;
}

// The interpretation of the OFFT string of a bar or a beam: the first character says
// how the orientation vector is read (G: components in the displacement system of GA, B: in the basic system), the
// second and third how the offsets at ends A and B are read (G: in the displacement system of the grid point, B: basic,
// O or E: in the offset system, whose x axis goes from GA to GB and whose z axis is perpendicular to it and to the
// orientation vector).
struct OffsetFlags
{
    char vector = 'G';
    char endA = 'G';
    char endB = 'G';
};

OffsetFlags parseOffsetFlags(const std::string& kind)
{
    OffsetFlags flags;
    const std::string_view text = model::barOffsetLetters(kind);
    if (text.size() > 0)
        flags.vector = text[0];
    if (text.size() > 1)
        flags.endA = text[1];
    if (text.size() > 2)
        flags.endB = text[2];
    return flags;
}

struct BarInput
{
    OffsetFlags flags;
    bool hasOrientationNode = false; // the orientation vector goes from GA to the grid point G0 (the third node)
    Vec3 orientation;                // X1 X2 X3 when there is no G0
    bool hasOrientationVector = false;
    Vec3 offsetA;
    Vec3 offsetB;
};

struct BarGeometry
{
    Vec3 endA = base::Vec3{NOT_DEFINED, NOT_DEFINED, NOT_DEFINED};
    Vec3 endB = base::Vec3{NOT_DEFINED, NOT_DEFINED, NOT_DEFINED};
    base::Mat3 frame = nanFrame();
};

// The end points (offsets included) and the element coordinate system of a bar or a beam; nodes are GA, GB and G0 in
// that order.
BarGeometry computeBar(const Evaluation& evaluation, const BarInput& input)
{
    BarGeometry result;
    const Vec3& pa = evaluation.points[0];
    const Vec3& pb = evaluation.points[1];
    if (!isFinite(pa) || !isFinite(pb))
        return result;
    const base::Mat3 rotationA =
        evaluation.nodeRows[0] >= 0
            ? evaluation.context.displacementRotation(static_cast<std::uint32_t>(evaluation.nodeRows[0]))
            : base::Mat3::identity();
    const base::Mat3 rotationB =
        evaluation.nodeRows[1] >= 0
            ? evaluation.context.displacementRotation(static_cast<std::uint32_t>(evaluation.nodeRows[1]))
            : base::Mat3::identity();

    // The orientation vector, in the basic system.
    Vec3 v = undefinedVector();
    if (input.hasOrientationNode)
    {
        const Vec3& go = evaluation.points[2];
        if (isFinite(go))
            v = go - pa;
    }
    else if (input.hasOrientationVector)
    {
        v = input.flags.vector == 'B' ? input.orientation : rotationA * input.orientation;
    }

    // The offset vectors, in the basic system. The offset system uses the segment GA-GB itself and the orientation
    // vector.
    const auto offsetOf = [&](char flag, const Vec3& offset, const base::Mat3& rotation)
    {
        if (flag == 'B')
            return offset;
        if (flag == 'O' || flag == 'E')
        {
            const base::Mat3 system = frameFromAxisAndPlaneVector(pb - pa, v);
            return isFinite(system.column(0)) ? system * offset : undefinedVector();
        }
        return rotation * offset;
    };
    const Vec3 offsetA = offsetOf(input.flags.endA, input.offsetA, rotationA);
    const Vec3 offsetB = offsetOf(input.flags.endB, input.offsetB, rotationB);
    if (!isFinite(offsetA) || !isFinite(offsetB))
        return result;
    result.endA = pa + offsetA;
    result.endB = pb + offsetB;
    if (isFinite(v))
        result.frame = frameFromAxisAndPlaneVector(result.endB - result.endA, v);
    return result;
}

void evaluateBar(const Evaluation& evaluation, const BarInput& input, double area, ElementValues& values)
{
    const BarGeometry bar = computeBar(evaluation, input);
    if (!isFinite(bar.endA) || !isFinite(bar.endB))
        return;
    evaluateRod(evaluation, bar.endA, bar.endB, area, values);
    if (evaluation.wants(Quantity::Frame))
        values.frame = bar.frame;
}

template <typename NodeArray>
void appendNodes(const NodeArray& nodes, std::vector<base::EntityId>& ids)
{
    ids.insert(ids.end(), nodes.begin(), nodes.end());
}

// --- rods and tubes
// -------------------------------------------------------------------------------------------------------

struct Rod2Shape
{
    static void nodes(const model::Rod2Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::Rod2Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const double area = evaluation.wants(Quantity::Area) || evaluation.wants(Quantity::Volume)
                                ? evaluation.context.sectionArea(table.card(evaluation.row).propId)
                                : NOT_DEFINED;
        evaluateRod(evaluation, evaluation.points[0], evaluation.points[1], area, values);
    }
};

struct Rod2InlineSectionShape
{
    static void nodes(const model::Rod2InlineSectionTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::Rod2InlineSectionTable& table, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateRod(evaluation, evaluation.points[0], evaluation.points[1], table.card(evaluation.row).area, values);
    }
};

struct Tube2Shape
{
    static void nodes(const model::Tube2Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::Tube2Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const double area = evaluation.wants(Quantity::Area) || evaluation.wants(Quantity::Volume)
                                ? evaluation.context.sectionArea(table.card(evaluation.row).propId)
                                : NOT_DEFINED;
        evaluateRod(evaluation, evaluation.points[0], evaluation.points[1], area, values);
    }
};

// --- bars and beams
// -------------------------------------------------------------------------------------------------------

struct Bar2Shape
{
    static void nodes(const model::Bar2Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::Bar2Card card = table.card(row);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
        ids.push_back(card.orientationNode);
    }
    static void evaluate(const model::Bar2Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::Bar2Card card = table.card(evaluation.row);
        BarInput input;
        input.flags = parseOffsetFlags(card.offsetKind);
        input.hasOrientationNode = card.orientationNode != 0;
        input.orientation = Vec3{card.orientation[0], card.orientation[1], card.orientation[2]};
        input.hasOrientationVector = true;
        input.offsetA = Vec3{card.offsetA[0], card.offsetA[1], card.offsetA[2]};
        input.offsetB = Vec3{card.offsetB[0], card.offsetB[1], card.offsetB[2]};
        const double area = evaluation.wants(Quantity::Area) || evaluation.wants(Quantity::Volume)
                                ? evaluation.context.sectionArea(card.propId)
                                : NOT_DEFINED;
        evaluateBar(evaluation, input, area, values);
    }
};

struct Beam2Shape
{
    static void nodes(const model::Beam2Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::Beam2Card card = table.card(row);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
        ids.push_back(card.orientationNode);
    }
    static void evaluate(const model::Beam2Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::Beam2Card card = table.card(evaluation.row);
        BarInput input;
        input.flags = parseOffsetFlags(card.offsetKind);
        input.hasOrientationNode = card.orientationNode != 0;
        input.orientation = Vec3{card.orientation[0], card.orientation[1], card.orientation[2]};
        input.hasOrientationVector = true;
        input.offsetA = Vec3{card.offsetA[0], card.offsetA[1], card.offsetA[2]};
        input.offsetB = Vec3{card.offsetB[0], card.offsetB[1], card.offsetB[2]};
        const double area = evaluation.wants(Quantity::Area) || evaluation.wants(Quantity::Volume)
                                ? evaluation.context.sectionArea(card.propId)
                                : NOT_DEFINED;
        evaluateBar(evaluation, input, area, values);
    }
};

// CBEAM3: GA, GB and the middle node GC; the orientation is GO or the vector X in the displacement system of GA, and
// the offsets are in the displacement systems of GA and GB (the OFFT flag does not exist). The length is the one of the
// chord.
struct Beam3Shape
{
    static void nodes(const model::Beam3Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::Beam3Card card = table.card(row);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
        ids.push_back(card.orientationNode);
        ids.push_back(card.nodeC);
    }
    static void evaluate(const model::Beam3Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::Beam3Card card = table.card(evaluation.row);
        BarInput input;
        input.hasOrientationNode = card.orientationNode != 0;
        input.orientation = Vec3{card.orientation[0], card.orientation[1], card.orientation[2]};
        input.hasOrientationVector = card.hasVector();
        input.offsetA = Vec3{card.offsetA[0], card.offsetA[1], card.offsetA[2]};
        input.offsetB = Vec3{card.offsetB[0], card.offsetB[1], card.offsetB[2]};
        const double area = evaluation.wants(Quantity::Area) || evaluation.wants(Quantity::Volume)
                                ? evaluation.context.sectionArea(card.propId)
                                : NOT_DEFINED;
        evaluateBar(evaluation, input, area, values);
        // With the middle node GC the axis is the quadratic curve through the three offset points, taken at t = 0, 1/2,
        // 1: the centre is its middle point and the length is its arc length.
        const base::Vec3& middle = evaluation.points[3];
        if (card.nodeC == 0 || !isFinite(middle))
            return;
        const BarGeometry bar = computeBar(evaluation, input);
        if (!isFinite(bar.endA) || !isFinite(bar.endB))
            return;
        const Vec3 offsetC{card.offsetC[0], card.offsetC[1], card.offsetC[2]};
        const base::Mat3 rotationC =
            evaluation.nodeRows[3] >= 0
                ? evaluation.context.displacementRotation(static_cast<std::uint32_t>(evaluation.nodeRows[3]))
                : base::Mat3::identity();
        const Vec3 pc = middle + rotationC * offsetC;
        if (evaluation.wants(Quantity::Center))
            values.center = pc;
        if (evaluation.wants(Quantity::Length) || evaluation.wants(Quantity::Volume))
        {
            const Vec3 second = 2.0 * (bar.endB + bar.endA - 2.0 * pc); // the coefficient of t^2
            const Vec3 first = bar.endB - bar.endA - second;            // the coefficient of t
            // The derivative 2 a t + b is integrated by 8-point Gauss-Legendre, exact to rounding for a smooth curve.
            static constexpr double NODES[4] = {0.18343464249564980, 0.52553240991632899, 0.79666647741362674,
                                                0.96028985649753623};
            static constexpr double WEIGHTS[4] = {0.36268378337836198, 0.31370664587788729, 0.22238103445337447,
                                                  0.10122853629037626};
            double arc = 0.0;
            for (std::size_t index = 0; index < 4; ++index)
            {
                for (const double sign : {-1.0, 1.0})
                {
                    const double t = 0.5 * (1.0 + sign * NODES[index]);
                    arc += 0.5 * WEIGHTS[index] * base::norm(2.0 * t * second + first);
                }
            }
            if (evaluation.wants(Quantity::Length))
                values.length = arc;
            if (evaluation.wants(Quantity::Volume))
                values.volume = area * arc;
        }
    }
};

// CBEND: a curved bar; only the middle of the chord is given (its length depends on the geometry option and the
// property).
struct CurvedBeam2Shape
{
    static void nodes(const model::CurvedBeam2Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::CurvedBeam2Card card = table.card(row);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
        ids.push_back(card.orientationNode);
    }
    static void evaluate(const model::CurvedBeam2Table&, const Evaluation& evaluation, ElementValues& values)
    {
        if (evaluation.wants(Quantity::Center) && isFinite(evaluation.points[0]) && isFinite(evaluation.points[1]))
            values.center = 0.5 * (evaluation.points[0] + evaluation.points[1]);
    }
};

// --- bushes, gaps
// ----------------------------------------------------------------------------------------------------------

// The element coordinate system of a bush or a gap: the system CID itself when there is one (at GA, which locates a
// cylindrical or spherical one), else the line GA-GB as x axis with the orientation vector (GO, or X in the
// displacement system of GA) in the x-y plane. Nothing when neither is given.
base::Mat3 bushFrame(const Evaluation& evaluation, base::EntityId cid, bool hasOrientationNode, const Vec3& orientation,
                     bool hasOrientationVector)
{
    const Vec3& pa = evaluation.points[0];
    const Vec3& pb = evaluation.points[1];
    if (!isFinite(pa))
        return nanFrame();
    if (cid >= 0)
        return evaluation.context.systemAxesAt(cid, pa);
    if (!isFinite(pb))
        return nanFrame();
    Vec3 v = undefinedVector();
    if (hasOrientationNode)
    {
        if (isFinite(evaluation.points[2]))
            v = evaluation.points[2] - pa;
    }
    else if (hasOrientationVector)
    {
        const base::Mat3 rotation =
            evaluation.nodeRows[0] >= 0
                ? evaluation.context.displacementRotation(static_cast<std::uint32_t>(evaluation.nodeRows[0]))
                : base::Mat3::identity();
        v = rotation * orientation;
    }
    if (!isFinite(v))
        return nanFrame();
    return frameFromAxisAndPlaneVector(pb - pa, v);
}

struct BushingShape
{
    static void nodes(const model::BushingTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::BushingCard card = table.card(row);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
        ids.push_back(card.orientationNode);
    }
    static void evaluate(const model::BushingTable& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::BushingCard card = table.card(evaluation.row);
        const Vec3& pa = evaluation.points[0];
        const Vec3& pb = evaluation.points[1];
        if (evaluation.wants(Quantity::Center) && isFinite(pa))
        {
            // The place of the spring-damper: the point (S1, S2, S3) of the system OCID, or S of the way from GA to GB.
            if (card.offsetFrameId != model::BushingCard::NO_OFFSET_FRAME_ID)
                values.center = evaluation.context.pointInSystem(card.offsetFrameId,
                                                                 Vec3{card.offset[0], card.offset[1], card.offset[2]});
            else if (isFinite(pb))
                values.center = pa + card.offsetFraction * (pb - pa);
            else
                values.center = pa;
        }
        if (evaluation.wants(Quantity::Length) && isFinite(pa) && isFinite(pb))
            values.length = base::norm(pb - pa);
        if (evaluation.wants(Quantity::Frame))
            values.frame =
                bushFrame(evaluation, card.frameId, card.orientationNode != 0,
                          Vec3{card.orientation[0], card.orientation[1], card.orientation[2]}, card.hasVector());
    }
};

struct Gap2Shape
{
    static void nodes(const model::Gap2Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::Gap2Card card = table.card(row);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
        ids.push_back(card.orientationNode);
    }
    static void evaluate(const model::Gap2Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::Gap2Card card = table.card(evaluation.row);
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
        if (evaluation.wants(Quantity::Frame))
            values.frame =
                bushFrame(evaluation, card.frameId, card.orientationNode != 0,
                          Vec3{card.orientation[0], card.orientation[1], card.orientation[2]}, card.hasVector());
    }
};

// CBUSH1D: a one-dimensional bush along GA-GB (or along the x axis of CID); only its x axis is defined, so the frame is
// given when CID names a system.
struct Bushing1DShape
{
    static void nodes(const model::Bushing1DTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::Bushing1DCard card = table.card(row);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
    }
    static void evaluate(const model::Bushing1DTable& table, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
        if (evaluation.wants(Quantity::Frame))
        {
            const base::EntityId cid = table.card(evaluation.row).frameId;
            if (cid >= 0 && isFinite(evaluation.points[0]))
                values.frame = evaluation.context.systemAxesAt(cid, evaluation.points[0]);
        }
    }
};

// --- springs, dampers and the scalar-and-grid mass elements
// ---------------------------------------------------------------

struct SpringDofPropertyShape
{
    static void nodes(const model::SpringDofPropertyTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::SpringDofPropertyCard card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
    }
    static void evaluate(const model::SpringDofPropertyTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

struct SpringDofInlineShape
{
    static void nodes(const model::SpringDofInlineTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::SpringDofInlineCard card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
    }
    static void evaluate(const model::SpringDofInlineTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

struct DamperDofPropertyShape
{
    static void nodes(const model::DamperDofPropertyTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::DamperDofPropertyCard card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
    }
    static void evaluate(const model::DamperDofPropertyTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

struct DamperDofInlineShape
{
    static void nodes(const model::DamperDofInlineTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::DamperDofInlineCard card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
    }
    static void evaluate(const model::DamperDofInlineTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

struct ViscousDamper2Shape
{
    static void nodes(const model::ViscousDamper2Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::ViscousDamper2Card card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
    }
    static void evaluate(const model::ViscousDamper2Table&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

struct MassDofPropertyShape
{
    static void nodes(const model::MassDofPropertyTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::MassDofPropertyCard card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
    }
    static void evaluate(const model::MassDofPropertyTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

struct MassDofInlineShape
{
    static void nodes(const model::MassDofInlineTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::MassDofInlineCard card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
    }
    static void evaluate(const model::MassDofInlineTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

struct PlotLine2Shape
{
    static void nodes(const model::PlotLine2Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::PlotLine2Card card = table.card(row);
        ids.push_back(card.node1);
        ids.push_back(card.node2);
    }
    static void evaluate(const model::PlotLine2Table&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

// The two-noded marker element of the Euler domain: a segment between its grid points, with no property geometry (its
// PID names a PMARKER, which has no section).
struct MarkerBodyShape
{
    static void nodes(const model::MarkerBodyTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        for (const base::EntityId node : table.nodes(row))
            ids.push_back(node);
    }
    static void evaluate(const model::MarkerBodyTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateSegment(evaluation, evaluation.points[0], evaluation.points[1], values);
    }
};

// --- fasteners and welds
// -------------------------------------------------------------------------------------------------

// A fastener or a weld sits at the point GS when it has one, else between its two grid points GA and GB.
void evaluateConnector(const Evaluation& evaluation, ElementValues& values)
{
    const Vec3& at = evaluation.points[0];
    if (isFinite(at))
    {
        if (evaluation.wants(Quantity::Center))
            values.center = at;
        return;
    }
    evaluateSegment(evaluation, evaluation.points[1], evaluation.points[2], values);
}

struct FastenerShape
{
    static void nodes(const model::FastenerTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::FastenerCard card = table.card(row);
        ids.push_back(card.spotNode);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
    }
    static void evaluate(const model::FastenerTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateConnector(evaluation, values);
    }
};

struct WeldShape
{
    static void nodes(const model::WeldTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::WeldCard card = table.card(row);
        ids.push_back(card.spotNode);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
    }
    static void evaluate(const model::WeldTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateConnector(evaluation, values);
    }
};

} // namespace

void registerLineProviders(ProviderRegistry& registry)
{
    registry.add<model::Rod2Table, Rod2Shape>();
    registry.add<model::Rod2InlineSectionTable, Rod2InlineSectionShape>();
    registry.add<model::Tube2Table, Tube2Shape>();
    registry.add<model::Bar2Table, Bar2Shape>();
    registry.add<model::Beam2Table, Beam2Shape>();
    registry.add<model::Beam3Table, Beam3Shape>();
    registry.add<model::CurvedBeam2Table, CurvedBeam2Shape>();
    registry.add<model::BushingTable, BushingShape>();
    registry.add<model::Gap2Table, Gap2Shape>();
    registry.add<model::Bushing1DTable, Bushing1DShape>();
    registry.add<model::SpringDofPropertyTable, SpringDofPropertyShape>();
    registry.add<model::SpringDofInlineTable, SpringDofInlineShape>();
    registry.add<model::DamperDofPropertyTable, DamperDofPropertyShape>();
    registry.add<model::DamperDofInlineTable, DamperDofInlineShape>();
    registry.add<model::ViscousDamper2Table, ViscousDamper2Shape>();
    registry.add<model::MassDofPropertyTable, MassDofPropertyShape>();
    registry.add<model::MassDofInlineTable, MassDofInlineShape>();
    registry.add<model::PlotLine2Table, PlotLine2Shape>();
    registry.add<model::MarkerBodyTable, MarkerBodyShape>();
    registry.add<model::FastenerTable, FastenerShape>();
    registry.add<model::WeldTable, WeldShape>();
}

} // namespace oor::analysis::detail

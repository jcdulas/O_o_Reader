#include "ShellGeometry.hpp"
#include "oor/model/cards/elements/axisymTria/AxisymTriaTable.hpp"
#include "oor/model/cards/elements/axisymTria6/AxisymTria6Table.hpp"
#include "oor/model/cards/elements/planeStrainTria3/PlaneStrainTria3Table.hpp"
#include "oor/model/cards/elements/planeStrainTria6/PlaneStrainTria6Table.hpp"
#include "oor/model/cards/elements/planeStressTria3/PlaneStressTria3Table.hpp"
#include "oor/model/cards/elements/planeStressTria6/PlaneStressTria6Table.hpp"
#include "oor/model/cards/elements/shellTria3/ShellTria3Table.hpp"
#include "oor/model/cards/elements/shellTria6/ShellTria6Table.hpp"
#include "oor/model/cards/elements/shellTria3Drilling/ShellTria3DrillingTable.hpp"

// The triangular elements: the three-node and six-node shells, the plane stress and plane strain triangles and the
// axisymmetric ones. The geometry is the one of the three corner nodes (format reference: the edges are straight unless
// a mid-side node says otherwise, and the area, the normal and the element axes are those of the corners); the mid-side
// nodes are read so that a missing node is reported, and used by nothing else.

namespace oor::analysis::detail
{

namespace
{

template <typename NodeArray>
void appendNodes(const NodeArray& nodes, std::vector<base::EntityId>& ids)
{
    ids.insert(ids.end(), nodes.begin(), nodes.end());
}

// CTRIA3: three nodes, corner thicknesses T1..T3, THETA or MCID.
struct ShellTria3Shape
{
    static void nodes(const model::ShellTria3Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::ShellTria3Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::ShellTria3Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(3);
        input.pid = card.propId;
        input.cornerThickness = card.cornerThickness.data();
        input.thicknessFlag = card.thicknessFlag;
        input.thetaInDegrees = card.thetaInDegrees;
        input.mcid = card.matFrameId;
        evaluateTriangle(evaluation, input, values);
    }
};

// CTRIAR: the same geometry as CTRIA3 (the drilling-rotation variant).
struct ShellTria3DrillingShape
{
    static void nodes(const model::ShellTria3DrillingTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::ShellTria3DrillingTable& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::ShellTria3DrillingCard card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(3);
        input.pid = card.propId;
        input.cornerThickness = card.cornerThickness.data();
        input.thicknessFlag = card.thicknessFlag;
        input.thetaInDegrees = card.thetaInDegrees;
        input.mcid = card.matFrameId;
        evaluateTriangle(evaluation, input, values);
    }
};

// CTRIA6: three corners G1..G3 and three mid-side nodes G4..G6.
struct ShellTria6Shape
{
    static void nodes(const model::ShellTria6Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::ShellTria6Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::ShellTria6Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(3);
        input.pid = card.propId;
        input.cornerThickness = card.cornerThickness.data();
        input.thicknessFlag = card.thicknessFlag;
        input.thetaInDegrees = card.thetaInDegrees;
        input.mcid = card.matFrameId;
        evaluateTriangle(evaluation, input, values);
    }
};

// CPLSTN3 and CPLSTN6: plane strain triangles (another implementation): no thickness, THETA only.
struct PlaneStrainTria3Shape
{
    static void nodes(const model::PlaneStrainTria3Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::PlaneStrainTria3Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::PlaneStrainTria3Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(3);
        input.pid = card.propId;
        input.fixedThickness = 1.0; // plane strain: one unit deep
        input.thetaInDegrees = card.thetaInDegrees;
        evaluateTriangle(evaluation, input, values);
    }
};

struct PlaneStrainTria6Shape
{
    static void nodes(const model::PlaneStrainTria6Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::PlaneStrainTria6Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::PlaneStrainTria6Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(3);
        input.pid = card.propId;
        input.fixedThickness = 1.0; // plane strain: one unit deep
        input.thetaInDegrees = card.thetaInDegrees;
        evaluateTriangle(evaluation, input, values);
    }
};

// CPLSTS3 (plane stress, with corner thicknesses) and CPLSTS6.
struct PlaneStressTria3Shape
{
    static void nodes(const model::PlaneStressTria3Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::PlaneStressTria3Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::PlaneStressTria3Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(3);
        input.pid = card.propId;
        input.cornerThickness = card.cornerThickness.data();
        input.thicknessFlag = card.thicknessFlag;
        input.thetaInDegrees = card.thetaInDegrees;
        evaluateTriangle(evaluation, input, values);
    }
};

struct PlaneStressTria6Shape
{
    static void nodes(const model::PlaneStressTria6Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::PlaneStressTria6Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::PlaneStressTria6Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(3);
        input.pid = card.propId;
        input.thetaInDegrees = card.thetaInDegrees;
        evaluateTriangle(evaluation, input, values);
    }
};

// CTRIAX6: the corners are G1, G3 and G5 and the edge points G2, G4 and G6 (they alternate around the perimeter). An
// axisymmetric element: the volume is the ring the cross-section sweeps.
struct AxisymTria6Shape
{
    static void nodes(const model::AxisymTria6Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::AxisymTria6Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::AxisymTria6Card card = table.card(evaluation.row);
        const base::Vec3 corners[3] = {evaluation.points[0], evaluation.points[2], evaluation.points[4]};
        ShellInput input;
        input.corners = corners;
        input.hasThickness = false;
        input.thetaInDegrees = card.thetaInDegrees;
        input.axisymmetric = true;
        evaluateTriangle(evaluation, input, values);
    }
};

// CTRIAX: corners G1..G3 and edge points G4..G6; axisymmetric.
struct AxisymTriaShape
{
    static void nodes(const model::AxisymTriaTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::AxisymTriaTable& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::AxisymTriaCard card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(3);
        input.pid = card.propId;
        input.hasThickness = false;
        input.thetaInDegrees = card.thetaInDegrees;
        input.mcid = card.matFrameId;
        input.axisymmetric = true;
        evaluateTriangle(evaluation, input, values);
    }
};

} // namespace

void registerTriangleProviders(ProviderRegistry& registry)
{
    registry.add<model::ShellTria3Table, ShellTria3Shape>();
    registry.add<model::ShellTria3DrillingTable, ShellTria3DrillingShape>();
    registry.add<model::ShellTria6Table, ShellTria6Shape>();
    registry.add<model::PlaneStrainTria3Table, PlaneStrainTria3Shape>();
    registry.add<model::PlaneStrainTria6Table, PlaneStrainTria6Shape>();
    registry.add<model::PlaneStressTria3Table, PlaneStressTria3Shape>();
    registry.add<model::PlaneStressTria6Table, PlaneStressTria6Shape>();
    registry.add<model::AxisymTria6Table, AxisymTria6Shape>();
    registry.add<model::AxisymTriaTable, AxisymTriaShape>();
}

} // namespace oor::analysis::detail

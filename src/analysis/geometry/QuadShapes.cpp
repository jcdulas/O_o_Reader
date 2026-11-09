#include "ShellGeometry.hpp"
#include "oor/model/cards/elements/axisymQuad/AxisymQuadTable.hpp"
#include "oor/model/cards/elements/axisymQuad4/AxisymQuad4Table.hpp"
#include "oor/model/cards/elements/axisymQuad8/AxisymQuad8Table.hpp"
#include "oor/model/cards/elements/planeStrainQuad4/PlaneStrainQuad4Table.hpp"
#include "oor/model/cards/elements/planeStrainQuad8/PlaneStrainQuad8Table.hpp"
#include "oor/model/cards/elements/planeStressQuad4/PlaneStressQuad4Table.hpp"
#include "oor/model/cards/elements/planeStressQuad8/PlaneStressQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4/ShellQuad4Table.hpp"
#include "oor/model/cards/elements/shellQuad8/ShellQuad8Table.hpp"
#include "oor/model/cards/elements/shellQuad4Drilling/ShellQuad4DrillingTable.hpp"
#include "oor/model/cards/elements/planeStrainQuad/PlaneStrainQuadTable.hpp"
#include "oor/model/cards/elements/shearPanel4/ShearPanel4Table.hpp"

// The quadrilateral elements: the four-node and eight-node shells, the shear panel, the plane stress, plane strain and
// axisymmetric quadrilaterals, and the nine-node nonlinear plane strain element. As for the triangles, the geometry is
// the one of the four corner nodes: the area is half the cross product of the diagonals (a warped quadrilateral gives
// the area of its projection on the mean plane), the normal is the direction of that cross product, and the element x
// axis bisects the angle between the diagonals (format reference, CQUAD4 element coordinate system). The mid-side and
// centre nodes are read only so that a missing one is reported.

namespace oor::analysis::detail
{

namespace
{

template <typename NodeArray>
void appendNodes(const NodeArray& nodes, std::vector<base::EntityId>& ids)
{
    ids.insert(ids.end(), nodes.begin(), nodes.end());
}

struct ShellQuad4Shape
{
    static void nodes(const model::ShellQuad4Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::ShellQuad4Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::ShellQuad4Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.pid = card.propId;
        input.cornerThickness = card.cornerThickness.data();
        input.thicknessFlag = card.thicknessFlag;
        input.thetaInDegrees = card.thetaInDegrees;
        input.mcid = card.matFrameId;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

struct ShellQuad4DrillingShape
{
    static void nodes(const model::ShellQuad4DrillingTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::ShellQuad4DrillingTable& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::ShellQuad4DrillingCard card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.pid = card.propId;
        input.cornerThickness = card.cornerThickness.data();
        input.thicknessFlag = card.thicknessFlag;
        input.thetaInDegrees = card.thetaInDegrees;
        input.mcid = card.matFrameId;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

struct ShellQuad8Shape
{
    static void nodes(const model::ShellQuad8Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::ShellQuad8Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::ShellQuad8Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.pid = card.propId;
        input.cornerThickness = card.cornerThickness.data();
        input.thicknessFlag = card.thicknessFlag;
        input.thetaInDegrees = card.thetaInDegrees;
        input.mcid = card.matFrameId;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

// CQUAD: the nonlinear plane strain element with nine nodes (a PLPLANE property: no thickness).
struct PlaneStrainQuadShape
{
    static void nodes(const model::PlaneStrainQuadTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::PlaneStrainQuadTable& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::PlaneStrainQuadCard card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.pid = card.propId;
        input.hasThickness = false;
        input.thetaInDegrees = card.thetaInDegrees;
        input.mcid = card.matFrameId;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

// CSHEAR: the element coordinate system of a shear panel has its x axis from G1 to G2 (format reference Figure 9-65),
// the z axis the normal of the diagonals; it has no material direction. Its thickness is the one of the PSHEAR.
struct ShearPanel4Shape
{
    static void nodes(const model::ShearPanel4Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::ShearPanel4Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::ShearPanel4Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.pid = card.propId;
        input.shearPanel = true;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

// The axisymmetric quadrilaterals: rings, with the volume the cross-section sweeps.
struct AxisymQuadShape
{
    static void nodes(const model::AxisymQuadTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::AxisymQuadTable& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::AxisymQuadCard card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.pid = card.propId;
        input.hasThickness = false;
        input.thetaInDegrees = card.thetaInDegrees;
        input.mcid = card.matFrameId;
        input.axisymmetric = true;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

struct AxisymQuad4Shape
{
    static void nodes(const model::AxisymQuad4Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::AxisymQuad4Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::AxisymQuad4Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.pid = card.propId;
        input.hasThickness = false;
        input.thetaInDegrees = card.thetaInDegrees;
        input.axisymmetric = true;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

struct AxisymQuad8Shape
{
    static void nodes(const model::AxisymQuad8Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::AxisymQuad8Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::AxisymQuad8Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.pid = card.propId;
        input.hasThickness = false;
        input.thetaInDegrees = card.thetaInDegrees;
        input.axisymmetric = true;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

// The plane strain (CPLSTN) and plane stress (CPLSTS) quadrilaterals.
struct PlaneStrainQuad4Shape
{
    static void nodes(const model::PlaneStrainQuad4Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::PlaneStrainQuad4Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::PlaneStrainQuad4Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.pid = card.propId;
        input.fixedThickness = 1.0; // plane strain: one unit deep
        input.thetaInDegrees = card.thetaInDegrees;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

struct PlaneStrainQuad8Shape
{
    static void nodes(const model::PlaneStrainQuad8Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::PlaneStrainQuad8Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::PlaneStrainQuad8Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.pid = card.propId;
        input.fixedThickness = 1.0; // plane strain: one unit deep
        input.thetaInDegrees = card.thetaInDegrees;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

struct PlaneStressQuad4Shape
{
    static void nodes(const model::PlaneStressQuad4Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::PlaneStressQuad4Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::PlaneStressQuad4Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.pid = card.propId;
        input.cornerThickness = card.cornerThickness.data();
        input.thicknessFlag = card.thicknessFlag;
        input.thetaInDegrees = card.thetaInDegrees;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

struct PlaneStressQuad8Shape
{
    static void nodes(const model::PlaneStressQuad8Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        appendNodes(table.card(row).nodes, ids);
    }
    static void evaluate(const model::PlaneStressQuad8Table& table, const Evaluation& evaluation, ElementValues& values)
    {
        const model::PlaneStressQuad8Card card = table.card(evaluation.row);
        ShellInput input;
        input.corners = evaluation.points.first(4);
        input.pid = card.propId;
        input.thetaInDegrees = card.thetaInDegrees;
        evaluateQuadrilateral(evaluation, input, values);
    }
};

} // namespace

void registerQuadProviders(ProviderRegistry& registry)
{
    registry.add<model::ShellQuad4Table, ShellQuad4Shape>();
    registry.add<model::ShellQuad4DrillingTable, ShellQuad4DrillingShape>();
    registry.add<model::ShellQuad8Table, ShellQuad8Shape>();
    registry.add<model::PlaneStrainQuadTable, PlaneStrainQuadShape>();
    registry.add<model::ShearPanel4Table, ShearPanel4Shape>();
    registry.add<model::AxisymQuadTable, AxisymQuadShape>();
    registry.add<model::AxisymQuad4Table, AxisymQuad4Shape>();
    registry.add<model::AxisymQuad8Table, AxisymQuad8Shape>();
    registry.add<model::PlaneStrainQuad4Table, PlaneStrainQuad4Shape>();
    registry.add<model::PlaneStrainQuad8Table, PlaneStrainQuad8Shape>();
    registry.add<model::PlaneStressQuad4Table, PlaneStressQuad4Shape>();
    registry.add<model::PlaneStressQuad8Table, PlaneStressQuad8Shape>();
}

} // namespace oor::analysis::detail

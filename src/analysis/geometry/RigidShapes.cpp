#include "ElementProvider.hpp"
#include "oor/analysis/frames/FrameMath.hpp"
#include "oor/model/cards/rigid_elements/rigidBar/RigidBarTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBar1/RigidBar1Table.hpp"
#include "oor/model/cards/rigid_elements/rigidBodyMulti/RigidBodyMultiTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBodyStar/RigidBodyStarTable.hpp"
#include "oor/model/cards/rigid_elements/rigidInterpolation/RigidInterpolationTable.hpp"
#include "oor/model/cards/rigid_elements/rigidJoint/RigidJointTable.hpp"
#include "oor/model/cards/rigid_elements/rigidRod/RigidRodTable.hpp"
#include "oor/model/cards/rigid_elements/rigidSpline/RigidSplineTable.hpp"
#include "oor/model/cards/rigid_elements/rigidTriaPlate/RigidTriaPlateTable.hpp"
#include "oor/model/cards/rigid_elements/rigidTriaPlate1/RigidTriaPlate1Table.hpp"

// The rigid elements. Their grid points are the geometry they have: the centre is the average of all the grid points an
// element connects (independent and dependent, the reference grid point of an RBE3 and the grid points of its groups),
// and the elements with two grid points (RBAR, RBAR1, RROD, RJOINT) have the length of the segment between them.
// RSSCON, which names elements or grid points by its type, has none.

namespace oor::analysis::detail
{

namespace
{

// The average of the grid points the element has (a node that is blank, or not a grid point, is not counted).
void evaluateConnection(const Evaluation& evaluation, ElementValues& values)
{
    base::Vec3 sum;
    std::size_t count = 0;
    for (const base::Vec3& point : evaluation.points)
    {
        if (!isFinite(point))
            continue;
        sum += point;
        ++count;
    }
    if (count > 0 && evaluation.wants(Quantity::Center))
        values.center = sum / static_cast<double>(count);
}

void evaluateTwoPointConnection(const Evaluation& evaluation, ElementValues& values)
{
    evaluateConnection(evaluation, values);
    if (evaluation.wants(Quantity::Length) && isFinite(evaluation.points[0]) && isFinite(evaluation.points[1]))
        values.length = base::norm(evaluation.points[1] - evaluation.points[0]);
}

struct RigidBodyStarShape
{
    static void nodes(const model::RigidBodyStarTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::RigidBodyStarCard card = table.card(row);
        ids.push_back(card.independent);
        ids.insert(ids.end(), card.dependents.begin(), card.dependents.end());
    }
    static void evaluate(const model::RigidBodyStarTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateConnection(evaluation, values);
    }
};

struct RigidInterpolationShape
{
    static void nodes(const model::RigidInterpolationTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::RigidInterpolationCard card = table.card(row);
        ids.push_back(card.referenceNode);
        for (const model::RigidInterpolationGroup& group : card.groups)
            ids.insert(ids.end(), group.nodes.begin(), group.nodes.end());
        for (const model::RigidInterpolationDependent& dependent : card.dependents)
            ids.push_back(dependent.node);
    }
    static void evaluate(const model::RigidInterpolationTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateConnection(evaluation, values);
    }
};

struct RigidBodyMultiShape
{
    static void nodes(const model::RigidBodyMultiTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::RigidBodyMultiCard card = table.card(row);
        for (const model::DofPair& pair : card.independent)
            ids.push_back(pair.node);
        for (const model::DofPair& pair : card.dependent)
            ids.push_back(pair.node);
    }
    static void evaluate(const model::RigidBodyMultiTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateConnection(evaluation, values);
    }
};

struct RigidBarShape
{
    static void nodes(const model::RigidBarTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::RigidBarCard card = table.card(row);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
    }
    static void evaluate(const model::RigidBarTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateTwoPointConnection(evaluation, values);
    }
};

struct RigidBar1Shape
{
    static void nodes(const model::RigidBar1Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::RigidBar1Card card = table.card(row);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
    }
    static void evaluate(const model::RigidBar1Table&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateTwoPointConnection(evaluation, values);
    }
};

struct RigidRodShape
{
    static void nodes(const model::RigidRodTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::RigidRodCard card = table.card(row);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
    }
    static void evaluate(const model::RigidRodTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateTwoPointConnection(evaluation, values);
    }
};

struct RigidJointShape
{
    static void nodes(const model::RigidJointTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::RigidJointCard card = table.card(row);
        ids.push_back(card.nodeA);
        ids.push_back(card.nodeB);
    }
    static void evaluate(const model::RigidJointTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateTwoPointConnection(evaluation, values);
    }
};

struct RigidSplineShape
{
    static void nodes(const model::RigidSplineTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        for (const model::DofPair& pair : table.card(row).points)
            ids.push_back(pair.node);
    }
    static void evaluate(const model::RigidSplineTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluateConnection(evaluation, values);
    }
};

// RTRPLT and RTRPLT1: a rigid triangular plate on three grid points; its centre is the average of the three, and as a
// triangle it has an area and a normal.
void evaluatePlate(const Evaluation& evaluation, ElementValues& values)
{
    evaluateConnection(evaluation, values);
    const base::Vec3& first = evaluation.points[0];
    const base::Vec3& second = evaluation.points[1];
    const base::Vec3& third = evaluation.points[2];
    if (!isFinite(first) || !isFinite(second) || !isFinite(third))
        return;
    const base::Vec3 areaVector = cross(second - first, third - first);
    if (evaluation.wants(Quantity::Area))
        values.area = 0.5 * base::norm(areaVector);
    if (evaluation.wants(Quantity::Normal))
        values.normal = unitOrNan(areaVector);
}

struct RigidTriaPlateShape
{
    static void nodes(const model::RigidTriaPlateTable& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::RigidTriaPlateCard card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }
    static void evaluate(const model::RigidTriaPlateTable&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluatePlate(evaluation, values);
    }
};

struct RigidTriaPlate1Shape
{
    static void nodes(const model::RigidTriaPlate1Table& table, std::uint32_t row, std::vector<base::EntityId>& ids)
    {
        const model::RigidTriaPlate1Card card = table.card(row);
        ids.insert(ids.end(), card.nodes.begin(), card.nodes.end());
    }
    static void evaluate(const model::RigidTriaPlate1Table&, const Evaluation& evaluation, ElementValues& values)
    {
        evaluatePlate(evaluation, values);
    }
};

} // namespace

void registerRigidProviders(ProviderRegistry& registry)
{
    registry.add<model::RigidBodyStarTable, RigidBodyStarShape>();
    registry.add<model::RigidInterpolationTable, RigidInterpolationShape>();
    registry.add<model::RigidBodyMultiTable, RigidBodyMultiShape>();
    registry.add<model::RigidBarTable, RigidBarShape>();
    registry.add<model::RigidBar1Table, RigidBar1Shape>();
    registry.add<model::RigidRodTable, RigidRodShape>();
    registry.add<model::RigidJointTable, RigidJointShape>();
    registry.add<model::RigidSplineTable, RigidSplineShape>();
    registry.add<model::RigidTriaPlateTable, RigidTriaPlateShape>();
    registry.add<model::RigidTriaPlate1Table, RigidTriaPlate1Shape>();
}

} // namespace oor::analysis::detail

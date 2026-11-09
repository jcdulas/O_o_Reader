#include "oor/analysis/connections/RigidElementTerms.hpp"

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

namespace oor::analysis
{

namespace
{

template <typename TableT>
const TableT& tableOf(const model::Model& cards)
{
    return static_cast<const TableT&>(*cards.findTable(model::typeIndexOf<TableT>()));
}

} // namespace

std::vector<RigidTerm> RigidElementTerms::of(const model::Model& cards, model::CardLocation location,
                                             RigidThermal* thermal)
{
    using namespace oor::model;
    std::vector<RigidTerm> rows;
    RigidThermal expansion;
    const auto expands = [&](const auto& card)
    { expansion = RigidThermal{card.alphaCoefficient, card.referenceTemperature}; };
    const std::uint16_t type = location.table;
    const auto add = [&](bool independent, base::EntityId node, std::int32_t components,
                         double weight = std::numeric_limits<double>::quiet_NaN())
    {
        if (node != 0)
            rows.push_back(RigidTerm{independent, node, components, weight});
    };
    if (cards.findTable(type) == nullptr)
        return rows;
    if (type == typeIndexOf<RigidBodyStarTable>())
    {
        const RigidBodyStarCard card = tableOf<RigidBodyStarTable>(cards).card(location.row);
        expands(card);
        add(true, card.independent, ALL_COMPONENTS);
        for (const base::EntityId node : card.dependents)
            add(false, node, card.components);
    }
    else if (type == typeIndexOf<RigidInterpolationTable>())
    {
        const RigidInterpolationCard card = tableOf<RigidInterpolationTable>(cards).card(location.row);
        expands(card);
        for (const RigidInterpolationGroup& group : card.groups)
        {
            for (const base::EntityId node : group.nodes)
                add(true, node, group.components, group.weight);
        }
        add(false, card.referenceNode, card.refComponents);
        for (const RigidInterpolationDependent& dependent : card.dependents)
            add(false, dependent.node, dependent.components);
    }
    else if (type == typeIndexOf<RigidBarTable>())
    {
        const RigidBarCard card = tableOf<RigidBarTable>(cards).card(location.row);
        expands(card);
        if (card.independentComponentsA != 0)
            add(true, card.nodeA, card.independentComponentsA);
        if (card.independentComponentsB != 0)
            add(true, card.nodeB, card.independentComponentsB);
        if (card.componentsAtA != 0)
            add(false, card.nodeA, card.componentsAtA);
        if (card.componentsAtB != 0)
            add(false, card.nodeB, card.componentsAtB);
    }
    else if (type == typeIndexOf<RigidBar1Table>())
    {
        const RigidBar1Card card = tableOf<RigidBar1Table>(cards).card(location.row);
        expands(card);
        add(true, card.nodeA, ALL_COMPONENTS);
        add(false, card.nodeB, card.componentsB);
    }
    else if (type == typeIndexOf<RigidBodyMultiTable>())
    {
        const RigidBodyMultiCard card = tableOf<RigidBodyMultiTable>(cards).card(location.row);
        expands(card);
        for (const DofPair& pair : card.independent)
            add(true, pair.node, pair.components);
        for (const DofPair& pair : card.dependent)
            add(false, pair.node, pair.components);
    }
    else if (type == typeIndexOf<RigidRodTable>())
    {
        const RigidRodCard card = tableOf<RigidRodTable>(cards).card(location.row);
        expands(card);
        // One end carries the dependent translation; the other is independent.
        const bool aDependent = card.componentsAtA != 0;
        add(!aDependent, card.nodeA, aDependent ? card.componentsAtA : 0);
        add(aDependent, card.nodeB, aDependent ? 0 : card.componentsAtB);
    }
    else if (type == typeIndexOf<RigidSplineTable>())
    {
        const RigidSplineCard card = tableOf<RigidSplineTable>(cards).card(location.row);
        for (std::size_t index = 0; index < card.points.size(); ++index)
        {
            const bool end = index == 0 || index + 1 == card.points.size();
            add(end, card.points[index].node, end ? ALL_COMPONENTS : card.points[index].components);
        }
    }
    else if (type == typeIndexOf<RigidJointTable>())
    {
        const RigidJointCard card = tableOf<RigidJointTable>(cards).card(location.row);
        add(true, card.nodeA, ALL_COMPONENTS);
        add(false, card.nodeB, card.componentsB);
    }
    else if (type == typeIndexOf<RigidTriaPlateTable>())
    {
        const RigidTriaPlateCard card = tableOf<RigidTriaPlateTable>(cards).card(location.row);
        expands(card);
        for (std::size_t index = 0; index < 3; ++index)
        {
            if (card.independent[index] != 0)
                add(true, card.nodes[index], card.independent[index]);
        }
        for (std::size_t index = 0; index < 3; ++index)
        {
            if (card.dependent[index] != 0)
                add(false, card.nodes[index], card.dependent[index]);
        }
    }
    else if (type == typeIndexOf<RigidTriaPlate1Table>())
    {
        const RigidTriaPlate1Card card = tableOf<RigidTriaPlate1Table>(cards).card(location.row);
        expands(card);
        add(true, card.nodes[0], ALL_COMPONENTS);
        add(false, card.nodes[1], card.components[0]);
        add(false, card.nodes[2], card.components[1]);
    }
    if (thermal != nullptr)
        *thermal = expansion;
    return rows;
}

} // namespace oor::analysis

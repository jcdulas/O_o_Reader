#include "ConstraintDofs.hpp"

#include "oor/model/cards/constraints/multiPointConstr/MultiPointConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstr/SinglePointConstrTable.hpp"
#include "oor/model/cards/constraints/singlePointConstrList/SinglePointConstrListTable.hpp"
#include "oor/model/cards/nodes/node/NodeTable.hpp"
#include "oor/model/cards/rigid_elements/ComponentSets.hpp"
#include "oor/model/cards/rigid_elements/rigidBar/RigidBarTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBodyMulti/RigidBodyMultiTable.hpp"
#include "oor/model/cards/rigid_elements/rigidBodyStar/RigidBodyStarTable.hpp"
#include "oor/model/cards/rigid_elements/rigidInterpolation/RigidInterpolationTable.hpp"
#include "oor/model/cards/rigid_elements/rigidRod/RigidRodTable.hpp"

#include <algorithm>

namespace oor::analysis
{

void appendDofs(base::EntityId node, std::int32_t components, model::CardLocation where, std::vector<DofUse>& out)
{
    if (components <= 0)
    {
        out.push_back(DofUse{dofKey(node, 0), where});
        return;
    }
    const unsigned mask = model::components::maskOf(components);
    for (unsigned digit = 1; digit <= 6; ++digit)
    {
        if ((mask & (1U << digit)) != 0)
            out.push_back(DofUse{dofKey(node, digit), where});
    }
}

std::vector<DofUse> rigidDependentDofs(const model::Model& model)
{
    std::vector<DofUse> dofs;
    forEachCard<model::RigidBodyStarTable>(model,
                                           [&](const model::RigidBodyStarCard& card, model::CardLocation where)
                                           {
                                               for (const base::EntityId dependent : card.dependents)
                                                   appendDofs(dependent, card.components, where, dofs);
                                           });
    forEachCard<model::RigidInterpolationTable>(
        model,
        [&](const model::RigidInterpolationCard& card, model::CardLocation where)
        {
            if (card.dependents.empty())
            {
                appendDofs(card.referenceNode, card.refComponents, where, dofs);
                return;
            }
            for (const model::RigidInterpolationDependent& dependent : card.dependents)
                appendDofs(dependent.node, dependent.components, where, dofs);
        });
    forEachCard<model::RigidBarTable>(model,
                                      [&](const model::RigidBarCard& card, model::CardLocation where)
                                      {
                                          if (card.componentsAtA > 0)
                                              appendDofs(card.nodeA, card.componentsAtA, where, dofs);
                                          if (card.componentsAtB > 0)
                                              appendDofs(card.nodeB, card.componentsAtB, where, dofs);
                                      });
    forEachCard<model::RigidRodTable>(model,
                                      [&](const model::RigidRodCard& card, model::CardLocation where)
                                      {
                                          if (card.componentsAtA > 0)
                                              appendDofs(card.nodeA, card.componentsAtA, where, dofs);
                                          if (card.componentsAtB > 0)
                                              appendDofs(card.nodeB, card.componentsAtB, where, dofs);
                                      });
    forEachCard<model::RigidBodyMultiTable>(model,
                                            [&](const model::RigidBodyMultiCard& card, model::CardLocation where)
                                            {
                                                for (const model::DofPair& pair : card.dependent)
                                                    appendDofs(pair.node, pair.components, where, dofs);
                                            });
    return dofs;
}

namespace
{

// Adds a link from each of `dependent` to each of `independent`.
void link(const std::vector<DofUse>& dependent, const std::vector<DofUse>& independent, model::CardLocation where,
          std::vector<DofLink>& links)
{
    for (const DofUse& from : dependent)
    {
        for (const DofUse& to : independent)
            links.push_back(DofLink{from.key, to.key, where});
    }
}

std::vector<DofUse> dofsOf(base::EntityId node, std::int32_t components, model::CardLocation where)
{
    std::vector<DofUse> dofs;
    appendDofs(node, components, where, dofs);
    return dofs;
}

} // namespace

std::vector<DofLink> rigidDofLinks(const model::Model& model)
{
    std::vector<DofLink> links;
    forEachCard<model::RigidBodyStarTable>(
        model,
        [&](const model::RigidBodyStarCard& card, model::CardLocation where)
        {
            const std::vector<DofUse> independent = dofsOf(card.independent, model::components::ALL, where);
            for (const base::EntityId dependent : card.dependents)
                link(dofsOf(dependent, card.components, where), independent, where, links);
        });
    forEachCard<model::RigidInterpolationTable>(
        model,
        [&](const model::RigidInterpolationCard& card, model::CardLocation where)
        {
            std::vector<DofUse> groupDofs;
            for (const model::RigidInterpolationGroup& group : card.groups)
            {
                for (const base::EntityId node : group.nodes)
                    appendDofs(node, group.components, where, groupDofs);
            }
            std::vector<DofUse> dependent;
            std::vector<DofUse> independent;
            if (card.dependents.empty())
            {
                // The reference degrees of freedom depend on the ones of the weighted groups.
                appendDofs(card.referenceNode, card.refComponents, where, dependent);
                independent = groupDofs;
            }
            else
            {
                // With UM the dependent degrees of freedom are chosen among the ones of the groups: they depend on the
                // others and on the reference grid point.
                for (const model::RigidInterpolationDependent& each : card.dependents)
                    appendDofs(each.node, each.components, where, dependent);
                for (const DofUse& use : groupDofs)
                {
                    if (std::ranges::none_of(dependent, [&use](const DofUse& chosen) { return chosen.key == use.key; }))
                        independent.push_back(use);
                }
                appendDofs(card.referenceNode, card.refComponents, where, independent);
            }
            link(dependent, independent, where, links);
        });
    forEachCard<model::RigidBarTable>(model,
                                      [&](const model::RigidBarCard& card, model::CardLocation where)
                                      {
                                          std::vector<DofUse> dependent;
                                          if (card.componentsAtA > 0)
                                              appendDofs(card.nodeA, card.componentsAtA, where, dependent);
                                          if (card.componentsAtB > 0)
                                              appendDofs(card.nodeB, card.componentsAtB, where, dependent);
                                          std::vector<DofUse> independent;
                                          if (card.independentComponentsA > 0)
                                              appendDofs(card.nodeA, card.independentComponentsA, where, independent);
                                          if (card.independentComponentsB > 0)
                                              appendDofs(card.nodeB, card.independentComponentsB, where, independent);
                                          link(dependent, independent, where, links);
                                      });
    forEachCard<model::RigidRodTable>(
        model,
        [&](const model::RigidRodCard& card, model::CardLocation where)
        {
            std::vector<DofUse> dependent;
            if (card.componentsAtA > 0)
                appendDofs(card.nodeA, card.componentsAtA, where, dependent);
            if (card.componentsAtB > 0)
                appendDofs(card.nodeB, card.componentsAtB, where, dependent);
            // The other translations of both ends carry it.
            std::vector<DofUse> independent;
            for (const base::EntityId node : {card.nodeA, card.nodeB})
            {
                for (unsigned digit = 1; digit <= 3; ++digit)
                {
                    const std::uint64_t key = dofKey(node, digit);
                    if (std::ranges::none_of(dependent, [key](const DofUse& use) { return use.key == key; }))
                        independent.push_back(DofUse{key, where});
                }
            }
            link(dependent, independent, where, links);
        });
    forEachCard<model::RigidBodyMultiTable>(model,
                                            [&](const model::RigidBodyMultiCard& card, model::CardLocation where)
                                            {
                                                std::vector<DofUse> independent;
                                                for (const model::DofPair& pair : card.independent)
                                                    appendDofs(pair.node, pair.components, where, independent);
                                                for (const model::DofPair& pair : card.dependent)
                                                    link(dofsOf(pair.node, pair.components, where), independent, where,
                                                         links);
                                            });
    return links;
}

std::vector<DofUse> mpcDependentDofs(const model::Model& model, const std::vector<base::EntityId>& sets)
{
    std::vector<DofUse> dofs;
    forEachCard<model::MultiPointConstrTable>(
        model,
        [&](const model::MultiPointConstrCard& card, model::CardLocation where)
        {
            if (!std::ranges::binary_search(sets, card.id) || card.terms.empty())
                return;
            const model::MultiPointConstrTerm& dependent = card.terms.front();
            dofs.push_back(DofUse{dofKey(dependent.node, static_cast<unsigned>(dependent.component)), where});
        });
    return dofs;
}

std::vector<DofUse> spcDofs(const model::Model& model, const std::vector<base::EntityId>& sets,
                            const std::vector<base::EntityId>& nodes)
{
    std::vector<DofUse> dofs;
    forEachCard<model::SinglePointConstrTable>(model,
                                               [&](const model::SinglePointConstrCard& card, model::CardLocation where)
                                               {
                                                   if (!std::ranges::binary_search(sets, card.id))
                                                       return;
                                                   appendDofs(card.nodes[0], card.components[0], where, dofs);
                                                   if (card.hasSecondGroup())
                                                       appendDofs(card.nodes[1], card.components[1], where, dofs);
                                               });
    forEachCard<model::SinglePointConstrListTable>(
        model,
        [&](const model::SinglePointConstrListCard& card, model::CardLocation where)
        {
            if (!std::ranges::binary_search(sets, card.id))
                return;
            for (const model::NodeRun& run : card.nodes)
            {
                // The grid points that exist in the run: the run names the ends, and the gaps
                // between are skipped.
                auto position = std::ranges::lower_bound(nodes, run.first);
                for (; position != nodes.end() && *position <= std::max(run.first, run.last); ++position)
                    appendDofs(*position, card.components, where, dofs);
            }
        });
    forEachCard<model::NodeTable>(model,
                                  [&](const model::NodeCard& card, model::CardLocation where)
                                  {
                                      if (card.permanentConstraints > 0)
                                          appendDofs(card.id, card.permanentConstraints, where, dofs);
                                  });
    return dofs;
}

} // namespace oor::analysis

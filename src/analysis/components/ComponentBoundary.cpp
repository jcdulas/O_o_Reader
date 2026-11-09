#include "oor/analysis/components/ComponentBoundary.hpp"

#include "oor/analysis/validation/engine/ValidationSupport.hpp"

#include <algorithm>

namespace oor::analysis
{

std::size_t Boundary::interfaceCount() const noexcept
{
    return static_cast<std::size_t>(
        std::count_if(nodes.begin(), nodes.end(), [](const BoundaryNode& node) { return node.isInterface; }));
}

std::size_t Boundary::retainedCount() const noexcept
{
    return static_cast<std::size_t>(
        std::count_if(nodes.begin(), nodes.end(), [](const BoundaryNode& node) { return node.isRetained; }));
}

namespace
{

// The interface nodes of a component as boundary nodes.
std::vector<BoundaryNode> interfaceOf(const model::Model& model, const ComponentModel& components,
                                      std::int32_t component)
{
    std::vector<BoundaryNode> nodes;
    for (const InterfaceNode& entry : components.interfaceNodes(component))
    {
        BoundaryNode node;
        node.node = entry.node;
        node.isInterface = true;
        node.connectionCount = entry.connectionCount;
        for (std::uint32_t index = 0; index < entry.connectionCount; ++index)
        {
            const Connection& connection =
                components.connections()[components.interfaceConnections()[entry.firstConnection + index]];
            if (connection.kind != ConnectionKind::Mpc && connection.kind != ConnectionKind::Contact)
                node.connectionElements.push_back(connection.id);
        }
        std::sort(node.connectionElements.begin(), node.connectionElements.end());
        nodes.push_back(std::move(node));
    }
    (void)model;
    return nodes;
}

// Adds retained nodes (sorted, distinct) to the boundary nodes (sorted by node).
std::vector<BoundaryNode> withRetained(std::vector<BoundaryNode> nodes, const std::vector<base::EntityId>& retained)
{
    std::vector<BoundaryNode> merged;
    merged.reserve(nodes.size() + retained.size());
    std::size_t left = 0;
    std::size_t right = 0;
    while (left < nodes.size() || right < retained.size())
    {
        if (right == retained.size() || (left < nodes.size() && nodes[left].node < retained[right]))
            merged.push_back(std::move(nodes[left++]));
        else if (left == nodes.size() || retained[right] < nodes[left].node)
        {
            BoundaryNode node;
            node.node = retained[right++];
            node.isRetained = true;
            merged.push_back(std::move(node));
        }
        else
        {
            nodes[left].isRetained = true;
            merged.push_back(std::move(nodes[left]));
            ++left;
            ++right;
        }
    }
    return merged;
}

std::vector<std::int64_t> allSubcases(const model::Model& model)
{
    std::vector<std::int64_t> numbers;
    for (const model::Subcase* subcase : analysisSubcases(model.control().caseControl))
        numbers.push_back(subcase->id);
    return numbers;
}

} // namespace

std::vector<Boundary> ComponentBoundaries::compute(const model::Model& model, const ComponentModel& components,
                                                   const LoadCaseGroups* groups, const BoundaryOptions& options)
{
    const std::size_t count = components.componentCount();
    std::vector<std::vector<BoundaryNode>> interfaces(count);
    std::vector<std::vector<base::EntityId>> unitNodes(count);
    for (std::size_t index = 0; index < count; ++index)
    {
        const auto component = static_cast<std::int32_t>(index + 1);
        if (!options.components.empty() &&
            std::find(options.components.begin(), options.components.end(), component) == options.components.end())
            continue;
        interfaces[index] = interfaceOf(model, components, component);
        const auto nodes = components.nodesOf(component);
        unitNodes[index].assign(nodes.begin(), nodes.end());
    }
    return compute(model, interfaces, unitNodes, groups, options);
}

std::vector<Boundary> ComponentBoundaries::compute(const model::Model& model,
                                                   const std::vector<std::vector<BoundaryNode>>& interfaces,
                                                   const std::vector<std::vector<base::EntityId>>& unitNodes,
                                                   const LoadCaseGroups* groups, const BoundaryOptions& options)
{
    std::vector<Boundary> boundaries;
    std::vector<std::int32_t> wanted = options.components;
    if (wanted.empty())
    {
        for (std::size_t index = 0; index < interfaces.size(); ++index)
            wanted.push_back(static_cast<std::int32_t>(index + 1));
    }
    std::sort(wanted.begin(), wanted.end());
    wanted.erase(std::unique(wanted.begin(), wanted.end()), wanted.end());
    std::vector<base::EntityId> retainedSorted = options.retainedNodes;
    std::sort(retainedSorted.begin(), retainedSorted.end());
    retainedSorted.erase(std::unique(retainedSorted.begin(), retainedSorted.end()), retainedSorted.end());
    const std::vector<std::int64_t> every = allSubcases(model);

    for (const std::int32_t component : wanted)
    {
        if (component < 1 || static_cast<std::size_t>(component) > interfaces.size())
            continue;
        const std::vector<BoundaryNode>& interface = interfaces[static_cast<std::size_t>(component - 1)];
        const std::vector<base::EntityId>& nodesOfUnit = unitNodes[static_cast<std::size_t>(component - 1)];
        switch (options.retain)
        {
        case RetainMode::None:
        {
            Boundary boundary;
            boundary.component = component;
            boundary.nodes = interface;
            boundary.subcases = every;
            boundaries.push_back(std::move(boundary));
            break;
        }
        case RetainMode::Nodes:
        {
            std::vector<base::EntityId> mine;
            for (const base::EntityId node : retainedSorted)
            {
                if (std::binary_search(nodesOfUnit.begin(), nodesOfUnit.end(), node))
                    mine.push_back(node);
            }
            Boundary boundary;
            boundary.component = component;
            boundary.nodes = withRetained(interface, mine);
            boundary.subcases = every;
            boundaries.push_back(std::move(boundary));
            break;
        }
        case RetainMode::LoadNodes:
        {
            if (groups == nullptr)
                break;
            for (const LoadGroupNodes& group : groups->groups())
            {
                if (group.component != component)
                    continue;
                if (!options.loadGroups.empty() && std::find(options.loadGroups.begin(), options.loadGroups.end(),
                                                             group.group) == options.loadGroups.end())
                    continue;
                if (!options.subcases.empty())
                {
                    const bool any = std::any_of(group.subcases.begin(), group.subcases.end(),
                                                 [&](std::int64_t subcase)
                                                 {
                                                     return std::find(options.subcases.begin(), options.subcases.end(),
                                                                      subcase) != options.subcases.end();
                                                 });
                    if (!any)
                        continue;
                }
                Boundary boundary;
                boundary.component = component;
                boundary.loadGroup = group.group;
                boundary.nodes = withRetained(interface, group.nodes);
                boundary.subcases = group.subcases;
                boundaries.push_back(std::move(boundary));
            }
            break;
        }
        }
    }
    return boundaries;
}

} // namespace oor::analysis

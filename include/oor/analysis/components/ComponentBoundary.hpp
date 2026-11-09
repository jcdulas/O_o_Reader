#pragma once

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/LoadCaseGroups.hpp"
#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <optional>
#include <vector>

namespace oor::analysis
{

// A node of a boundary: an interface node (a connection uses it), a retained node (the caller or a load case asked for
// it), or both.
struct BoundaryNode
{
    base::EntityId node = 0;
    bool isInterface = false;
    bool isRetained = false;
    std::size_t connectionCount = 0;                 // the connections that use the node, MPC equations included
    std::vector<base::EntityId> connectionElements; // the elements among them (an MPC equation has none), sorted
    // With the spiders absorbed (UnitBoundaries::absorbSpiders): the reference point of an eligible spider stands for
    // its legs. 0 and empty for any other node.
    base::EntityId spiderElement = 0;
    std::vector<base::EntityId> replacedLegs; // sorted
};

// The nodes on which a component (or a group: `component` is then the number of the group) is reduced.
struct Boundary
{
    std::int32_t component = 0;
    std::optional<std::int32_t> loadGroup; // set for a boundary per load case group
    std::vector<BoundaryNode> nodes;       // sorted by node ID
    std::vector<std::int64_t> subcases;    // the load cases this boundary serves
    [[nodiscard]] std::size_t interfaceCount() const noexcept;
    [[nodiscard]] std::size_t retainedCount() const noexcept;
};

enum class RetainMode : std::uint8_t
{
    None,     // the interface nodes alone
    Nodes,    // the interface nodes plus the nodes the caller names
    LoadNodes // the interface nodes plus the load application nodes of each group of load cases
};

struct BoundaryOptions
{
    RetainMode retain = RetainMode::None;
    std::vector<base::EntityId> retainedNodes; // for RetainMode::Nodes
    std::vector<std::int32_t> components;       // empty: every component
    std::vector<std::int64_t> subcases;         // LoadNodes: keep the groups that hold one of these load cases
    std::vector<std::int32_t> loadGroups;       // LoadNodes: keep these groups
};

// The boundaries of the components of a model.
//
// The default boundary of a component is its interface nodes alone: the nodes through which connections link it to the
// others. A load is not put on the boundary (the solver condenses it), and a node that is only constrained or only
// requested for output is never a boundary node. With retained nodes, the boundary is the interface plus those of the
// nodes that belong to the component. With the load nodes there is one boundary per component and load case group: the
// interface plus the application nodes of the group. A boundary depends on the nodes and connections of its own
// component and on the connections that touch it, not on the content of its neighbours.
class ComponentBoundaries
{
public:
    // `groups` is only read for RetainMode::LoadNodes.
    [[nodiscard]] static std::vector<Boundary> compute(const model::Model& model, const ComponentModel& components,
                                                       const LoadCaseGroups* groups, const BoundaryOptions& options);

    // The boundaries of units whose interface nodes are given (the groups of GroupSet): `interfaces[u -
    // 1]` holds the interface nodes of unit u, sorted by node, and `units` the numbers of the units to make boundaries
    // for. The retained nodes and the load nodes follow the rules of the components; `nodesOf(u)` (sorted) says which
    // retained nodes belong to unit u.
    [[nodiscard]] static std::vector<Boundary> compute(const model::Model& model,
                                                       const std::vector<std::vector<BoundaryNode>>& interfaces,
                                                       const std::vector<std::vector<base::EntityId>>& unitNodes,
                                                       const LoadCaseGroups* groups, const BoundaryOptions& options);
};

} // namespace oor::analysis

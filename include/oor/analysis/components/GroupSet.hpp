#pragma once

#include "oor/analysis/components/ComponentBoundary.hpp"
#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/ModelEntities.hpp"
#include "oor/analysis/components/NodeEntityIndex.hpp"
#include "oor/analysis/components/UnitMembership.hpp"
#include "oor/analysis/connections/ConnectionClusters.hpp"
#include "oor/base/ids/EntityId.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace oor::analysis
{

// What the IDs of a group name.
enum class GroupKind : std::uint8_t
{
    Components, // component IDs (connected_components): the group merges them
    Elements    // element IDs, connections included: the group is a zone
};

// A group as the caller gives it.
struct GroupDefinition
{
    std::string name;
    GroupKind kind = GroupKind::Components;
    std::vector<std::int64_t> ids;
};

// A group resolved on the model. Entities are indexes into ModelEntities.
struct ResolvedGroup
{
    std::string name;
    GroupKind kind = GroupKind::Components;
    std::vector<std::uint32_t> listed;   // the elements given, or the elements of the components given; increasing
    std::vector<std::uint32_t> absorbed; // the other entities whose grid points are all in the group; increasing
    std::vector<base::EntityId> nodes;  // sorted
    std::vector<std::int64_t> missing;   // the IDs given that name no component or no element
    std::vector<std::uint32_t>
        contacts; // the contact joins (ComponentModel::contactJoins) whose grid points are all in it

    [[nodiscard]] bool isMember(std::uint32_t entity) const noexcept;
};

// The groups a caller gives: lists of components to merge, or lists of element IDs (zones),
// condensed as units. Group u (from 1) is the u-th definition.
//
// The nodes of a group are the grid points of its listed elements, plus the connection-only grid points of the clusters
// of connections it absorbs: a cluster (ConnectionClusters) whose other grid points are all in the group. Every other
// entity whose grid points are all in the group belongs to it too ("absorbed"): a connection between two of its
// components, a rigid element or an MPC equation inside a zone, an element a zone encloses. The interface nodes of a
// group are its nodes that an entity outside it acts on (a connection, or for a zone an element next to it). A contact
// join of the components (a pair of contact bodies that meets) is treated the same way: absorbed, with its contact
// cards in the stamp of the group, when its grid points are all in the group; else its grid points in the group are
// interface nodes. Immutable; holds references to the entities and the index it was resolved with.
class GroupSet final : public UnitMembership
{
public:
    [[nodiscard]] static GroupSet resolve(const ModelEntities& entities, const NodeEntityIndex& index,
                                          const ComponentModel& components, const ConnectionClusters& clusters,
                                          const std::vector<GroupDefinition>& definitions);

    [[nodiscard]] const std::vector<ResolvedGroup>& groups() const noexcept { return m_groups; }
    [[nodiscard]] const ResolvedGroup& group(std::int32_t unit) const
    {
        return m_groups[static_cast<std::size_t>(unit - 1)];
    }

    [[nodiscard]] std::size_t unitCount() const noexcept override { return m_groups.size(); }
    void unitsOfNode(base::EntityId node, std::vector<std::int32_t>& units) const override;
    void unitsOfElement(model::CardLocation element, std::vector<std::int32_t>& units) const override;
    [[nodiscard]] std::span<const base::EntityId> nodesOf(std::int32_t unit) const override;

    // The interface nodes of a group, sorted, with the outside entities that act on each (`connectionCount`, MPC
    // equations included; `connectionElements`: their element IDs).
    [[nodiscard]] std::vector<BoundaryNode> interfaceOf(std::int32_t unit) const;
    // The cards of a group (its elements and absorbed MPC cards), for its revision stamp.
    [[nodiscard]] std::vector<model::CardLocation> memberCards(std::int32_t unit) const;
    // The absorbed entities that are connections (ElementRoles) or MPC equations.
    [[nodiscard]] std::size_t connectionCount(std::int32_t unit) const;
    // The members that are elements and not connections: the listed ones and those it encloses.
    [[nodiscard]] std::size_t elementCount(std::int32_t unit) const;

private:
    GroupSet(const ModelEntities& entities, const NodeEntityIndex& index) : m_entities(&entities), m_index(&index) {}

    const ModelEntities* m_entities;
    const NodeEntityIndex* m_index;
    std::vector<ResolvedGroup> m_groups;
    std::vector<std::uint8_t> m_isConnection; // by entity: 1 for a connection of ComponentModel (MPC equations too)
    std::vector<std::vector<base::EntityId>> m_contactNodes;     // by contact join: its grid points, sorted
    std::vector<std::vector<model::CardLocation>> m_contactCards; // by contact join: its contact cards
    bool m_contactIsConnection = false;                           // the joins are connections ("connection" mode)
};

} // namespace oor::analysis

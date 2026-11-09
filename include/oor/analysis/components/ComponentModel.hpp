#pragma once

#include "oor/analysis/components/ModelEntities.hpp"
#include "oor/analysis/connections/ContactOptions.hpp"
#include "oor/analysis/connections/ContactSearch.hpp"
#include "oor/analysis/connections/WeldedNodes.hpp"
#include "oor/base/ids/EntityId.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace oor::analysis
{

// One component: its number and what it holds.
struct ComponentSummary
{
    std::int32_t id = 0; // from 1, by decreasing element count, ties by the smallest element ID
    std::size_t elementCount = 0;
    std::size_t nodeCount = 0;
    std::size_t shellCount = 0;
    std::size_t solidCount = 0;
    std::size_t lineCount = 0;
    std::size_t pointCount = 0; // the concentrated masses on its nodes
    std::int32_t smallestElementId = 0;
};

// What kind of thing connects components.
enum class ConnectionKind : std::uint8_t
{
    Fastener, // springs, dampers, bushes, gaps, fasteners, welds, general elements
    Rigid,    // rigid elements
    Line,     // a structural 1D element whose nodes are on two components
    Mass,     // a concentrated mass between nodes of two components
    Mpc,      // a multipoint constraint equation
    Contact   // a pair of contact bodies that meet, in the "connection" mode of contact
};

// A connection: whatever acts as one is part of no component. `card` is the element (the MPC card for an equation) and
// `id` its element ID (the set ID of an MPC). A contact connection has its table as `card` and the table ID as `id`
// (its secondary body and 0 when the Bulk Data has no table).
struct Connection
{
    ConnectionKind kind = ConnectionKind::Fastener;
    model::CardLocation card;
    base::EntityId id = 0;
};

// A pair of contact bodies that meets, as the components see it.
struct ContactJoin
{
    std::uint32_t pair = 0; // into ContactSearch::pairs() of contactSearch()
    // The grid points it joins, sorted: the grid points that meet and the grid points (corners and mid-side points) of
    // the faces they meet.
    std::vector<base::EntityId> nodes;
    std::vector<std::int32_t> components;    // the components of those grid points, sorted (one in "merge" mode)
    std::optional<std::uint32_t> connection; // into connections(), in "connection" mode
};

// A node of a component that a connection uses, with the connections that do: indexes into connections().
struct InterfaceNode
{
    base::EntityId node = 0;
    std::uint32_t firstConnection = 0; // into interfaceConnections()
    std::uint32_t connectionCount = 0;
};

// The components of a model and the connections between them, found from the connectivity alone.
//
// A component is a set of two-dimensional and three-dimensional elements connected to one another through at least one
// common node, plus the concentrated masses on its nodes, plus the structural 1D elements (rods, tubes, bars, beams,
// bends) that are attached to it. A 1D element whose nodes are on two different components bridges them and is a
// connection; so are the connectors (springs, dampers, bushes, gaps, fasteners, welds), the rigid elements, a mass
// between two components, and the MPC equations: whatever acts as a connection is part of no component. A 1D element
// with no node on a shell or a solid makes components among themselves (a frame is a component). Grid points that rigid
// elements of zero length weld (WeldedNodes) are one point: the meshes they join are one component, and the welding
// elements are members of it, not connections. Components are
// numbered from 1 by decreasing element count, ties by the smallest element ID, so the numbering is deterministic and
// does not depend on the number of threads.
//
// The interface nodes of a component are its nodes that some connection uses: the solver keeps connections at the
// assembly level, so every node they touch must stay in the boundary of the component, whether the connection links it
// to another component or to nothing.
//
// A CFAST or a CWELD acts on the grid points of the elements it pierces (ModelEntities), so those grid points are
// interface nodes of the components they belong to.
//
// Contact between bodies (ContactSearch) acts as the contact options say: in "merge" mode (the default) each grid point
// of a pair that meets is joined with the grid points of the face it meets, so bodies in contact are one component; in
// "connection" mode each pair that meets is a connection of kind Contact on those grid points, which are then interface
// nodes; in "ignore" mode contact plays no part.
//
// Built from the whole model in one pass (a union-find over node numbers), cached in the model until its content
// changes.
class ComponentModel
{
public:
    // The components of the model with the default contact options ("merge", the default distance), cached with it
    // (valid until the content of the model changes).
    [[nodiscard]] static std::shared_ptr<const ComponentModel> of(const model::Model& model,
                                                                  base::ThreadPool* pool = nullptr);
    // The same with other contact options, cached for each.
    [[nodiscard]] static std::shared_ptr<const ComponentModel> of(const model::Model& model,
                                                                  const ContactOptions& contact,
                                                                  base::ThreadPool* pool = nullptr);
    // Builds without the cache (the fastener piercings, the entities and the contact search too).
    [[nodiscard]] static ComponentModel build(const model::Model& model, base::ThreadPool* pool = nullptr);
    // Builds from the entities of the model and its contact search (null, or ContactMode::Ignore: no contact), with
    // the grid points rigid elements of zero length weld (null: found from the entities). `connections` are the
    // elements the caller declares connections (sorted and distinct element IDs, ContactOptions::connections): each is
    // part of no component, a connection of the kind of its family when that family is one (a connector, a rigid
    // element: never a weld then, and `welds` must not count it), a ConnectionKind::Fastener otherwise (a shell, a
    // solid, a 1D element, a mass). An ID that is no element of the entities throws base::UsageError.
    [[nodiscard]] static ComponentModel build(const model::Model& model, const ModelEntities& entities,
                                              std::shared_ptr<const ContactSearch> contacts, ContactMode mode,
                                              const WeldedNodes* welds = nullptr,
                                              std::span<const base::EntityId> connections = {});

    [[nodiscard]] const std::vector<ComponentSummary>& components() const noexcept { return m_components; }
    [[nodiscard]] std::size_t componentCount() const noexcept { return m_components.size(); }
    [[nodiscard]] const ComponentSummary& component(std::int32_t id) const
    {
        return m_components[static_cast<std::size_t>(id - 1)];
    }

    // The component an element belongs to; 0 for an element that belongs to none (a connection, a plot element, a mass
    // on a node no component uses).
    [[nodiscard]] std::int32_t componentOfElement(model::CardLocation element) const noexcept;
    // The component that uses a node; 0 when no member element uses it.
    [[nodiscard]] std::int32_t componentOfNode(base::EntityId node) const noexcept;
    // The nodes of a component, sorted.
    [[nodiscard]] std::span<const base::EntityId> nodesOf(std::int32_t id) const noexcept;

    [[nodiscard]] const std::vector<Connection>& connections() const noexcept { return m_connections; }
    // The nodes of a connection (a connector's nodes, an MPC equation's grid points), in the order of the card.
    [[nodiscard]] std::span<const base::EntityId> connectionNodes(std::size_t connection) const noexcept
    {
        return std::span<const base::EntityId>(m_connectionNodes)
            .subspan(m_connectionStart[connection], m_connectionStart[connection + 1] - m_connectionStart[connection]);
    }
    // The interface nodes of a component, sorted by node ID.
    [[nodiscard]] std::span<const InterfaceNode> interfaceNodes(std::int32_t id) const noexcept;
    // The connections of the interface nodes: `interfaceConnections()[node.firstConnection ...]`, indexes into
    // connections() in increasing order.
    [[nodiscard]] std::span<const std::uint32_t> interfaceConnections() const noexcept
    {
        return m_interfaceConnections;
    }

    // The contact the components were built with: the mode, the search (null in "ignore" mode or without contact) and
    // its pairs that meet.
    [[nodiscard]] ContactMode contactMode() const noexcept { return m_contactMode; }
    [[nodiscard]] const ContactSearch* contactSearch() const noexcept { return m_contacts.get(); }
    [[nodiscard]] const std::vector<ContactJoin>& contactJoins() const noexcept { return m_contactJoins; }
    // The joins that act on a component (merged into it, or connections on its grid points): indexes into
    // contactJoins(), increasing.
    [[nodiscard]] std::vector<std::uint32_t> contactJoinsOf(std::int32_t component) const;

private:
    friend class ComponentBuilder;

    ContactMode m_contactMode = ContactMode::Ignore;
    std::shared_ptr<const ContactSearch> m_contacts;
    std::vector<ContactJoin> m_contactJoins;

    std::vector<ComponentSummary> m_components;
    std::vector<std::vector<std::int32_t>> m_elementComponent; // by card type, then row
    std::vector<base::EntityId> m_nodeIds;                    // the nodes some member element uses, sorted
    std::vector<std::int32_t> m_nodeComponent;                 // parallel to m_nodeIds
    std::vector<std::uint32_t> m_componentNodeStart;           // CSR over m_componentNodes, one entry per component + 1
    std::vector<base::EntityId> m_componentNodes;
    std::vector<Connection> m_connections;
    std::vector<std::uint32_t> m_connectionStart;
    std::vector<base::EntityId> m_connectionNodes;
    std::vector<std::uint32_t> m_interfaceStart; // per component + 1, into m_interface
    std::vector<InterfaceNode> m_interface;
    std::vector<std::uint32_t> m_interfaceConnections;
};

} // namespace oor::analysis

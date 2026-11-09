#include "oor/analysis/components/ComponentModel.hpp"

#include "NodeIndexMap.hpp"

#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/components/ElementRoles.hpp"
#include "oor/analysis/connections/FastenerPiercings.hpp"
#include "oor/base/errors/Errors.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <numeric>
#include <string>

namespace oor::analysis
{

namespace
{

constexpr std::int32_t NO_COMPONENT = 0;

// Union-find over node numbers, with path halving. One thread: the unions of a million elements cost a few tens of
// milliseconds, and the result must not depend on the order threads would have joined sets in.
class DisjointSets
{
public:
    explicit DisjointSets(std::size_t count) : m_parent(count) { std::iota(m_parent.begin(), m_parent.end(), 0); }
    [[nodiscard]] std::int32_t find(std::int32_t node) noexcept
    {
        while (m_parent[static_cast<std::size_t>(node)] != node)
        {
            const std::int32_t grandparent =
                m_parent[static_cast<std::size_t>(m_parent[static_cast<std::size_t>(node)])];
            m_parent[static_cast<std::size_t>(node)] = grandparent;
            node = grandparent;
        }
        return node;
    }
    void join(std::int32_t first, std::int32_t second) noexcept
    {
        const std::int32_t left = find(first);
        const std::int32_t right = find(second);
        if (left == right)
            return;
        // The smaller number becomes the root: the roots do not depend on the order of the unions.
        if (left < right)
            m_parent[static_cast<std::size_t>(right)] = left;
        else
            m_parent[static_cast<std::size_t>(left)] = right;
    }

private:
    std::vector<std::int32_t> m_parent;
};

struct Counts
{
    std::size_t shell = 0;
    std::size_t solid = 0;
    std::size_t line = 0;
    std::size_t point = 0;
    std::int32_t smallestId = std::numeric_limits<std::int32_t>::max();
    [[nodiscard]] std::size_t total() const noexcept { return shell + solid + line + point; }
};

} // namespace

class ComponentBuilder
{
public:
    ComponentBuilder(const ModelEntities& entities, std::shared_ptr<const ContactSearch> contacts, ContactMode mode,
                     const WeldedNodes& welds, std::span<const base::EntityId> connections)
        : m_entities(entities), m_contacts(std::move(contacts)), m_mode(mode), m_welds(welds)
    {
        while (m_elementCount < entities.size() && !entities.entity(m_elementCount).isEquation)
            ++m_elementCount;
        if (m_mode == ContactMode::Ignore || (m_contacts && m_contacts->isEmpty()))
            m_contacts = nullptr;
        if (!m_contacts)
            m_mode = ContactMode::Ignore;
        declare(connections);
    }

    ComponentModel build(const model::Model& model);

private:
    [[nodiscard]] std::span<const base::EntityId> nodesOf(std::size_t element) const noexcept
    {
        return m_entities.nodesOf(element);
    }
    // The role of an element in the components: its family's, or Connector for an element the caller declares a
    // connection whose family is not one.
    [[nodiscard]] ElementRole roleOf(std::size_t element) const noexcept
    {
        const ElementRole role = m_entities.entity(element).role;
        if (m_declared.empty() || m_declared[element] == 0 || isConnection(role))
            return role;
        return ElementRole::Connector;
    }
    // Marks the elements declared connections; throws base::UsageError for an ID that is no element of the entities.
    void declare(std::span<const base::EntityId> connections);
    // The grid points a pair that meets acts on: each grid point that meets and the corners of the face it meets.
    [[nodiscard]] static std::vector<base::EntityId> joinedNodes(const ContactPairResult& result);
    void addContactJoins(ComponentModel& components) const;
    // Joins in `sets` the grid points of each welded set that are in the map and that `keep` accepts.
    template <typename Sets, typename Keep>
    void joinWelded(const detail::NodeIndexMap& map, Sets& sets, Keep keep) const;

    const ModelEntities& m_entities;
    std::shared_ptr<const ContactSearch> m_contacts;
    ContactMode m_mode = ContactMode::Ignore;
    std::size_t m_elementCount = 0; // the elements come first in the entities, the MPC equations after them
    const WeldedNodes& m_welds;
    std::vector<std::uint8_t> m_declared; // by element, 1 for an element declared a connection (empty: none)
};

void ComponentBuilder::declare(std::span<const base::EntityId> connections)
{
    if (connections.empty())
        return;
    m_declared.assign(m_elementCount, 0);
    std::vector<std::uint8_t> found(connections.size(), 0);
    for (std::size_t index = 0; index < m_elementCount; ++index)
    {
        const base::EntityId id = m_entities.entity(index).id;
        const auto at = std::lower_bound(connections.begin(), connections.end(), id);
        if (at == connections.end() || *at != id)
            continue;
        m_declared[index] = 1;
        found[static_cast<std::size_t>(at - connections.begin())] = 1;
    }
    std::string missing;
    std::size_t count = 0;
    for (std::size_t position = 0; position < connections.size(); ++position)
    {
        if (found[position] != 0)
            continue;
        if (++count <= 10)
            missing += (missing.empty() ? "" : ", ") + std::to_string(connections[position]);
    }
    if (count > 0)
        throw base::UsageError("connections: " + std::to_string(count) +
                               " ID(s) name no element of the structure (no element has it, or a plot element): " +
                               missing + (count > 10 ? ", ..." : ""));
}

template <typename Sets, typename Keep>
void ComponentBuilder::joinWelded(const detail::NodeIndexMap& map, Sets& sets, Keep keep) const
{
    // the first grid point of each set met in the map, by the grid point that stands for the set
    std::map<base::EntityId, std::int32_t> firstOfSet;
    for (const auto& [node, representative] : m_welds.nodes())
    {
        const std::int32_t position = map.indexOf(node);
        if (position < 0 || !keep(position))
            continue;
        const auto [entry, inserted] = firstOfSet.try_emplace(representative, position);
        if (!inserted)
            sets.join(entry->second, position);
    }
}

std::vector<base::EntityId> ComponentBuilder::joinedNodes(const ContactPairResult& result)
{
    std::vector<base::EntityId> nodes;
    for (const ContactMeeting& meeting : result.meetings)
    {
        nodes.push_back(meeting.nid);
        meeting.face->appendGridPoints(nodes);
    }
    std::sort(nodes.begin(), nodes.end());
    nodes.erase(std::unique(nodes.begin(), nodes.end()), nodes.end());
    return nodes;
}

void ComponentBuilder::addContactJoins(ComponentModel& components) const
{
    components.m_contactMode = m_mode;
    components.m_contacts = m_contacts;
    if (!m_contacts)
        return;
    const std::vector<ContactPair>& pairs = m_contacts->pairs();
    for (std::size_t index = 0; index < pairs.size(); ++index)
    {
        const ContactPairResult& result = m_contacts->results()[index];
        if (!result.meets)
            continue;
        ContactJoin join;
        join.pair = static_cast<std::uint32_t>(index);
        join.nodes = joinedNodes(result);
        for (const base::EntityId node : join.nodes)
        {
            const std::int32_t component = components.componentOfNode(node);
            if (component != NO_COMPONENT)
                join.components.push_back(component);
        }
        std::sort(join.components.begin(), join.components.end());
        join.components.erase(std::unique(join.components.begin(), join.components.end()), join.components.end());
        components.m_contactJoins.push_back(std::move(join));
    }
}

ComponentModel ComponentBuilder::build(const model::Model& model)
{
    // The nodes that the elements which make components use: the map of node numbers.
    std::vector<base::EntityId> formingNodes;
    for (std::size_t index = 0; index < m_elementCount; ++index)
    {
        const ElementRole role = roleOf(index);
        if (role == ElementRole::Shell || role == ElementRole::Solid || role == ElementRole::Line)
        {
            const auto nodes = nodesOf(index);
            formingNodes.insert(formingNodes.end(), nodes.begin(), nodes.end());
        }
    }
    const detail::NodeIndexMap map(formingNodes);
    formingNodes = {};
    DisjointSets sets(map.size());
    std::vector<std::uint8_t> seeded(map.size(), 0);

    // Pass 1: shells and solids join all their nodes.
    for (std::size_t index = 0; index < m_elementCount; ++index)
    {
        const ElementRole role = roleOf(index);
        if (role != ElementRole::Shell && role != ElementRole::Solid)
            continue;
        std::int32_t first = -1;
        for (const base::EntityId node : nodesOf(index))
        {
            const std::int32_t position = map.indexOf(node);
            seeded[static_cast<std::size_t>(position)] = 1;
            if (first < 0)
                first = position;
            else
                sets.join(first, position);
        }
    }
    // Contact in "merge" mode: a grid point that meets a face joins its grid points, so the bodies in contact are one
    // component (a grid point no shell or solid uses, of a BCGRID body, joins nothing).
    if (m_mode == ContactMode::Merge)
    {
        std::vector<base::EntityId> facePoints;
        for (const ContactPairResult& result : m_contacts->results())
        {
            for (const ContactMeeting& meeting : result.meetings)
            {
                const std::int32_t position = map.indexOf(meeting.nid);
                if (position < 0)
                    continue;
                facePoints.clear();
                meeting.face->appendGridPoints(facePoints);
                for (const base::EntityId point : facePoints)
                {
                    const std::int32_t other = map.indexOf(point);
                    if (other >= 0)
                        sets.join(position, other);
                }
            }
        }
    }
    // Welded grid points are one point: their sets are joined, and a grid point welded to one a shell or a solid uses
    // counts as used by it (a 1D element on it is attached to that component).
    if (!m_welds.empty())
    {
        joinWelded(map, sets, [](std::int32_t) { return true; });
        std::vector<std::uint8_t> seededRoot(map.size(), 0);
        for (std::size_t node = 0; node < map.size(); ++node)
        {
            if (seeded[node] != 0)
                seededRoot[static_cast<std::size_t>(sets.find(static_cast<std::int32_t>(node)))] = 1;
        }
        for (const auto& entry : m_welds.nodes())
        {
            const std::int32_t position = map.indexOf(entry.first);
            if (position >= 0 && seededRoot[static_cast<std::size_t>(sets.find(position))] != 0)
                seeded[static_cast<std::size_t>(position)] = 1;
        }
    }
    std::vector<std::int32_t> seedRoot(map.size(), -1);
    for (std::size_t node = 0; node < map.size(); ++node)
    {
        if (seeded[node] != 0)
            seedRoot[node] = sets.find(static_cast<std::int32_t>(node));
    }

    // Pass 2: a structural 1D element whose nodes are on two components bridges them (a connection). The other 1D
    // elements are grouped through the nodes no shell or solid uses (a chain of beams is one group); a group that
    // reaches two components bridges them as a whole (every element of it is a connection), a group that reaches one is
    // attached to it, and a group that reaches none is a component of its own.
    std::vector<std::uint8_t> bridging(m_elementCount, 0);
    std::map<std::size_t, std::vector<base::EntityId>> chainNodes;
    std::vector<std::size_t> pending;
    for (std::size_t index = 0; index < m_elementCount; ++index)
    {
        if (roleOf(index) != ElementRole::Line)
            continue;
        std::array<std::int32_t, 4> roots{-1, -1, -1, -1};
        std::size_t distinct = 0;
        for (const base::EntityId node : nodesOf(index))
        {
            const std::int32_t root = seedRoot[static_cast<std::size_t>(map.indexOf(node))];
            if (root < 0)
                continue;
            if (std::find(roots.begin(), roots.begin() + static_cast<std::ptrdiff_t>(distinct), root) ==
                roots.begin() + static_cast<std::ptrdiff_t>(distinct))
            {
                if (distinct < roots.size())
                    roots[distinct] = root;
                ++distinct;
            }
        }
        if (distinct >= 2)
            bridging[index] = 1;
        else
            pending.push_back(index);
    }
    DisjointSets freeNodes(map.size());
    for (const std::size_t index : pending)
    {
        std::int32_t first = -1;
        for (const base::EntityId node : nodesOf(index))
        {
            const std::int32_t position = map.indexOf(node);
            if (seedRoot[static_cast<std::size_t>(position)] >= 0)
                continue;
            if (first < 0)
                first = position;
            else
                freeNodes.join(first, position);
        }
    }
    // a chain of 1D elements goes on through welded grid points no shell or solid uses
    joinWelded(map, freeNodes, [&](std::int32_t position) { return seedRoot[static_cast<std::size_t>(position)] < 0; });
    // The components each group of 1D elements reaches: (group, component), sorted and made unique.
    std::vector<std::pair<std::int32_t, std::int32_t>> reached;
    std::vector<std::pair<std::int32_t, base::EntityId>> terminals; // the nodes of components a group is attached to
    for (const std::size_t index : pending)
    {
        std::int32_t group = -1;
        std::int32_t root = -1;
        base::EntityId terminal = 0;
        for (const base::EntityId node : nodesOf(index))
        {
            const std::int32_t position = map.indexOf(node);
            if (seedRoot[static_cast<std::size_t>(position)] >= 0)
            {
                root = seedRoot[static_cast<std::size_t>(position)];
                terminal = node;
            }
            else if (group < 0)
                group = freeNodes.find(position);
        }
        if (group >= 0 && root >= 0)
        {
            reached.emplace_back(group, root);
            terminals.emplace_back(group, terminal);
        }
    }
    std::sort(terminals.begin(), terminals.end());
    terminals.erase(std::unique(terminals.begin(), terminals.end()), terminals.end());
    std::sort(reached.begin(), reached.end());
    reached.erase(std::unique(reached.begin(), reached.end()), reached.end());
    std::vector<std::int32_t> reachCount(map.size(), 0);
    for (const auto& entry : reached)
        ++reachCount[static_cast<std::size_t>(entry.first)];
    for (const std::size_t index : pending)
    {
        std::int32_t group = -1;
        for (const base::EntityId node : nodesOf(index))
        {
            const std::int32_t position = map.indexOf(node);
            if (seedRoot[static_cast<std::size_t>(position)] < 0)
            {
                group = freeNodes.find(position);
                break;
            }
        }
        if (group >= 0 && reachCount[static_cast<std::size_t>(group)] >= 2)
        {
            // The chain as a whole is the connection: the element is listed with the nodes of the components the chain
            // reaches.
            bridging[index] = 1;
            std::vector<base::EntityId>& nodes = chainNodes[index];
            const auto own = nodesOf(index);
            nodes.assign(own.begin(), own.end());
            for (auto at =
                     std::lower_bound(terminals.begin(), terminals.end(), std::make_pair(group, base::EntityId{0}));
                 at != terminals.end() && at->first == group; ++at)
            {
                if (std::find(nodes.begin(), nodes.end(), at->second) == nodes.end())
                    nodes.push_back(at->second);
            }
            continue;
        }
        std::int32_t first = -1;
        for (const base::EntityId node : nodesOf(index))
        {
            const std::int32_t position = map.indexOf(node);
            if (first < 0)
                first = position;
            else
                sets.join(first, position);
        }
    }

    // Which element belongs to which set: shells, solids and the 1D elements that do not bridge; a mass whose nodes in
    // the map are in one set (in two sets it is a connection, in none it belongs to nothing).
    constexpr std::int32_t UNASSIGNED = -1;
    constexpr std::int32_t SPANNING = -2;
    std::vector<std::int32_t> elementRoot(m_elementCount, UNASSIGNED);
    for (std::size_t index = 0; index < m_elementCount; ++index)
    {
        const ModelEntity& entry = m_entities.entity(index);
        switch (roleOf(index))
        {
        case ElementRole::Shell:
        case ElementRole::Solid:
            if (!nodesOf(index).empty())
                elementRoot[index] = sets.find(map.indexOf(nodesOf(index).front()));
            break;
        case ElementRole::Line:
            if (bridging[index] == 0 && !nodesOf(index).empty())
                elementRoot[index] = sets.find(map.indexOf(nodesOf(index).front()));
            break;
        case ElementRole::Rigid:
            // a welding element belongs to the component of the grid points it welds
            if (m_welds.isWeld(entry.card))
            {
                for (const base::EntityId node : nodesOf(index))
                {
                    const std::int32_t position = map.indexOf(node);
                    if (position >= 0)
                    {
                        elementRoot[index] = sets.find(position);
                        break;
                    }
                }
            }
            break;
        case ElementRole::Mass:
        {
            std::int32_t root = UNASSIGNED;
            for (const base::EntityId node : nodesOf(index))
            {
                const std::int32_t position = map.indexOf(node);
                if (position < 0)
                    continue;
                const std::int32_t found = sets.find(position);
                if (root == UNASSIGNED)
                    root = found;
                else if (root != found)
                    root = SPANNING;
            }
            elementRoot[index] = root;
            break;
        }
        default:
            break;
        }
    }

    // The members of each set; a set with no member is not a component.
    std::vector<Counts> counts(map.size());
    for (std::size_t index = 0; index < m_elementCount; ++index)
    {
        const std::int32_t root = elementRoot[index];
        if (root < 0 || roleOf(index) == ElementRole::Rigid)
            continue; // a welding element is a member, not one of the elements the component is counted by
        Counts& entry = counts[static_cast<std::size_t>(root)];
        switch (roleOf(index))
        {
        case ElementRole::Shell:
            ++entry.shell;
            break;
        case ElementRole::Solid:
            ++entry.solid;
            break;
        case ElementRole::Line:
            ++entry.line;
            break;
        default:
            ++entry.point;
            break;
        }
        entry.smallestId = std::min(entry.smallestId, m_entities.entity(index).id);
    }
    std::vector<std::int32_t> roots;
    for (std::size_t root = 0; root < counts.size(); ++root)
    {
        if (counts[root].total() > 0)
            roots.push_back(static_cast<std::int32_t>(root));
    }
    std::sort(roots.begin(), roots.end(),
              [&](std::int32_t left, std::int32_t right)
              {
                  const Counts& a = counts[static_cast<std::size_t>(left)];
                  const Counts& b = counts[static_cast<std::size_t>(right)];
                  if (a.total() != b.total())
                      return a.total() > b.total();
                  return a.smallestId < b.smallestId;
              });
    std::vector<std::int32_t> componentOfRoot(map.size(), NO_COMPONENT);
    ComponentModel result;
    for (std::size_t number = 0; number < roots.size(); ++number)
    {
        const Counts& entry = counts[static_cast<std::size_t>(roots[number])];
        componentOfRoot[static_cast<std::size_t>(roots[number])] = static_cast<std::int32_t>(number + 1);
        ComponentSummary summary;
        summary.id = static_cast<std::int32_t>(number + 1);
        summary.elementCount = entry.total();
        summary.shellCount = entry.shell;
        summary.solidCount = entry.solid;
        summary.lineCount = entry.line;
        summary.pointCount = entry.point;
        summary.smallestElementId = entry.smallestId;
        result.m_components.push_back(summary);
    }

    // Membership of the elements, and of the nodes that member elements use.
    const std::size_t typeCount = model::CardRegistry::instance().size();
    result.m_elementComponent.resize(typeCount);
    std::vector<std::int32_t> nodeComponent(map.size(), NO_COMPONENT);
    for (std::size_t index = 0; index < m_elementCount; ++index)
    {
        const ModelEntity& entry = m_entities.entity(index);
        const std::int32_t root = elementRoot[index];
        const std::int32_t component = root >= 0 ? componentOfRoot[static_cast<std::size_t>(root)] : NO_COMPONENT;
        if (component == NO_COMPONENT)
            continue;
        std::vector<std::int32_t>& rows = result.m_elementComponent[entry.card.table];
        if (rows.empty())
            rows.assign(model.findTable(entry.card.table)->rows().rowCount(), NO_COMPONENT);
        rows[entry.card.row] = component;
        for (const base::EntityId node : nodesOf(index))
        {
            const std::int32_t position = map.indexOf(node);
            if (position >= 0)
                nodeComponent[static_cast<std::size_t>(position)] = component;
        }
    }
    for (std::size_t position = 0; position < map.size(); ++position)
    {
        if (nodeComponent[position] == NO_COMPONENT)
            continue;
        result.m_nodeIds.push_back(map.ids()[position]);
        result.m_nodeComponent.push_back(nodeComponent[position]);
        ++result.m_components[static_cast<std::size_t>(nodeComponent[position] - 1)].nodeCount;
    }
    // The nodes of each component, sorted (the ids are visited in increasing order).
    result.m_componentNodeStart.assign(result.m_components.size() + 1, 0);
    for (const std::int32_t component : result.m_nodeComponent)
        ++result.m_componentNodeStart[static_cast<std::size_t>(component)];
    for (std::size_t component = 0; component < result.m_components.size(); ++component)
        result.m_componentNodeStart[component + 1] += result.m_componentNodeStart[component];
    result.m_componentNodes.resize(result.m_nodeIds.size());
    {
        std::vector<std::uint32_t> fill(result.m_componentNodeStart.begin(), result.m_componentNodeStart.end() - 1);
        for (std::size_t position = 0; position < result.m_nodeIds.size(); ++position)
            result.m_componentNodes[fill[static_cast<std::size_t>(result.m_nodeComponent[position] - 1)]++] =
                result.m_nodeIds[position];
    }

    // The connections: connectors, rigid elements, bridging 1D elements, masses between components, MPC equations.
    result.m_connectionStart.push_back(0);
    const auto addConnection =
        [&](ConnectionKind kind, model::CardLocation card, base::EntityId id, std::span<const base::EntityId> nodes)
    {
        result.m_connections.push_back(Connection{kind, card, id});
        result.m_connectionNodes.insert(result.m_connectionNodes.end(), nodes.begin(), nodes.end());
        result.m_connectionStart.push_back(static_cast<std::uint32_t>(result.m_connectionNodes.size()));
    };
    for (std::size_t index = 0; index < m_elementCount; ++index)
    {
        const ModelEntity& entry = m_entities.entity(index);
        const model::CardLocation card = entry.card;
        switch (roleOf(index))
        {
        case ElementRole::Connector:
            addConnection(ConnectionKind::Fastener, card, entry.id, nodesOf(index));
            break;
        case ElementRole::Rigid:
            if (!m_welds.isWeld(card))
                addConnection(ConnectionKind::Rigid, card, entry.id, nodesOf(index));
            break;
        case ElementRole::Line:
            if (bridging[index] != 0)
            {
                const auto chain = chainNodes.find(index);
                addConnection(
                    ConnectionKind::Line, card, entry.id,
                    chain != chainNodes.end() ? std::span<const base::EntityId>(chain->second) : nodesOf(index));
            }
            break;
        case ElementRole::Mass:
            if (elementRoot[index] == SPANNING)
                addConnection(ConnectionKind::Mass, card, entry.id, nodesOf(index));
            break;
        default:
            break;
        }
    }
    for (std::size_t index = m_elementCount; index < m_entities.size(); ++index)
    {
        const ModelEntity& equation = m_entities.entity(index);
        addConnection(ConnectionKind::Mpc, equation.card, equation.id, m_entities.nodesOf(index));
    }
    // A mass on a grid point that only connections use (the mass of a spider's reference point, a lumped mass on
    // springs) is part of the connection system: a connection of its own, in the cluster of that point (units;
    // a companion package's P5.3). A mass on a point nothing else uses stays part of nothing.
    std::vector<base::EntityId> connectionPoints(result.m_connectionNodes.begin(), result.m_connectionNodes.end());
    std::sort(connectionPoints.begin(), connectionPoints.end());
    connectionPoints.erase(std::unique(connectionPoints.begin(), connectionPoints.end()), connectionPoints.end());
    for (std::size_t index = 0; index < m_elementCount; ++index)
    {
        const ModelEntity& entry = m_entities.entity(index);
        if (roleOf(index) != ElementRole::Mass || elementRoot[index] != UNASSIGNED)
            continue;
        const auto nodes = nodesOf(index);
        const bool onConnection =
            std::any_of(nodes.begin(), nodes.end(), [&](base::EntityId node)
                        { return std::binary_search(connectionPoints.begin(), connectionPoints.end(), node); });
        if (onConnection)
            addConnection(ConnectionKind::Mass, entry.card, entry.id, nodes);
    }
    addContactJoins(result);
    if (m_mode == ContactMode::Connection)
    {
        for (ContactJoin& join : result.m_contactJoins)
        {
            const ContactPair& pair = m_contacts->pairs()[join.pair];
            join.connection = static_cast<std::uint32_t>(result.m_connections.size());
            addConnection(ConnectionKind::Contact, pair.table ? *pair.table : pair.secondary->card, pair.tableId,
                          join.nodes);
        }
    }

    // The interface nodes: the nodes of a component that some connection uses.
    struct Use
    {
        std::int32_t component;
        base::EntityId node;
        std::uint32_t connection;
        [[nodiscard]] bool operator<(const Use& other) const noexcept
        {
            if (component != other.component)
                return component < other.component;
            if (node != other.node)
                return node < other.node;
            return connection < other.connection;
        }
        [[nodiscard]] bool operator==(const Use&) const noexcept = default;
    };
    std::vector<Use> uses;
    for (std::size_t connection = 0; connection < result.m_connections.size(); ++connection)
    {
        for (const base::EntityId node : result.connectionNodes(connection))
        {
            const std::int32_t component = result.componentOfNode(node);
            if (component != NO_COMPONENT)
                uses.push_back(Use{component, node, static_cast<std::uint32_t>(connection)});
        }
    }
    std::sort(uses.begin(), uses.end());
    uses.erase(std::unique(uses.begin(), uses.end()), uses.end());
    result.m_interfaceStart.assign(result.m_components.size() + 1, 0);
    for (std::size_t position = 0; position < uses.size();)
    {
        std::size_t end = position;
        while (end < uses.size() && uses[end].component == uses[position].component &&
               uses[end].node == uses[position].node)
            ++end;
        InterfaceNode node;
        node.node = uses[position].node;
        node.firstConnection = static_cast<std::uint32_t>(result.m_interfaceConnections.size());
        node.connectionCount = static_cast<std::uint32_t>(end - position);
        for (std::size_t item = position; item < end; ++item)
            result.m_interfaceConnections.push_back(uses[item].connection);
        result.m_interface.push_back(node);
        ++result.m_interfaceStart[static_cast<std::size_t>(uses[position].component)];
        position = end;
    }
    for (std::size_t component = 0; component < result.m_components.size(); ++component)
        result.m_interfaceStart[component + 1] += result.m_interfaceStart[component];
    return result;
}

ComponentModel ComponentModel::build(const model::Model& model, base::ThreadPool* pool)
{
    const FastenerPiercings piercings = FastenerPiercings::compute(model, pool);
    std::shared_ptr<const ContactSearch> contacts =
        ContactSearch::compute(model, std::nullopt, ContactRule::Components, pool);
    return build(model, ModelEntities::build(model, &piercings, pool), std::move(contacts), ContactMode::Merge);
}

ComponentModel ComponentModel::build(const model::Model& model, const ModelEntities& entities,
                                     std::shared_ptr<const ContactSearch> contacts, ContactMode mode,
                                     const WeldedNodes* welds, std::span<const base::EntityId> connections)
{
    if (welds != nullptr)
        return ComponentBuilder(entities, std::move(contacts), mode, *welds, connections).build(model);
    const WeldedNodes found = WeldedNodes::build(model, entities, connections);
    return ComponentBuilder(entities, std::move(contacts), mode, found, connections).build(model);
}

std::shared_ptr<const ComponentModel> ComponentModel::of(const model::Model& model, base::ThreadPool* pool)
{
    return ComponentCache::of(model).components(model, ContactOptions{}, pool);
}

std::shared_ptr<const ComponentModel> ComponentModel::of(const model::Model& model, const ContactOptions& contact,
                                                         base::ThreadPool* pool)
{
    return ComponentCache::of(model).components(model, contact, pool);
}

std::vector<std::uint32_t> ComponentModel::contactJoinsOf(std::int32_t component) const
{
    std::vector<std::uint32_t> joins;
    for (std::size_t index = 0; index < m_contactJoins.size(); ++index)
    {
        const std::vector<std::int32_t>& touched = m_contactJoins[index].components;
        if (std::binary_search(touched.begin(), touched.end(), component))
            joins.push_back(static_cast<std::uint32_t>(index));
    }
    return joins;
}

std::int32_t ComponentModel::componentOfElement(model::CardLocation element) const noexcept
{
    if (element.table >= m_elementComponent.size())
        return NO_COMPONENT;
    const std::vector<std::int32_t>& rows = m_elementComponent[element.table];
    return element.row < rows.size() ? rows[element.row] : NO_COMPONENT;
}

std::int32_t ComponentModel::componentOfNode(base::EntityId node) const noexcept
{
    const auto found = std::lower_bound(m_nodeIds.begin(), m_nodeIds.end(), node);
    if (found == m_nodeIds.end() || *found != node)
        return NO_COMPONENT;
    return m_nodeComponent[static_cast<std::size_t>(found - m_nodeIds.begin())];
}

std::span<const base::EntityId> ComponentModel::nodesOf(std::int32_t id) const noexcept
{
    if (id < 1 || static_cast<std::size_t>(id) > m_components.size())
        return {};
    const std::size_t component = static_cast<std::size_t>(id - 1);
    return std::span<const base::EntityId>(m_componentNodes)
        .subspan(m_componentNodeStart[component],
                 m_componentNodeStart[component + 1] - m_componentNodeStart[component]);
}

std::span<const InterfaceNode> ComponentModel::interfaceNodes(std::int32_t id) const noexcept
{
    if (id < 1 || static_cast<std::size_t>(id) > m_components.size())
        return {};
    const std::size_t component = static_cast<std::size_t>(id - 1);
    return std::span<const InterfaceNode>(m_interface)
        .subspan(m_interfaceStart[component], m_interfaceStart[component + 1] - m_interfaceStart[component]);
}

} // namespace oor::analysis

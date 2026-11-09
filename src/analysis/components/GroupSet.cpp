#include "oor/analysis/components/GroupSet.hpp"

#include <algorithm>
#include <initializer_list>
#include <map>
#include <optional>
#include <utility>

namespace oor::analysis
{

namespace
{

bool contains(const std::vector<base::EntityId>& sorted, base::EntityId node) noexcept
{
    return std::binary_search(sorted.begin(), sorted.end(), node);
}

void sortUnique(std::vector<std::uint32_t>& values)
{
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}

void sortUnique(std::vector<base::EntityId>& values)
{
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}

} // namespace

bool ResolvedGroup::isMember(std::uint32_t entity) const noexcept
{
    return std::binary_search(listed.begin(), listed.end(), entity) ||
           std::binary_search(absorbed.begin(), absorbed.end(), entity);
}

GroupSet GroupSet::resolve(const ModelEntities& entities, const NodeEntityIndex& index,
                           const ComponentModel& components, const ConnectionClusters& clusters,
                           const std::vector<GroupDefinition>& definitions)
{
    GroupSet result(entities, index);
    // Which entities are connections of the component model, and the entity of each MPC card.
    result.m_isConnection.assign(entities.size(), 0);
    std::map<std::pair<std::uint16_t, std::uint32_t>, std::uint32_t> equationEntity;
    for (std::size_t position = 0; position < entities.size(); ++position)
    {
        const ModelEntity& entity = entities.entity(position);
        if (entity.isEquation)
            equationEntity.emplace(std::make_pair(entity.card.table, entity.card.row),
                                   static_cast<std::uint32_t>(position));
    }
    for (std::size_t connection = 0; connection < components.connections().size(); ++connection)
    {
        const Connection& item = components.connections()[connection];
        std::optional<std::size_t> position = entities.indexOf(item.card);
        if (item.kind == ConnectionKind::Mpc)
        {
            const auto found = equationEntity.find(std::make_pair(item.card.table, item.card.row));
            position = found == equationEntity.end() ? std::nullopt : std::optional<std::size_t>(found->second);
        }
        if (position)
            result.m_isConnection[*position] = 1;
    }
    // The contact joins, with the cards that make them.
    result.m_contactIsConnection = components.contactMode() == ContactMode::Connection;
    for (const ContactJoin& join : components.contactJoins())
    {
        result.m_contactNodes.push_back(join.nodes);
        result.m_contactCards.push_back(components.contactSearch()->pairs()[join.pair].cards);
    }
    // The elements by ID, for the groups given as element lists.
    std::vector<std::pair<base::EntityId, std::uint32_t>> byId;
    for (std::size_t position = 0; position < entities.size(); ++position)
    {
        if (!entities.entity(position).isEquation)
            byId.emplace_back(entities.entity(position).id, static_cast<std::uint32_t>(position));
    }
    std::sort(byId.begin(), byId.end());

    for (const GroupDefinition& definition : definitions)
    {
        ResolvedGroup group;
        group.name = definition.name;
        group.kind = definition.kind;
        if (definition.kind == GroupKind::Components)
        {
            std::vector<std::int32_t> wanted;
            for (const std::int64_t id : definition.ids)
            {
                if (id >= 1 && static_cast<std::size_t>(id) <= components.componentCount())
                    wanted.push_back(static_cast<std::int32_t>(id));
                else
                    group.missing.push_back(id);
            }
            for (std::size_t position = 0; position < entities.size(); ++position)
            {
                const ModelEntity& entity = entities.entity(position);
                if (entity.isEquation)
                    continue;
                const std::int32_t component = components.componentOfElement(entity.card);
                if (component != 0 && std::find(wanted.begin(), wanted.end(), component) != wanted.end())
                    group.listed.push_back(static_cast<std::uint32_t>(position));
            }
        }
        else
        {
            for (const std::int64_t id : definition.ids)
            {
                const auto found =
                    std::lower_bound(byId.begin(), byId.end(), id,
                                     [](const auto& entry, std::int64_t wanted) { return entry.first < wanted; });
                if (found != byId.end() && found->first == id)
                    group.listed.push_back(found->second);
                else
                    group.missing.push_back(id);
            }
        }
        sortUnique(group.listed);
        for (const std::uint32_t entity : group.listed)
        {
            const auto nodes = entities.nodesOf(entity);
            group.nodes.insert(group.nodes.end(), nodes.begin(), nodes.end());
        }
        group.nodes.erase(std::remove(group.nodes.begin(), group.nodes.end(), 0), group.nodes.end());
        sortUnique(group.nodes);

        // The clusters whose grid points held by components are all in the group: their connection-only grid points
        // join it.
        std::vector<base::EntityId> added;
        for (const ConnectionCluster& cluster : clusters.clusters())
        {
            bool inside = true;
            for (const std::uint32_t connection : cluster.connections)
            {
                for (const base::EntityId node : components.connectionNodes(connection))
                {
                    if (node != 0 && !clusters.isConnectionOnly(node) && !contains(group.nodes, node))
                        inside = false;
                }
            }
            if (inside && !cluster.components.empty())
                added.insert(added.end(), cluster.connectionOnlyNodes.begin(), cluster.connectionOnlyNodes.end());
        }
        group.nodes.insert(group.nodes.end(), added.begin(), added.end());
        sortUnique(group.nodes);

        // Every other entity whose grid points are all in the group.
        std::vector<std::uint32_t> touching;
        for (const base::EntityId node : group.nodes)
        {
            const auto at = index.entitiesAt(node);
            touching.insert(touching.end(), at.begin(), at.end());
        }
        sortUnique(touching);
        for (const std::uint32_t entity : touching)
        {
            if (std::binary_search(group.listed.begin(), group.listed.end(), entity))
                continue;
            const auto nodes = entities.nodesOf(entity);
            const bool inside = !nodes.empty() && std::all_of(nodes.begin(), nodes.end(), [&](base::EntityId node)
                                                              { return node == 0 || contains(group.nodes, node); });
            if (inside)
                group.absorbed.push_back(entity);
        }
        for (std::size_t join = 0; join < result.m_contactNodes.size(); ++join)
        {
            const std::vector<base::EntityId>& nodes = result.m_contactNodes[join];
            if (std::all_of(nodes.begin(), nodes.end(),
                            [&](base::EntityId node) { return contains(group.nodes, node); }))
                group.contacts.push_back(static_cast<std::uint32_t>(join));
        }
        result.m_groups.push_back(std::move(group));
    }
    return result;
}

void GroupSet::unitsOfNode(base::EntityId node, std::vector<std::int32_t>& units) const
{
    for (std::size_t position = 0; position < m_groups.size(); ++position)
    {
        if (contains(m_groups[position].nodes, node))
            units.push_back(static_cast<std::int32_t>(position + 1));
    }
}

void GroupSet::unitsOfElement(model::CardLocation element, std::vector<std::int32_t>& units) const
{
    const auto entity = m_entities->indexOf(element);
    if (!entity)
        return;
    for (std::size_t position = 0; position < m_groups.size(); ++position)
    {
        if (m_groups[position].isMember(static_cast<std::uint32_t>(*entity)))
            units.push_back(static_cast<std::int32_t>(position + 1));
    }
}

std::span<const base::EntityId> GroupSet::nodesOf(std::int32_t unit) const
{
    if (unit < 1 || static_cast<std::size_t>(unit) > m_groups.size())
        return {};
    return group(unit).nodes;
}

std::vector<BoundaryNode> GroupSet::interfaceOf(std::int32_t unit) const
{
    std::vector<BoundaryNode> interface;
    const ResolvedGroup& resolved = group(unit);
    // The contact joins outside the group that act on its grid points.
    std::map<base::EntityId, std::uint32_t> contactsAt;
    for (std::size_t join = 0; join < m_contactNodes.size(); ++join)
    {
        if (std::binary_search(resolved.contacts.begin(), resolved.contacts.end(), static_cast<std::uint32_t>(join)))
            continue;
        for (const base::EntityId node : m_contactNodes[join])
        {
            if (contains(resolved.nodes, node))
                ++contactsAt[node];
        }
    }
    for (const base::EntityId node : resolved.nodes)
    {
        BoundaryNode entry;
        entry.node = node;
        for (const std::uint32_t entity : m_index->entitiesAt(node))
        {
            if (resolved.isMember(entity))
                continue;
            ++entry.connectionCount;
            if (!m_entities->entity(entity).isEquation)
                entry.connectionElements.push_back(m_entities->entity(entity).id);
        }
        if (const auto found = contactsAt.find(node); found != contactsAt.end())
            entry.connectionCount += found->second;
        if (entry.connectionCount == 0)
            continue;
        entry.isInterface = true;
        sortUnique(entry.connectionElements);
        interface.push_back(std::move(entry));
    }
    return interface;
}

std::vector<model::CardLocation> GroupSet::memberCards(std::int32_t unit) const
{
    std::vector<model::CardLocation> cards;
    const ResolvedGroup& resolved = group(unit);
    for (const auto* list : {&resolved.listed, &resolved.absorbed})
    {
        for (const std::uint32_t entity : *list)
            cards.push_back(m_entities->entity(entity).card);
    }
    for (const std::uint32_t join : resolved.contacts)
        cards.insert(cards.end(), m_contactCards[join].begin(), m_contactCards[join].end());
    return cards;
}

std::size_t GroupSet::connectionCount(std::int32_t unit) const
{
    const ResolvedGroup& resolved = group(unit);
    std::size_t count = m_contactIsConnection ? resolved.contacts.size() : 0;
    for (const auto* list : {&resolved.listed, &resolved.absorbed})
    {
        for (const std::uint32_t entity : *list)
            count += m_isConnection[entity];
    }
    return count;
}

std::size_t GroupSet::elementCount(std::int32_t unit) const
{
    const ResolvedGroup& resolved = group(unit);
    std::size_t count = 0;
    for (const auto* list : {&resolved.listed, &resolved.absorbed})
    {
        for (const std::uint32_t entity : *list)
            count += m_isConnection[entity] == 0 ? 1 : 0;
    }
    return count;
}

} // namespace oor::analysis

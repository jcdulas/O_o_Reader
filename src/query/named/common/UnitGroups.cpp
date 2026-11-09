#include "oor/query/named/common/UnitGroups.hpp"

#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/query/QueryErrors.hpp"

namespace oor::query
{

UnitGroups::UnitGroups(const model::Model& model, const std::vector<NamedIdSet>& groups, const std::string& query,
                       const analysis::ContactOptions& contact, base::ThreadPool* pool)
{
    analysis::ComponentCache& cache = analysis::ComponentCache::of(model);
    m_entities = cache.entities(model, pool);
    m_components = cache.components(model, contact, pool);
    m_index = std::make_unique<analysis::NodeEntityIndex>(*m_entities);
    m_clusters =
        std::make_unique<analysis::ConnectionClusters>(analysis::ConnectionClusters::build(model, *m_components, pool));
    std::vector<analysis::GroupDefinition> definitions;
    for (const NamedIdSet& group : groups)
    {
        analysis::GroupDefinition definition;
        definition.name = group.name;
        definition.kind = group.kind == "elements" ? analysis::GroupKind::Elements : analysis::GroupKind::Components;
        definition.ids = group.ids;
        definitions.push_back(std::move(definition));
    }
    m_groups = std::make_unique<analysis::GroupSet>(
        analysis::GroupSet::resolve(*m_entities, *m_index, *m_components, *m_clusters, definitions));
    for (const analysis::ResolvedGroup& resolved : m_groups->groups())
    {
        if (resolved.missing.empty())
            continue;
        std::string ids;
        for (const std::int64_t id : resolved.missing)
            ids += (ids.empty() ? "" : ", ") + std::to_string(id);
        throw QueryError(query + ": the group '" + resolved.name + "' names " +
                         (resolved.kind == analysis::GroupKind::Elements ? "elements" : "components") +
                         " the model does not have: " + ids);
    }
}

} // namespace oor::query

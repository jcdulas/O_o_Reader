#pragma once

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/GroupSet.hpp"
#include "oor/analysis/components/ModelEntities.hpp"
#include "oor/analysis/components/NodeEntityIndex.hpp"
#include "oor/analysis/connections/ConnectionClusters.hpp"
#include "oor/analysis/connections/ContactOptions.hpp"
#include "oor/base/threading/ThreadPool.hpp"
#include "oor/model/core/Model.hpp"
#include "oor/query/named/QueryValue.hpp"

#include <memory>
#include <string>
#include <vector>

namespace oor::query
{

// The groups of a `groups` argument resolved on a model, with everything they were resolved with
// (the entities, their index, the components of the contact options and the clusters of connections), kept alive
// together. An ID that names no component or no element raises QueryError naming the group; `query` names the query in
// the message.
class UnitGroups
{
public:
    UnitGroups(const model::Model& model, const std::vector<NamedIdSet>& groups, const std::string& query,
               const analysis::ContactOptions& contact, base::ThreadPool* pool);

    [[nodiscard]] const analysis::GroupSet& groups() const noexcept { return *m_groups; }
    [[nodiscard]] const analysis::ModelEntities& entities() const noexcept { return *m_entities; }
    [[nodiscard]] const analysis::NodeEntityIndex& index() const noexcept { return *m_index; }
    [[nodiscard]] const analysis::ComponentModel& components() const noexcept { return *m_components; }
    [[nodiscard]] const analysis::ConnectionClusters& clusters() const noexcept { return *m_clusters; }
    // The name of group `unit` (from 1).
    [[nodiscard]] const std::string& nameOf(std::int32_t unit) const { return m_groups->group(unit).name; }

private:
    std::shared_ptr<const analysis::ModelEntities> m_entities;
    std::shared_ptr<const analysis::ComponentModel> m_components;
    std::unique_ptr<analysis::NodeEntityIndex> m_index;
    std::unique_ptr<analysis::ConnectionClusters> m_clusters;
    std::unique_ptr<analysis::GroupSet> m_groups;
};

} // namespace oor::query

#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/LoadCaseGroups.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ContactArguments.hpp"
#include "oor/query/named/common/UnitGroups.hpp"
#include "oor/query/named/topology/t4_components/ComponentQueries.hpp"

#include <algorithm>
#include <cstring>
#include <optional>

namespace oor::query
{

namespace
{

std::int64_t asInt64(std::uint64_t stamp)
{
    std::int64_t value = 0;
    std::memcpy(&value, &stamp, sizeof(value));
    return value;
}

std::vector<std::int32_t> componentsFrom(const QueryArguments& arguments)
{
    std::vector<std::int32_t> components;
    if (const auto ids = arguments.ids("components"))
    {
        for (const std::int64_t id : *ids)
            components.push_back(static_cast<std::int32_t>(id));
    }
    return components;
}

// loadCaseGroups: per component, the load cases grouped by identical sets of load application nodes.
class LoadCaseGroupsQuery final : public QueryBase
{
public:
    LoadCaseGroupsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::string detail = run.arguments.textOr("detail", "groups");
        if (detail != "groups" && detail != "nodes")
            throw QueryError("loadCaseGroups: detail is \"groups\" or \"nodes\", not \"" + detail + "\"");
        const model::Model& model = *run.model;
        const auto components =
            analysis::ComponentModel::of(model, contactOptionsOf(run.arguments, "loadCaseGroups"), run.context.pool());
        std::vector<std::int64_t> subcases;
        if (const auto ids = run.arguments.ids("subcases"))
            subcases = *ids;
        const bool byGroup = run.arguments.has("groups");
        std::optional<UnitGroups> units;
        if (byGroup)
            units.emplace(model, run.arguments.groups("groups"), "loadCaseGroups",
                          contactOptionsOf(run.arguments, "loadCaseGroups"), run.context.pool());
        const analysis::LoadCaseGroups groups =
            byGroup ? analysis::LoadCaseGroups::compute(model, units->groups(), subcases)
                    : analysis::LoadCaseGroups::compute(model, *components, subcases);
        const std::vector<std::int32_t> wanted = byGroup ? std::vector<std::int32_t>{} : componentsFrom(run.arguments);
        const auto keep = [&](std::int32_t component)
        { return wanted.empty() || std::find(wanted.begin(), wanted.end(), component) != wanted.end(); };
        // The key of the rows: the component, or the name of the group.
        const auto keyColumn = [&](TableBuilder& builder, std::vector<std::int64_t> keys)
        {
            if (!byGroup)
            {
                builder.int64("componentId", std::move(keys));
                return;
            }
            std::vector<std::string> names;
            for (const std::int64_t unit : keys)
                names.push_back(units->nameOf(static_cast<std::int32_t>(unit)));
            builder.text("group", names);
        };

        TableBuilder builder;
        if (detail == "nodes")
        {
            std::vector<std::int64_t> component;
            std::vector<std::int64_t> group;
            std::vector<std::int32_t> nid;
            for (const analysis::LoadGroupNodes& entry : groups.groups())
            {
                if (!keep(entry.component))
                    continue;
                for (const base::EntityId node : entry.nodes)
                {
                    component.push_back(entry.component);
                    group.push_back(entry.group);
                    nid.push_back(node);
                }
            }
            keyColumn(builder, std::move(component));
            builder.int64("loadGroup", std::move(group));
            builder.int32("nodeId", std::move(nid));
            return builder.finish();
        }
        std::vector<std::int64_t> component;
        std::vector<std::int64_t> group;
        std::vector<std::int32_t> subcase;
        std::vector<std::int32_t> loadSid;
        std::vector<std::uint8_t> loadSidValid;
        std::vector<std::int64_t> loaded;
        std::vector<std::uint8_t> distributed;
        std::vector<std::int64_t> revision;
        for (const analysis::LoadCaseRow& row : groups.rows())
        {
            if (!keep(row.component))
                continue;
            component.push_back(row.component);
            group.push_back(row.group);
            subcase.push_back(static_cast<std::int32_t>(row.subcase));
            loadSid.push_back(static_cast<std::int32_t>(row.loadSid));
            loadSidValid.push_back(row.loadSid != 0 ? 1 : 0);
            loaded.push_back(static_cast<std::int64_t>(row.loadedNodeCount));
            distributed.push_back(row.isDistributed ? 1 : 0);
            revision.push_back(asInt64(row.loadRevision));
        }
        keyColumn(builder, std::move(component));
        builder.int64("loadGroup", std::move(group));
        builder.int32("subcase", std::move(subcase));
        builder.int32("loadSetId", std::move(loadSid), std::move(loadSidValid));
        builder.int64("loadedNodeCount", std::move(loaded));
        builder.flag("isDistributed", distributed);
        builder.int64("loadRevision", std::move(revision));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "loadCaseGroups";
        info.family = "T4";
        info.description =
            "For each component, the load cases (the analysis subcases of the Case Control) grouped by identical sets "
            "of load "
            "application nodes on that component: the nodes of the component that carry a force, a moment, a scalar "
            "load or "
            "an enforced motion. Group 0 holds the load cases that apply no concentrated load to the component (a case "
            "with "
            "only pressures, gravity or temperatures is there, and isDistributed tells it from a case with no load at "
            "all); the other groups are numbered in the order their first case appears. loadRevision is the load "
            "revision "
            "stamp of the component for the case: a hash of the loads applied to it, combination factors included, as "
            "a "
            "signed int64. A SUBCOM loads what the subcases it combines load, with their SUBSEQ factors. "
            "detail=\"nodes\": the application nodes of each group. groups={name: {\"components\": IDs}} or "
            "{name: {\"elements\": IDs}} groups the load cases of those groups instead, with "
            "the same rules (a node of two overlapping groups loads both): the rows are keyed by `group` in place of "
            "componentId.";
        info.detailParameter = "detail";
        info.parameters = {parameter("components", ParameterKind::IdList, "None", "keep these components"),
                           parameter("subcases", ParameterKind::IdList, "None", "keep these load cases"),
                           parameter("groups", ParameterKind::Groups, "None",
                                     "{name: {\"components\": IDs}} or {name: {\"elements\": IDs}}: group load cases "
                                     "per group"),
                           parameter("detail", ParameterKind::Text, "None", "None or \"nodes\"")};
        const std::vector<ParameterInfo> contact = contactParameters();
        info.parameters.insert(info.parameters.end() - 1, contact.begin(), contact.end()); // before `detail`
        info.defaultSort = "componentId, loadGroup, subcase";
        ColumnListBuilder groups;
        groups.int64("componentId").int64("loadGroup").int32("subcase").int32("loadSetId").int64("loadedNodeCount");
        groups.flag("isDistributed").int64("loadRevision");
        info.columnSets.push_back(groups.build());
        ColumnListBuilder nodes;
        nodes.int64("componentId").int64("loadGroup").int32("nodeId");
        info.columnSets.push_back(nodes.build("nodes"));
        return info;
    }
};

} // namespace

void addLoadCaseGroupsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<LoadCaseGroupsQuery>());
}

} // namespace oor::query

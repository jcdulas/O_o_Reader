#include "oor/analysis/components/ComponentBoundary.hpp"
#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/LoadCaseGroups.hpp"
#include "oor/analysis/connections/ConnectionSharing.hpp"
#include "oor/analysis/connections/SpiderAbsorption.hpp"
#include "oor/analysis/connections/Spiders.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ContactArguments.hpp"
#include "oor/query/named/common/UnitGroups.hpp"
#include "oor/query/named/topology/t4_components/ComponentQueries.hpp"

#include <optional>

namespace oor::query
{

namespace
{

// componentBoundaries: the nodes on which each component is reduced.
class ComponentBoundariesQuery final : public QueryBase
{
public:
    ComponentBoundariesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::string detail = run.arguments.textOr("detail", "nodes");
        if (detail != "nodes" && detail != "summary")
            throw QueryError("componentBoundaries: detail is \"nodes\" or \"summary\", not \"" + detail + "\"");
        const model::Model& model = *run.model;
        const auto components = analysis::ComponentModel::of(
            model, contactOptionsOf(run.arguments, "componentBoundaries"), run.context.pool());
        const bool byGroup = run.arguments.has("groups");
        const bool withSpiders = run.arguments.boolOr("spiders", false);
        std::optional<UnitGroups> groups;
        if (byGroup)
            groups.emplace(model, run.arguments.groups("groups"), "componentBoundaries",
                           contactOptionsOf(run.arguments, "componentBoundaries"), run.context.pool());

        analysis::BoundaryOptions options;
        if (const auto ids = run.arguments.ids("components"); ids && !byGroup)
        {
            for (const std::int64_t id : *ids)
                options.components.push_back(static_cast<std::int32_t>(id));
        }
        if (const auto ids = run.arguments.ids("subcases"))
            options.subcases = *ids;
        if (const auto ids = run.arguments.ids("loadGroups"))
        {
            for (const std::int64_t id : *ids)
                options.loadGroups.push_back(static_cast<std::int32_t>(id));
        }
        std::optional<analysis::LoadCaseGroups> loadGroups;
        if (run.arguments.isText("retain"))
        {
            const std::string retain = run.arguments.textOr("retain", "");
            if (retain != "loadNodes")
                throw QueryError("componentBoundaries: retain is None, a list of nodes or \"loadNodes\", not \"" +
                                 retain + "\"");
            options.retain = analysis::RetainMode::LoadNodes;
            if (byGroup)
                loadGroups = analysis::LoadCaseGroups::compute(model, groups->groups());
            else
                loadGroups = analysis::LoadCaseGroups::compute(model, *components);
        }
        else if (const auto ids = run.arguments.ids("retain"))
        {
            options.retain = analysis::RetainMode::Nodes;
            for (const std::int64_t id : *ids)
                options.retainedNodes.push_back(static_cast<base::EntityId>(id));
        }
        std::vector<analysis::Boundary> boundaries;
        if (byGroup)
        {
            const std::size_t count = groups->groups().unitCount();
            std::vector<std::vector<analysis::BoundaryNode>> interfaces(count);
            std::vector<std::vector<base::EntityId>> unitNodes(count);
            for (std::size_t index = 0; index < count; ++index)
            {
                const auto unit = static_cast<std::int32_t>(index + 1);
                interfaces[index] = groups->groups().interfaceOf(unit);
                unitNodes[index] = groups->groups().group(unit).nodes;
            }
            boundaries = analysis::ComponentBoundaries::compute(model, interfaces, unitNodes,
                                                                loadGroups ? &*loadGroups : nullptr, options);
        }
        else
            boundaries = analysis::ComponentBoundaries::compute(model, *components, loadGroups ? &*loadGroups : nullptr,
                                                                options);
        if (withSpiders)
        {
            const analysis::ConnectionSharing sharing = analysis::ConnectionSharing::build(*components);
            const analysis::ComponentMembership membership(*components);
            const analysis::UnitMembership& units =
                byGroup ? static_cast<const analysis::UnitMembership&>(groups->groups()) : membership;
            const std::vector<analysis::Spider> spiders =
                analysis::Spiders::find(model, *components, sharing, units, run.context.pool());
            analysis::SpiderAbsorption::HeldTest isHeld;
            if (byGroup)
            {
                isHeld = [&](std::int32_t unit, const analysis::Spider& spider)
                {
                    const auto entity = groups->entities().indexOf(spider.card);
                    return entity && groups->groups().group(unit).isMember(static_cast<std::uint32_t>(*entity));
                };
            }
            analysis::SpiderAbsorption::apply(boundaries, spiders, sharing, *components, isHeld);
        }
        // The key of the rows: the component, or the name of the group.
        const auto keyColumn = [&](TableBuilder& builder, const std::vector<std::int64_t>& units)
        {
            if (!byGroup)
            {
                builder.int64("componentId", std::vector<std::int64_t>(units));
                return;
            }
            std::vector<std::string> names;
            for (const std::int64_t unit : units)
                names.push_back(groups->nameOf(static_cast<std::int32_t>(unit)));
            builder.text("group", names);
        };

        TableBuilder builder;
        if (detail == "summary")
        {
            std::vector<std::int64_t> component;
            std::vector<std::int64_t> group;
            std::vector<std::uint8_t> groupValid;
            std::vector<std::int64_t> interfaceCount;
            std::vector<std::int64_t> retainedCount;
            std::vector<std::int64_t> boundaryCount;
            std::vector<std::int64_t> offsets{0};
            std::vector<std::int32_t> subcases;
            for (const analysis::Boundary& boundary : boundaries)
            {
                component.push_back(boundary.component);
                group.push_back(boundary.loadGroup.value_or(0));
                groupValid.push_back(boundary.loadGroup ? 1 : 0);
                interfaceCount.push_back(static_cast<std::int64_t>(boundary.interfaceCount()));
                retainedCount.push_back(static_cast<std::int64_t>(boundary.retainedCount()));
                boundaryCount.push_back(static_cast<std::int64_t>(boundary.nodes.size()));
                for (const std::int64_t subcase : boundary.subcases)
                    subcases.push_back(static_cast<std::int32_t>(subcase));
                offsets.push_back(static_cast<std::int64_t>(subcases.size()));
            }
            keyColumn(builder, component);
            builder.int64("loadGroup", std::move(group), std::move(groupValid));
            builder.int64("interfaceNodeCount", std::move(interfaceCount));
            builder.int64("retainedNodeCount", std::move(retainedCount));
            builder.int64("boundaryNodeCount", std::move(boundaryCount));
            builder.intList("subcases", std::move(offsets), std::move(subcases));
            return builder.finish();
        }
        std::vector<std::int64_t> component;
        std::vector<std::int64_t> group;
        std::vector<std::uint8_t> groupValid;
        std::vector<std::int32_t> nid;
        std::vector<std::uint8_t> isInterface;
        std::vector<std::uint8_t> isRetained;
        std::vector<std::int64_t> connectionCount;
        std::vector<std::int64_t> offsets{0};
        std::vector<std::int32_t> connections;
        std::vector<std::int32_t> spiderElement;
        std::vector<std::uint8_t> spiderValid;
        std::vector<std::int64_t> legOffsets{0};
        std::vector<std::int32_t> legs;
        for (const analysis::Boundary& boundary : boundaries)
        {
            for (const analysis::BoundaryNode& node : boundary.nodes)
            {
                spiderElement.push_back(node.spiderElement);
                spiderValid.push_back(node.spiderElement != 0 ? 1 : 0);
                legs.insert(legs.end(), node.replacedLegs.begin(), node.replacedLegs.end());
                legOffsets.push_back(static_cast<std::int64_t>(legs.size()));
                component.push_back(boundary.component);
                group.push_back(boundary.loadGroup.value_or(0));
                groupValid.push_back(boundary.loadGroup ? 1 : 0);
                nid.push_back(node.node);
                isInterface.push_back(node.isInterface ? 1 : 0);
                isRetained.push_back(node.isRetained ? 1 : 0);
                connectionCount.push_back(static_cast<std::int64_t>(node.connectionCount));
                connections.insert(connections.end(), node.connectionElements.begin(), node.connectionElements.end());
                offsets.push_back(static_cast<std::int64_t>(connections.size()));
            }
        }
        keyColumn(builder, component);
        builder.int64("loadGroup", std::move(group), std::move(groupValid));
        builder.int32("nodeId", std::move(nid));
        builder.flag("isInterface", isInterface);
        builder.flag("isRetained", isRetained);
        builder.int64("connectionCount", std::move(connectionCount));
        builder.intList("connections", std::move(offsets), std::move(connections));
        if (withSpiders)
        {
            builder.int32("spiderElementId", std::move(spiderElement), std::move(spiderValid));
            builder.intList("replacedLegs", std::move(legOffsets), std::move(legs));
        }
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "componentBoundaries";
        info.family = "T4";
        info.description =
            "The nodes on which each component is reduced. retain=None (the default, static analyses): the interface "
            "nodes "
            "alone, the nodes of the component that connections (connectors, rigid elements, MPC equations, bridging "
            "1D "
            "elements) use; one boundary per component. retain=<nodes>: the interface nodes plus those of these nodes "
            "that "
            "belong to the component. retain=\"loadNodes\": the interface nodes plus the load application nodes, one "
            "boundary "
            "per component and load case group (subcases and loadGroups restrict them; see loadCaseGroups). Nodes "
            "that "
            "are only constrained or only requested for output are never in a boundary. connectionCount counts the "
            "connections at the node, MPC equations included; connections lists the elements among them. A boundary "
            "does not depend on the content of the neighbouring components. detail=\"summary\": one row per boundary "
            "with "
            "the counts and the load cases it serves. groups={name: {\"components\": IDs}} or {name: {\"elements\": "
            "IDs}} gives the boundaries of groups instead: the rows are keyed by `group` in "
            "place of componentId, the boundary is the grid points of the group that something outside it acts on "
            "(groupRevisions says what a group holds), and connections lists the outside elements there. "
            "spiders=True absorbs the eligible spiders (spiders): their legs leave the boundary and the reference "
            "point stands for them, with two more columns, spiderElementId and replacedLegs.";
        info.detailParameter = "detail";
        info.parameters = {
            parameter("components", ParameterKind::IdList, "None", "keep these components"),
            parameter("retain", ParameterKind::IdListOrText, "None", "None, nodes to retain, or \"loadNodes\""),
            parameter("subcases", ParameterKind::IdList, "None",
                      "with retain=\"loadNodes\": keep the groups of these load cases"),
            parameter("loadGroups", ParameterKind::IdList, "None", "with retain=\"loadNodes\": keep these groups"),
            parameter("groups", ParameterKind::Groups, "None",
                      "{name: {\"components\": IDs}} or {name: {\"elements\": IDs}}: the boundaries of groups"),
            parameter("spiders", ParameterKind::Bool, "False", "absorb the eligible spiders into their unit"),
            parameter("detail", ParameterKind::Text, "None", "None or \"summary\"")};
        const std::vector<ParameterInfo> contact = contactParameters();
        info.parameters.insert(info.parameters.end() - 1, contact.begin(), contact.end()); // before `detail`
        info.defaultSort = "componentId, loadGroup, nodeId";
        ColumnListBuilder nodes;
        nodes.int64("componentId").int64("loadGroup").int32("nodeId").flag("isInterface").flag("isRetained");
        nodes.int64("connectionCount").intList("connections");
        info.columnSets.push_back(nodes.build());
        ColumnListBuilder summary;
        summary.int64("componentId").int64("loadGroup").int64("interfaceNodeCount").int64("retainedNodeCount");
        summary.int64("boundaryNodeCount").intList("subcases");
        info.columnSets.push_back(summary.build("summary"));
        return info;
    }
};

} // namespace

void addComponentBoundariesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ComponentBoundariesQuery>());
}

} // namespace oor::query

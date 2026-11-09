#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/ConstraintStamps.hpp"
#include "oor/analysis/components/UnitMembership.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ContactArguments.hpp"
#include "oor/query/named/common/UnitGroups.hpp"
#include "oor/query/named/topology/t4_components/ComponentQueries.hpp"

#include <cstring>
#include <initializer_list>
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

// constraintStamps: the constraint stamp of each component (or group) and SPC set.
class ConstraintStampsQuery final : public QueryBase
{
public:
    ConstraintStampsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const bool byGroup = run.arguments.has("groups");
        std::optional<UnitGroups> groups;
        std::shared_ptr<const analysis::ComponentModel> components;
        std::optional<analysis::ComponentMembership> membership;
        const analysis::UnitMembership* units = nullptr;
        std::vector<std::int32_t> wanted;
        if (byGroup)
        {
            groups.emplace(model, run.arguments.groups("groups"), "constraintStamps",
                           contactOptionsOf(run.arguments, "constraintStamps"), run.context.pool());
            units = &groups->groups();
        }
        else
        {
            components = analysis::ComponentModel::of(model, contactOptionsOf(run.arguments, "constraintStamps"),
                                                      run.context.pool());
            membership.emplace(*components);
            units = &*membership;
            if (const auto ids = run.arguments.ids("components"))
            {
                for (const std::int64_t id : *ids)
                    wanted.push_back(static_cast<std::int32_t>(id));
            }
        }
        std::vector<base::EntityId> sets;
        if (const auto ids = run.arguments.ids("spcSets"))
        {
            for (const std::int64_t id : *ids)
                sets.push_back(static_cast<base::EntityId>(id));
        }
        const std::vector<analysis::ConstraintStamp> stamps =
            analysis::ConstraintStamps::compute(model, *units, sets, wanted);
        if (stamps.empty())
            return emptyTable(info().columnSets[byGroup ? 1 : 0]);
        std::vector<std::int64_t> component;
        std::vector<std::string> group;
        std::vector<std::int32_t> set;
        std::vector<std::uint8_t> setValid;
        std::vector<std::int64_t> constrained;
        std::vector<std::int64_t> revision;
        for (const analysis::ConstraintStamp& stamp : stamps)
        {
            component.push_back(stamp.unit);
            if (byGroup)
                group.push_back(groups->nameOf(stamp.unit));
            set.push_back(stamp.spcSet.value_or(0));
            setValid.push_back(stamp.spcSet ? 1 : 0);
            constrained.push_back(static_cast<std::int64_t>(stamp.constrainedNodeCount));
            revision.push_back(asInt64(stamp.revision));
        }
        TableBuilder builder;
        if (byGroup)
            builder.text("group", group);
        else
            builder.int64("componentId", std::move(component));
        builder.int32("spcSet", std::move(set), std::move(setValid));
        builder.int64("constrainedNodeCount", std::move(constrained));
        builder.int64("constraintRevision", std::move(revision));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "constraintStamps";
        info.family = "T4";
        info.description =
            "The constraint stamp of each component, or of each group with `groups` (rows keyed by `group`), for each "
            "SPC set: a content hash (64 bits, as a signed int64) of the constraints on its nodes, for caching a "
            "reduction: the "
            "components and enforced values the SPC and SPC1 cards of the set give (SPCADD expanded), the enforced "
            "motions of the SPCD cards and the permanent constraints of the grid points (GRID PS). A reduction made "
            "with the constraints applied before reducing stays valid while the stamp is the same; an edit of a "
            "constraint on other nodes leaves it alone. spcSets: the sets asked for; by default every set an SPC, "
            "SPC1 or SPCADD card defines or a subcase selects (a model with none gives one row per unit with a null "
            "spcSet: the permanent constraints and enforced motions alone). constrainedNodeCount counts the nodes "
            "of the unit that have an entry.";
        info.parameters = {parameter("components", ParameterKind::IdList, "None", "keep these components"),
                           parameter("groups", ParameterKind::Groups, "None",
                                     "{name: {\"components\": IDs}} or {name: {\"elements\": IDs}}: stamps of groups"),
                           parameter("spcSets", ParameterKind::IdList, "None", "the SPC sets")};
        const std::vector<ParameterInfo> contact = contactParameters();
        info.parameters.insert(info.parameters.end(), contact.begin(), contact.end());
        info.defaultSort = "componentId (or the order of the groups), spcSet";
        for (const bool byGroup : {false, true})
        {
            ColumnListBuilder columns;
            if (byGroup)
                columns.text("group");
            else
                columns.int64("componentId");
            columns.int32("spcSet").int64("constrainedNodeCount").int64("constraintRevision");
            info.columnSets.push_back(byGroup ? columns.build("groups") : columns.build());
        }
        return info;
    }
};

} // namespace

void addConstraintStampsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ConstraintStampsQuery>());
}

} // namespace oor::query

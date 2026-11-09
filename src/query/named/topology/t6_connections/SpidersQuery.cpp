#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/analysis/components/EntityDofs.hpp"
#include "oor/analysis/components/UnitMembership.hpp"
#include "oor/analysis/connections/ConnectionSharing.hpp"
#include "oor/analysis/connections/Spiders.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ContactArguments.hpp"
#include "oor/query/named/common/UnitGroups.hpp"
#include "oor/query/named/topology/t6_connections/ConnectionQueries.hpp"

#include <algorithm>
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

std::string componentsText(std::int32_t digits)
{
    return digits != 0 ? std::to_string(digits) : std::string();
}

// spiders: the RBE2 and RBE3 whose legs are all on one component or group.
class SpidersQuery final : public QueryBase
{
public:
    SpidersQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const auto components =
            analysis::ComponentModel::of(model, contactOptionsOf(run.arguments, "spiders"), run.context.pool());
        const analysis::ConnectionSharing sharing = analysis::ConnectionSharing::build(*components);
        const bool byGroup = run.arguments.has("groups");
        std::optional<UnitGroups> groups;
        std::optional<analysis::ComponentMembership> membership;
        const analysis::UnitMembership* units = nullptr;
        if (byGroup)
        {
            groups.emplace(model, run.arguments.groups("groups"), "spiders", contactOptionsOf(run.arguments, "spiders"),
                           run.context.pool());
            units = &groups->groups();
        }
        else
        {
            membership.emplace(*components);
            units = &*membership;
        }
        const std::vector<analysis::Spider> spiders =
            analysis::Spiders::find(model, *components, sharing, *units, run.context.pool());
        std::vector<std::int64_t> wanted;
        if (const auto ids = run.arguments.ids("components"); ids && !byGroup)
            wanted = *ids;

        std::vector<std::int64_t> component;
        std::vector<std::string> group;
        std::vector<std::uint8_t> unitValid;
        std::vector<std::string> card;
        std::vector<std::int32_t> eid;
        std::vector<std::int32_t> reference;
        std::vector<std::string> referenceDofs;
        std::vector<std::int64_t> legOffsets{0};
        std::vector<std::int32_t> legNodes;
        std::vector<std::string> legDofs;
        std::vector<double> legWeights;
        std::vector<std::int64_t> weightOffsets{0};
        std::vector<std::uint8_t> weightValid;
        std::vector<std::uint8_t> isSpider;
        std::vector<std::string> reason;
        std::vector<std::uint8_t> reasonValid;
        std::vector<std::uint8_t> eligible;
        std::vector<std::int64_t> blockingNodeOffsets{0};
        std::vector<std::int32_t> blockingNodes;
        std::vector<std::int64_t> blockingOffsets{0};
        std::vector<std::int32_t> blocking;
        std::vector<std::int64_t> revision;
        for (const analysis::Spider& spider : spiders)
        {
            if (!wanted.empty() && std::find(wanted.begin(), wanted.end(), spider.unit) == wanted.end())
                continue;
            component.push_back(spider.unit);
            group.push_back(byGroup && spider.unit != 0 ? groups->nameOf(spider.unit) : std::string());
            unitValid.push_back(spider.unit != 0 ? 1 : 0);
            card.emplace_back(spider.isInterpolation ? "RigidInterpolation" : "RigidBodyStar");
            eid.push_back(spider.eid);
            reference.push_back(spider.reference);
            referenceDofs.push_back(componentsText(spider.referenceComponents));
            for (const analysis::SpiderLeg& leg : spider.legs)
            {
                legNodes.push_back(leg.node);
                legDofs.push_back(componentsText(leg.components));
                if (spider.isInterpolation)
                    legWeights.push_back(leg.weight);
            }
            legOffsets.push_back(static_cast<std::int64_t>(legNodes.size()));
            weightOffsets.push_back(static_cast<std::int64_t>(legWeights.size()));
            weightValid.push_back(spider.isInterpolation ? 1 : 0);
            isSpider.push_back(spider.isSpider ? 1 : 0);
            reason.emplace_back(analysis::spiderReasonName(spider.reason));
            reasonValid.push_back(spider.isSpider ? 0 : 1);
            eligible.push_back(spider.isEligible ? 1 : 0);
            blockingNodes.insert(blockingNodes.end(), spider.blockingNodes.begin(), spider.blockingNodes.end());
            blockingNodeOffsets.push_back(static_cast<std::int64_t>(blockingNodes.size()));
            std::vector<std::int32_t> others;
            for (const std::uint32_t connection : spider.blockingConnections)
                others.push_back(components->connections()[connection].id);
            std::sort(others.begin(), others.end());
            others.erase(std::unique(others.begin(), others.end()), others.end());
            blocking.insert(blocking.end(), others.begin(), others.end());
            blockingOffsets.push_back(static_cast<std::int64_t>(blocking.size()));
            revision.push_back(asInt64(spider.revision));
        }
        if (eid.empty())
            return emptyTable(info().columnSets[byGroup ? 1 : 0]);
        TableBuilder builder;
        if (byGroup)
            builder.text("group", group, unitValid);
        else
            builder.int64("componentId", std::move(component), unitValid);
        builder.dictionary("card", card);
        builder.int32("elementId", std::move(eid));
        builder.int32("referenceNodeId", std::move(reference));
        builder.text("referenceDofs", referenceDofs);
        builder.intList("legNodeIds", legOffsets, std::move(legNodes));
        builder.textList("legDofs", std::move(legOffsets), legDofs);
        builder.realList("legWeights", std::move(weightOffsets), std::move(legWeights), std::move(weightValid));
        builder.flag("isSpider", isSpider);
        builder.dictionary("reason", reason, std::move(reasonValid));
        builder.flag("isEligible", eligible);
        builder.intList("blockingNodeIds", std::move(blockingNodeOffsets), std::move(blockingNodes));
        builder.intList("blockingConnections", std::move(blockingOffsets), std::move(blocking));
        builder.int64("revision", std::move(revision));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "spiders";
        info.family = "T6";
        info.description =
            "The RBE2 and RBE3 as spiders of a unit: a spider has all its legs (the Gi of an "
            "RBE3, the dependent points of an RBE2) on one component, or on one group with `groups` (rows keyed by "
            "`group`; a card whose legs are in two overlapping groups has a row for each). referenceNodeId is GN of an "
            "RBE2 (referenceDofs 123456) or REFGRID of an RBE3 (REFC); legNodeIds, legDofs and legWeights (null for "
            "an RBE2) as the card gives them. A card that is not a spider says why: um (an RBE3 with UM), "
            "referenceIsLeg, referenceDependentElsewhere (the reference point is a dependent point of another "
            "rigid element or the first term of an MPC equation), legsOnSeveralUnits (the legs are on several "
            "units, or one on none: the unit is then null). isEligible: a spider no leg of which another connection "
            "touches, which can be absorbed into its unit (componentBoundaries(spiders=True)); blockingNodeIds and "
            "blockingConnections say what touches them (an MPC equation by its set ID). revision is the revision "
            "stamp of the card and of what it refers to (the grid points and their systems), as a signed int64.";
        info.parameters = {
            parameter("components", ParameterKind::IdList, "None", "keep the spiders of these components"),
            parameter("groups", ParameterKind::Groups, "None",
                      "{name: {\"components\": IDs}} or {name: {\"elements\": IDs}}: spiders of groups")};
        const std::vector<ParameterInfo> contact = contactParameters();
        info.parameters.insert(info.parameters.end(), contact.begin(), contact.end());
        info.defaultSort = "elementId";
        for (const bool byGroup : {false, true})
        {
            ColumnListBuilder columns;
            if (byGroup)
                columns.text("group");
            else
                columns.int64("componentId");
            columns.dictionary("card")
                .int32("elementId")
                .int32("referenceNodeId")
                .text("referenceDofs")
                .intList("legNodeIds");
            columns.textList("legDofs").realList("legWeights").flag("isSpider").dictionary("reason");
            columns.flag("isEligible").intList("blockingNodeIds").intList("blockingConnections").int64("revision");
            info.columnSets.push_back(byGroup ? columns.build("groups") : columns.build());
        }
        return info;
    }
};

} // namespace

void addSpidersQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<SpidersQuery>());
}

} // namespace oor::query

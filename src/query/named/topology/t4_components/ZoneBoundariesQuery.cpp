#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/analysis/components/Zones.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/UnitGroups.hpp"
#include "oor/query/named/topology/t4_components/ComponentQueries.hpp"

namespace oor::query
{

namespace
{

// zoneBoundaries: the boundary points of each zone, with what acts on them from outside.
class ZoneBoundariesQuery final : public QueryBase
{
public:
    ZoneBoundariesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const UnitGroups groups(model, run.arguments.groups("groups"), "zoneBoundaries", analysis::ContactOptions{},
                                run.context.pool());
        const auto piercings = analysis::FastenerPiercings::of(model, run.context.pool());
        std::vector<std::string> name;
        std::vector<std::int32_t> nid;
        std::vector<std::string> dof;
        std::vector<std::uint8_t> piercedPoint;
        std::vector<std::int64_t> offsets{0};
        std::vector<std::string> kinds;
        std::vector<std::string> cards;
        std::vector<std::int32_t> ids;
        for (std::size_t index = 0; index < groups.groups().unitCount(); ++index)
        {
            const auto unit = static_cast<std::int32_t>(index + 1);
            for (const analysis::ZoneBoundaryPoint& point : analysis::Zones::boundaryOf(
                     model, groups.groups(), unit, groups.entities(), groups.index(), piercings.get()))
            {
                name.push_back(groups.nameOf(unit));
                nid.push_back(point.node);
                dof.push_back(analysis::EntityDofs::text(point.components));
                piercedPoint.push_back(point.isPiercedPoint ? 1 : 0);
                for (const analysis::ZoneCause& cause : point.causes)
                {
                    const analysis::ModelEntity& entity = groups.entities().entity(cause.entity);
                    kinds.emplace_back(analysis::causeKindName(cause.kind));
                    cards.emplace_back(model.findTable(entity.card.table)->info().name);
                    ids.push_back(entity.id);
                }
                offsets.push_back(static_cast<std::int64_t>(ids.size()));
            }
        }
        if (nid.empty())
            return emptyTable(info().defaultColumns());
        TableBuilder builder;
        builder.text("group", name);
        builder.int32("nodeId", std::move(nid));
        builder.text("dof", dof);
        builder.flag("isPiercedPoint", piercedPoint);
        builder.textList("causeKinds", offsets, kinds);
        builder.textList("causeCards", offsets, cards);
        builder.intList("causeIds", std::move(offsets), std::move(ids));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "zoneBoundaries";
        info.family = "T4";
        info.description =
            "The boundary points of each group used as a zone (a group given as "
            "{name: {\"elements\": IDs}}, connections included): its grid points that something outside it acts on, "
            "an element next to it, a rigid element or an MPC equation it does not hold, a connector, and for a CFAST "
            "or CWELD whose other side is outside, the grid points of the shell it pierces (isPiercedPoint). The "
            "zone holds the rigid elements, MPC equations and elements whose grid points are all in it "
            "(enclosedElements). dof: the components through which the outside acts on the point, as text: the union "
            "of those of the outside entities there (a shell 123456, a solid 123, a CBUSH the components its PBUSH "
            "gives a stiffness for, a fastener 123 on the grid points of the shell it pierces, a rigid element or an "
            "MPC equation the components it names). causeKinds (element, rigid, mpc, connection), causeCards and "
            "causeIds are aligned: one item per outside entity (an MPC equation by its set ID).";
        info.parameters = {parameter("groups", ParameterKind::Groups, "(required)",
                                     "{name: {\"elements\": IDs}} (or {name: {\"components\": IDs}})")};
        info.defaultSort = "the order of the groups given, nodeId";
        ColumnListBuilder columns;
        columns.text("group").int32("nodeId").text("dof").flag("isPiercedPoint").textList("causeKinds");
        columns.textList("causeCards").intList("causeIds");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addZoneBoundariesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ZoneBoundariesQuery>());
}

} // namespace oor::query

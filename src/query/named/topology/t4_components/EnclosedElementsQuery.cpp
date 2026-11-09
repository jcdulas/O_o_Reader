#include "oor/analysis/components/Zones.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/UnitGroups.hpp"
#include "oor/query/named/topology/t4_components/ComponentQueries.hpp"

namespace oor::query
{

namespace
{

// enclosedElements: the elements each zone holds without listing them.
class EnclosedElementsQuery final : public QueryBase
{
public:
    EnclosedElementsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        const UnitGroups groups(model, run.arguments.groups("groups"), "enclosedElements", analysis::ContactOptions{},
                                run.context.pool());
        std::vector<std::string> name;
        std::vector<std::int32_t> eid;
        std::vector<std::string> type;
        std::vector<std::uint8_t> point;
        for (std::size_t index = 0; index < groups.groups().unitCount(); ++index)
        {
            const auto unit = static_cast<std::int32_t>(index + 1);
            for (const analysis::EnclosedElement& element :
                 analysis::Zones::enclosedBy(groups.groups(), unit, groups.entities()))
            {
                const analysis::ModelEntity& entity = groups.entities().entity(element.entity);
                name.push_back(groups.nameOf(unit));
                eid.push_back(entity.id);
                type.emplace_back(model.findTable(entity.card.table)->info().name);
                point.push_back(element.isPointElement ? 1 : 0);
            }
        }
        if (eid.empty())
            return emptyTable(info().defaultColumns());
        TableBuilder builder;
        builder.text("group", name);
        builder.int32("elementId", std::move(eid));
        builder.dictionary("type", type);
        builder.flag("isPointElement", point);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "enclosedElements";
        info.family = "T4";
        info.description =
            "The elements each group encloses (zones): outside the list the caller gave, all "
            "their grid points in the group (a rigid element or a connector inside a zone, a mass on one of its "
            "points, a connection between two of its components). The group holds them (groupRevisions). "
            "isPointElement: an element whose grid points are one point (a concentrated mass, a grounded spring). "
            "By group, then element type and row.";
        info.parameters = {parameter("groups", ParameterKind::Groups, "(required)",
                                     "{name: {\"elements\": IDs}} (or {name: {\"components\": IDs}})")};
        info.defaultSort = "the order of the groups given, then the card types";
        ColumnListBuilder columns;
        columns.text("group").int32("elementId").dictionary("type").flag("isPointElement");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addEnclosedElementsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<EnclosedElementsQuery>());
}

} // namespace oor::query

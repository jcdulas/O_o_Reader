#include "DesignQuerySupport.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/topology/t9_design/DesignQueries.hpp"

#include <algorithm>
#include <cmath>

namespace oor::query
{

namespace
{

void addFinite(double& sum, double value)
{
    if (std::isfinite(value))
        sum += value;
}

// designVariableElements: what each design variable drives.
class DesignVariableElementsQuery final : public QueryBase
{
public:
    DesignVariableElementsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::string detail = run.arguments.textOr("detail", "variables");
        if (detail != "variables" && detail != "elements")
            throw QueryError("detail is \"variables\" or \"elements\", not '" + detail + "'");
        const model::DesignModel design = model::DesignModel::read(*run.model);
        const std::optional<std::vector<std::int64_t>> wanted = sortedIds(run.arguments.ids("ids"));
        std::vector<std::size_t> chosen;
        for (std::size_t index = 0; index < design.variables().size(); ++index)
        {
            const auto id = static_cast<std::int64_t>(design.variables()[index].id);
            if (!wanted || std::binary_search(wanted->begin(), wanted->end(), id))
                chosen.push_back(index);
        }
        if (chosen.empty())
            return emptyTable(info().columnsFor(detail));
        DrivenCards driven(run.model, design, run.context);
        const std::vector<std::vector<std::size_t>> moved = relationsMovedBy(design);
        return detail == "variables" ? variables(run, design, driven, moved, chosen)
                                     : elements(run, design, driven, moved, chosen);
    }

private:
    [[nodiscard]] std::shared_ptr<const QueryTable> variables(const QueryRun& run, const model::DesignModel& design,
                                                              DrivenCards& driven,
                                                              const std::vector<std::vector<std::size_t>>& moved,
                                                              const std::vector<std::size_t>& chosen) const
    {
        (void)run;
        const std::size_t count = chosen.size();
        std::vector<std::int32_t> id(count);
        std::vector<std::string> label(count);
        std::vector<std::int64_t> propertyCount(count), elementCount(count);
        std::vector<double> area(count, 0.0), volume(count, 0.0), mass(count, 0.0);
        std::vector<std::int64_t> offsets{0};
        std::vector<std::int32_t> propertyIds;
        const ElementMassData& masses = driven.data().masses();
        const std::vector<double>& areas = driven.data().areas();
        const std::vector<double>& volumes = driven.data().volumes();
        for (std::size_t row = 0; row < count; ++row)
        {
            const model::DesignVariable& variable = design.variables()[chosen[row]];
            id[row] = variable.id;
            label[row] = variable.label;
            const RelationDrive drive = driven.of(moved[chosen[row]]);
            propertyCount[row] = static_cast<std::int64_t>(drive.properties.size());
            elementCount[row] = static_cast<std::int64_t>(drive.elements.size());
            for (const std::size_t element : drive.elements)
            {
                addFinite(area[row], areas[element]);
                addFinite(volume[row], volumes[element]);
                addFinite(mass[row], masses.mass[element]);
            }
            propertyIds.insert(propertyIds.end(), drive.properties.begin(), drive.properties.end());
            offsets.push_back(static_cast<std::int64_t>(propertyIds.size()));
        }
        TableBuilder builder;
        builder.int32("designVariable", std::move(id));
        builder.text("label", label);
        builder.int64("propertyCount", std::move(propertyCount));
        builder.int64("elementCount", std::move(elementCount));
        builder.real("area", std::move(area));
        builder.real("volume", std::move(volume));
        builder.real("mass", std::move(mass));
        builder.intList("propertyIds", std::move(offsets), std::move(propertyIds));
        return builder.finish();
    }

    [[nodiscard]] std::shared_ptr<const QueryTable> elements(const QueryRun& run, const model::DesignModel& design,
                                                             DrivenCards& driven,
                                                             const std::vector<std::vector<std::size_t>>& moved,
                                                             const std::vector<std::size_t>& chosen) const
    {
        (void)run;
        std::vector<std::int32_t> variable, relationId, targetId, eid;
        std::vector<std::string> targetCard, type;
        const CardList& elements = driven.elements();
        for (const std::size_t index : chosen)
        {
            for (const std::size_t relationIndex : moved[index])
            {
                const model::DesignRelation& relation = design.relations()[relationIndex];
                for (const std::size_t element : driven.of(relationIndex).elements)
                {
                    variable.push_back(design.variables()[index].id);
                    relationId.push_back(relation.id);
                    targetCard.push_back(relation.targetCard);
                    targetId.push_back(relation.targetId);
                    eid.push_back(elements.ids[element]);
                    type.emplace_back(typeName(elements.types[element]));
                }
            }
        }
        if (variable.empty())
            return emptyTable(info().columnsFor("elements"));
        TableBuilder builder;
        builder.int32("designVariable", std::move(variable));
        builder.int32("relationId", std::move(relationId));
        builder.dictionary("targetCard", targetCard);
        builder.int32("targetId", std::move(targetId));
        builder.int32("elementId", std::move(eid));
        builder.dictionary("type", type);
        return builder.finish();
    }

    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "designVariableElements";
        info.family = "T9";
        info.description =
            "What each design variable drives. `detail=\"variables\"` (the default): the number of properties and "
            "elements, their total area, volume and mass, and the property IDs. `detail=\"elements\"`: one row for "
            "each "
            "element of each relation of the variable. A DVPREL drives the elements of its property, a DVMREL the "
            "elements of the properties that use its material, a DVCREL its element; the variables a DLINK makes "
            "dependent on a variable are driven by it too. The totals count each element once and leave out an element "
            "whose area, volume or mass cannot be computed.";
        info.parameters = {parameter("ids", ParameterKind::IdList, "None", "keep these design variables"),
                           parameter("detail", ParameterKind::Text, "\"variables\"", "\"variables\" or \"elements\"")};
        info.detailParameter = "detail";
        info.defaultSort = "designVariable";
        ColumnListBuilder variables;
        variables.int32("designVariable").text("label").int64("propertyCount").int64("elementCount");
        variables.reals("area volume mass").intList("propertyIds");
        info.columnSets.push_back(variables.build("variables"));
        ColumnListBuilder elements;
        elements.int32("designVariable").int32("relationId").dictionary("targetCard").int32("targetId");
        elements.int32("elementId").dictionary("type");
        info.columnSets.push_back(elements.build("elements"));
        return info;
    }
};

} // namespace

void addDesignVariableElementsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<DesignVariableElementsQuery>());
}

} // namespace oor::query

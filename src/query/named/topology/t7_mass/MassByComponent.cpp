#include "oor/analysis/components/ComponentModel.hpp"
#include "oor/query/named/common/MassGroups.hpp"
#include "oor/query/named/topology/t7_mass/MassQueries.hpp"

#include <map>

namespace oor::query
{

namespace
{

// massByComponent: the mass of each component.
class MassByComponentQuery final : public QueryBase
{
public:
    MassByComponentQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const CardList elements = listCards(*run.model, typesOfCategory(model::CardCategory::Element));
        const auto components = analysis::ComponentModel::of(*run.model, run.context.pool());
        std::map<std::int32_t, GroupTotals> groups; // 0: the elements that belong to no component
        if (elements.size() > 0)
        {
            ElementData data(run.model, elements, run.context);
            const ElementMassData& masses = data.masses();
            for (std::size_t index = 0; index < elements.size(); ++index)
                addElementMass(groups[components->componentOfElement(elements.location(index))], masses, index, 1.0,
                               true, true);
        }
        std::vector<std::int64_t> keys;
        std::vector<std::uint8_t> valid;
        std::vector<GroupTotals> totals;
        for (const analysis::ComponentSummary& summary : components->components())
        {
            keys.push_back(summary.id);
            valid.push_back(1);
            totals.push_back(std::move(groups[summary.id]));
        }
        if (const auto none = groups.find(0); none != groups.end() && none->second.mass() != 0.0)
        {
            keys.push_back(0);
            valid.push_back(0);
            totals.push_back(std::move(none->second));
        }
        if (totals.empty())
            return emptyTable(info().defaultColumns());
        const std::vector<std::size_t> order = orderByMass(totals);
        std::vector<std::int64_t> orderedKeys;
        std::vector<std::uint8_t> orderedValid;
        for (const std::size_t index : order)
        {
            orderedKeys.push_back(keys[index]);
            orderedValid.push_back(valid[index]);
        }
        TableBuilder builder;
        builder.int64("componentId", std::move(orderedKeys), std::move(orderedValid));
        addMassColumns(builder, totals, order);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "massByComponent";
        info.family = "T7";
        info.description =
            "The mass of each component (see connectedComponents): the number of elements, the structural and "
            "non-structural mass, their sum and the centre of gravity. Every component is listed. Elements that belong "
            "to no "
            "component (connectors with mass, masses on a node that no component uses) are in a row with a null "
            "componentId "
            "when they carry mass.";
        info.defaultSort = "-mass";
        ColumnListBuilder columns;
        columns.int64("componentId").int64("elementCount").reals("structuralMass nonstructuralMass mass cx cy cz");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMassByComponentQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MassByComponentQuery>());
}

} // namespace oor::query

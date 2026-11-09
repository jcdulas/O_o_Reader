#include "oor/query/named/common/MassGroups.hpp"
#include "oor/query/named/topology/t7_mass/MassQueries.hpp"

#include <map>

namespace oor::query
{

namespace
{

// massByType: the mass of the elements of each card type.
class MassByTypeQuery final : public QueryBase
{
public:
    MassByTypeQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const CardList elements = listCards(*run.model, typesOfCategory(model::CardCategory::Element));
        if (elements.size() == 0)
            return emptyTable(info().defaultColumns());
        ElementData data(run.model, elements, run.context);
        const ElementMassData& masses = data.masses();
        std::map<std::string, GroupTotals> groups;
        for (std::size_t index = 0; index < elements.size(); ++index)
            addElementMass(groups[std::string(typeName(elements.types[index]))], masses, index, 1.0, true, true);
        std::vector<std::string> keys;
        std::vector<GroupTotals> totals;
        for (auto& [name, group] : groups)
        {
            keys.push_back(name);
            totals.push_back(std::move(group));
        }
        const std::vector<std::size_t> order = orderByMass(totals);
        std::vector<std::string> orderedKeys;
        for (const std::size_t index : order)
            orderedKeys.push_back(keys[index]);
        TableBuilder builder;
        builder.dictionary("type", orderedKeys);
        addMassColumns(builder, totals, order);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "massByType";
        info.family = "T7";
        info.description =
            "The mass of the elements of each card type (CQUAD4, CONM2, ...): the number of elements, the structural "
            "and "
            "non-structural mass, their sum, and the centre of gravity. An element whose mass cannot be computed is "
            "left out.";
        info.defaultSort = "-mass";
        ColumnListBuilder columns;
        columns.dictionary("type").int64("elementCount").reals("structuralMass nonstructuralMass mass cx cy cz");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMassByTypeQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MassByTypeQuery>());
}

} // namespace oor::query

#include "oor/query/named/common/MassGroups.hpp"
#include "oor/query/named/topology/t7_mass/MassQueries.hpp"

#include <map>

namespace oor::query
{

namespace
{

// massByProperty: the mass of the elements of each property.
class MassByPropertyQuery final : public QueryBase
{
public:
    MassByPropertyQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const CardList elements = listCards(*run.model, typesOfCategory(model::CardCategory::Element));
        if (elements.size() == 0)
            return emptyTable(info().defaultColumns());
        ElementData data(run.model, elements, run.context);
        const std::vector<std::int32_t>& pids = data.properties();
        const ElementMassData& masses = data.masses();
        std::map<std::int32_t, GroupTotals> groups; // 0: the elements with no property
        for (std::size_t index = 0; index < elements.size(); ++index)
            addElementMass(groups[pids[index]], masses, index, 1.0, true, true);
        std::vector<std::int32_t> keys;
        std::vector<GroupTotals> totals;
        std::vector<std::uint8_t> valid;
        for (auto& [pid, group] : groups)
        {
            if (pid == 0 && group.mass() == 0.0)
                continue; // springs and rigid elements: no property, no mass
            keys.push_back(pid);
            valid.push_back(pid != 0 ? 1 : 0);
            totals.push_back(std::move(group));
        }
        const std::vector<std::size_t> order = orderByMass(totals);
        std::vector<std::int32_t> orderedKeys;
        std::vector<std::uint8_t> orderedValid;
        for (const std::size_t index : order)
        {
            orderedKeys.push_back(keys[index]);
            orderedValid.push_back(valid[index]);
        }
        TableBuilder builder;
        builder.int32("propId", std::move(orderedKeys), std::move(orderedValid));
        addMassColumns(builder, totals, order);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "massByProperty";
        info.family = "T7";
        info.description =
            "The mass of the elements of each property: the number of elements, the structural mass (the material), "
            "the "
            "non-structural mass (the NSM fields and the NSM set the Case Control selects) and their sum, and the "
            "centre of "
            "gravity. Elements with no property (concentrated masses) are in a row with a null propId when they carry "
            "mass. An "
            "element whose mass cannot be computed is left out.";
        info.defaultSort = "-mass";
        ColumnListBuilder columns;
        columns.int32("propId").int64("elementCount").reals("structuralMass nonstructuralMass mass cx cy cz");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMassByPropertyQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MassByPropertyQuery>());
}

} // namespace oor::query

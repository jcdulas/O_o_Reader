#include "oor/query/named/common/MassGroups.hpp"
#include "oor/query/named/topology/t7_mass/MassQueries.hpp"

#include <map>

namespace oor::query
{

namespace
{

// massByMaterial: the structural mass carried by each material; the non-structural mass has its own row.
class MassByMaterialQuery final : public QueryBase
{
public:
    MassByMaterialQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const CardList elements = listCards(*run.model, typesOfCategory(model::CardCategory::Element));
        if (elements.size() == 0)
            return emptyTable(info().defaultColumns());
        ElementData data(run.model, elements, run.context);
        std::map<std::int32_t, GroupTotals> groups =
            materialGroups(data); // 0: no material, and the non-structural mass
        std::vector<std::int32_t> keys;
        std::vector<std::uint8_t> valid;
        std::vector<GroupTotals> totals;
        for (auto& [material, group] : groups)
        {
            if (material == 0 && group.mass() == 0.0)
                continue;
            keys.push_back(material);
            valid.push_back(material != 0 ? 1 : 0);
            totals.push_back(std::move(group));
        }
        if (totals.empty())
            return emptyTable(info().defaultColumns());
        const std::vector<std::size_t> order = orderByMass(totals);
        std::vector<std::int32_t> orderedKeys;
        std::vector<std::uint8_t> orderedValid;
        for (const std::size_t index : order)
        {
            orderedKeys.push_back(keys[index]);
            orderedValid.push_back(valid[index]);
        }
        TableBuilder builder;
        builder.int32("matId", std::move(orderedKeys), std::move(orderedValid));
        addMassColumns(builder, totals, order);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "massByMaterial";
        info.family = "T7";
        info.description =
            "The structural mass carried by each material (a laminate shares its mass between the materials of its "
            "plies in "
            "proportion to density times thickness), with the number of elements that use it and the centre of "
            "gravity. The "
            "non-structural mass of all the elements, and the mass of the elements that have no material (concentrated "
            "masses), are in the row with a null mid.";
        info.defaultSort = "-mass";
        ColumnListBuilder columns;
        columns.int32("matId").int64("elementCount").reals("structuralMass nonstructuralMass mass cx cy cz");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMassByMaterialQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MassByMaterialQuery>());
}

} // namespace oor::query

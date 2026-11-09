#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardFacts.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/ColumnHelpers.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/topology/t1_inventory/InventoryQueries.hpp"
#include "oor/query/providers/ModelSource.hpp"

#include <algorithm>
#include <unordered_map>

namespace oor::query
{

namespace
{

CardList subsetOf(const CardList& list, const std::vector<std::size_t>& keep)
{
    CardList subset;
    subset.types.reserve(keep.size());
    subset.rows.reserve(keep.size());
    subset.ids.reserve(keep.size());
    subset.keys.reserve(keep.size());
    for (const std::size_t index : keep)
    {
        subset.types.push_back(list.types[index]);
        subset.rows.push_back(list.rows[index]);
        subset.ids.push_back(list.ids[index]);
        subset.keys.push_back(list.keys[index]);
    }
    return subset;
}

// elementsTable: every element with its property, material, nodes and geometry.
class ElementsTableQuery final : public QueryBase
{
public:
    ElementsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        model::Model& model = *run.model;
        const std::vector<std::string> typeNames = run.arguments.texts("types");
        const std::vector<std::uint16_t> types = typeNames.empty()
                                                     ? typesOfCategory(model::CardCategory::Element)
                                                     : typesNamed(typeNames, model::CardCategory::Element, "types");
        CardList list = listCards(model, types, run.arguments.ids("elementIds"));
        CardFacts facts(model);

        // The property and material filters look at every element first, then the table is made for the ones that stay.
        const auto propertyIds = sortedIds(run.arguments.ids("propertyIds"));
        const auto materialIds = sortedIds(run.arguments.ids("materialIds"));
        if ((propertyIds || materialIds) && list.size() > 0)
        {
            ListColumns all(run.model, list, run.context);
            const std::vector<std::int32_t> pids =
                all.has("propId") ? all.integers("propId") : std::vector<std::int32_t>(list.size(), 0);
            std::vector<std::int32_t> mids;
            if (materialIds)
                mids = mainMaterials(model, facts, list, pids);
            std::vector<std::size_t> keep;
            for (std::size_t index = 0; index < list.size(); ++index)
            {
                if (propertyIds && !std::binary_search(propertyIds->begin(), propertyIds->end(), pids[index]))
                    continue;
                if (materialIds && !std::binary_search(materialIds->begin(), materialIds->end(), mids[index]))
                    continue;
                keep.push_back(index);
            }
            list = subsetOf(list, keep);
        }
        if (list.size() == 0)
            return emptyTable(info().defaultColumns());

        ListColumns columns(run.model, list, run.context);
        const ListOrigin origin = originOf(list);
        // No element of the list has a property (point masses and springs only): the column is all null.
        const bool hasProperty = columns.has("propId");
        const std::vector<std::int32_t> pids =
            hasProperty ? asInt32(columns.get("propId")) : std::vector<std::int32_t>(list.size(), 0);
        const std::vector<std::int32_t> mids = mainMaterials(model, facts, list, pids);
        std::vector<std::uint8_t> midValid(mids.size());
        for (std::size_t index = 0; index < mids.size(); ++index)
            midValid[index] = mids[index] != 0 ? 1 : 0;
        const NodeLists nodes = nodesOfCards(model, list, run.context);
        std::vector<std::int64_t> nodeCount(list.size());
        for (std::size_t index = 0; index < list.size(); ++index)
            nodeCount[index] = nodes.offsets[index + 1] - nodes.offsets[index];
        std::vector<std::string> typeNamesOfRows(list.size());
        for (std::size_t index = 0; index < list.size(); ++index)
            typeNamesOfRows[index] = std::string(typeName(list.types[index]));

        TableBuilder builder;
        builder.column("elementId", QueryColumn::primitive(std::vector<std::int32_t>(list.ids)), ColumnOrigin::Stored,
                       "id");
        builder.dictionary("type", typeNamesOfRows);
        if (hasProperty)
        {
            builder.column("propId", columns.get("propId"), ColumnOrigin::Stored, "propId");
            builder.column("propertyType", columns.get("propType"), ColumnOrigin::Computed, "propType");
        }
        else
        {
            builder.int32("propId", pids, std::vector<std::uint8_t>(list.size(), 0));
            builder.dictionary("propertyType", std::vector<std::string>(list.size()),
                               std::vector<std::uint8_t>(list.size(), 0));
        }
        builder.int32("matId", mids, midValid);
        builder.int64("nodeCount", std::move(nodeCount));
        builder.intList("nodes", std::vector<std::int64_t>(nodes.offsets), std::vector<std::int32_t>(nodes.nodes));
        builder.real("thickness", columns.reals("thickness"));
        builder.real("length", columns.reals("length"));
        builder.real("area", columns.reals("area"));
        builder.real("volume", columns.reals("volume"));
        builder.real("cx", columns.reals("centerX"));
        builder.real("cy", columns.reals("centerY"));
        builder.real("cz", columns.reals("centerZ"));
        builder.real("mass", columns.reals("mass"));
        builder.rows(origin.types, origin.keys, run.link);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "elementsTable";
        info.family = "T1";
        info.description =
            "The elements: type, property and its type, the main material, the nodes, and the geometry and mass. A "
            "geometric "
            "column that does not apply to the type of the element is null (the volume of a shell). The main material "
            "is the "
            "one the element card gives (CONROD), else the first one its property names (MID1 of a PSHELL, else MID2; "
            "the first "
            "ply of a laminate).";
        info.parameters = {
            parameter("elementIds", ParameterKind::IdList, "None", "keep these elements"),
            parameter("types", ParameterKind::TextList, "None", "keep these card types (\"CQUAD4\", ...)"),
            parameter("propertyIds", ParameterKind::IdList, "None", "keep the elements of these properties"),
            parameter("materialIds", ParameterKind::IdList, "None",
                      "keep the elements whose main material is one of these")};
        info.defaultSort = "elementId";
        ColumnListBuilder columns;
        columns.int32("elementId")
            .dictionary("type")
            .int32("propId", true)
            .dictionary("propertyType")
            .int32("matId")
            .int64("nodeCount");
        columns.intList("nodes");
        columns.reals("thickness length area volume cx cy cz mass");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addElementsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ElementsTableQuery>());
}

} // namespace oor::query

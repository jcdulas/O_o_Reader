#include "oor/analysis/components/ElementRoles.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/common/PropertyZoning.hpp"
#include "oor/query/named/topology/t4_components/ComponentQueries.hpp"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace oor::query
{

namespace
{

// propertyZones: the contiguous zones of one property.
class PropertyZonesQuery final : public QueryBase
{
public:
    PropertyZonesQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const std::string detail = run.arguments.textOr("detail", "zones");
        if (detail != "zones" && detail != "elements")
            throw QueryError("propertyZones: detail is \"zones\" or \"elements\", not \"" + detail + "\"");
        const model::Model& model = *run.model;
        std::vector<std::uint16_t> types;
        for (const std::uint16_t type : typesOfCategory(model::CardCategory::Element))
        {
            const analysis::ElementRole role = analysis::roleOfType(type);
            if (role == analysis::ElementRole::Shell || role == analysis::ElementRole::Solid ||
                role == analysis::ElementRole::Line)
                types.push_back(type);
        }
        const CardList all = listCards(model, types, run.arguments.ids("elementIds"));
        ElementData data(run.model, all, run.context);
        const std::size_t total = all.size();
        std::vector<std::int32_t> pidsAll;
        if (total > 0)
            pidsAll = data.properties();
        // The elements that have a property.
        std::vector<std::uint32_t> kept;
        for (std::size_t index = 0; index < total; ++index)
        {
            if (pidsAll[index] != 0)
                kept.push_back(static_cast<std::uint32_t>(index));
        }
        const std::size_t count = kept.size();
        if (count == 0)
            return emptyTable(info().columnsFor(detail == "zones" ? "" : "elements"));
        CardList list;
        for (const std::uint32_t index : kept)
        {
            list.types.push_back(all.types[index]);
            list.rows.push_back(all.rows[index]);
            list.ids.push_back(all.ids[index]);
            list.keys.push_back(all.keys[index]);
        }
        const NodeLists nodes = nodesOfCards(model, list, run.context);
        std::vector<std::int32_t> pids(count);
        for (std::size_t index = 0; index < count; ++index)
            pids[index] = pidsAll[kept[index]];

        const std::vector<std::uint32_t> rootOf = propertyZoneRoots(nodes, pids);
        // Zones by decreasing element count, ties by the smallest element ID (the list is in order of ID).
        std::vector<std::int64_t> zoneOfRoot(count, 0);
        std::vector<std::size_t> counts(count, 0);
        std::vector<std::int32_t> smallest(count, std::numeric_limits<std::int32_t>::max());
        for (std::size_t index = 0; index < count; ++index)
        {
            const std::uint32_t root = rootOf[index];
            ++counts[root];
            smallest[root] = std::min(smallest[root], list.ids[index]);
        }
        std::vector<std::uint32_t> roots;
        for (std::size_t item = 0; item < count; ++item)
        {
            if (counts[item] > 0)
                roots.push_back(static_cast<std::uint32_t>(item));
        }
        std::sort(roots.begin(), roots.end(),
                  [&](std::uint32_t left, std::uint32_t right)
                  {
                      if (counts[left] != counts[right])
                          return counts[left] > counts[right];
                      return smallest[left] < smallest[right];
                  });
        for (std::size_t number = 0; number < roots.size(); ++number)
            zoneOfRoot[roots[number]] = static_cast<std::int64_t>(number + 1);

        TableBuilder builder;
        if (detail == "elements")
        {
            std::vector<std::int64_t> zone(count);
            for (std::size_t index = 0; index < count; ++index)
                zone[index] = zoneOfRoot[rootOf[index]];
            builder.int32("elementId", std::vector<std::int32_t>(list.ids));
            builder.int32("propId", std::vector<std::int32_t>(pids));
            builder.int64("zoneId", std::move(zone));
            return builder.finish();
        }
        const std::vector<double>& areas = data.areas();
        const ElementMassData& masses = data.masses();
        std::vector<std::int64_t> zoneId(roots.size());
        std::vector<std::int32_t> zonePid(roots.size());
        std::vector<std::int64_t> zoneCount(roots.size());
        std::vector<double> zoneArea(roots.size(), 0.0);
        std::vector<double> zoneMass(roots.size(), 0.0);
        for (std::size_t number = 0; number < roots.size(); ++number)
        {
            zoneId[number] = static_cast<std::int64_t>(number + 1);
            zoneCount[number] = static_cast<std::int64_t>(counts[roots[number]]);
        }
        for (std::size_t index = 0; index < count; ++index)
        {
            const auto number = static_cast<std::size_t>(zoneOfRoot[rootOf[index]] - 1);
            zonePid[number] = pids[index];
            if (std::isfinite(areas[kept[index]]))
                zoneArea[number] += areas[kept[index]];
            if (std::isfinite(masses.mass[kept[index]]))
                zoneMass[number] += masses.mass[kept[index]];
        }
        builder.int64("zoneId", std::move(zoneId));
        builder.int32("propId", std::move(zonePid));
        builder.int64("elementCount", std::move(zoneCount));
        builder.real("area", std::move(zoneArea));
        builder.real("mass", std::move(zoneMass));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "propertyZones";
        info.family = "T4";
        info.description =
            "The contiguous zones of one property: shells, solids and structural 1D elements of the same property that "
            "are "
            "connected through common nodes (connectors, rigid elements and masses are not part of a zone). Zones are "
            "numbered from 1 by decreasing element count, ties by the smallest element ID. detail=\"zones\": one row "
            "each "
            "with the area (shells) and the mass; detail=\"elements\": the zone of each element.";
        info.detailParameter = "detail";
        info.parameters = {parameter("detail", ParameterKind::Text, "\"zones\"", "\"zones\" or \"elements\""),
                           parameter("elementIds", ParameterKind::IdList, "None", "only these elements make zones")};
        info.defaultSort = "zoneId";
        ColumnListBuilder zones;
        zones.int64("zoneId").int32("propId").int64("elementCount").real("area").real("mass");
        info.columnSets.push_back(zones.build("zones"));
        ColumnListBuilder elements;
        elements.int32("elementId").int32("propId").int64("zoneId");
        info.columnSets.push_back(elements.build("elements"));
        return info;
    }
};

} // namespace

void addPropertyZonesQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<PropertyZonesQuery>());
}

} // namespace oor::query

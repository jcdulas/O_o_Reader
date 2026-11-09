#include "oor/analysis/mass/Laminate.hpp"
#include "oor/analysis/mass/SectionFormulas.hpp"
#include "oor/base/errors/Errors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardFacts.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/topology/t1_inventory/InventoryQueries.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <unordered_map>

namespace oor::query
{

namespace
{

constexpr double NOT_A_NUMBER = std::numeric_limits<double>::quiet_NaN();

struct Totals
{
    std::int64_t count = 0;
    double length = 0.0;
    double area = 0.0;
    double volume = 0.0;
    double mass = 0.0;
};

void addFinite(double& sum, double value)
{
    if (std::isfinite(value))
        sum += value;
}

// The thickness a property gives its shells: the total of a laminate, T of a plate.
double thicknessOf(model::Model& model, const CardFacts& facts, model::CardLocation property)
{
    if (analysis::Laminate::supports(property.table))
    {
        try
        {
            double total = 0.0;
            for (const analysis::LaminatePly& ply : analysis::Laminate::stack(model, property))
                total += ply.thickness;
            return total;
        }
        catch (const base::OorError&)
        {
            return NOT_A_NUMBER;
        }
    }
    return facts.realField(model, property, "thickness");
}

double sectionAreaOf(const model::Model& model, model::CardLocation property)
{
    if (!analysis::SectionFormulas::supports(property.table))
        return NOT_A_NUMBER;
    try
    {
        const auto section = analysis::SectionFormulas::of(model, property, analysis::ShearFactorRule::None);
        return section ? section->meanArea() : NOT_A_NUMBER;
    }
    catch (const base::OorError&)
    {
        return NOT_A_NUMBER;
    }
}

// propertiesTable: each property with its materials, thickness or section, and what its elements add up to.
class PropertiesTableQuery final : public QueryBase
{
public:
    PropertiesTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        model::Model& model = *run.model;
        const std::vector<std::string> typeNames = run.arguments.texts("types");
        const std::vector<std::uint16_t> types =
            typeNames.empty() ? typesInSpace(model::CardCategory::Property, base::IdSpace::Property)
                              : typesNamed(typeNames, model::CardCategory::Property, "types");
        const CardList list = listCards(model, types, run.arguments.ids("propertyIds"));
        if (list.size() == 0)
            return emptyTable(info().defaultColumns());

        // What the elements of each property add up to.
        const CardList elements = listCards(model, typesOfCategory(model::CardCategory::Element));
        ElementData data(run.model, elements, run.context);
        std::unordered_map<std::int32_t, Totals> byProperty;
        if (elements.size() > 0)
        {
            const std::vector<std::int32_t>& pids = data.properties();
            const std::vector<double>& lengths = data.lengths();
            const std::vector<double>& areas = data.areas();
            const std::vector<double>& volumes = data.volumes();
            const ElementMassData& masses = data.masses();
            for (std::size_t index = 0; index < elements.size(); ++index)
            {
                Totals& totals = byProperty[pids[index]];
                ++totals.count;
                addFinite(totals.length, lengths[index]);
                addFinite(totals.area, areas[index]);
                addFinite(totals.volume, volumes[index]);
                addFinite(totals.mass, masses.mass[index]);
            }
        }

        CardFacts facts(model);
        const std::size_t count = list.size();
        std::vector<std::string> typeOfRow(count);
        std::vector<std::int64_t> offsets{0};
        std::vector<std::int32_t> materialIds;
        std::vector<double> thickness(count);
        std::vector<double> sectionArea(count);
        std::vector<double> nsm(count);
        std::vector<std::int64_t> elementCount(count, 0);
        std::vector<double> totalLength(count, 0.0);
        std::vector<double> totalArea(count, 0.0);
        std::vector<double> totalVolume(count, 0.0);
        std::vector<double> mass(count, 0.0);
        for (std::size_t index = 0; index < count; ++index)
        {
            const model::CardLocation location = list.location(index);
            typeOfRow[index] = std::string(typeName(list.types[index]));
            std::vector<base::EntityId> ids = facts.references(location, model::ReferenceTarget::Material);
            std::vector<std::int32_t> distinct;
            for (const base::EntityId id : ids)
            {
                if (std::find(distinct.begin(), distinct.end(), id) == distinct.end())
                    distinct.push_back(id);
            }
            materialIds.insert(materialIds.end(), distinct.begin(), distinct.end());
            offsets.push_back(static_cast<std::int64_t>(materialIds.size()));
            thickness[index] = thicknessOf(model, facts, location);
            sectionArea[index] = sectionAreaOf(model, location);
            nsm[index] = facts.realField(model, location, "nonStructMass");
            if (const auto found = byProperty.find(list.ids[index]); found != byProperty.end())
            {
                elementCount[index] = found->second.count;
                totalLength[index] = found->second.length;
                totalArea[index] = found->second.area;
                totalVolume[index] = found->second.volume;
                mass[index] = found->second.mass;
            }
        }
        const ListOrigin origin = originOf(list);
        TableBuilder builder;
        builder.column("propId", QueryColumn::primitive(std::vector<std::int32_t>(list.ids)), ColumnOrigin::Stored,
                       "id");
        builder.dictionary("type", typeOfRow);
        builder.intList("materialIds", std::move(offsets), std::move(materialIds));
        builder.real("thickness", std::move(thickness));
        builder.real("sectionArea", std::move(sectionArea));
        std::vector<std::uint8_t> nsmValid(count);
        for (std::size_t index = 0; index < count; ++index)
            nsmValid[index] = facts.hasField(list.types[index], "nonStructMass") ? 1 : 0;
        builder.real("nonStructMass", std::move(nsm), std::move(nsmValid), ColumnOrigin::Stored, "nonStructMass");
        builder.int64("elementCount", std::move(elementCount));
        builder.real("totalLength", std::move(totalLength));
        builder.real("totalArea", std::move(totalArea));
        builder.real("totalVolume", std::move(totalVolume));
        builder.real("mass", std::move(mass));
        builder.rows(origin.types, origin.keys, run.link);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "propertiesTable";
        info.family = "T1";
        info.description =
            "The properties: type, the materials they name, thickness (a plate's T, a laminate's total), cross-section "
            "area "
            "(rods, bars, beams), the non-structural mass of the card, and what their elements add up to: count, total "
            "length, area and volume, and mass (non-structural mass included). Elements whose mass cannot be computed "
            "are "
            "left out of the mass.";
        info.parameters = {
            parameter("propertyIds", ParameterKind::IdList, "None", "keep these properties"),
            parameter("types", ParameterKind::TextList, "None", "keep these card types (\"ShellProp\", ...)")};
        info.defaultSort = "propId";
        ColumnListBuilder columns;
        columns.int32("propId")
            .dictionary("type")
            .intList("materialIds")
            .real("thickness")
            .real("sectionArea")
            .real("nonStructMass", true);
        columns.int64("elementCount");
        columns.reals("totalLength totalArea totalVolume mass");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addPropertiesTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<PropertiesTableQuery>());
}

} // namespace oor::query

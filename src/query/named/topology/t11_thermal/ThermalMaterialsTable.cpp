#include "oor/model/cards/materials/heatAnisoMatTemperature/HeatAnisoMatTemperatureTable.hpp"
#include "oor/model/cards/materials/heatIsoMatTemperature/HeatIsoMatTemperatureTable.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t11_thermal/ThermalQueries.hpp"

#include <algorithm>
#include <limits>
#include <map>
#include <set>

namespace oor::query
{

namespace
{

// One value of the heat transfer materials: the column of the query and the stored column of the cards it reads.
struct Constant
{
    const char* column;
    const char* stored;
};

// The values of the heat transfer materials, by column: HeatIsoMat (MAT4) fills the isotropic ones (k, h, mu),
// HeatAnisoMat (MAT5) the conductivity matrix; cp, rho and hgen are on both. The stored names are the neutral field
// names of the dictionary (K is `conductivity`, CP `specificHeat`, H `convectionCoefficient`, MU `viscosity`). The
// cards have no reference temperature: tref is null for every row.
constexpr Constant CONSTANTS[] = {{"k", "conductivity"},
                                  {"kxx", "conductivityXx"},
                                  {"kxy", "conductivityXy"},
                                  {"kxz", "conductivityXz"},
                                  {"kyy", "conductivityYy"},
                                  {"kyz", "conductivityYz"},
                                  {"kzz", "conductivityZz"},
                                  {"specificHeat", "specificHeat"},
                                  {"density", "density"},
                                  {"h", "convectionCoefficient"},
                                  {"mu", "viscosity"},
                                  {"heatGeneration", "heatGenerationFactor"},
                                  {"referenceTemperature", "tref"}};

// thermalMaterialsTable: each MAT4 and MAT5 with its constants and the tables that make a value depend on the
// temperature.
class ThermalMaterialsTableQuery final : public QueryBase
{
public:
    ThermalMaterialsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        model::Model& model = *run.model;
        const std::vector<std::uint16_t> types =
            typesInSpace(model::CardCategory::ThermalMaterial, base::IdSpace::ThermalMaterial);
        const CardList list = listCards(model, types, run.arguments.ids("materialIds"));
        if (list.size() == 0)
            return emptyTable(info().defaultColumns());
        const std::size_t count = list.size();

        // The tables of each material: the MATT4 and MATT5 with the same MID (the companions of the material).
        std::map<base::EntityId, std::set<base::EntityId>> tablesOf;
        if (const auto* table = model.findTable<model::HeatIsoMatTemperatureTable>())
        {
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row))
                    continue;
                const model::HeatIsoMatTemperatureCard card = table->card(row);
                for (const base::EntityId id :
                     {card.conductivityTableId, card.specificHeatTableId, card.convectionCoefficientTableId,
                      card.viscosityTableId, card.heatGenerationFactorTableId})
                {
                    if (id != 0)
                        tablesOf[card.id].insert(id);
                }
            }
        }
        if (const auto* table = model.findTable<model::HeatAnisoMatTemperatureTable>())
        {
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row))
                    continue;
                const model::HeatAnisoMatTemperatureCard card = table->card(row);
                for (const base::EntityId id :
                     {card.conductivityXxTableId, card.conductivityXyTableId, card.conductivityXzTableId,
                      card.conductivityYyTableId, card.conductivityYzTableId, card.conductivityZzTableId,
                      card.specificHeatTableId, card.heatGenerationFactorTableId})
                {
                    if (id != 0)
                        tablesOf[card.id].insert(id);
                }
            }
        }

        ListColumns columns(run.model, list, run.context);
        const ListOrigin origin = originOf(list);
        std::vector<std::string> typeOfRow(count);
        std::vector<std::vector<std::int32_t>> tables(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            typeOfRow[index] = std::string(typeName(list.types[index]));
            if (const auto found = tablesOf.find(list.ids[index]); found != tablesOf.end())
                tables[index].assign(found->second.begin(), found->second.end());
        }
        TableBuilder builder;
        builder.column("matId", QueryColumn::primitive(std::vector<std::int32_t>(list.ids)), ColumnOrigin::Stored,
                       "id");
        builder.dictionary("type", typeOfRow);
        for (const Constant& constant : CONSTANTS)
        {
            if (columns.has(constant.stored))
                builder.column(constant.column, columns.get(constant.stored), ColumnOrigin::Stored, constant.stored);
            else
                builder.real(constant.column, std::vector<double>(count, std::numeric_limits<double>::quiet_NaN()));
        }
        builder.intLists("temperatureTables", tables);
        builder.rows(origin.types, origin.keys, run.link);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "thermalMaterialsTable";
        info.family = "T11";
        info.description =
            "The heat transfer materials: type (MAT4 or MAT5), the conductivity k, the specific heat cp, the density "
            "rho, "
            "the free convection coefficient h, the viscosity mu and the heat generation multiplier hgen of a MAT4, "
            "the "
            "conductivity matrix kxx kxy kxz kyy kyz kzz of a MAT5 (cp, rho and hgen are on both), null where the card "
            "type "
            "has no such field (a blank K, H or MU is null too); referenceTemperature is null: the cards have no "
            "reference "
            "temperature. "
            "temperatureTables lists the TABLEMi tables the MATT4 or MATT5 of the material gives, which make a value "
            "depend "
            "on the temperature (empty when there is none).";
        info.parameters = {parameter("materialIds", ParameterKind::IdList, "None", "keep these materials")};
        info.defaultSort = "matId";
        ColumnListBuilder columns;
        columns.int32("matId").dictionary("type");
        columns.reals("k kxx kxy kxz kyy kyz kzz cp rho h mu hgen referenceTemperature", true);
        columns.intList("temperatureTables");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addThermalMaterialsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<ThermalMaterialsTableQuery>());
}

} // namespace oor::query

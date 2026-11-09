#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardFacts.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/common/ElementData.hpp"
#include "oor/query/named/topology/t1_inventory/InventoryQueries.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>

namespace oor::query
{

namespace
{

// One constant of the materials: the column of the query and the stored column of the cards it reads.
struct Constant
{
    const char* column;
    const char* stored;
};

// The constants of the materials, by column: IsoMat (MAT1) fills the isotropic ones, OrthoMat2D (MAT8) the
// orthotropic ones.
constexpr Constant CONSTANTS[] = {{"e", "youngsModulus"},
                                  {"g", "shearModulus"},
                                  {"nu", "poissonRatio"},
                                  {"density", "density"},
                                  {"a", "thermalExpansion"},
                                  {"referenceTemperature", "referenceTemperature"},
                                  {"structuralDamping", "structuralDamping"},
                                  {"st", "tensionAllowable"},
                                  {"sc", "compressionAllowable"},
                                  {"ss", "shearAllowable"},
                                  {"e1", "youngsModulus1"},
                                  {"e2", "youngsModulus2"},
                                  {"nu12", "poissonRatio12"},
                                  {"g12", "shearModulus12"},
                                  {"xt", "tensionStrength1"},
                                  {"xc", "compressionStrength1"},
                                  {"yt", "tensionStrength2"},
                                  {"yc", "compressionStrength2"},
                                  {"s", "shearStrength"}};

// materialsTable: each material with its constants, the properties and elements that use it, and the mass it carries.
class MaterialsTableQuery final : public QueryBase
{
public:
    MaterialsTableQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        model::Model& model = *run.model;
        const std::vector<std::string> typeNames = run.arguments.texts("types");
        const std::vector<std::uint16_t> types =
            typeNames.empty() ? typesInSpace(model::CardCategory::Material, base::IdSpace::Material)
                              : typesNamed(typeNames, model::CardCategory::Material, "types");
        const CardList list = listCards(model, types, run.arguments.ids("materialIds"));
        if (list.size() == 0)
            return emptyTable(info().defaultColumns());
        const std::size_t count = list.size();

        // Which properties and elements use each material, and the mass it carries.
        CardFacts facts(model);
        std::map<std::int32_t, std::int64_t> propertyCount;
        for (const std::uint16_t type : typesInSpace(model::CardCategory::Property, base::IdSpace::Property))
        {
            const model::CardTable* table = model.findTable(type);
            if (table == nullptr)
                continue;
            for (std::uint32_t row = 0; row < table->rows().rowCount(); ++row)
            {
                if (!table->rows().isAlive(row))
                    continue;
                const std::vector<base::EntityId> ids =
                    facts.references(model::CardLocation{type, row}, model::ReferenceTarget::Material);
                const std::set<std::int32_t> distinct(ids.begin(), ids.end());
                for (const std::int32_t id : distinct)
                    ++propertyCount[id];
            }
        }
        const CardList elements = listCards(model, typesOfCategory(model::CardCategory::Element));
        ElementData data(run.model, elements, run.context);
        std::map<std::int32_t, std::int64_t> elementCount;
        std::map<std::int32_t, double> massOf;
        if (elements.size() > 0)
        {
            const std::vector<std::int32_t>& pids = data.properties();
            const ElementMassData& masses = data.masses();
            for (std::size_t index = 0; index < elements.size(); ++index)
            {
                const double structural = masses.structural[index];
                std::int32_t material = masses.material[index];
                if (material != 0)
                {
                    ++elementCount[material];
                    if (std::isfinite(structural))
                        massOf[material] += structural;
                    continue;
                }
                // A laminate carries its mass in its plies: each material takes the share of its plies.
                if (const auto* shares = data.laminateShares(pids[index]))
                {
                    for (const auto& [id, share] : *shares)
                    {
                        ++elementCount[id];
                        if (std::isfinite(structural))
                            massOf[id] += structural * share;
                    }
                }
            }
        }

        ListColumns columns(run.model, list, run.context);
        const ListOrigin origin = originOf(list);
        std::vector<std::string> typeOfRow(count);
        std::vector<std::int64_t> properties(count, 0);
        std::vector<std::int64_t> counts(count, 0);
        std::vector<double> mass(count, 0.0);
        for (std::size_t index = 0; index < count; ++index)
        {
            typeOfRow[index] = std::string(typeName(list.types[index]));
            const std::int32_t id = list.ids[index];
            if (const auto found = propertyCount.find(id); found != propertyCount.end())
                properties[index] = found->second;
            if (const auto found = elementCount.find(id); found != elementCount.end())
                counts[index] = found->second;
            if (const auto found = massOf.find(id); found != massOf.end())
                mass[index] = found->second;
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
        builder.int64("propertyCount", std::move(properties));
        builder.int64("elementCount", std::move(counts));
        builder.real("mass", std::move(mass));
        builder.rows(origin.types, origin.keys, run.link);
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "materialsTable";
        info.family = "T1";
        info.description =
            "The materials: type, the isotropic constants of a MAT1 (e g nu density a referenceTemperature "
            "structuralDamping st sc ss), the orthotropic ones of a MAT8 (e1 e2 nu12 g12 xt xc yt yc s; density, a, "
            "referenceTemperature and structuralDamping are those of the card too), null where the card type has no "
            "such field; the number of properties that name the material, the number of elements that use it "
            "(through the material of the element or of the plies of its property) and the structural mass it "
            "carries (a laminate shares its mass between its materials in proportion to density times thickness of "
            "the plies).";
        info.parameters = {
            parameter("materialIds", ParameterKind::IdList, "None", "keep these materials"),
            parameter("types", ParameterKind::TextList, "None", "keep these card types (\"IsoMat\", ...)")};
        info.defaultSort = "matId";
        ColumnListBuilder columns;
        columns.int32("matId").dictionary("type");
        columns.reals("e g nu density a referenceTemperature structuralDamping st sc ss e1 e2 nu12 g12 xt xc yt yc s",
                      true);
        columns.int64("propertyCount").int64("elementCount").real("mass");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addMaterialsTableQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<MaterialsTableQuery>());
}

} // namespace oor::query

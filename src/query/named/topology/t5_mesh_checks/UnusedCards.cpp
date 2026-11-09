#include "oor/analysis/components/ComponentCache.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/named/TableBuilder.hpp"
#include "oor/query/named/common/CardList.hpp"
#include "oor/query/named/topology/t5_mesh_checks/MeshCheckQueries.hpp"

#include <algorithm>

namespace oor::query
{

namespace
{

// The card types of a category of the query: the cards that are the thing (a property, a material), not the ones that
// complete it.
std::vector<std::uint16_t> typesOf(const std::string& category)
{
    if (category == "node")
    {
        std::vector<std::uint16_t> types = typesOfCategory(model::CardCategory::Node);
        const std::vector<std::uint16_t> points = typesOfCategory(model::CardCategory::Point);
        types.insert(types.end(), points.begin(), points.end());
        return types;
    }
    if (category == "property")
        return typesInSpace(model::CardCategory::Property, base::IdSpace::Property);
    if (category == "material")
        return typesInSpace(model::CardCategory::Material, base::IdSpace::Material);
    if (category == "coord")
        return typesOfCategory(model::CardCategory::Coordinate);
    // The tables: the TABLEDi and their kin, and the tables of several variables and of the fatigue loads, which
    // have a category of their own.
    std::vector<std::uint16_t> tables = typesOfCategory(model::CardCategory::Table);
    for (const model::CardCategory other :
         {model::CardCategory::MultiDimensionalTable, model::CardCategory::FatigueTable})
    {
        const std::vector<std::uint16_t> found = typesOfCategory(other);
        tables.insert(tables.end(), found.begin(), found.end());
    }
    return tables;
}

// unusedCards: nodes, properties, materials, coordinate systems and tables that no card refers to.
class UnusedCardsQuery final : public QueryBase
{
public:
    UnusedCardsQuery() : QueryBase(makeInfo()) {}

    [[nodiscard]] std::shared_ptr<const QueryTable> run(const QueryRun& run) const override
    {
        const model::Model& model = *run.model;
        std::vector<std::string> categories = run.arguments.texts("categories");
        if (categories.empty())
            categories = {"node", "property", "material", "coord", "table"};
        for (const std::string& category : categories)
        {
            if (category != "node" && category != "property" && category != "material" && category != "coord" &&
                category != "table")
                throw QueryError(
                    "unusedCards: categories are \"node\", \"property\", \"material\", \"coord\" and \"table\", not "
                    "\"" +
                    category + "\"");
        }
        const auto references = analysis::ComponentCache::of(model).references(model);
        std::vector<std::string> category;
        std::vector<std::string> card;
        std::vector<std::int32_t> id;
        for (const std::string& name : categories)
        {
            const CardList list = listCards(model, typesOf(name));
            for (std::size_t index = 0; index < list.size(); ++index)
            {
                if (references->isReferenced(list.location(index)))
                    continue;
                category.push_back(name);
                card.emplace_back(typeName(list.types[index]));
                id.push_back(list.ids[index]);
            }
        }
        TableBuilder builder;
        builder.dictionary("category", category);
        builder.dictionary("card", card);
        builder.int32("id", std::move(id));
        return builder.finish();
    }

private:
    static QueryInfo makeInfo()
    {
        QueryInfo info;
        info.name = "unusedCards";
        info.family = "T5";
        info.description =
            "The grid points and scalar points, properties, materials, coordinate systems and tables (the TABLEDi and "
            "their kin, the TABL3Di and TABLE3D, and the TABLFTG) that no card refers to "
            "(a reference is a field that names the card: the node of an element, the property of an element, the "
            "material "
            "of a property...). A grid point that only sits inside a THRU range of a card is not seen as used. Sorted "
            "by "
            "category in the order asked (node, property, material, coord, table by default), then ID.";
        info.parameters = {parameter("categories", ParameterKind::TextList, "None",
                                     "any of \"node\", \"property\", \"material\", \"coord\", \"table\"")};
        info.defaultSort = "category (as asked), id";
        ColumnListBuilder columns;
        columns.dictionary("category").dictionary("card").int32("id");
        info.columnSets.push_back(columns.build());
        return info;
    }
};

} // namespace

void addUnusedCardsQuery(NamedQueryRegistry& registry)
{
    registry.add(std::make_unique<UnusedCardsQuery>());
}

} // namespace oor::query

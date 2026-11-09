#include "oor/api/CardTables.hpp"

#include "oor/analysis/coordinates/CoordinateTransform.hpp"
#include "oor/model/core/CardRegistry.hpp"
#include "oor/query/QueryErrors.hpp"
#include "oor/query/operators/ExecutionContext.hpp"
#include "oor/query/plan/Query.hpp"
#include "oor/query/providers/ModelSource.hpp"
#include "oor/query/table/PlainColumn.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace oor::api
{

namespace
{

constexpr std::size_t ROTATION_VALUES = 9;
constexpr std::size_t POSITION_VALUES = 3;

// The categories cardTypesOf lists, by the name a caller gives.
struct CategoryName
{
    const char* name;
    model::CardCategory category;
};

constexpr CategoryName CATEGORIES[] = {
    {"nodes", model::CardCategory::Node},
    {"points", model::CardCategory::Point},
    {"coordinates", model::CardCategory::Coordinate},
    {"elements", model::CardCategory::Element},
    {"properties", model::CardCategory::Property},
    {"materials", model::CardCategory::Material},
    {"loads", model::CardCategory::Load},
    {"constraints", model::CardCategory::Constraint},
    {"methods", model::CardCategory::Method},
};

model::CardCategory categoryNamed(const std::string& name)
{
    for (const CategoryName& entry : CATEGORIES)
        if (name == entry.name)
            return entry.category;
    throw query::QueryError("unknown card category '" + name +
                            "': expected nodes, points, coordinates, elements, properties, materials, loads, "
                            "constraints or methods");
}

// The card types a card table source names: one card type, or every type of the nodes or the points.
std::vector<std::uint16_t> typesOfSource(const std::string& source)
{
    if (source == "nodes")
        return query::typesOfCategory(model::CardCategory::Node);
    if (source == "points")
        return query::typesOfCategory(model::CardCategory::Point);
    const std::optional<std::uint16_t> type = model::CardRegistry::instance().typeIndexOf(source);
    if (!type || *type == model::CardRegistry::UNKNOWN_TYPE)
        throw query::QueryError("unknown card type '" + source +
                                "': expected a card type such as ShellQuad4 or ShellProp, \"nodes\" or \"points\"");
    return {*type};
}

std::shared_ptr<query::ModelSource> sourceOf(std::shared_ptr<model::Model> model, const std::string& source)
{
    return std::make_shared<query::ModelSource>(std::move(model), source, typesOfSource(source));
}

std::shared_ptr<const query::QueryTable> runSelect(std::shared_ptr<query::ModelSource> source,
                                                   const std::vector<std::string>& columns)
{
    std::vector<query::SelectItem> items;
    items.reserve(columns.size());
    for (const std::string& name : columns)
        items.push_back(query::SelectItem{name, {}, {}});
    // One thread: the tables a solver reads are made once per run, and the WebAssembly module has no other.
    query::ExecutionContext context;
    return query::Query(std::move(source)).select(std::move(items)).run(context);
}

// The values of a numeric column of a table, flattened (a fixed-size list gives its values row after row).
std::vector<double> numbersOf(const query::QueryTable& table, const std::string& name)
{
    const std::optional<std::size_t> index = table.find(name);
    if (!index)
        throw query::QueryError("the table has no column '" + name + "'");
    return query::plainColumn(table, *index).numbers;
}

} // namespace

CardTableColumns cardTableColumns(const std::string& source)
{
    // The columns do not depend on the cards there are: an empty model gives them.
    const auto model = std::make_shared<model::Model>();
    CardTableColumns result;
    for (const query::ColumnDescription& column : sourceOf(model, source)->columns())
    {
        if (column.origin == query::ColumnOrigin::Stored)
            result.stored.push_back(column.name);
        else
            result.computed.push_back(column.name);
    }
    return result;
}

std::shared_ptr<const query::QueryTable> cardTable(const Deck& deck, const std::string& source,
                                                   const std::vector<std::string>& columns)
{
    std::shared_ptr<query::ModelSource> cards = sourceOf(deck.model(), source);
    if (!columns.empty())
        return runSelect(std::move(cards), columns);
    std::vector<std::string> stored;
    for (const query::ColumnDescription& column : cards->columns())
        if (column.origin == query::ColumnOrigin::Stored)
            stored.push_back(column.name);
    return runSelect(std::move(cards), stored);
}

std::vector<std::string> cardTypesOf(const Deck& deck, const std::string& category)
{
    const model::CardCategory wanted = categoryNamed(category);
    const model::CardRegistry& registry = model::CardRegistry::instance();
    std::vector<std::string> names;
    for (std::uint16_t type = 0; type < registry.size(); ++type)
    {
        if (type == model::CardRegistry::UNKNOWN_TYPE || registry.entry(type).info.category != wanted)
            continue;
        const model::CardTable* table = deck.model()->findTable(type);
        if (table != nullptr && table->rows().aliveCount() > 0)
            names.emplace_back(registry.entry(type).info.name);
    }
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<double> displacementRotations(const Deck& deck)
{
    // The grid points in the rows of the "nodes" table: their CD and their position in the basic system, from which
    // the directions of a curvilinear CD system are taken.
    const std::shared_ptr<const query::QueryTable> nodes =
        cardTable(deck, "nodes", {"displacementFrameId", "xyzGlobal"});
    const std::vector<double> cds = numbersOf(*nodes, "displacementFrameId");
    const std::vector<double> positions = numbersOf(*nodes, "xyzGlobal");
    analysis::CoordinateTransform transform(*deck.model());
    std::vector<double> rotations(cds.size() * ROTATION_VALUES);
    for (std::size_t row = 0; row < cds.size(); ++row)
    {
        // A blank CD is the basic system.
        const auto cd = std::isnan(cds[row]) ? base::EntityId{0} : static_cast<base::EntityId>(cds[row]);
        const base::Vec3 position{positions[POSITION_VALUES * row], positions[POSITION_VALUES * row + 1],
                                  positions[POSITION_VALUES * row + 2]};
        const base::Mat3 rotation = transform.displacementRotation(cd, position);
        std::copy(rotation.values.begin(), rotation.values.end(), rotations.begin() + row * ROTATION_VALUES);
    }
    return rotations;
}

ModelVersions modelVersions(const Deck& deck)
{
    return {deck.model()->structureVersion(), deck.model()->contentVersion()};
}

} // namespace oor::api
